# PIC32CM PL10 port of the ASDF keyboard firmware

## Background and motivation

The ASDF firmware currently targets only 8-bit AVR parts. All hardware
dependencies are isolated behind the `asdf_arch_*` API: `src/CMakeLists.txt`
copies `src/Arch/asdf_arch_${ARCH_FAMILY}.{c,h}` into `asdf_arch.{c,h}` at
configure time, and the core (`asdf.c`, the keymaps, buffer, modifiers,
`main.c`) touches hardware only through that header. Adding a target is a matter
of supplying a new arch implementation plus its toolchain wiring, with no core
changes.

This spec describes a port to the **Microchip PIC32CM PL10** family — a 2026
Arm Cortex-M0+ line that is 5V (1.8–5.5 V), pin-to-pin compatible with AVR MCUs,
available in SPDIP-28, and built on the same SAM-lineage peripheral architecture
(PORT/GCLK/SERCOM/EVSYS/PTC) as the SAM C20. It is the firmware's first non-AVR
target, so it also introduces the ARM build infrastructure (CMake toolchain
file, linker script, startup/vector table, vendored CMSIS + DFP headers) the
repository has never carried.

The PL10 was chosen over the SAM C20 because it aligns with ASDF's retrocomputer
audience: an AVR-pin-compatible, 5V, SPDIP-28 M0+ can socket onto existing 8-bit
ASDF boards, it is a current part with long-term support, and its Peripheral
Touch Controller is a future option for touch keys. (SAM C20 is retained only as
a rejected alternative; see Decisions.)

### Two design points, two existing references

The port is a **family with two variants**, each a faithful re-expression of an
existing AVR arch's scan/read logic on the PIC32CM PORT register model. The
external keyboard-support hardware is unchanged in both cases, so the scan/read
*operations* are identical to the AVR reference — only the MCU pin-I/O
primitives, clock/tick, and toolchain differ.

- **Variant A — SPDIP-28, "328p-class."** Reference: `asdf_arch_atmega328p`.
  Drops into the 328P socket with some PCB modification; **the support chips stay
  — the 74LS138 row decoder and the column shift register.** Rows are driven as a
  4-bit *encoded* value into the '138 decoder; columns are read *serially*
  through the shift register (a column-data input plus `COLCLK`/`COLMODE`); ASCII
  is an 8-bit parallel output. This serial/encoded design is what fits a 28-pin
  package.

- **Variant B — 48/64-pin, "2560-class."** Reference: `asdf_arch_atmega2560`.
  Drops into the atmega2560 design with a bit of rework. Rows are 16
  directly-driven one-hot active-low lines; columns are read as an 8-bit parallel
  field (inverted); ASCII is 8-bit parallel. Includes the OSI-keyboard scanner
  variant (`osi_read_row` with the KBE/RW handshake) for full 2560 parity.

## Goals

- A new arch family with two complete implementations —
  `asdf_arch_pic32cm_pl10_dip28.{c,h}` (Variant A) and
  `asdf_arch_pic32cm_pl10_q64.{c,h}` (Variant B) — each implementing the full
  `asdf_arch_*` contract and reproducing its AVR reference's scan/read logic
  verbatim on the PIC32CM PORT register model.
- ARM build infrastructure that coexists with the AVR builds without disturbing
  them: `ARCH=pic32cm_pl10_dip28` / `ARCH=pic32cm_pl10_q64` select an
  `arm-none-eabi-gcc` toolchain and produce `asdf-vX.Y.Z-<arch>.{elf,bin,hex}`.
- Self-contained builds: PIC32CM PL10 device-support files are vendored, so a
  clean checkout builds with only `arm-none-eabi-gcc` installed.
- The core and all existing AVR targets build and behave exactly as before.

## Non-goals

- **Hardware validation.** No PL10 hardware is available. The verification
  ceiling is a clean `arm-none-eabi-gcc` build producing artifacts that fit the
  device memory map. Peripheral logic is written to be datasheet-correct but is
  not exercised on silicon. (See "Intended verification roadmap" for the planned
  behavioral coverage on backplane.)
- **simavr integration tests.** simavr is AVR-only; the existing `test/simavr/`
  stage cannot cover a Cortex-M0+ target and is out of scope. The host
  unit-test stage (`ARCH=test`) is unaffected and continues to pass.
