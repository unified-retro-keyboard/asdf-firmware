# MISRA C:2025 deviations

ASDF is checked against MISRA C:2025 with PC-lint Plus, as part of the `lint`
presets (see the README). It is not certified, and makes no MISRA compliance claim: the
guidelines are a checklist for the code, and each place the code departs from
one is recorded here.

Each record gives the guideline, its category, where the deviation applies,
why, and what keeps the risk down. The option that enforces it is in
`lint/misra.lnt`, or, for one file or one line, a `//lint` comment there that
cites the record. No mandatory guideline is deviated.

## D1 — Rule 2.5 (advisory): unused macros

- **Scope:** headers.
- **Reason:** headers define whole sets: every ASCII code in `asdf_ascii.h`,
  every pin and port of an adapter in its `asdf_arch_*.h`. Each firmware uses
  only some of them.
- **Risk:** an unused macro generates no code.

## D2 — Rules 2.8 and 8.7 (advisory): unused definitions; external linkage

- **Scope:** the core's public functions, interrupt handlers, and the newlib
  system-call stubs.
- **Reason:** the core is a library. A firmware calls only part of its API; the
  rest serves embedding applications (`asdf_simple.h`) and the tests.
  Interrupt handlers and the stubs are found by name, by the vector table and
  by newlib.
- **Risk:** the linker drops unreferenced functions (`--gc-sections`).

## D3 — Directive 4.9 (advisory): function-like macros

- **Scope:** the key macros (`KEY_SEND` and the rest, `ASDF_KEY`), the flash
  access macros (`FLASH_READ`, `FLASH_READ_PTR`, `FLASH_MEMCPY`), and small
  constant helpers.
- **Reason:** key macros must be constant expressions, since keymaps are
  static initializers in flash. The flash macros wrap avr-libc's, which are
  macros.
- **Risk:** each macro parenthesizes its parameters and its expansion, and
  evaluates each parameter once.

## D4 — Rule 11.4 (required): conversion between integer and pointer

- **Scope:** expansions of the hardware register macros (avr-libc's `_SFR_*`,
  CMSIS `*_REGS`, `SysTick`, `NVIC`, `SCB`) and avr-libc's `pgm_read_*`.
- **Reason:** memory-mapped registers and flash addresses are integers the
  hardware defines; the vendor headers convert them to pointers.
- **Risk:** the conversions are the vendor's, for the device the build
  targets. ASDF code converts no integer to a pointer itself.

## D5 — Rule 5.10 (required): reserved identifiers

