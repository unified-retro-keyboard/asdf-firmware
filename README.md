# ASDF Keyboard scanning firmware

*A Switch Dispatch Framework*

ASDF scans a key matrix, debounces keypresses and releases, and runs an action,
which could be to send an 8-bit code. The supported application is a parallel
ASCII keyboard. Serial, USB, or anything else is a `send_code` routine you
supply, or a loop that reads the codes (see "Using the keyboard in your own
program" below).

A keymap includes one matrix for each modifier state (plain, shift, caps, and
control), plus that keymap's outputs and options. The keymap definition matrices
are written in YAML under `src/Keymaps/`. The definitions are turned into C at
build time. A key names an action. Sending a code is one such action,
so a key can send any byte. Debounce time, repeat rate, and buffer sizes are
specified in `src/asdf_config.h`.

The scanner accepts up to 16 rows and 8 columns (`ASDF_MAX_ROWS` and
`ASDF_MAX_COLS` in `src/asdf.h`). A row is an `asdf_cols_t`, one bit per
column, so eight columns keeps the row a byte on an 8-bit machine. Wider rows
mean widening `asdf_cols_t` (for example to `uint16_t`) and raising
`ASDF_MAX_COLS` to match.


## Downloads

The latest downloads are available at:

- **Download page**: <https://unified-retro-keyboard.github.io/asdf-firmware/> — per-target `.hex` files and the release notes for every version.

- **GitHub Releases**: <https://github.com/unified-retro-keyboard/asdf-firmware/releases/latest> — the same `.hex` files plus `.elf` builds and a zipped copy of the docs site, attached as release assets.

## Supported keymaps:

The following keymaps may be selected via the DIP switches:

  * (0): ADM-style ASCII keyboard
  * (1): ADM-style ASCII keyboard (all caps)
  * (2): Apple 2 ASCII keyboard (upper/lower)
  * (3): Apple 2 ASCII keyboard (standard all caps)
  * (4): Sol-20 ASCII keyboard
  * (5): Franklin ACE 1000 keyboard (Thanks to Chris Ryu)


## Keyboard features

  * N-key rollover (Requires a diode per switch.  A diodeless keyboard is prone to ghosting.)
  * Per-key debouncing
  * Auto repeat and manual repeat.  Autorepeat may be enabled or disabled via DIP switch.
  * Positive or negative strobe polarity selection via DIP switch.
  * Keymap selection by DIP switch
  * Embeddable, so keycodes can be ingested by a larger application, or pushed onto other channels such as USB, Bluetooth, or Ethernet.

## Compiling and configuration

### Changing project name and version number.

- Edit the file "CMakeLists.txt"
- You will see a "project" section near the beginning of the file.

        project("asdf"
            VERSION 1.8.0
            DESCRIPTION "A customizable keyboard matrix controller for retrocomputers"
            LANGUAGES C)

- You can change the project name from "asdf" to whatever you like, and change the version number as you see fit.  These values will be used to name the resulting hex files, and also to name the download links in the GitHub page, if you choose to create one.

        project("my-keyboard"
            VERSION 1.0
            DESCRIPTION "My customized keyboard firmware"
            LANGUAGES C)

### building using github actions:

If you have commit privileges to the repository, or if you have your own fork,
then push a commit to `main`, or open a pull request against `main`, to trigger
an automatic build and test of every target.

Pushing a `vX.Y.Z` tag also triggers the release pipeline (documentation deploy and GitHub Release with built `.hex` / `.elf` assets).

The tag build generates a github page with downloadable hex files. You will find
the link to the github page in the "Actions" tab of the repository.

You will also need to activate GitHub pages.  To do this:

- Click "Settings" at the top of this GitHub page, then along the menu bar on the left, select "Pages" in the "Code and Actions Section."
- In the "Build and deployment" section, select "Deploy from Branch"
- The "Branch" section will display a message that github pages is disabled.  Select the branch "gh-pages" from the dropdown, and the "disabled" message will be replaced with a message that the site is being built from "gh-pages".  Once you have triggered a build, you will see a message at the top of this page with a link to the live page.

### build with CMake presets

The build needs CMake 3.25 or later, [uv](https://docs.astral.sh/uv/) (it runs
the key matrix generator), and the compiler for each target: `avr-gcc` for the
AVR firmware, `arm-none-eabi-gcc` for the PIC32CM PL10 firmware, and the host
`gcc` for the tests. The simavr tests also need `libsimavr-dev` and
`libelf-dev`.

Each target in `targets.csv` has a preset in `CMakePresets.json`, which builds
it in `build-<target>`. List them with:

        cmake --list-presets=all

A workflow preset configures, builds, and tests a target in one command:

        cmake --workflow --preset atmega2560

The steps can also be run one at a time:

        cmake --preset atmega2560
        cmake --build --preset atmega2560
        ctest --preset atmega2560

The presets are:

- **Firmware** (`atmega328p`, `atmega168p`, `atmega2560`, `atmega1280`,
  `atmega640`, `pic32cm_pl10_q64`, `pic32cm_pl10_dip28`): builds the firmware.
  Its test checks the flash and RAM use against the budget in
  `test/size-ceilings.csv`.
- **`test`**: the host unit tests.
- **`test-sanitize`**: the host unit tests under AddressSanitizer and
  UndefinedBehaviorSanitizer; any finding fails the test.
- **`test-coverage`**: the host unit tests with coverage, followed by a gcovr
  report of the portable core's coverage.
- **`simavr_test`**: runs the AVR firmware ELFs in simavr (see
  `test/simavr/README.md`). Build the AVR firmware first; a missing ELF fails
  that target's tests, and the error names the preset that builds it.

To install the hex files to `dist/`, and the AVR hex files and download links
to `docs/source/` for the documentation:

        cmake --install build-atmega2560

With `-DASDF_WERROR=ON`, or `ASDF_WERROR=1` in the environment, warnings are
errors, as in CI.

`size-report.sh` reports the flash and RAM use of every built firmware target,
and with `-b test/size-baseline.csv` the change from the baseline.

### build several targets at once

`make-targets.sh` runs the workflow preset for several targets:

        bash make-targets.sh -t atmega328p -t test   # the named presets
        bash make-targets.sh -a avr                  # every AVR target
        bash make-targets.sh -a                      # every target
        bash make-targets.sh -xia avr                # clean rebuild, then install
        bash make-targets.sh -l                      # list the targets

Run `bash make-targets.sh -h` for all the options.

### build the documentation

Install the hex files first (`bash make-targets.sh -ia avr`), then:

        cd docs
        uv run make html

## Porting

This firmware was written in modular, portable C99, to be compiled with GCC
(avr-gcc for the Atmega). The hardware-specific files are in Arch/*.[ch]. To
adapt the Atmega port for additional hardware, enter the ./src/Arch directory,
and copy the files asdf_arch_atmega2560.c and asdf_arch_atmega2560.h to new
filenames, and edit them to suit the hardware changes.

A non-AVR port already exists as a worked example: the `pic32cm_pl10_q64` and
`pic32cm_pl10_dip28` targets build the firmware for the Microchip PIC32CM PL10
(Arm Cortex-M0+, 5V). They reuse the same arch abstraction — `pic32cm_pl10_q64`
mirrors the atmega2560 (16 one-hot rows, parallel columns) and
`pic32cm_pl10_dip28` mirrors the atmega328p (encoded rows to a 74LS138, serial
shift-register columns). Shared ARM mechanics live in
`src/Arch/asdf_arch_pic32cm_common.{c,h}`; vendored device-support files are in
`src/third_party/cmsis/`. These targets are **build-verified only** (clean
`arm-none-eabi-gcc` cross-compile that fits the device flash/RAM); they have not
yet been validated on hardware or in an emulator. The core runs at 24 MHz (the
OSCHF internal-oscillator maximum; the PL10 flash is single-cycle, so no wait
states are needed). Building them requires `arm-none-eabi-gcc`.

The firmware runs from flash on a small microcontroller. Keymaps and the action
table are constant data. All of a keyboard's changeable state is held in one
keyboard object (`asdf_t`), so the core keeps no hidden state and multiple
independent keyboards can be maintained. The stock firmware uses one keyboard.
Another keyboard is another `asdf_t`, and it has to fit in the part's RAM.

The code favors readability over cleverness. Bit tests are plain shifts and
masks. I left the bithacks out because they were not faster for 8-bit values.

To port to a new processor architecture, you may use the atmega2560 files as an
example, and create a pair of architecture-specific .c and .h files for the new
hardware. They define a hardware state object, `asdf_arch_t`, which embeds the
keyboard's platform (`asdf_platform_t`, in `src/asdf_platform.h`), and:

- `asdf_arch_init(arch)`: initializes the CPU and hardware, fills in the
  platform, and starts a 1 ms tick interrupt.

- `asdf_arch_tick(arch)`: returns the number of 1 ms ticks since the last call.

- `ASDF_ARCH_TICK_ISR` and `asdf_arch_count_tick(arch)`: the tick interrupt
  vector, and the function it calls to count a tick. The application (main.c)
  defines the interrupt, since it owns the hardware state.

The platform's operations are what the keyboard needs from the hardware:

- `read_row`: output a row to the matrix, and read all the columns on that row.

- `send_code`: output a code to the computer, via serial, parallel, I2C, or
  whatever is appropriate.

- `set_output`: drive one of the physical outputs (LED1-3, OUT1-3, and the
  open-collector and open-emitter variants of OUT1-3) from the virtual output
  layer.

- `set_strobe_polarity`, `pulse_delay_short`, and `reset`: set the output strobe
  polarity, wait for a short output pulse (a few microseconds), and return the
  output configuration to its defaults when a keymap is selected.

## Using the keyboard in your own program

There are two ways to run the keyboard from an application:

- **The simple wrapper** (`src/asdf_simple.h`), for a program with a single
  keyboard that delivers the codes itself (USB, serial, and so on). It owns the
  hardware, the keyboard, and the tick interrupt, and has four calls:

        asdf_begin();
        while (1) {
          asdf_poll();
          while (asdf_available()) {
            send_somewhere(asdf_read());
          }
        }

- **Keyboard objects** (`src/asdf_keyboard.h`), for anything else: more than one
  keyboard, a custom platform, or code sent through the platform. The
  application owns each `asdf_t` and its hardware, and passes it to each call:
  `asdf_init()` once, then `asdf_process()` (which sends codes through the
  platform) or `asdf_update()` (which leaves them queued for
  `asdf_next_code()`) with the elapsed ticks. `src/main.c` is an example.