- **Touch / PTC, CCL, EVSYS, MVIO.** The PL10's touch controller and 8-bit-style
  peripherals are noted as future options; the base port uses none of them.
- **A vendor HAL.** No ASF4/START dependency. Direct CMSIS register access,
  matching the bare-register style of every existing arch file.
- **Bootloader / flashing tooling.** The build emits standard artifacts and the
  docs note the flashing path (SWD via OpenOCD, or a SAM-BA/UF2 bootloader if
  present); no flashing is performed or scripted here.
- **A second board respin.** This work targets the existing 328P and atmega2560
  board topologies (support chips retained); new PCB layout is out of scope.

## Decisions

| Decision | Choice |
|---|---|
| Target family | Microchip PIC32CM PL10 (Arm Cortex-M0+, 5V, SAM-lineage peripherals) |
| Variant A | SPDIP-28, 328p-class — ref `asdf_arch_atmega328p`; 74LS138 + shift register retained |
| Variant B | 48/64-pin (e.g. PIC32CM6408PL10048), 2560-class — ref `asdf_arch_atmega2560`; OSI variant included |
| Rejected | SAM C20 (older 2015 part, no DIP/AVR-pin story); single-variant scope |
| Peripheral access | Direct CMSIS register access, no HAL |
| Device-support files | Vendored under `src/third_party/cmsis/` (CMSIS-Core + PIC32CM PL10 DFP) |
| Clock | Internal oscillator → GCLK0; `F_CPU` pinned at implementation from the PL10 datasheet (all timing derives from `F_CPU`, so the value is non-critical) |
| Tick source | SysTick, 1 ms |
| Memory (per current parts) | ≤128 KB flash, ≤16 KB SRAM — ample; ASDF fits a 32 KB AVR |

## Architecture

### Layering (unchanged)

The configure-time copy in `src/CMakeLists.txt:46-47` keys on `${ARCH_FAMILY}`,
so each variant is selected by setting `ARCH_FAMILY` to its arch-source basename.
Two `ARCH` targets map to two `ARCH_FAMILY` values map to two arch source pairs,
mirroring the existing `atmega328p` vs `atmega2560` split.

### Shared ARM mechanics (both variants)

- **GPIO model.** AVR `PORT/DDR/PIN/BIT` macros are replaced by a `(group, bit)`
  model over the PIC32CM PORT peripheral (`Group[0]` = PA, `Group[1]` = PB), with
  `static inline` helpers `pin_set/pin_clear/pin_read/pin_dir_out/pin_dir_in`
  over `OUTSET/OUTCLR/IN/DIRSET/DIRCLR` + `PINCFG[].INEN`, plus masked
  whole-field writes for the multi-bit row/column/ASCII fields. These are the
  PIC32CM analogue of the AVR file's `set_bit`/`clear_bit`.
- **Tick.** SysTick at a 1 ms period (`reload = F_CPU/1000 − 1`);
  `SysTick_Handler` sets a `volatile` flag that `asdf_arch_tick()` drains — the
  same pattern as the AVR Timer tick.
- **Clock.** `asdf_arch_init_clock()` brings the internal oscillator onto GCLK0;
  `F_CPU` is set to the selected frequency. Exact oscillator/GCLK field names are
  confirmed against the vendored PL10 header during implementation.
- **Delays.** `delay_ms` and the µs strobe/pulse/settling delays use a calibrated
  busy-loop derived from `F_CPU` (Cortex-M0+ has no DWT cycle counter).
- **Flash / PROGMEM.** Cortex-M flash is directly addressable, so `#define FLASH`
  is empty and `FLASH_READ_MATRIX_ELEMENT(m,r,c)` is `(m)[r][c]`. The `<avr/*>`
  includes are dropped for the vendored PIC32CM header.

### Variant A — SPDIP-28, 328p-class

The scan/read logic is copied from `asdf_arch_atmega328p.c` unchanged in
structure; only the pin-I/O primitives become PIC32CM register accesses.

- **Rows:** 4-bit encoded value written to a contiguous row-select field, decoded
  externally by the retained **74LS138(s)** into one-hot row lines.
- **Columns:** serial read through the retained **shift register** — a single
  column-data input sampled while pulsing `COLCLK`, with `COLMODE` selecting the
  shift/load behavior, exactly as the 328P arch clocks it.
- **ASCII / strobe / LEDs / OUTs:** as the 328P arch.

