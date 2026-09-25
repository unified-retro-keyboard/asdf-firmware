ASDF Firmware Release Notes
===========================

Version 1.8.0 (Release)
-----------------------

This release pays down accumulated technical debt.

Highlights
~~~~~~~~~~

- Factored out all the local state into a keyboard object (``asdf_t``),
  allowing multiple keyboards to run independently.

- Added a simple wrapper for an application to instantiate and run a single
  keyboard, with codes sent to the client application, for example, an arduino
  implementing a keyboard over USB, BT, or ethernet.

- Keyboard processing doesn't block. Scanning can continue while output buffers
  are drained and timed I/O events are playing out.

- Keymaps are now implemented as a tuple of {keypress_function, keypress_param,
  release_function, release_param} to provide greater keymap flexibility
  without significantly more complexity. This fixes a few longstanding issues:

  - No reserved codes for functions, so all 8-bit codes can be generated. User
    function hooks are no longer required.

  - The keymap can insert user functions directly into the function table. So
    the unsafe and undefined behavior pointer casts can be eliminated.

  - Press and Release actions are no longer tightly coupled. They can be
    independently specified. This permits richer behavior.

  - Parameters attached to functions pave the way for key-based keyboard
    settings, to permit reuse of functions like ``send_code()``, and also
    permitting key-based configuration, which can eventually replace or augment
    the DIP switch settings.

- Cleaner keymap configuration via a YAML file makes modifying and creating
  keymaps easier.

- Replaced older block-style headers with modern Doxygen headers, improving
  readability.

- The code is checked with PC-lint Plus against MISRA C:2025, with each
  deviation recorded in ``lint/MISRA-DEVIATIONS.md``.

Details
~~~~~~~

- *Bug Fix*: Keymap switches no longer replay held keys. Only configuration
  switches (keymap select, strobe polarity, autorepeat) are re-applied, and the
  switch is deferred to the end of the scan.
- *Bug Fix*: REPEAT pressed during autorepeat now keeps repeating.
- *Bug Fix*: At startup, the lowest-numbered keymap that exists is selected,
  rather than assuming keymap 0.
- *Bug Fix*: A "No Action" key no longer queues a code.
- *Bug Fix*: The Applesoft test key now works in ``apple2_caps``, as it does in
  ``apple2``.
- *Bug Fix*: The AVR firmware now sets the clock prescaler to 1. The old code
  wrote CLKPR in a way the hardware ignores, so a board with the CKDIV8 fuse
  programmed ran every delay and tick 8x slow.
- *Feature*: Any code 0x00–0xFF can be sent. ``KEY_SEND`` repeats,
  ``KEY_SEND_ONCE`` sends once, and any action can opt into autorepeat.
- *Feature*: ``asdf_dropped_codes()`` and ``asdf_dropped_messages()`` report
  queue overflows; ``asdf_keymap_errors()`` reports keymap entries that could
  not be applied.
- *Feature*: The typed platform interface (``asdf_platform_t``) replaces the
  cast hook table.
- *Feature*: Messages print from flash; nanoprintf is dropped.
- *Feature*: Predicates and flags are ``bool``, and the platform's
  ``set_strobe_polarity`` callback takes a ``bool``. Platform adapters and
  applications written for earlier versions need the same change.
- *Build*: ATmega88P dropped: four-byte keys no longer fit its 8 KB flash. The
  ATmega328P, 168P, 2560, 1280 and 640 remain.
- *Build*: Builds now need `uv <https://docs.astral.sh/uv/>`_ to run the keymap
  generator. ``pyproject.toml`` and ``uv.lock`` replace the Pipfile.
- *Build*: CMake workflow presets configure, build and test each target;
  ``make-targets.sh`` and CI use them. CMake 3.25 or later is required.
- *Build*: Firmware builds check per-target flash and RAM budgets.
- *Build*: Pedantic, shadow and conversion warnings are enabled, and CI treats
  warnings as errors.
- *Build*: PC-lint Plus lint presets for the core (``lint``) and each firmware
  (``lint-<target>``), with each target's compiler configuration; they need a
  licensed PC-lint Plus.
- *Build*: Removed the unused OSI 542 row reader (``asdf_arch_osi_read_row``)
  from the ATmega2560 and PIC32CM Q64 adapters.
- *Build*: The lint presets check MISRA C:2025 (``lint/misra.lnt``);
  deviations are recorded in ``lint/MISRA-DEVIATIONS.md``.
- *Test*: Host tests also run under AddressSanitizer and
  UndefinedBehaviorSanitizer, and CI reports line coverage of the core.
- *Test*: Randomized key event tests check state invariants on three
  interleaved keyboards.
- *Test*: Production keymaps are built and checked on the host.


Version 1.7.1 (Release)
-----------------------

This release fixes mis-mapped keys in the Sol-20 and Apple II keymaps.  If you are running 1.7.0 and use one of these keymaps, you should update to 1.7.1.

