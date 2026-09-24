# Reentrant ASDF Firmware Refactor Plan

## Purpose

Refactor the ASDF keyboard firmware into an allocation-free, instance-based
library that can be embedded safely in larger programs while retaining a simple
single-keyboard interface (the simple wrapper) for Arduino-style environments.

The existing design remains the behavioral reference. The matrix scanner,
keymaps, actions, and hardware support should evolve incrementally rather than
being rewritten at once.

## Motivation

The firmware is organized into useful components, but those components store
their mutable state in module-level singletons. This makes the source reusable,
but prevents independent keyboard instances and creates hidden coupling between
the scanner, buffers, keymaps, modifiers, repeat logic, hooks, and virtual
outputs.

Several related issues should be addressed as part of the refactor:

- Hooks are stored as `void (*)(void)` and cast to incompatible function types.
- Message pacing and long output pulses use millisecond-scale blocking delays.
- Keymap setup is procedural and configures several global subsystems by side
  effect.
- A keymap change can occur while a scan is in progress.
- Buffer overflow and several invalid inputs are silently ignored.
- The one-bit architecture tick flag can lose elapsed ticks.
- The host test build defines `C_FLAGS` but applies `CFLAGS`, so its intended
  warning set is not consistently enabled.

## Definitions and concurrency contract

For this project, *reentrant* means:

1. All mutable keyboard-library state belongs to an explicit instance.
2. Operations on one instance cannot affect another instance.
3. Separate instances may be driven independently or concurrently.
4. Normal operations on the same instance are serialized by its owner.
5. Interrupt code has a deliberately small interface, initially limited to
   recording elapsed ticks in platform-owned state.

The first version will not promise arbitrary concurrent or recursive calls on
the same instance. Callbacks must not mutate an instance while it is being
scanned; requests such as keymap changes will be recorded and applied at a safe
boundary. If same-instance multi-threaded access is later required, it can be
provided by an RTOS adapter without putting locks in the portable core.

The goal is to remove hidden *mutable* globals from the core. Immutable keymap
tables, constant lookup tables, interrupt vectors, and memory-mapped register
definitions may retain static storage duration.

## Design principles

- No mandatory dynamic allocation.
- Support the ATmega328P and ATmega2560 as the primary AVR targets. Smaller
  or secondary AVR targets are kept only while they fit cleanly (see Resource
  constraints).
- Keep the portable core in C99.
- Keep hardware policy in platform adapters.
- Prefer typed interfaces over generic function-pointer registries.
- Make time explicit and keep core operations nonblocking.
- Provide a simple single-keyboard wrapper.
- Change one subsystem at a time and retain behavioral tests throughout.
- Measure flash, static RAM, and worst-case stack growth at every phase.

## Target architecture

### Explicit keyboard instance

The application owns an `asdf_t` object. It contains nested state objects rather
than exposing unrelated module singletons:

```c
typedef struct {
  asdf_scanner_state_t scanner;
  asdf_ring_t keycodes;
  asdf_ring_t messages;
  asdf_modifier_state_t modifiers;
  asdf_repeat_state_t repeat;
  asdf_keymap_state_t keymap;
  asdf_virtual_state_t virtual_outputs;
  asdf_physical_state_t physical_outputs;
  asdf_pacing_state_t pacing;
  const asdf_config_t *config;
  const asdf_platform_t *platform;
} asdf_t;
```

The final layout may be flattened or conditionally compiled if nested structs
cost extra space on a target. State types should nevertheless remain logically
separate so their invariants can be tested directly.

### Typed platform interface

Replace the untyped hook table with operations that match their actual
signatures and carry an adapter-owned context pointer:

```c
typedef struct {
  void *user;
  asdf_cols_t (*read_row)(void *user, uint8_t row);
  void (*send_code)(void *user, asdf_keycode_t code);
  void (*set_output)(void *user, asdf_physical_dev_t output, uint8_t value);
  void (*set_strobe_polarity)(void *user, uint8_t positive);
  asdf_irq_state_t (*irq_disable)(void *user);
  void (*irq_restore)(void *user, asdf_irq_state_t state);
} asdf_platform_t;
```