- **Scope:** `_exit`, `_sbrk`, `_close`, `_fstat`, `_getpid`, `_isatty`,
  `_kill`, `_lseek`, `_read`, `_write` (newlib stubs); `__vector_*` (AVR
  interrupt handlers, named by avr-libc's `ISR()`); `__addr16`, `__result`,
  `__ToDo`, `__c` (locals inside avr-libc's `pgm_read_*`, `ATOMIC_BLOCK`, and
  `PSTR`).
- **Reason:** the toolchain requires these names, or defines them in its own
  macros.
- **Risk:** each name is the one the toolchain expects; ASDF declares no other
  reserved names.

## D6 — Rule 1.1 (required): identifiers longer than 31 characters

- **Scope:** `asdf_modifier_shiftlock_toggle_activate`,
  `asdf_modifier_shiftlock_on_activate`, `asdf_repeat_is_autorepeat_enabled`,
  `asdf_action_send_repeatable_code`.
- **Reason:** the names follow the `asdf_<module>_<thing>` pattern. C99
  guarantees only 31 significant characters in external identifiers.
- **Risk:** every toolchain ASDF supports (gcc, avr-gcc, arm-none-eabi-gcc)
  uses the full name, and no two identifiers match in their first 31
  characters, so Rule 5.1 holds.

## D7 — Rule 3.1 (required): `//` within a comment

- **Scope:** `@code` examples in Doxygen comments.
- **Reason:** the examples carry their own `//` comments, as the code they
  show would.
- **Risk:** the rule guards against a comment accidentally ending or nesting;
  a `//` inside a `/* */` comment does neither.

## D8 — Rule 11.5 (advisory): conversion from pointer to void

- **Scope:** platform callbacks, which receive the adapter as the `void *user`
  of `asdf_platform_t`.
- **Reason:** the core calls the platform without knowing the adapter's type;
  each adapter's callbacks convert `user` back to its own type.
- **Risk:** an adapter sets `user` to itself when it builds its platform, and
  only its own callbacks convert it.

## D9 — Directive 4.8 (advisory): complete type definitions

- **Scope:** `asdf_t` and the state types it contains.
- **Reason:** callers allocate keyboards (statically, with no heap), so the
  complete type must be visible.
- **Risk:** the headers document which functions operate on each type; no
  caller reaches into its members.

## D10 — Rule 7.1 (required): octal constant

- **Scope:** newlib's `S_IFCHR`, in the `_fstat` stub.
- **Reason:** a library macro; POSIX file modes are octal.
- **Risk:** the value is newlib's own.

## D11 — Directive 4.5 (advisory): typographically ambiguous names

- **Scope:** `ASCII_*` names in `asdf_ascii.h` (`ASCII_CTRL_L` and
  `ASCII_CTRL_I`, for example).
- **Reason:** the names are the standard ASCII control characters.
- **Risk:** each is defined once, with its code.

## D12 — Directive 4.6 and Rule 8.13 (advisory): basic types; const parameters

- **Scope:** the newlib system-call stubs, `main`, and `asdf_putc`.
- **Reason:** newlib declares the stubs' signatures, including `int` and
  non-`const` buffers; the C standard declares `main`; `asdf_putc` returns
  `int` with `EOF`, like `putc`.
- **Risk:** the signatures are the library's; the values stay in range.

## D13 — Rule 11.6 (required): conversion between integer and `void *`

- **Scope:** `FLASH_READ_PTR` on AVR.
- **Reason:** a pointer stored in flash is read back with `pgm_read_word`,
  which returns an integer.
- **Risk:** AVR data pointers are 16 bits, the width `pgm_read_word` reads.

## D14 — Directive 4.3 (required): assembly language encapsulated

- **Scope:** `asdf_keymap_descriptor`, `asdf_action_keymap_id`,
  `asdf_print_flash`, the AVR `asdf_arch_init`, and the PIC32CM
  `arch_delay_us`.
- **Reason:** these functions use avr-libc's flash, interrupt, and delay
  macros (`pgm_read_*`, `sei`, `_delay_*`) or CMSIS's `__NOP()`, which contain
  inline assembly. The library macros are the encapsulation the directive asks
  for; ASDF writes no assembly of its own.
- **Risk:** the functions are listed by name, so a new function that mixes in
  assembly is reported.

## D15 — Rule 10.5 (advisory): cast from an unsigned value to an enum

- **Scope:** loops over every value of an enum that ends in a count
  (`ASDF_MOD_NUM_MODIFIERS`, `ASDF_PHYSICAL_NUM_RESOURCES`), and action
  parameters that name an enum value (`KEY_VIRTUAL`'s virtual output). Each
  site carries `//lint !e9030 D15`.
- **Reason:** C has no loop over an enum's values; the counter is unsigned and
  is converted back for each value. Key parameters are stored as `uint8_t` in
  flash, whatever they mean.
- **Risk:** the loop bound is the enum's count, and each function that takes
  the enum checks its range.

## D16 — Rule 8.13 (advisory): pointer parameter could point to const

- **Scope:** functions whose signature is set by a function-pointer type:
  actions (`asdf_action_fn_t`) and platform callbacks (`asdf_platform_t`).
  Each site carries `//lint !e818 D16`.
- **Reason:** every action takes a modifiable keyboard, and every callback a
  modifiable `user` pointer, because some of them change it.
- **Risk:** none: the pointer is only less restricted than it could be.

## D17 — Rule 8.9 (advisory): object used by one function only

- **Scope:** the keymap descriptors (`*_keymap`), the action table
  (`asdf_action_table`), and the test adapter's platform
  (`asdf_arch_platform`).
- **Reason:** each is defined in its own module and used by a function in
  another (the keymap registry, `asdf_action`, the tests), so it cannot be
  moved into that function.
- **Risk:** none: the objects are `const` and in flash.

## D18 — Rules 14.2, 14.4, and 10.3 (required): avr-libc's interfaces

- **Scope:** uses of avr-libc's `ATOMIC_BLOCK` (Rules 14.2, 14.4), and calls
  to avr-libc's `_delay_us`, whose parameter is a `double` (Rule 10.3). Each
  site carries a `//lint` comment citing D18.
- **Reason:** `ATOMIC_BLOCK` is a `for` statement that runs its body once with
  interrupts disabled, restoring them on exit. `_delay_us` computes its delay
  at compile time from a floating-point argument, and needs a constant.
- **Risk:** both are avr-libc's documented interfaces. The delays are
  compile-time constants, so no floating-point code is generated.