If you are using any version of the software before 1.7.0, you are strongly encouraged to update to 1.7.1


Highlights
~~~~~~~~~~

- *Bug Fix*: On the Sol-20 keymap, SHIFT and SHIFT LOCK now emit the upper legend for the ``[``, ``\``, ``]``, ``;`` and ``:`` keys.
- *Bug Fix*: On the Apple II keymap, the REPEAT key now repeats instead of emitting a character.

Details
~~~~~~~

- *Bug Fix*: ``sol_shift_map`` held the unshifted codes for five bit-paired
  punctuation keys, so SHIFT and SHIFT LOCK emitted ``[ \ ] ; :`` instead of ``{
  | } + *``.  Long running bug affecting all released versions.
- *Bug Fix*: ``apple_plain_matrix`` and ``apple_shift_matrix`` carried ``^`` and
  ``@`` at the REPEAT key position, so on the ``apple2`` keymap REPEAT emitted a
  character and only repeated with CAPS LOCK on or CTRL held. The
  ``apple2_caps`` keymap was unaffected.

Version 1.7.0 (Release)
-----------------------

This release includes fixes for virtual outputs affecting multiple keymaps.

Highlights
~~~~~~~~~~

- *Bug Fix*: On keymap switching, ensure virtual outputs start in the correct state
- *Feature*: Added integration testing based on simavr.

Details
~~~~~~~

- *Bug Fix*: ``asdf_keymaps_switch()`` now reapplies virtual outputs for the new map; added tests for the keymap switching path.
- *Bug Fix*: Fixed a ``printf`` format issue in the keymap code.
- *Feature*: Added matrix and DIP-switch injection, output capture, VCD trace capture, and per-keymap simavr tests.
- *Build*: Split CI into firmware build, host unit test, integration test, documentation, and release jobs.
- *Build*: Added portable simavr/libelf detection and CI install steps for ``libelf-dev``.
- *Build*: Added ``version.sh`` so scripts and CI read the project version from ``CMakeLists.txt``.
- *Build*: Tag builds verify that ``vX.Y.Z`` matches the CMake project version before publishing release assets.

Version 1.6.6 (Interim)
-----------------------

Highlights
~~~~~~~~~~

- *Bug Fix*: Fixed a serious keymap lookup bug that could cause Sol-20 and Franklin keymaps to fail
- *Feature*: Added firmware support for Franklin ACE 1000 replacement keyboards.
- *Feature*: Added a configurable row-scanner hook for new keyboard hardware.

Details
~~~~~~~

- *Bug Fix*: Fixed keymap lookup against the wrong modifier index in
  ``keymaps.c:get_codes()`` that could cause incorrect keymappings, and could cause
  keymaps with more rows or higher keymap numbers (Sol-20 or Franklin Ace) to
  crash.
- *Feature*: Added the Franklin ACE 1000 keymap and updated the README feature list.
- *Feature*: Added ``asdf_arch_set_row_scanner()`` and default row scanner declarations in the architecture headers.
- *Build*: Removed unused row/column list plumbing and an unused keymap helper.

Version 1.6.5 (Release: 2023-01-02)
-----------------------------------

Highlights
~~~~~~~~~~

- *Feature*: Added firmware builds for additional AVR targets.
- *Feature*: Added Videx-style Apple key bindings and self-test strings.
- *Build*: Streamlined multi-target firmware builds.

Details
~~~~~~~

- *Feature*: Added support for ATmega88P, ATmega168P, ATmega640, and ATmega1280 targets.
- *Feature*: Added a user-bindable function for printing ASCII strings from keymaps.
- *Feature*: Added Videx-style Apple bindings, Apple keymap character tests, and an Applesoft keyword test sequence.
- *Bug Fix*: Corrected Videx key mappings and Apple test program bindings.
- *Build*: Reworked CMake target factor out processor family from the MCU.
- *Build*: Fixed install directory handling and ``make-targets.sh`` issues.

Version 1.6.4 (Release: 2022-12-27)
-----------------------------------

Highlights
~~~~~~~~~~

- *Build*: Added automated firmware builds and documentation publishing.
- *Build*: Added Sphinx documentation with firmware download links.
- *Bug Fix*: Fixed build issues with newer GCC versions.

Details
~~~~~~~

- *Build*: Added the firmware GitHub Actions workflow and GitHub Pages deployment support.
- *Build*: Improved ``make-targets.sh`` and the firmware README build instructions.
- *Build*: Added generated documentation index files for versioned firmware downloads.
- *Bug Fix*: Fixed a GCC 11.3 compiler warning path.
- *Bug Fix*: Added missing keymap setup files and missing keymap headers needed by tests and builds.

Version 1.6.3 (Release: 2021-12-05)
-----------------------------------

Highlights
~~~~~~~~~~