User actions should use one compatible callback type, for example:

```c
typedef void (*asdf_user_action_fn)(asdf_t *keyboard,
                                    void *user,
                                    uint8_t action,
                                    uint8_t pressed);
```

The exact interface should be kept small, because indirect calls cost flash
and RAM on AVR. There is no separate static-dispatch build for small parts: an
AVR target that cannot fit the instance-based core is dropped rather than given
its own dispatch path.

### Declarative keymaps

Replace setup functions that mutate global registries with immutable keymap
descriptors. A descriptor should contain:

- The plain, shift, caps, and control matrices and their dimensions.
- Virtual-to-physical output bindings and initial values.
- User-action bindings.
- Initial modifier and repeat configuration.
- Message pacing configuration.
- Optional scanner/platform selection metadata.

The generated keymap table should become a table of descriptor pointers rather
than a switch that invokes setup functions. Keymap selection applies one
descriptor to one `asdf_t` instance.

### Nonblocking processing

The core should be driven with elapsed time:

```c
void asdf_process(asdf_t *keyboard, uint16_t elapsed_ms);
uint8_t asdf_read(asdf_t *keyboard, asdf_keycode_t *code);
```

`asdf_process()` advances debounce, repeat, message pacing, and output pulse
state without sleeping. Platform code decides whether it is called from an
Arduino `loop()`, a bare-metal superloop, an RTOS task, or a simulator.

## Migration phases

### Phase 0: Establish the baseline

- Record clean host-test, simavr-test, AVR-build, and ARM-build results.
- Record `text`, `data`, and `bss` sizes for every firmware target.
- Add CI size reports and initially non-failing size-regression summaries.
- Fix the `C_FLAGS`/`CFLAGS` host-test mismatch.
- Enable useful warnings consistently across host, AVR, and ARM builds.
- Add characterization tests for initialization, keymap switching, modifier
  precedence, repeat timing, buffer priority, and output pulses.
- Record the current Shift Lock state transitions, including the intentional
  behavior where pressing and releasing Shift clears Shift Lock on layouts that
  use `ACTION_SHIFTLOCK_ON`.
- Characterize debounce recovery when raw input returns to the stable state and
  prove that a debounce counter is never decremented while zero.
- Test keymap-switch action replay explicitly: persistent DIP/configuration
  state must survive a switch, initial virtual outputs must be asserted after
  their bindings are installed, and edge-triggered hooks or pulses must not be
  invoked merely because a map was selected.
- Test invalid modifier, row, column, virtual-device, and physical-device values
  at every public boundary that accepts them.
- Record LF-to-CRLF behavior when the message queue has zero, one, or at least
  two slots remaining.
- Document current edge behavior where tests reveal ambiguity rather than
  silently changing it.

Exit criteria:

- The existing behavior is captured well enough to distinguish an intentional
  change from a refactor regression.
- Every supported target has a reproducible size baseline.

### Phase 1: Replace the global buffer allocator

- Introduce an `asdf_ring_t` operating on caller-provided storage.
- Embed the keycode and message storage in the owning keyboard instance.
- Return success/failure from enqueue operations.
- Track dropped keycodes/messages or expose an overflow status.
- Reject zero and invalid capacities explicitly.
- Normalize LF to CRLF without recursive enqueue calls. Reserve both bytes
  atomically, or return a failure without leaving a partial CRLF sequence.
- Retire integer buffer handles and the global buffer pool.
- Preserve message-buffer priority over typed keycodes.
- Replace the `printf`-style `asdf_print()` with a plain string writer that
  reads from flash (`PROGMEM` on AVR), and move the keymap ID messages and the
  Applesoft keyboard-test program into flash. No caller uses a format argument,
  so the nanoprintf dependency can be dropped.

