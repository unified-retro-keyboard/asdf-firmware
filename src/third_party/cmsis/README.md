# Vendored CMSIS-Core + PIC32CM PL10 device-support files

Unmodified third-party files supporting the ARM (`pic32cm_pl10_*`) firmware
targets. Vendored so a clean checkout builds with only `arm-none-eabi-gcc`
installed (no external CMSIS-Pack / DFP required), consistent with how
`nanoprintf` and `unity` are already vendored in this tree.

## Provenance

- **Microchip PIC32CM PL10 DFP** — `Microchip.PIC32CM-PL_DFP` version
  **1.4.418**, from <https://packs.download.microchip.com/>. Provides the
  device headers (`pic32c.h`, `pic32cm6408pl100*.h`, `system_*.h`,
  `component/`, `instance/`, `pio/`), the per-part GCC startup files
  (`startup_*.c`), the C `SystemInit` sources (`system_*.c`), and the per-part
  GCC linker scripts (`*_flash.ld`). License: see `DFP-LICENSE.txt`.
- **ARM CMSIS-Core** — from ARM-software/CMSIS_5 tag **5.9.0**
  (<https://github.com/ARM-software/CMSIS_5>): `core_cm0plus.h`,
  `cmsis_compiler.h`, `cmsis_gcc.h`, `cmsis_version.h`, `mpu_armv7.h`.
  License: Apache-2.0 (compatible with this repo's GPLv3).

## Parts used by the two arch variants

- `pic32cm_pl10_q64`  → **PIC32CM6408PL10064** (64-pin), define `__PIC32CM6408PL10064__`.
- `pic32cm_pl10_dip28` → **PIC32CM6408PL10028** (SPDIP-28), define `__PIC32CM6408PL10028__`.

Both share the `pic32cm6408pl` family include tree. `pic32c.h` selects the part
header from the `__PIC32CM6408PL100NN__` macro, which the per-target CMake
branch passes via `-D`. (Headers for the 032/048 packages are kept for
completeness; their startup/linker scripts are not vendored.)

## Memory map (from the DFP linker scripts)

- Flash (ROM): origin **`0x0C000000`**, length `0x10000` (64 KB).
- RAM: origin `0x20000000`, length `0x2000` (8 KB).

Note the non-zero flash origin (`0x0C000000`) — a PIC32CM trait, unlike the
`0x00000000` flash base of classic SAM Cortex-M0+ parts. The vendored
`*_flash.ld` carry these as `DEFINED()`-guarded defaults, so they are used
as-is.

## Verification status

The include tree was verified complete (every `#include` resolves) by
preprocessing `pic32c.h` for both part macros. A full `arm-none-eabi-gcc`
compile has **not** been run in the authoring environment (toolchain absent);
that is exercised by the `pic32cm_pl10_*` builds once the toolchain is present.

Do not edit these files; re-vendor from the upstream pack/repo to update.