- *Bug Fix*: Fixed slow repeat behavior on some keys.
- *Feature*: Added keyboard identification and self-test messages.
- *Feature*: The Sol-20 layout can send its ID message with Control-0.

Details
~~~~~~~

- *Feature*: Added configurable output rate for generated messages.
- *Feature*: Added ``asdf_arch_delay_ms()`` for millisecond delays.
- *Feature*: Added Apple II keyboard ID and test message bindings.
- *Feature*: Bound the Sol-20 ID message to Control-0.
- *Bug Fix*: A bit test optimization was causing slow key repeat on some matrix
  columns. Removed.
- *Bug Fix*: Fixed the ATmega2560 millisecond delay routine.
- *Bug Fix*: ``asdf_buffer_get()`` fixed to validate the buffer handle before use.

Version 1.6.2 (Release: 2021-11-29)
-----------------------------------

Highlights
~~~~~~~~~~

- *Bug Fix*: Fixed Apple II CAPS power LED behavior.
- *Feature*: Added Apple II CAPS and printing behavior.
- *Bug Fix*: Fixed startup problems from the keymap rewrite.

Details
~~~~~~~

- *Feature*: Added Apple II CAPS map behavior and related printing support.
- *Bug Fix*: Fixed the Apple CAPS power LED so it stays on.
- *Bug Fix*: Initialized buffers before other firmware subsystems.
- *Bug Fix*: Fixed extra indirection and library-related indirection errors.
- *Build*: Added CMake templates for generated keymap setup files.

Version 1.6.1 (Release: 2021-11-28)
-----------------------------------

Highlights
~~~~~~~~~~

- *Feature*: Restored the Apple II upper/lowercase layout.
- *Bug Fix*: Cleaned up initialization after the keymap rewrite.

Details
~~~~~~~

- *Feature*: Added back the Apple II upper/lowercase map after the keymap scheme change.
- *Bug Fix*: Removed redundant reset code and unused declarations.
- *Bug Fix*: Removed ``arch_init`` from ``main()``.
- *Build*: Updated generated keymap setup handling for the new layout.

Version 1.6.0 (Release: 2021-11-28)
-----------------------------------

Highlights
~~~~~~~~~~

- *Feature*: Reworked keymap definitions for easier layout maintenance.
- *Build*: Added generated setup files for keyboard layouts.
- *Build*: Updated the tests for the new layout system.

Details
~~~~~~~

- *Feature*: Replaced the old keymap table setup with per-keymap registration files.
- *Feature*: Added generated ``NUM_KEYMAPS`` and keymap table setup.
- *Build*: Fixed test CMake files and compile flags for the new scheme.
- *Bug Fix*: Fixed Sol map comments and keymap initialization cleanup items found during the conversion.

Version 1.5.1 (Release: 2021-11-08)
-----------------------------------

Highlights
~~~~~~~~~~

- *Bug Fix*: Fixed an ATmega328P output bug where OUT2 changed OUT1.
- *Build*: Cleaned up firmware file names and test build settings.

Details
~~~~~~~

- *Bug Fix*: Corrected the ATmega328P OUT2 output path, which was changing OUT1.
- *Build*: Changed build artifact names to place the version before the architecture.
- *Build*: Fixed CMake test files and C compile flags.

Version 1.5 (Release: 2021-03-04)
---------------------------------

Highlights
~~~~~~~~~~

- *Feature*: First tagged ASDF firmware release, with selectable layouts for ADM-style ASCII, Apple II, and Sol-20 keyboards.
- *Feature*: Added the virtual output layer for LEDs and TTL output signals.
- *Build*: Replaced the Makefile build flow with CMake.
- *Feature*: Added firmware support for ATmega2560 controllers and early OSI 542 scanning.

Details
~~~~~~~

- *Feature*: Added runtime keymap switching through DIP-switch selection.
- *Feature*: Added per-key debounce counters, repeat, autorepeat, and modifier handling.
- *Feature*: Added Apple II and Sol-20 keymaps, ASCII NULL, Shift-RESET clear screen, and Caps Lock naming cleanup.
- *Feature*: Added virtual LEDs, virtual outputs, hi-z-when-low outputs, and long/short pulse outputs.
- *Feature*: Split physical resources into ``asdf_physical.[ch]`` and added hooks for scanner, output, and initialization behavior.
- *Feature*: Added ATmega2560 architecture files and initial OSI 542 keyboard scan support.
- *Bug Fix*: Reinitialized repeat state and reset modifiers on keymap switch.
- *Bug Fix*: Fixed ATmega2560 row settling time and corrected several Sol-20 signal assignments.
- *Bug Fix*: Fixed long-delay handling so millisecond delays use ``_delay_ms()``.
- *Build*: Replaced the Makefile-only build with CMake.
- *Build*: Added unit tests for multiple keymaps and virtual outputs.