This phase should reduce state and code size, creating budget for later context
pointers and typed adapters.

Exit criteria:

- Ring-buffer tests require no global reset fixture.
- Two ring-buffer objects can be interleaved without interference.
- Existing output ordering remains unchanged.
- CRLF conversion and overflow behavior are deterministic and reported.
- No string literals remain in AVR `.data`, and nanoprintf is no longer linked.
- Identity and typed-string simavr traces remain byte-for-byte equivalent.

### Phase 2: Extract leaf-module state

Convert modules with few dependencies before changing the scanner:

1. Repeat state.
2. Modifier state.
3. Physical output shadows and allocation links.
4. Virtual output mappings.

Each operation receives its owning state explicitly. Split immutable tables,
such as physical output capabilities, from mutable shadows and links. Add
bounds checking at public API boundaries.

While migrating these modules, remove misleading control flow and compound
side effects: express the CAPS toggle as a separate state transition, terminate
the `V_SET_LO` case explicitly, and validate virtual/physical identifiers before
using them as array indices.

Exit criteria:

- Each module can be tested with two independent state objects.
- No mutable file-scope state remains in these modules.
- Existing modifier, repeat, and virtual-output tests remain behaviorally
  equivalent.

### Phase 3: Introduce typed platform operations

- Add `asdf_platform_t` and a fake host platform.
- Replace row-scanner and output hook casts with typed calls.
- Replace the physical handler table with a typed platform output operation or
  a compact typed driver table.
- Move data polarity and other per-device configuration into platform-owned
  state. Deferred to Phase 7, which makes adapters per-instance. The interrupt
  operations
  (`irq_disable`/`irq_restore`) turned out not to be needed: only the
  architecture's tick drain shares state with an interrupt, and it masks
  interrupts itself (Phase 6).
- Keep user actions separate from mandatory platform operations.
- Remove `asdf_hook_get()` once all incompatible uses have migrated.

Exit criteria:

- No call is made through an incompatible function-pointer type.
- Host tests can provide independent fake hardware for two keyboard instances.
- `-Wcast-function-type` passes for first-party code.

### Phase 4: Make keymaps instance-based and declarative

- Define immutable keymap and binding descriptors.
- Convert one test keymap first and prove the descriptor design.
- Convert the production keymaps incrementally.
- Change generated setup files to produce a descriptor registry.
- Store only the selected descriptor/index and mutable runtime state in
  `asdf_t`.
- Keymap selection resets runtime state (modifiers, repeat, last key, virtual
  outputs, hooks) rather than replaying held keys, then re-applies only the
  configuration actions of held switches (keymap select, strobe polarity,
  autorepeat select). Other held keys are not re-activated. This is already
  implemented. Configuration is identified by action type
  (`asdf_is_configuration_action`) rather than descriptor metadata, since
  whether an action is configuration does not depend on the keymap.
- Synchronize initial virtual outputs only after the new descriptor's complete
  virtual-to-physical binding set has been installed.
- Validate equal dimensions across modifier maps where required.
- Preserve flash placement/`PROGMEM` behavior on AVR.
- Apply map changes only after the current scan completes: keymap select
  actions record a request, applied at the end of the scan, so DIP switch bits
  that change together select the final keymap without passing through
  intermediate keymaps.

Exit criteria:

- Selecting a keymap mutates only the supplied instance and its platform.
- Two instances can use different keymaps simultaneously in host tests.
- Selecting a keymap preserves marked configuration state without invoking
  unmarked user actions or pulses.
- Initial virtual output shadows are asserted exactly once after their bindings
  are complete.
- All simavr keymap, identity, and typed-string traces remain equivalent.

### Phase 5: Move the scanner into `asdf_t`

- Move stable key rows, debounce counters, last-repeat key, print pacing, and
  queue ownership into the keyboard instance.