Representative pin map (adjustable; constrained to SPDIP-28-bonded, 5V-capable
PL10 pins and to keeping the support-chip wiring sane):

| Function | Pin(s) | Notes |
|---|---|---|
| Row select (4, encoded → '138) | PA00–PA03 | feeds external decoder |
| Column data in (serial) | PA04 | from shift register |
| COLCLK / COLMODE | PA05 / PA06 | shift-register control |
| ASCII out (8) | PA08–PA15 | 8-bit parallel |
| Strobe | PA07 | polarity configurable |
| LED1 / LED2 / LED3 | PA16 / PA17 / PA18 | |
| OUT1 / OUT2 / OUT3 | PA19 / PA20 / PA21 | open-drain variants supported |
| Reserved | SWCLK / SWDIO | per PL10 SPDIP pinout |

### Variant B — 48/64-pin, 2560-class

The scan/read logic is copied from `asdf_arch_atmega2560.c` unchanged in
structure. The 32-bit PORT lets all 16 row lines live on one contiguous field
driven in a **single masked `OUT` write** (the SAM/PIC32CM simplification over
the 2560's LOROW/HIROW split).

- **Rows:** 16 one-hot active-low lines on `PA00–PA15`; `~(1u<<row)` masked write.
- **Columns:** 8-bit parallel field, inverted on read.
- **OSI variant:** `osi_read_row` drives the KBE/RW handshake and switches the
  column pins to outputs for rows 0–7, deferring to the standard scanner above
  row 7. The OSI control lines **intentionally overlay the upper row-select
  lines** (KBE on the row field's bit-9 line, RW on bit-10), exactly as on the
  2560.
- **ASCII / strobe / LEDs / OUTs:** as the 2560 arch.

Representative pin map (adjustable):

| Function | Pin(s) | Notes |
|---|---|---|
| Row out (16, one-hot active-low) | PA00–PA15 | single 16-bit masked write |
| Column in (8, inverted) | PB00–PB07 | press pulls low |
| ASCII out (8) | PA16–PA23 | |
| Strobe | PA24 | |
| LED1 / LED2 / LED3 | PB08 / PB09 / PB10 | |
| OUT1 / OUT2 / OUT3 | PB11 / PB12 / PB13 | open-drain variants supported |
| OSI_KBE / OSI_RW | PA09 / PA10 | overlay upper row lines (2560 parity) |
| Reserved | SWCLK / SWDIO | per PL10 64-pin pinout |

### Toolchain and build

- **`cmake/generic-gcc-arm.cmake`** mirrors the *contract* of
  `generic-gcc-avr.cmake`: sets `arm-none-eabi-gcc`, `CMAKE_SYSTEM_NAME Generic`,
  `CMAKE_SYSTEM_PROCESSOR arm`; **defines `c_toolchain_flags()`** (called
  unconditionally at `src/CMakeLists.txt:52`) populating `CFLAGS` with
  `-mcpu=cortex-m0plus -mthumb` + warning/opt flags; supplies the link line
  (`-T <linker script> --specs=nano.specs --specs=nosys.specs
  -Wl,--gc-sections`) and a `POST_BUILD` `arm-none-eabi-objcopy` (→ `.bin`/`.hex`)
  + `arm-none-eabi-size` step. Shared by both variants.
- **Top-level `CMakeLists.txt`** gains two additive branches (alongside the AVR
  branches) — `elseif(ARCH MATCHES pic32cm_pl10_dip28)` and
  `elseif(ARCH MATCHES pic32cm_pl10_q64)` — each setting its `ARCH_FAMILY`,
  `ARCH_TYPE=ARM`, the toolchain file, and its per-variant `LINKER_SCRIPT`. No
  AVR branch is touched; each target builds in its own `build-<arch>` directory
  with a per-directory cached toolchain, so AVR is wholly unaffected.
- The AVR-only `custom_add_executable` wrapper is not defined for ARM, so the ARM
  build uses plain `add_executable`; linker script + objcopy apply via the ARM
  `POST_BUILD` path.
- **`make-targets.sh`** gains `add_valid_target pic32cm_pl10_dip28` and
  `add_valid_target pic32cm_pl10_q64`.

### Vendored device-support files

Under `src/third_party/cmsis/` (consistent with the vendored `nanoprintf` and
`unity`) — **vendored as of Task 1**, `Microchip.PIC32CM-PL_DFP` 1.4.418 + ARM
CMSIS_5 5.9.0:

- CMSIS-Core headers (`core_cm0plus.h` + `cmsis_gcc.h`/`cmsis_compiler.h`/
  `cmsis_version.h`/`mpu_armv7.h`);
- the `pic32cm6408pl` family device headers from the DFP (`pic32c.h`, the four
  package part headers, `system_*.h`, and the `component/`/`instance/`/`pio/`
  trees). `pic32c.h` selects the part header from `__PIC32CM6408PL100NN__`,
  which each CMake branch passes via `-D`;
- **per-part** `startup_*.c` (vector table + reset handler) and `system_*.c`
  (`SystemInit`), DFP-provided, **unmodified** — selected per variant
  (`...10064` for q64, `...10028` for dip28);
- the DFP per-part linker scripts `pic32cm6408pl100{64,28}_flash.ld`, used
  **unmodified**. Device memory map: flash @ **`0x0C000000`** (note: non-zero
  PIC32CM flash base, not `0x00000000`), SRAM @ `0x20000000`, 64 KB / 8 KB.

Provenance + license + part→variant mapping are recorded in
`src/third_party/cmsis/README.md`.

### Programming / output

Each target produces `asdf-vX.Y.Z-<arch>.{elf,bin,hex}`. Documentation notes the
flashing path (OpenOCD/SWD, or a SAM-BA/UF2 bootloader if present). Flashing is
not scripted or performed in this work.

## Testing and verification

- **Host unit tests (`ARCH=test`)** — unchanged; must still pass.
- **simavr integration tests** — out of scope (AVR-only); unchanged.
- **PL10 build verification (per variant)** — a clean `ARCH=pic32cm_pl10_dip28`
  and `ARCH=pic32cm_pl10_q64` configure+build with `arm-none-eabi-gcc` completes
  without errors and produces the three artifacts each; the `size` report
  confirms each image fits its part's flash/SRAM. This is the acceptance
  criterion for the port.
- **AVR regression** — `atmega328p` and `atmega2560` still build cleanly,
  confirming the toolchain-selection changes did not disturb the existing path.
- **CI** — adding the two PL10 targets to the GitHub Actions matrix is deferred
  (requires `gcc-arm-none-eabi` on the runner) and tracked as a follow-up.

### Intended verification roadmap (backplane)

Once the firmware builds clean, the planned path to behavioral coverage mirrors
what `test/simavr/` does for AVR, but on **backplane** (`~/vsrc/backplane`) —
Dave's agent-first machine-hosting emulator that composes machines from modules
wired via views onto a shared backing store, driven over REST through its Python
SDK, with deterministic execution, watchpoints, and forking. backplane's 6850
ACIA echo example is structurally identical to a matrix-scanner test: the
scanner's pins become store cells, the client drives column inputs and
watchpoints the strobe line, runs the loaded firmware ELF, and reads back the
ASCII byte.

Because a 2026 part has no third-party emulator (Renode has no PL10 platform),
backplane — not Renode — is the intended destination harness for the ARM target.
Prerequisites live in the backplane repo as their own spec→plan cycles, **out of
scope for this port**:

- an **ARM Cortex-M0+ core module** (likely harnessing an existing M0+ core, as
  the 6502 modules harness vrEmu6502);
- a **`PORT` GPIO peripheral module** exposing DIR/OUT/IN as views. SysTick and
  the NVIC come with the core; GCLK/OSCCTRL can be stubbed since the firmware
  only needs its clock-init writes to succeed.

Both ASDF support topologies model cleanly: Variant B's pins map straight to PORT
views; Variant A additionally needs simple behavioral models of the 74LS138
decoder and the column shift register as small backplane modules (or the test
can drive the decoded/serialized signals directly).

## Open questions / follow-ups

- Pin a concrete part number per variant (e.g. the 48-pin PIC32CM6408PL10048 for
  Variant B) and the internal-oscillator frequency, from the PL10 datasheet, at
  vendoring time.
- Add the two PL10 targets to CI once a runner with `arm-none-eabi-gcc` exists.
- Verify the AVR-pin-compatibility scope against the PL10 SPDIP pinout to bound
  the Variant-A PCB modification (footprint-level compatibility does not imply
  the matrix/ASCII signals land on the same functional pins).
- Touch/PTC as a future ASDF feature (touch keys, sliders) — separate spec.
</content>
