# ASDF Keyboard scanning firmware

This is a key matrix scanner that can detect and debounce keypress and release
events on a key matrix and either send codes or perform actions on keypress or
release. Keymaps are defined per application and may, for example, generate
ASCII codes, special keyscan codes, etc. The code is modular and may be
integrated into a larger system easily.

By default, the code supports any number of rows by 8 columns, which will give
the bestperformance on an 8-bit microcontroller. For more than 8 columns per
row, the row datatype would need to be changed to uint16_t to support 16
columns, etc.

The first supported application is a parallel ASCII output keyboard. If you want
serial or USB output, you can supply your own routines.

ASDF supports basic keyboard functionality and is configurable via a few
boolean variables, and via the key maps. The key maps are organized in
row,column format, with separate keymaps shift, capslock, and control-key modes.

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

  * N-key rollover
  * Debouncing
  * Auto repeat and manual repeat.  Autorepeat may be enabled or diabled via DIP switch.
  * Positive or negative strobe polarity selection via DIP switch.

## Compiling and configuration

### Changing project name and version number.

- Edit the file "CMakeLists.txt"
- You will see a "project" section near the beginning of the file.

        project("asdf"
            VERSION 1.7.1
            DESCRIPTION "A customizable keyboard matrix controller for retrocomputers"
            LANGUAGES C)

- You can change the project name from "asdf" to whatever you like, and change the version number as you see fit.  These values will be used to name the resulting hex files, and also to name the download links in the GitHub page, if you choose to create one.

        project("my-keyboard"
            VERSION 1.0
            DESCRIPTION "My customized keyboard firmware"
            LANGUAGES C)

### building using github actions:

If you have commit privileges to the repository, or if you have your own fork,
then push a commit to one of the following branches to trigger an automatic build:

- main
- asdf-build-test

Pushing a `vX.Y.Z` tag also triggers the release pipeline (documentation deploy and GitHub Release with built `.hex` / `.elf` assets).

This will generate a github page with downloadable hex files. You will find the
link to the github page in the "Actions" tab of the repository.

You will also need to activate GitHub pages.  To do this:

- Click "Settings" at the top of this GitHub page, then along the menu bar on the left, select "Pages" in the "Code and Actions Section."
- In the "Build and deployment" section, select "Deploy from Branch"
- The "Branch" section will display a message that github pages is disabled.  Select the branch "gh-pages" from the dropdown, and the "disabled" message will be replaced with a message that the site is being built from "gh-pages".  Once you have triggered a build, you will see a message at the top of this page with a link to the live page.
-

### build using the make-build-dirs.sh script.


1) Run the make-targets.sh script

    Options:

            -x   Before creating a build directgory or virtual env, remove
                 any pre-existing version
            -t   add an architecture directory
            -a   Add all valid architecture directories
            -i   Build each specified target and install to dist directory
            -p   Install pipenv virtual environment for python scripts
            -c   Clean all artifacts
            -s   Copy dist files to sphinx directory

    Valid targets: atmega168p, atmega328p, atmega640, atmega1280, atmega2560, pic32cm_pl10_q64, pic32cm_pl10_dip28, test, simavr_test

    (`test` runs the host-side Unity unit tests; `simavr_test` runs
    simavr-driven integration tests against the built AVR ELFs.)

    - To create build directories for all targets and install the python virtual
      environment:

            bash make-targets.sh -ap

    - To create a a build directory for atmega1280, deleting any pre-existing directory:

            bash make-targets.sh -xt atmega2560

    - To remove and rebuild the python virtual environment:

            bash make-targets.sh -xp

    - To copy hex files to sphinx source tree (Requires the hex files
      to be installed in ./dist either from make install in each target
      directory, or 'bash make-targets.sh -ai')

            bash make-targets.sh -s


    - From a fresh checkout, build all targets and install hex files in
      sphinx tree for the download links:

            bash make-targets.sh -pais

2) Enter the build directory for the desired architecture and build:
   Only needed if working on single target.  To make all targets at once,
   use the make-targest script described in step 1.

   Example: building the atmega2560 binary:

         cd build-atmega2560
         make

3) Build the sphinx documentation:

         cd docs
         pipenv run make html

### build manually (e.g., for development)

1) make build directories for the desired architectures:

        mkdir build-atmega328p build-atmega2560

2) enter each build directory and run cmake for the desired architecture.

        cd build-atmega2560
        cmake .. -DARCH=atmega2560 -DCMAKE_BUILD_TYPE=RELEASE
        make

3) to run unit tests, the process is the same as above, with "test" as the
   target:

        mkdir build-test
        cd build-test
        cmake .. -DARCH=test
        make && ctest

   The host tests can also be built with checks:

   - `-DASDF_SANITIZE=ON`: run under AddressSanitizer and
     UndefinedBehaviorSanitizer; any finding fails the test.
   - `-DASDF_COVERAGE=ON`: record coverage. After running the tests, report
     the portable core's coverage with
     `uvx gcovr --root . --filter src/ --exclude src/Arch/ --exclude src/Keymaps/ build-test`.
   - `-DASDF_WERROR=ON` (any build): make warnings errors, as CI does.

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

The firmware is designed to run from ROM on a slow vintage processor, with a
small RAM footprint. All of a keyboard's changeable state is held in one
keyboard object (`asdf_t`), so the core keeps no hidden state and any number of
keyboards can run independently. It is designed to compile on small
architectures, or to be hand-translated to assembly on small processors, or to
an HDL for a CPLD or FPGA.

The code was written to favor readability over cleverness. While tempted to
optimize bit testing via bithacks, I opted for code simplicity since the
performance benefit was not there for 8-bit values.

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