- Pass `asdf_t *` through internal action and lookup paths.
- Track the repeating key by coordinate as well as keycode so duplicate
  keycodes and N-key rollover have defined behavior.
- Make initialization idempotent and define reset semantics explicitly.
- Validate modifier, row, and column indices before selecting a map or
  accessing fixed arrays.
- Replace debounce pre-decrement with a saturating state transition that cannot
  underflow. Define and test whether a return to the stable raw state resets the
  debounce interval before changing the existing behavior.
- Add a dual-instance integration test that alternates scans, modifiers,
  keymaps, output, and repeat events.
- Hooks remain `void (void)` functions, so a hook cannot tell which keyboard
  fired it; the keymap ID-message hooks print to the default keyboard. The
  per-key press and release actions of Phase 8 replace them.

Exit criteria:

- No mutable file-scope state remains in the portable scanner/core.
- Invalid public indices cannot cause out-of-bounds reads or writes.
- Shift Lock, modifier precedence, and the selected debounce semantics are
  documented by state-transition tests.
- Independent instances show no cross-instance state leakage under randomized
  event sequences.

### Phase 6: Remove blocking timing

- Replace the one-bit tick flag with a counted or elapsed-time interface.
- Drain interrupt-owned tick state inside a short critical section so ticks
  cannot be lost during read-and-clear.
- Replace message-character delays with a next-eligible-output deadline.
- Replace the long output pulse delay (50 ms) with per-output pulse state and
  a deadline on the scan tick.
- Keep microsecond-scale timing, the 10 us output strobe and the 10 us short
  pulse, as bounded busy-waits in the platform adapter. They are far shorter
  than the scan tick, so scheduling them would add complexity, or stretch them
  to a full tick, for no practical gain.
- Defer any remaining structural mutations until a scan boundary (keymap
  changes are already deferred; see Phase 4).
- Define behavior for large elapsed-time jumps and counter saturation.
- Done as: the tick interrupt counts elapsed ticks (saturating at 255), read
  and cleared with interrupts masked. `asdf_process()` scans once per call and
  advances debounce, repeat, pacing, and pulses by the elapsed ticks, so timing
  follows real time even when a scan takes longer than a tick (the 8 MHz
  ATmega328P scans in about 1.3 ms). The 10 us strobe and short pulse remain
  bounded busy-waits in the adapters.

Exit criteria:

- No portable-core operation calls a busy-wait delay. Platform operations
  busy-wait only for bounded microsecond-scale timing (strobe and short pulse).
- Scanning continues while a message is paced or a long output pulse is active.
- Strobe and pulse widths are verified in simavr traces for each AVR adapter.
- Timing tests use a fake clock and require no real sleeping.

### Phase 7: Adapt each platform

- Add host/test, ATmega 328-class, ATmega 2560-class, PIC32CM DIP-28, and
  PIC32CM Q64 platform objects.
- Keep interrupt-vector ownership in the board/application layer.
- Have each ISR record ticks for the platform instance and do no scanning or
  callback work.
- Verify GPIO initialization, strobe polarity, row scanning, and output timing
  on each adapter.
- Drop any AVR target that no longer fits cleanly, rather than keeping a
  separate static-dispatch build for it.

- Done as: `asdf_platform_t` gained `set_output`, `set_strobe_polarity`,
  `pulse_delay_short`, and `reset` operations, so the core calls no
  `asdf_arch_*` function. The physical output state records the platform that
  drives it. Each firmware adapter defines an `asdf_arch_t` holding its
  platform, tick count, and data polarity; `main.c` owns that state and
  defines the tick interrupt (`ASDF_ARCH_TICK_ISR`), which only counts ticks.
  A keymap switch calls the platform's `reset` (default data and strobe
  polarity) instead of re-running the whole hardware init. The fake host
  platform records outputs, strobe polarity, short pulses, and resets per
  instance. The instance adapters cost about 210-240 bytes of AVR flash and
  10 bytes of RAM; every AVR target, including the ATmega88P, still fits with
  over 1 KiB of flash to spare, so none was dropped.

Exit criteria:

- Every existing target builds from the same instance-based core.
- AVR simavr behavior remains equivalent.
- PIC32CM builds remain warning-clean and within memory limits.

### Phase 8: Per-key press and release actions

Replace each keymap matrix element, today a single keycode in which values of
`ASDF_ACTION` (0xA0) and above name built-in actions, with a press action and
a release action:

```c
typedef struct {
  uint8_t press_fn;      // index into the action function table
  uint8_t press_param;
  uint8_t release_fn;
  uint8_t release_param;
} asdf_key_t;
```

- The functions are indices into one immutable table of typed action
  functions in flash, not pointers, so each costs one byte. A function
  receives the keyboard and its parameter, for example
  `void (*)(asdf_t *kb, uint8_t param)`.
- Sending a code becomes an action with the code as its parameter: a plain
  key is press `insert_code(value)`, release `no_action`. A key can therefore
  send any byte 0x00-0xFF. This removes the 0xA0 ceiling on output codes
  (for example, host-specific codes such as the MCM/70 emulator's) and the
  collision between codes and action numbers, including ACTION_NOTHING being
  queued as a code.
- Modifiers, repeat, keymap select, strobe polarity, virtual outputs, and
  keymap-specific functions (ID messages, keyboard tests) become table
  entries with parameters, replacing the numbered action codes, the
  `ACTION_FN_n` / `ASDF_HOOK_USER_n` indirection, and `void (void)` hooks. A
  keymap-specific function receives the keyboard that fired it.
- Release behavior is explicit per key rather than implied by the action
  code.
- Whether an action is configuration (re-applied on a keymap switch) becomes
  a property of its table entry.

Cost: each matrix element grows from 1 to 4 bytes, so keymap flash grows
roughly fourfold (a 9 x 8 keymap with four modifier maps goes from 288 to
1152 bytes). The ATmega328P and ATmega2560 have ample flash. The ATmega88P
may hold only one keymap, or be dropped under the AVR target policy; decide
at this phase's size checkpoint.

- Done as: `asdf_key_t` holds the press and release actions. The action table
  (`asdf_action_table`) has 256 entries in flash, one per action number: a GCC
  range designator sets every entry to `asdf_action_nothing`, then the built-in
  (`ASDF_BUILTIN_ACTIONS`) and keymap-provided entries override it, so dispatch
  needs no check. Built-in actions are numbered from 0 (`ACTION_NOTHING`, so an
  all-zero key does nothing); keymap-provided actions start at
  `ASDF_KEYMAP_ACTIONS` (0x80). The configuration test is
  `asdf_is_configuration_action` on the press action number. Hooks are gone:
  the keymap ID is a string in the descriptor, printed by the `KEYMAP_ID`
  action, and `EACH_SCAN` became the descriptor's `each_scan` action. Key
  matrices are written in YAML and generated into C at build time by
  `src/asdf_keymap_gen.py` (dependencies managed by uv); the master keymap files
  stay C and assign the named matrices. Intended corrections: a "nothing" key
  no longer queues a code, and the Applesoft test key now works in
  `apple2_caps` as it does in `apple2`. Flash grew about 4.5 KB (matrices 1.4
  to 5.5 KB, plus the 512-byte table); RAM fell 25 bytes. The ATmega88P no
  longer fits (over by 3.1 KB) and was dropped; the ATmega168P, ATmega640, and
  ATmega1280 remain.

Exit criteria:

- Every keymap is expressed in press/release actions, with simavr traces
  unchanged except for intended corrections.
- A key can send any code 0x00-0xFF.
- No `void (void)` hooks or numbered action codes remain.
- Per-target flash use is measured and each secondary target is kept or
  dropped explicitly.

### Phase 9: Remove the compatibility layer; add the simple wrapper

- Remove `asdf_compat.c` and its argument-less API (`asdf_init()`,
  `asdf_keyscan()`, `asdf_next_code()`, and the module wrappers). Only
  `main.c` and the host tests use it; the core and keymaps already take the
  keyboard explicitly.
- Convert the host tests to the instance (`_r`) API, each on its own `asdf_t`.
- Have `main.c` own its `asdf_t`, next to its `asdf_arch_t`.
- Add a minimal Arduino-compatible simple wrapper and example with `begin()`,
  `poll()`, `available()`, and `read()` semantics around one owned instance.
  Keep it in the application layer, not in the core.
- If a C++ wrapper is added, keep it header-light and implement behavior through
  the C99 core.
- Document when to use the simple wrapper and when to use explicit instances.
- Done as: `asdf_compat.c` and every argument-less declaration are removed.
  `main.c` owns its `asdf_t`; the host tests each own one (or just the module
  state they test) and use the `_r` API. `asdf_update_r()` runs the keyboard
  without sending, leaving codes for `asdf_next_code_r()`. The simple wrapper
  (`src/asdf_simple.[ch]`) owns the hardware, the keyboard, and the tick
  interrupt, and provides `asdf_begin()`, `asdf_poll()`, `asdf_available()`,
  and `asdf_read()`; the application delivers the codes. It is tested on the
  host (the test adapter now has the firmware adapters' `asdf_arch_t` API) and
  compiled, but not linked, in every firmware build. The README describes both
  ways to run the keyboard. Not done: an Arduino library layout
  (`library.properties`, `examples/`). On AVR that also needs a tick source
  that does not take over Timer 0, which the Arduino core uses for `millis()`.

Exit criteria:

- No argument-less wrappers or default keyboard remain in the core.
- The firmware can be built for a single keyboard with the simple wrapper,
  without understanding contexts or callbacks.
- An embedding application can instantiate the core without linking the
  simple wrapper.

### Phase 10: Harden quality gates

- Run host tests with AddressSanitizer and UndefinedBehaviorSanitizer.
- Enable `-Wpedantic`, `-Wcast-function-type`, `-Wshadow`,
  `-Wimplicit-fallthrough`, and selected conversion warnings; make stable
  warning sets fatal in CI.
- Add host coverage reporting for the portable core.
- Add randomized/fuzzed scan-event sequences with state invariants.
- Convert silent failures to returned status, counters, or documented
  assertions as appropriate for embedded builds.
- Add enforceable per-target flash and RAM ceilings.
- Add hardware-in-the-loop smoke tests for at least one AVR board and both
  PIC32CM variants before describing those targets as behaviorally validated.
- Replace stale procedure boilerplate with concise API contracts and module
  invariants.

## Compatibility strategy

The migration should avoid one commit that changes every public function.
During the transition:

- Add instance APIs alongside the existing APIs.
- Implement legacy functions through one compatibility-owned context whenever
  practical.
- Convert tests to instance APIs before removing the underlying singleton.
- Mark legacy APIs clearly; Phase 9 removes them, together with converting
  the tests and adding the simple wrapper.
- Avoid compatibility macros that silently select a global context inside core
  modules; wrappers should make that ownership visible.

## Resource constraints

The ATmega328P and ATmega2560 are the primary AVR targets. The other AVR
parts (ATmega88P, ATmega168P, ATmega640, ATmega1280) were added mainly to cope
with component scarcity, and the ATmega88P now costs more than the ATmega328P.
Secondary targets are supported only while they fit the instance-based core
cleanly; a target that does not fit is dropped rather than given special-case
code.

The ATmega88P was dropped in Phase 8, when four-byte keys no longer fit its
flash. The notes below record the constraints it imposed until then.

The ATmega88P is the tightest secondary target. The v1.7.1 image uses 7,766
bytes of its 8 KiB flash when initialized data is included, and 694 bytes of
its 1 KiB SRAM for `.data` plus `.bss`. That leaves 426 bytes of flash for code
growth and roughly 330 bytes for stack and runtime margin. The ATmega168P has
the same 1 KiB SRAM, so its RAM margin is equally tight.

Accordingly:

- Capture a fresh baseline before implementation.
- Treat size regressions as design feedback, not end-of-project cleanup.
- Recover space early by removing the general buffer allocator.
- Recover space early by dropping nanoprintf and storing strings in flash. In
  the v1.7.1 ATmega88P image, nanoprintf occupies 1,798 bytes of flash, and
  about 190 bytes of message strings are copied into SRAM as `.data`. Every
  `asdf_print()` call passes a literal string with no format arguments. This
  is the main source of flash headroom for instance-pointer passing on the
  ATmega88P, estimated at 400-1,000 bytes.
- Keep configuration and keymap descriptors immutable and in flash.
- Avoid storing a full platform function table per instance; store a pointer to
  shared immutable operations.
- Consider nibble-sized/saturating debounce storage only if measurements require
  it and behavioral tests prove equivalence.
- Do not require two runtime instances to fit on the 1 KiB-SRAM parts. The
  library must be capable of independent instances, while constrained products
  may allocate exactly one.
- Decide at the end of Phase 1, and again after Phase 5, whether each secondary
  target still fits with acceptable flash, RAM, and stack margins. Drop a
  target that does not.

## Major risks and mitigations

### Behavioral drift

Debounce, repeat, Shift Lock, map-switch reset and configuration, line-ending
conversion, and output timing contain implicit assumptions. Characterization
and simavr trace tests must precede structural changes.

### AVR flash and RAM growth

Context pointers and indirect platform calls can increase code size. Measure
each phase and simplify state representation early. If a secondary target
stops fitting, drop it; the ATmega328P and ATmega2560 must always fit.

### Harvard-architecture constant placement

Declarative descriptors must not accidentally move keymaps or function tables
from flash into SRAM. Inspect ELF sections and map files as part of AVR tests.

### Interrupt races

`volatile` alone does not make read-and-clear sequences safe. Keep the ISR
surface tiny and use platform-provided critical sections around shared tick
state.

### Simple wrapper becoming permanent coupling

Build and test the core without the simple wrapper in CI so convenience wrappers cannot
reintroduce hidden dependencies.

## Completion criteria

The refactor is complete when:

- The portable core has no hidden mutable file-scope state.
- Two host-side `asdf_t` instances can run interleaved with different keymaps,
  hardware fakes, modifier states, queues, and timing without interference.
- No incompatible function-pointer casts remain in first-party code.
- Core processing is nonblocking.
- Invalid public indices cannot access state outside their owning tables.
- Keymap selection resets runtime state, re-applies only explicitly marked
  configuration actions, and cannot invoke edge-triggered hooks or pulses as an
  initialization side effect.
- LF-to-CRLF conversion cannot leave an unreported partial sequence.
- Existing host and simavr behavior is preserved except for explicitly approved
  corrections.
- All AVR and ARM targets build cleanly.
- ATmega328P and ATmega2560 flash, RAM, and stack margins remain acceptable and
  enforced, as do those of every secondary AVR target still supported.
- The explicit-instance API and the simple wrapper are both documented and tested.
- The PIC32CM claims clearly distinguish build verification from hardware
  validation until hardware tests exist.

## Estimated effort

For one experienced embedded-C developer:

- Baseline and characterization: about 1 week.
- State extraction and typed interfaces: 2-3 weeks.
- Declarative keymaps and scanner migration: 1-2 weeks.
- Nonblocking timing and platform adapters: 1-2 weeks.
- Compatibility, size tuning, documentation, and full verification: 1-2 weeks.

The recommended refactor is therefore approximately 6-10 engineer-weeks,
excluding delays for obtaining and validating physical PIC32CM hardware.
