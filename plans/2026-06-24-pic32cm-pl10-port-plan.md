# PIC32CM PL10 Port Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a clean-compiling `pic32cm_pl10_q64` (2560-class) and `pic32cm_pl10_dip28` (328p-class) target family to the ASDF firmware, each producing `asdf-vX.Y.Z-<arch>.{elf,bin,hex}` for the Microchip PIC32CM PL10, each a faithful transliteration of its AVR reference arch onto the PIC32CM PORT register model.

**Architecture:** A new ARM arch family with two variants sharing one common ARM-mechanics file (clock, SysTick tick, delays, pin helpers) and one toolchain. Variant B (`q64`) reproduces `asdf_arch_atmega2560` (16-line one-hot rows, 8-bit parallel inverted columns, OSI variant). Variant A (`dip28`) reproduces `asdf_arch_atmega328p` (4-bit encoded rows → external 74LS138, serial shift-register column read via COLCLK/COLMODE). The core is untouched; AVR targets keep building because each target uses its own `build-<arch>` directory and cached toolchain.

**Tech Stack:** C99, CMake ≥ 3.19, `arm-none-eabi-gcc` (newlib-nano), CMSIS-Core for Cortex-M0+, Microchip PIC32CM PL10 DFP headers.

## Global Constraints

- Target family: **Microchip PIC32CM PL10** — Cortex-M0+, 5V, SAM-lineage peripherals (PORT/GCLK/SERCOM). Concrete parts pinned in Task 1 (e.g. a 64-pin part for `q64`; the 48-pin `PIC32CM6408PL10048` is acceptable for `q64` if a 64-pin pinout is unavailable; an SPDIP-28 part for `dip28`).
- `F_CPU` = the internal-oscillator frequency selected in Task 4 from the PL10 datasheet; all timing derives from it, so the exact value is non-critical. Defined once in the common header.
- Compiler flags floor: `-mcpu=cortex-m0plus -mthumb -std=gnu99 -Os -ffunction-sections -fdata-sections`; link with `-Wl,--gc-sections --specs=nano.specs --specs=nosys.specs`.
- Variant B reference: `src/Arch/asdf_arch_atmega2560.{c,h}`. Variant A reference: `src/Arch/asdf_arch_atmega328p.{c,h}`. Reproduce each reference's scan/read **logic** exactly; only pin-I/O primitives, clock/tick, and toolchain change.
- Vendored device files live under `src/third_party/cmsis/` (Apache-2.0). No ASF4/START HAL.
- **No AVR branch in `CMakeLists.txt` may be modified** — the PL10 paths are purely additive.
- Commit messages follow repo style: lowercase phrase, **no** `feat:`/`fix:` prefix, bullets, greppable symbol names, no AI attribution. (Ignore the writing-plans skill's `feat:` template; the example commit lines below use repo style.)
- **Plan docs under `plans/` are never committed.** Firmware/source changes are committed normally.
- Verification ceiling: clean cross-compile + artifacts that fit each part's flash/SRAM. No hardware, no simavr.

---

## File Structure

**Created:**
- `cmake/generic-gcc-arm.cmake` — ARM toolchain file; defines `c_toolchain_flags()`; objcopy/size POST_BUILD contract.
- `src/Arch/asdf_arch_pic32cm_common.h` — `F_CPU`, `FLASH`/`FLASH_READ_MATRIX_ELEMENT`, inline pin helpers, decls for shared functions (clock init, tick, delays).
- `src/Arch/asdf_arch_pic32cm_common.c` — shared ARM mechanics: clock init, SysTick tick + handler, calibrated delays.
- `src/Arch/asdf_arch_pic32cm_pl10_q64.{c,h}` — Variant B (2560-class).
- `src/Arch/asdf_arch_pic32cm_pl10_dip28.{c,h}` — Variant A (328p-class).
- `src/third_party/cmsis/` — CMSIS-Core + PL10 DFP headers, `startup_pic32cm_pl10.c`, `*_q64_flash.ld`, `*_dip28_flash.ld`, `README.md`.

**Modified:**
- `CMakeLists.txt` — two additive `elseif(ARCH MATCHES pic32cm_pl10_*)` branches.
- `src/CMakeLists.txt` — when `ARCH_TYPE STREQUAL ARM`: add `src/Arch` include dir, and append `startup_pic32cm_pl10.c` + `asdf_arch_pic32cm_common.c` to `SOURCES`; ARM link options + objcopy POST_BUILD.
- `make-targets.sh` — `add_valid_target pic32cm_pl10_q64` and `add_valid_target pic32cm_pl10_dip28`.

**Reference (read, do not modify):**
- `src/Arch/asdf_arch_atmega2560.{c,h}` and `asdf_arch_atmega328p.{c,h}` — behavioral references.
- `cmake/generic-gcc-avr.cmake` — the `c_toolchain_flags`/system-var contract to mirror.
- `src/CMakeLists.txt:46-47` — the `${ARCH_FAMILY}` configure-time copy that selects the arch.

---

## Task ordering rationale

Build infrastructure first (Tasks 1–3) so a **stub arch links into a valid ELF before any peripheral code is written**, de-risking the novel toolchain. Common ARM mechanics next (Task 4). Then Variant B (Tasks 5–10) — the simpler register mapping (direct one-hot + parallel read, no external-chip timing) shakes out clock/tick/GPIO on the easier target. Then Variant A (Tasks 11–13) reuses all common mechanics and adds only the encoded-row + serial-column logic. Final verification (Task 14).

---

### Task 1: Vendor PIC32CM PL10 device-support files

> **STATUS: DONE** (commit `89d7596` on branch `pic32cm-pl10-port`). Vendored
> `Microchip.PIC32CM-PL_DFP` **1.4.418** + ARM **CMSIS_5 5.9.0** Core into
> `src/third_party/cmsis/`. Concrete parts: **PIC32CM6408PL10064** (q64) and
> **PIC32CM6408PL10028** (dip28), both in the `pic32cm6408pl` family (64 KB
> flash @ **`0x0C000000`** / 8 KB RAM). The DFP ships **per-part** `startup_*.c`,
> `system_*.c`, and `*_flash.ld` — vendored **unmodified** (no hand-written
> linker script; the original Steps 2 below are obsolete). `pic32c.h` selects
> the part header from `__PIC32CM6408PL100NN__`, set per-target via `-D`.
> Include tree verified complete by host-gcc preprocess; the real
> `arm-none-eabi-gcc` compile is gated on toolchain install (see Task 3).

**Files (as vendored):** `src/third_party/cmsis/` — `core_cm0plus.h` (+ CMSIS deps),
`pic32c.h`, `pic32cm6408pl100{28,32,48,64}.h` + `system_*.h`, `component/`,
`instance/`, `pio/`, `startup_pic32cm6408pl100{64,28}.c`,
`system_pic32cm6408pl100{64,28}.c`, `pic32cm6408pl100{64,28}_flash.ld`,
`DFP-LICENSE.txt`, `README.md`.

**Original (superseded) plan:**
- Create: `src/third_party/cmsis/` (CMSIS-Core + PL10 DFP headers)
- Create: `src/third_party/cmsis/startup_pic32cm_pl10.c`
- Create: `src/third_party/cmsis/<part>_q64_flash.ld`, `src/third_party/cmsis/<part>_dip28_flash.ld`
- Create: `src/third_party/cmsis/README.md` (pack version, part→variant mapping, license)

**Interfaces:**
- Produces: `#include "<pic32cm_pl10_part>.h"` resolves PORT/GCLK/oscillator/SysTick definitions; each linker script defines `flash (rx)` @ `0x00000000` and `ram (rwx)` @ `0x20000000` with the part's sizes, `__stack`, and `KEEP(*(.vectors))`; `startup_pic32cm_pl10.c` provides `Reset_Handler` (data/bss init → `main`) and a weak `SysTick_Handler`.

- [ ] **Step 1: Obtain the DFP + CMSIS files**

Download the Microchip PIC32CM PL10 DFP (`.atpack` = zip) and ARM CMSIS-Core. Pin concrete parts: a 64-pin (or 48-pin `PIC32CM6408PL10048`) part for `q64`, and an SPDIP-28 part for `dip28`. Copy only what is needed under `src/third_party/cmsis/`:
- CMSIS-Core: `core_cm0plus.h`, `cmsis_gcc.h`, `cmsis_compiler.h`, `cmsis_version.h`.
- DFP `include/`: the part header(s) + the `component/`/`instance/`/`pio/` headers they `#include`.
- DFP `gcc/`: `startup_*.c` and `system_*.c` (basis for `startup_pic32cm_pl10.c`), and the GCC linker script (basis for the two `*_flash.ld`).

Record exact pack version + part numbers in `README.md`.

- [ ] **Step 2: Trim the linker scripts to each part's memory map**

Each `*_flash.ld`:
```ld
MEMORY
{
  flash (rx)  : ORIGIN = 0x00000000, LENGTH = <part flash, e.g. 0x10000 for 64K>
  ram   (rwx) : ORIGIN = 0x20000000, LENGTH = <part sram,  e.g. 0x2000  for 8K>
}
```
with `.text`/`.data`/`.bss`, `__stack = ORIGIN(ram) + LENGTH(ram);`, and `KEEP(*(.vectors))`.

- [ ] **Step 3: Confirm headers compile standalone**

Run:
```bash
arm-none-eabi-gcc -mcpu=cortex-m0plus -mthumb -c -x c -Isrc/third_party/cmsis \
  -o /tmp/pl10_check.o - <<'EOF'
#include "<pic32cm_pl10_part>.h"
int main(void){ PORT->Group[0].DIRSET.reg = 1u; return 0; }
EOF
```
Expected: compiles, object produced — proves the vendored header set is self-contained.

- [ ] **Step 4: Commit**

```bash
git add src/third_party/cmsis
git commit -m "vendor CMSIS-Core and PIC32CM PL10 device headers, startup, and linker scripts"
```

---

### Task 2: ARM toolchain file with the c_toolchain_flags contract

**Files:**
- Create: `cmake/generic-gcc-arm.cmake`
- Reference: `cmake/generic-gcc-avr.cmake`

**Interfaces:**
- Consumes: `LINKER_SCRIPT`, `ARCH_FAMILY` set by `CMakeLists.txt` (Task 3).
- Produces: sets `arm-none-eabi-gcc` + system vars; **defines `function(c_toolchain_flags)`** setting parent-scope `CFLAGS` (called at `src/CMakeLists.txt:52`).

- [ ] **Step 1: Write the toolchain file**

```cmake
# ARM bare-metal toolchain for Cortex-M0+ (PIC32CM PL10).
# Mirrors the contract of generic-gcc-avr.cmake: defines c_toolchain_flags().

find_program(ARM_CC arm-none-eabi-gcc)
find_program(ARM_OBJCOPY arm-none-eabi-objcopy)
find_program(ARM_SIZE_TOOL arm-none-eabi-size)

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMAKE_C_COMPILER ${ARM_CC})
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY) # no full link during compiler probe

set(CMAKE_C_FLAGS_RELEASE    "-Os -DNDEBUG")
set(CMAKE_C_FLAGS_DEBUG      "-O0 -g")
set(CMAKE_C_FLAGS_MINSIZEREL "-Os -DNDEBUG")

set(ARM_MCU_FLAGS "-mcpu=cortex-m0plus -mthumb")

# Contract: src/CMakeLists.txt calls c_toolchain_flags() to populate CFLAGS.
function(c_toolchain_flags)
  set(CFLAGS
    ${ARM_MCU_FLAGS}
    -std=gnu99
    -ffunction-sections -fdata-sections
    -Wall -Wextra
    PARENT_SCOPE)
endfunction(c_toolchain_flags)
```

- [ ] **Step 2: Commit** (validated by the Task 3 build)

```bash
git add cmake/generic-gcc-arm.cmake
git commit -m "add arm-none-eabi toolchain file defining c_toolchain_flags for Cortex-M0+"
```

---

### Task 3: Additive CMake branches + ARM build wiring + linkable q64 stub

**Files:**
- Modify: `CMakeLists.txt` (two branches after the atmega2560 `elseif`, before `endif()`)
- Modify: `src/CMakeLists.txt` (ARM include dir, common+startup sources, link options, objcopy POST_BUILD)
- Modify: `make-targets.sh`
- Create: `src/Arch/asdf_arch_pic32cm_common.h` (minimal: includes, `F_CPU`, `FLASH` macros, inline pin helpers, shared decls)
- Create: `src/Arch/asdf_arch_pic32cm_common.c` (stubbed shared mechanics)
- Create: `src/Arch/asdf_arch_pic32cm_pl10_q64.h` (pin-map placeholders + full API decls)
- Create: `src/Arch/asdf_arch_pic32cm_pl10_q64.c` (empty variant-specific bodies)

**Interfaces:**
- Consumes: `c_toolchain_flags()` (Task 2); part header + linker script (Task 1).
- Produces: a linkable `asdf-vX.Y.Z-pic32cm_pl10_q64.elf`; the complete API surface (function names/signatures copied verbatim from `asdf_arch_atmega2560.h`, including `asdf_arch_osi_read_row`).

- [ ] **Step 1: Add the two CMake branches**

In `CMakeLists.txt`, after the atmega2560 `elseif(...)` block:
```cmake
elseif(ARCH MATCHES pic32cm_pl10_q64)
  set(ARCH_FAMILY pic32cm_pl10_q64)
  set(ARCH_TYPE ARM)
  set(PART_DEFINE __PIC32CM6408PL10064__)
  set(LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/src/third_party/cmsis/pic32cm6408pl10064_flash.ld)
  set(ARM_STARTUP_BASE pic32cm6408pl10064)
  set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/cmake/generic-gcc-arm.cmake)
  set(FINAL_TARGET ${TARGET}.elf)
elseif(ARCH MATCHES pic32cm_pl10_dip28)
  set(ARCH_FAMILY pic32cm_pl10_dip28)
  set(ARCH_TYPE ARM)
  set(PART_DEFINE __PIC32CM6408PL10028__)
  set(LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/src/third_party/cmsis/pic32cm6408pl10028_flash.ld)
  set(ARM_STARTUP_BASE pic32cm6408pl10028)
  set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/cmake/generic-gcc-arm.cmake)
  set(FINAL_TARGET ${TARGET}.elf)
```

- [ ] **Step 2: Wire ARM sources + include + link in `src/CMakeLists.txt`**

Add the common + startup sources when ARM (near the `list(APPEND SOURCES ...)`):
```cmake
if(ARCH_TYPE STREQUAL "ARM")
  list(APPEND SOURCES
    ${CMAKE_SOURCE_DIR}/src/third_party/cmsis/startup_${ARM_STARTUP_BASE}.c
    ${CMAKE_SOURCE_DIR}/src/third_party/cmsis/system_${ARM_STARTUP_BASE}.c
    ${CMAKE_SOURCE_DIR}/src/Arch/asdf_arch_pic32cm_common.c)
endif()
```
After the `add_executable`/`target_compile_options` block:
```cmake
if(ARCH_TYPE STREQUAL "ARM")
  target_include_directories(${PROJECT_EXECUTABLE_TARGET_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src/third_party/cmsis
    ${CMAKE_SOURCE_DIR}/src/Arch)
  target_compile_definitions(${PROJECT_EXECUTABLE_TARGET_NAME} PRIVATE ${PART_DEFINE})
  target_link_options(${PROJECT_EXECUTABLE_TARGET_NAME} PRIVATE
    -mcpu=cortex-m0plus -mthumb
    -T${LINKER_SCRIPT}
    --specs=nano.specs --specs=nosys.specs
    -Wl,--gc-sections -Wl,-Map=${PROJECT_TARGET_NAME}.map)
  add_custom_command(TARGET ${PROJECT_EXECUTABLE_TARGET_NAME} POST_BUILD
    COMMAND arm-none-eabi-objcopy -O binary $<TARGET_FILE:${PROJECT_EXECUTABLE_TARGET_NAME}> ${PROJECT_TARGET_NAME}.bin
    COMMAND arm-none-eabi-objcopy -O ihex   $<TARGET_FILE:${PROJECT_EXECUTABLE_TARGET_NAME}> ${PROJECT_TARGET_NAME}.hex
    COMMAND arm-none-eabi-size $<TARGET_FILE:${PROJECT_EXECUTABLE_TARGET_NAME}>)
endif()
```

- [ ] **Step 3: Add make-targets entries**

In `make-targets.sh` after the atmega2560 line:
```bash
add_valid_target pic32cm_pl10_q64
add_valid_target pic32cm_pl10_dip28
```

- [ ] **Step 4: Write the common header (minimal)**

`src/Arch/asdf_arch_pic32cm_common.h`:
```c
#if !defined(ASDF_ARCH_PIC32CM_COMMON_H)
#define ASDF_ARCH_PIC32CM_COMMON_H
#include <stdint.h>
#include "pic32c.h"   // selects the part header from the -D__PIC32CM6408PL100NN__ macro

#define F_CPU 48000000UL   // confirmed/adjusted in Task 4 from system_*.c SystemInit

#define FLASH
#define FLASH_READ_MATRIX_ELEMENT(matrix, row, col) ((matrix)[(row)][(col)])

// (group, bit) pin helpers over the PIC32CM PORT peripheral.
static inline void pin_set(uint8_t g, uint8_t b)   { PORT->Group[g].OUTSET.reg = (1u << b); }
static inline void pin_clear(uint8_t g, uint8_t b) { PORT->Group[g].OUTCLR.reg = (1u << b); }
static inline void pin_toggle(uint8_t g, uint8_t b){ PORT->Group[g].OUTTGL.reg = (1u << b); }
static inline uint8_t pin_read(uint8_t g, uint8_t b){ return (PORT->Group[g].IN.reg >> b) & 1u; }
static inline void pin_dir_out(uint8_t g, uint8_t b){ PORT->Group[g].DIRSET.reg = (1u << b); }
static inline void pin_dir_in(uint8_t g, uint8_t b) {
  PORT->Group[g].DIRCLR.reg = (1u << b);
  PORT->Group[g].PINCFG[b].reg = PORT_PINCFG_INEN;
}

// Shared mechanics (bodies in Task 4):
void asdf_arch_common_clock_init(void);
void asdf_arch_common_tick_init(void);
uint8_t asdf_arch_tick(void);
void asdf_arch_delay_ms(uint16_t delay_ms);
void arch_delay_us(uint16_t us);
#endif
```

- [ ] **Step 5: Write the common .c (stubbed)**

`src/Arch/asdf_arch_pic32cm_common.c`:
```c
#include "asdf_arch_pic32cm_common.h"
void asdf_arch_common_clock_init(void) {}
void asdf_arch_common_tick_init(void) {}
uint8_t asdf_arch_tick(void) { return 0; }
void arch_delay_us(uint16_t us) { (void)us; }
void asdf_arch_delay_ms(uint16_t ms) { (void)ms; }
```

- [ ] **Step 6: Write the q64 stub header + .c**

`asdf_arch_pic32cm_pl10_q64.h`: copy the **declarations** block of `asdf_arch_atmega2560.h` (every `asdf_arch_*` prototype incl. `asdf_arch_osi_read_row`), replace AVR includes/macros with `#include "asdf_arch_pic32cm_common.h"`, and add:
```c
#define ASDF_STROBE_LENGTH_US 10
#define ASDF_KEYBOARD_ROW_SETTLING_TIME_US 4
#define ASDF_ARCH_DEFAULT_ROW_SCANNER asdf_arch_read_row
#define ASDF_ARCH_DEFAULT_OUTPUT asdf_arch_send_code
#define ASDF_ARCH_DIPSWITCH_ROW 8
```
`asdf_arch_pic32cm_pl10_q64.c`: empty bodies for every variant-specific prototype:
```c
#include "asdf_arch.h"
void asdf_arch_init(void) {}
asdf_cols_t asdf_arch_read_row(uint8_t row) { (void)row; return 0; }
asdf_cols_t asdf_arch_osi_read_row(uint8_t row) { (void)row; return 0; }
void asdf_arch_send_code(asdf_keycode_t code) { (void)code; }
/* ... empty body for every remaining variant-specific prototype ... */
```

- [ ] **Step 7: Configure + build the stub**

Run: `bash make-targets.sh -x -t pic32cm_pl10_q64`
Expected: configures with the ARM toolchain, compiles core + common + q64 stub, links against the linker script, prints an `arm-none-eabi-size` line; `build-pic32cm_pl10_q64/src/asdf-v1.7.0-pic32cm_pl10_q64.{elf,hex}` exist.

- [ ] **Step 8: Verify AVR is undisturbed**

Run: `bash make-targets.sh -t atmega2560`
Expected: builds cleanly with avr-gcc (separate build dir, separate cached toolchain).

- [ ] **Step 9: Commit**

```bash
git add CMakeLists.txt src/CMakeLists.txt make-targets.sh src/Arch/asdf_arch_pic32cm_common.h src/Arch/asdf_arch_pic32cm_common.c src/Arch/asdf_arch_pic32cm_pl10_q64.h src/Arch/asdf_arch_pic32cm_pl10_q64.c
git commit -m "add pic32cm_pl10 build targets with linking arch stub

- two additive elseif(ARCH MATCHES pic32cm_pl10_*) branches selecting the arm toolchain
- ARM include/link/objcopy wiring and shared common-source plumbing
- stub asdf_arch_pic32cm_pl10_q64 + common file that link into a valid ELF"
```

---

### Task 4: Common ARM mechanics — clock, SysTick tick, delays

**Files:**
- Modify: `src/Arch/asdf_arch_pic32cm_common.c` (real bodies)
- Modify: `src/Arch/asdf_arch_pic32cm_common.h` (set final `F_CPU`)

**Interfaces:**
- Produces: `asdf_arch_common_clock_init()` (internal osc → GCLK0 @ `F_CPU`), `asdf_arch_common_tick_init()` (SysTick 1 ms), `asdf_arch_tick()` (drains a 1 ms flag), `arch_delay_us`/`asdf_arch_delay_ms`. Consumed by both variants.

- [ ] **Step 1: Implement clock, tick, delays**

```c
#include "asdf_arch_pic32cm_common.h"

void asdf_arch_common_clock_init(void)
{
  // Bring the internal oscillator onto GCLK generator 0 (CPU/peripheral clock).
  // Exact oscillator/GCLK field names per the vendored PL10 header; if the DFP
  // system_*.c SystemInit() already sets the target frequency, this verifies it.
  GCLK->GENCTRL[0].reg = GCLK_GENCTRL_SRC_OSC48M | GCLK_GENCTRL_GENEN;
  while (GCLK->SYNCBUSY.reg & GCLK_SYNCBUSY_GENCTRL0) {}
}

static volatile uint8_t tick = 0;
void SysTick_Handler(void) { tick = 1; }

void asdf_arch_common_tick_init(void)
{
  SysTick_Config(F_CPU / 1000u); // reload for 1 ms, clears, enables IRQ + counter
}

uint8_t asdf_arch_tick(void)
{
  uint8_t result = tick;
  tick = 0;
  return result;
}

// Cortex-M0+ has no DWT cycle counter; calibrated busy-loop (~4 cycles/iter).
void arch_delay_us(uint16_t us)
{
  volatile uint32_t loops = ((uint32_t)us * (F_CPU / 1000000u)) / 4u;
  while (loops--) { __asm volatile("nop"); }
}
void asdf_arch_delay_ms(uint16_t ms) { while (ms--) arch_delay_us(1000); }
```

- [ ] **Step 2: Set the final `F_CPU`** in the common header to the chosen internal-oscillator frequency (confirm the GCLK source macro name against the PL10 header).

- [ ] **Step 3: Build**

Run: `bash make-targets.sh -t pic32cm_pl10_q64`
Expected: clean build; `SysTick_Config` resolves from `core_cm0plus.h`; no duplicate `SysTick_Handler` (the startup file's is weak).

- [ ] **Step 4: Commit**

```bash
git add src/Arch/asdf_arch_pic32cm_common.c src/Arch/asdf_arch_pic32cm_common.h
git commit -m "implement shared PIC32CM clock init, 1 ms SysTick tick, and busy-loop delays"
```

---

### Task 5: Variant B GPIO pin map + pin init

**Files:**
- Modify: `src/Arch/asdf_arch_pic32cm_pl10_q64.h` (pin map)
- Modify: `src/Arch/asdf_arch_pic32cm_pl10_q64.c` (`asdf_arch_init` pin directions)

**Interfaces:**
- Produces: `ROW_GROUP/ROW_MASK`, `COL_GROUP/COL_MASK`, `ASCII_GROUP/ASCII_SHIFT/ASCII_MASK`, `STROBE_*`, `LED*_*`, `OUT*_*`, `OSI_*` macros; `asdf_arch_init` configures pin directions and calls common clock/tick init.

- [ ] **Step 1: Add the pin map (header)**

```c
#define ROW_GROUP   0
#define ROW_MASK    0x0000FFFFu   // PA00..PA15 one-hot active-low
#define COL_GROUP   1
#define COL_MASK    0x000000FFu   // PB00..PB07 inverted inputs
#define ASCII_GROUP 0
#define ASCII_SHIFT 16            // PA16..PA23
#define ASCII_MASK  (0xFFu << ASCII_SHIFT)
#define STROBE_GROUP 0
#define STROBE_PIN   24
#define LED1_GROUP 1
#define LED1_PIN   8
#define LED2_GROUP 1
#define LED2_PIN   9
#define LED3_GROUP 1
#define LED3_PIN   10
#define OUT1_GROUP 1
#define OUT1_PIN   11
#define OUT2_GROUP 1
#define OUT2_PIN   12
#define OUT3_GROUP 1
#define OUT3_PIN   13
#define OSI_KBE_GROUP 0
#define OSI_KBE_PIN   9    // overlays row-field bit 9 (2560 HIROW bit 1)
#define OSI_RW_GROUP  0
#define OSI_RW_PIN    10   // overlays row-field bit 10 (2560 HIROW bit 2)
```

- [ ] **Step 2: Implement pin init + `asdf_arch_init`**

```c
static void asdf_arch_init_pins(void)
{
  PORT->Group[ROW_GROUP].DIRSET.reg   = ROW_MASK;   // rows: outputs
  PORT->Group[ASCII_GROUP].DIRSET.reg = ASCII_MASK; // ascii: outputs
  for (uint8_t b = 0; b < 8; b++) {                 // cols: inputs
    PORT->Group[COL_GROUP].DIRCLR.reg = (1u << b);
    PORT->Group[COL_GROUP].PINCFG[b].reg = PORT_PINCFG_INEN;
  }
  pin_dir_out(STROBE_GROUP, STROBE_PIN);
  pin_dir_out(LED1_GROUP, LED1_PIN); pin_dir_out(LED2_GROUP, LED2_PIN); pin_dir_out(LED3_GROUP, LED3_PIN);
  pin_dir_out(OUT1_GROUP, OUT1_PIN); pin_dir_out(OUT2_GROUP, OUT2_PIN); pin_dir_out(OUT3_GROUP, OUT3_PIN);
  pin_dir_out(OSI_KBE_GROUP, OSI_KBE_PIN); pin_set(OSI_KBE_GROUP, OSI_KBE_PIN);
  pin_dir_out(OSI_RW_GROUP, OSI_RW_PIN);   pin_set(OSI_RW_GROUP, OSI_RW_PIN);
}

void asdf_arch_init(void)
{
  asdf_arch_common_clock_init();
  asdf_arch_common_tick_init();
  asdf_arch_init_pins();
}
```

- [ ] **Step 3: Build**

Run: `bash make-targets.sh -t pic32cm_pl10_q64`
Expected: clean build.

- [ ] **Step 4: Commit**

```bash
git add src/Arch/asdf_arch_pic32cm_pl10_q64.h src/Arch/asdf_arch_pic32cm_pl10_q64.c
git commit -m "add pic32cm_pl10_q64 GPIO pin map and direction init"
```

---

### Task 6: Variant B standard 16-row scanner + column read

**Files:**
- Modify: `src/Arch/asdf_arch_pic32cm_pl10_q64.c` (`asdf_arch_read_row`)

**Interfaces:**
- Consumes: pin map + `arch_delay_us` (common).
- Produces: `asdf_cols_t asdf_arch_read_row(uint8_t row)` — one-hot active-low row, inverted columns. Reproduces `asdf_arch_atmega2560.c:773`.

- [ ] **Step 1: Implement read_row**

```c
asdf_cols_t asdf_arch_read_row(uint8_t row)
{
  uint32_t one_hot = ~(1u << row) & ROW_MASK;   // active-low: selected row = 0
  uint32_t out = PORT->Group[ROW_GROUP].OUT.reg;
  PORT->Group[ROW_GROUP].OUT.reg = (out & ~ROW_MASK) | (one_hot & ROW_MASK);
  arch_delay_us(ASDF_KEYBOARD_ROW_SETTLING_TIME_US);
  return (asdf_cols_t)(~(PORT->Group[COL_GROUP].IN.reg) & COL_MASK);
}
```

- [ ] **Step 2: Build**

Run: `bash make-targets.sh -t pic32cm_pl10_q64`
Expected: clean build.

- [ ] **Step 3: Commit**

```bash
git add src/Arch/asdf_arch_pic32cm_pl10_q64.c
git commit -m "implement pic32cm_pl10_q64 16-line one-hot row scan in asdf_arch_read_row"
```

---

### Task 7: Variant B OSI scanner variant

**Files:**
- Modify: `src/Arch/asdf_arch_pic32cm_pl10_q64.c` (`asdf_arch_osi_read_row`)

**Interfaces:**
- Consumes: pin map, `asdf_arch_read_row` (Task 6).
- Produces: `asdf_cols_t asdf_arch_osi_read_row(uint8_t row)` — reproduces `asdf_arch_atmega2560.c:805`.

- [ ] **Step 1: Implement osi_read_row**

```c
asdf_cols_t asdf_arch_osi_read_row(uint8_t row)
{
  if (row > 7) {
    return asdf_arch_read_row(row);
  }
  pin_clear(OSI_KBE_GROUP, OSI_KBE_PIN);                       // enable OSI keyboard
  for (uint8_t b = 0; b < 8; b++) pin_dir_out(COL_GROUP, b);   // cols -> outputs
  PORT->Group[COL_GROUP].OUT.reg =
    (PORT->Group[COL_GROUP].OUT.reg & ~COL_MASK) | ((1u << row) & COL_MASK);
  pin_clear(OSI_RW_GROUP, OSI_RW_PIN);
  pin_set(OSI_RW_GROUP, OSI_RW_PIN);                           // RW strobe
  for (uint8_t b = 0; b < 8; b++) pin_dir_in(COL_GROUP, b);    // cols -> inputs
  return (asdf_cols_t)(PORT->Group[COL_GROUP].IN.reg & COL_MASK);
}
```
(Matches the 2560 control flow — non-inverted read in this branch, preserved.)

- [ ] **Step 2: Build**

Run: `bash make-targets.sh -t pic32cm_pl10_q64`
Expected: clean build.

- [ ] **Step 3: Commit**

```bash
git add src/Arch/asdf_arch_pic32cm_pl10_q64.c
git commit -m "implement pic32cm_pl10_q64 OSI scanner variant asdf_arch_osi_read_row"
```

---

### Task 8: Variant B ASCII output + strobe polarity

**Files:**
- Modify: `src/Arch/asdf_arch_pic32cm_pl10_q64.c` (`asdf_arch_send_code`, strobe setters, `asdf_arch_null_output`)

**Interfaces:**
- Consumes: `ASCII_*`, `STROBE_*`, pin helpers, `arch_delay_us`.
- Produces: `asdf_arch_send_code`, `asdf_arch_set_pos_strobe`, `asdf_arch_set_neg_strobe`, `asdf_arch_null_output`.

**Note — strobe transliteration gotcha:** the AVR reference toggles the strobe by writing the `PINx` register (`set_bit(&ASDF_STROBE_PIN,...)` twice). PIC32CM has no PIN-write-toggle; use `pin_toggle` (the `OUTTGL` register) twice, with the idle level established by the polarity setter at init. This reproduces the AVR toggle semantics regardless of polarity.

- [ ] **Step 1: Implement strobe polarity + send_code**

```c
static uint8_t data_polarity = ASDF_DEFAULT_DATA_POLARITY;

void asdf_arch_set_pos_strobe(void) { pin_clear(STROBE_GROUP, STROBE_PIN); } // idle low
void asdf_arch_set_neg_strobe(void) { pin_set(STROBE_GROUP, STROBE_PIN); }   // idle high

void asdf_arch_send_code(asdf_keycode_t code)
{
  uint32_t out = PORT->Group[ASCII_GROUP].OUT.reg;
  PORT->Group[ASCII_GROUP].OUT.reg =
    (out & ~ASCII_MASK) | ((((uint32_t)(uint8_t)code ^ data_polarity) << ASCII_SHIFT) & ASCII_MASK);
  pin_toggle(STROBE_GROUP, STROBE_PIN);            // assert (idle -> active)
  arch_delay_us(ASDF_STROBE_LENGTH_US);
  pin_toggle(STROBE_GROUP, STROBE_PIN);            // deassert (active -> idle)
}

void asdf_arch_null_output(uint8_t value) { (void)value; }
```

- [ ] **Step 2: Build**

Run: `bash make-targets.sh -t pic32cm_pl10_q64`
Expected: clean build.

- [ ] **Step 3: Commit**

```bash
git add src/Arch/asdf_arch_pic32cm_pl10_q64.c
git commit -m "implement pic32cm_pl10_q64 asdf_arch_send_code with OUTTGL strobe and polarity"
```

---

### Task 9: Variant B LEDs and OUT1-3 with open-drain variants

**Files:**
- Modify: `src/Arch/asdf_arch_pic32cm_pl10_q64.c`

**Interfaces:**
- Produces: `asdf_arch_led{1,2,3}_set`, `asdf_arch_out{1,2,3}_set`, and `*_open_hi_set`/`*_open_lo_set`. Match the 2560 polarity line-for-line.

- [ ] **Step 1: Implement LED + OUT setters**

```c
void asdf_arch_led1_set(uint8_t v){ v ? pin_set(LED1_GROUP,LED1_PIN) : pin_clear(LED1_GROUP,LED1_PIN); }
void asdf_arch_led2_set(uint8_t v){ v ? pin_set(LED2_GROUP,LED2_PIN) : pin_clear(LED2_GROUP,LED2_PIN); }
void asdf_arch_led3_set(uint8_t v){ v ? pin_set(LED3_GROUP,LED3_PIN) : pin_clear(LED3_GROUP,LED3_PIN); }
void asdf_arch_out1_set(uint8_t v){ v ? pin_set(OUT1_GROUP,OUT1_PIN) : pin_clear(OUT1_GROUP,OUT1_PIN); }
void asdf_arch_out2_set(uint8_t v){ v ? pin_set(OUT2_GROUP,OUT2_PIN) : pin_clear(OUT2_GROUP,OUT2_PIN); }
void asdf_arch_out3_set(uint8_t v){ v ? pin_set(OUT3_GROUP,OUT3_PIN) : pin_clear(OUT3_GROUP,OUT3_PIN); }
```

- [ ] **Step 2: Implement open-drain variants**

```c
static inline void open_lo(uint8_t g, uint8_t b, uint8_t v) {
  pin_clear(g, b);                  // latch low
  if (v) pin_dir_out(g, b);         // true => drive low
  else   pin_dir_in(g, b);          // false => Hi-Z
}
static inline void open_hi(uint8_t g, uint8_t b, uint8_t v) {
  pin_clear(g, b);
  if (v) pin_dir_in(g, b);          // true => Hi-Z
  else   pin_dir_out(g, b);         // false => drive low
}
void asdf_arch_out1_open_hi_set(uint8_t v){ open_hi(OUT1_GROUP,OUT1_PIN,v); }
void asdf_arch_out1_open_lo_set(uint8_t v){ open_lo(OUT1_GROUP,OUT1_PIN,v); }
void asdf_arch_out2_open_hi_set(uint8_t v){ open_hi(OUT2_GROUP,OUT2_PIN,v); }
void asdf_arch_out2_open_lo_set(uint8_t v){ open_lo(OUT2_GROUP,OUT2_PIN,v); }
void asdf_arch_out3_open_hi_set(uint8_t v){ open_hi(OUT3_GROUP,OUT3_PIN,v); }
void asdf_arch_out3_open_lo_set(uint8_t v){ open_lo(OUT3_GROUP,OUT3_PIN,v); }
```
Cross-check the true/false → Hi-Z/low polarity against the 2560 `*_open_hi_set`/`*_open_lo_set` bodies and match them exactly.

- [ ] **Step 3: Build**

Run: `bash make-targets.sh -t pic32cm_pl10_q64`
Expected: clean build.

- [ ] **Step 4: Commit**

```bash
git add src/Arch/asdf_arch_pic32cm_pl10_q64.c
git commit -m "implement pic32cm_pl10_q64 LED and OUT1-3 setters with open-drain variants"
```

---

### Task 10: Variant B milestone verification

- [ ] **Step 1: Clean q64 build + size**

Run: `bash make-targets.sh -x -t pic32cm_pl10_q64`
Then: `arm-none-eabi-size build-pic32cm_pl10_q64/src/asdf-v1.7.0-pic32cm_pl10_q64.elf`
Expected: artifacts present; `text+data` ≤ flash, `data+bss` ≤ SRAM for the chosen part. Record numbers.

- [ ] **Step 2: AVR + host regression**

Run: `bash make-targets.sh -t atmega2560 -t test`
Expected: 2560 builds clean; Unity host suite passes.

- [ ] **Step 3: Commit** (no-op if nothing changed; otherwise any cleanup)

---

### Task 11: Variant A (dip28) arch files that link

**Files:**
- Create: `src/Arch/asdf_arch_pic32cm_pl10_dip28.h` (328p-class pin map + API decls; include common)
- Create: `src/Arch/asdf_arch_pic32cm_pl10_dip28.c` (empty variant bodies + `asdf_arch_init` calling common)

**Interfaces:**
- Consumes: common header/mechanics.
- Produces: a linkable `asdf-vX.Y.Z-pic32cm_pl10_dip28.elf`. API decls copied verbatim from `asdf_arch_atmega328p.h` (note: **no** `osi_read_row` here).

- [ ] **Step 1: Write the header with the 328p-class pin map**

```c
#if !defined(ASDF_ARCH_H)
#define ASDF_ARCH_H
#include "asdf_arch_pic32cm_common.h"

#define ASDF_STROBE_LENGTH_US 10
#define ASDF_ARCH_DEFAULT_ROW_SCANNER asdf_arch_read_row
#define ASDF_ARCH_DEFAULT_OUTPUT asdf_arch_send_code
#define ASDF_ARCH_DIPSWITCH_ROW 8

// 4-bit encoded row select -> external 74LS138 decoder
#define ROW_GROUP   0
#define ROW_SHIFT   0
#define ROW_MASK    0x0000000Fu     // PA00..PA03
// serial shift-register column read
#define COL_GROUP   0
#define COL_PIN     4               // PA04 data-in
#define COLCLK_GROUP 0
#define COLCLK_PIN   5              // PA05
#define COLMODE_GROUP 0
#define COLMODE_PIN  6              // PA06  (LOAD=low, SHIFT=high)
#define STROBE_GROUP 0
#define STROBE_PIN   7
#define ASCII_GROUP 0
#define ASCII_SHIFT 8               // PA08..PA15
#define ASCII_MASK  (0xFFu << ASCII_SHIFT)
#define LED1_GROUP 0
#define LED1_PIN   16
#define LED2_GROUP 0
#define LED2_PIN   17
#define LED3_GROUP 0
#define LED3_PIN   18
#define OUT1_GROUP 0
#define OUT1_PIN   19
#define OUT2_GROUP 0
#define OUT2_PIN   20
#define OUT3_GROUP 0
#define OUT3_PIN   21

/* ... all asdf_arch_* prototypes copied verbatim from asdf_arch_atmega328p.h ... */
#endif
```

- [ ] **Step 2: Write the stub .c**

```c
#include "asdf_arch.h"
void asdf_arch_init(void) { asdf_arch_common_clock_init(); asdf_arch_common_tick_init(); }
asdf_cols_t asdf_arch_read_row(uint8_t row) { (void)row; return 0; }
void asdf_arch_send_code(asdf_keycode_t code) { (void)code; }
/* ... empty body for every remaining 328p prototype ... */
```

- [ ] **Step 3: Build**

Run: `bash make-targets.sh -x -t pic32cm_pl10_dip28`
Expected: configures + links; artifacts present.

- [ ] **Step 4: Commit**

```bash
git add src/Arch/asdf_arch_pic32cm_pl10_dip28.h src/Arch/asdf_arch_pic32cm_pl10_dip28.c
git commit -m "add linking pic32cm_pl10_dip28 arch skeleton (328p-class)"
```

---

### Task 12: Variant A encoded-row drive + serial shift-register column read

**Files:**
- Modify: `src/Arch/asdf_arch_pic32cm_pl10_dip28.c` (`asdf_arch_init_pins`, `asdf_arch_read_row`)

**Interfaces:**
- Produces: `asdf_arch_read_row` reproducing `asdf_arch_atmega328p.c:770` — encoded row write, then the LOAD/SHIFT serial column loop.

- [ ] **Step 1: Pin init**

```c
static void asdf_arch_init_pins(void)
{
  PORT->Group[ROW_GROUP].DIRSET.reg = ROW_MASK;          // row-select outputs
  PORT->Group[ASCII_GROUP].DIRSET.reg = ASCII_MASK;      // ascii outputs
  pin_dir_out(COLCLK_GROUP, COLCLK_PIN);
  pin_dir_out(COLMODE_GROUP, COLMODE_PIN);
  pin_dir_in(COL_GROUP, COL_PIN);                         // serial col data input
  pin_dir_out(STROBE_GROUP, STROBE_PIN);
  pin_dir_out(LED1_GROUP, LED1_PIN); pin_dir_out(LED2_GROUP, LED2_PIN); pin_dir_out(LED3_GROUP, LED3_PIN);
  pin_dir_out(OUT1_GROUP, OUT1_PIN); pin_dir_out(OUT2_GROUP, OUT2_PIN); pin_dir_out(OUT3_GROUP, OUT3_PIN);
}
```
Add `asdf_arch_init_pins();` to `asdf_arch_init`.

- [ ] **Step 2: Implement read_row (encoded row + serial columns)**

```c
asdf_cols_t asdf_arch_read_row(uint8_t row)
{
  asdf_cols_t cols = 0;

  uint32_t out = PORT->Group[ROW_GROUP].OUT.reg;
  PORT->Group[ROW_GROUP].OUT.reg =
    (out & ~ROW_MASK) | (((uint32_t)row << ROW_SHIFT) & ROW_MASK);

  // LOAD: pulse clock with COLMODE low, then return to SHIFT.
  pin_clear(COLMODE_GROUP, COLMODE_PIN);
  pin_set(COLCLK_GROUP, COLCLK_PIN);
  pin_clear(COLCLK_GROUP, COLCLK_PIN);
  pin_set(COLMODE_GROUP, COLMODE_PIN);

  for (uint8_t i = 0; i < ASDF_MAX_COLS; i++) {
    cols |= (asdf_cols_t)((!pin_read(COL_GROUP, COL_PIN)) << i); // invert: press = low
    pin_set(COLCLK_GROUP, COLCLK_PIN);
    pin_clear(COLCLK_GROUP, COLCLK_PIN);
  }
  return cols;
}
```
(Structure mirrors the 328P loop: encoded row, LOAD pulse, then `ASDF_MAX_COLS` inverted serial reads each followed by a clock pulse.)

- [ ] **Step 3: Build**

Run: `bash make-targets.sh -t pic32cm_pl10_dip28`
Expected: clean build (`ASDF_MAX_COLS` resolves from the core config header).

- [ ] **Step 4: Commit**

```bash
git add src/Arch/asdf_arch_pic32cm_pl10_dip28.c
git commit -m "implement pic32cm_pl10_dip28 encoded-row drive and serial shift-register column read"
```

---

### Task 13: Variant A output, strobe, LEDs, OUTs

**Files:**
- Modify: `src/Arch/asdf_arch_pic32cm_pl10_dip28.c`

**Interfaces:**
- Produces: `asdf_arch_send_code` (OUTTGL strobe + `data_polarity`), strobe polarity setters, LED + OUT setters and open-drain variants — same bodies as Variant B (Tasks 8–9), on the dip28 pin map.

- [ ] **Step 1: Implement send_code + strobe + LED/OUT setters**

Reuse the exact bodies from Task 8 (send_code with `pin_toggle` strobe + `data_polarity`, `set_pos/neg_strobe`, `null_output`) and Task 9 (LED/OUT push-pull + `open_hi`/`open_lo`), referencing the dip28 `*_GROUP`/`*_PIN` macros. The 328P reference has the same output contract as the 2560, so the code is identical apart from the pin macros.

```c
static uint8_t data_polarity = ASDF_DEFAULT_DATA_POLARITY;

void asdf_arch_set_pos_strobe(void) { pin_clear(STROBE_GROUP, STROBE_PIN); }
void asdf_arch_set_neg_strobe(void) { pin_set(STROBE_GROUP, STROBE_PIN); }

void asdf_arch_send_code(asdf_keycode_t code)
{
  uint32_t out = PORT->Group[ASCII_GROUP].OUT.reg;
  PORT->Group[ASCII_GROUP].OUT.reg =
    (out & ~ASCII_MASK) | ((((uint32_t)(uint8_t)code ^ data_polarity) << ASCII_SHIFT) & ASCII_MASK);
  pin_toggle(STROBE_GROUP, STROBE_PIN);
  arch_delay_us(ASDF_STROBE_LENGTH_US);
  pin_toggle(STROBE_GROUP, STROBE_PIN);
}
void asdf_arch_null_output(uint8_t value) { (void)value; }
```
Then the six LED/OUT setters and the six open-drain variants exactly as Task 9.

- [ ] **Step 2: Build**

Run: `bash make-targets.sh -t pic32cm_pl10_dip28`
Expected: clean build.

- [ ] **Step 3: Commit**

```bash
git add src/Arch/asdf_arch_pic32cm_pl10_dip28.c
git commit -m "implement pic32cm_pl10_dip28 output: send_code strobe, LEDs, and OUT1-3"
```

---

### Task 14: Final verification — both variants, AVR + host regression

- [ ] **Step 1: Clean build both PL10 variants**

Run: `bash make-targets.sh -x -t pic32cm_pl10_q64 -t pic32cm_pl10_dip28`
Expected: both configure + compile + link + objcopy succeed; both print `arm-none-eabi-size`.

- [ ] **Step 2: Size-fit check**

Run `arm-none-eabi-size` on each `*.elf`. Expected: each fits its part's flash/SRAM. Record numbers.

- [ ] **Step 3: Artifacts exist**

Run: `ls build-pic32cm_pl10_q64/src/*.{elf,bin,hex} build-pic32cm_pl10_dip28/src/*.{elf,bin,hex}`
Expected: all six present.

- [ ] **Step 4: AVR + host regression**

Run: `bash make-targets.sh -t atmega328p -t atmega2560 -t test`
Expected: both AVR targets build cleanly; Unity host suite passes.

- [ ] **Step 5: Doc note**

```bash
# Note both PL10 targets as build-verified (no simavr/hardware coverage yet) in README.md
git add README.md
git commit -m "note pic32cm_pl10 targets as build-verified pending backplane coverage"
```

---

## Self-Review

**Spec coverage:**
- Two-variant family, shared common file → Tasks 3,4 + Variant B (5–10) + Variant A (11–13). ✓
- Variant B = 2560 transliteration (one-hot rows, parallel inverted cols, OSI) → Tasks 6,7. ✓
- Variant A = 328p transliteration (encoded rows → '138, serial shift-register cols) → Task 12. ✓
- OUTTGL strobe gotcha (no AVR PIN-toggle trick) → Tasks 8, 13 (note + code). ✓
- ARM toolchain coexisting with AVR, additive branches, separate build dirs → Tasks 2,3; regression Tasks 10,14. ✓
- Vendored DFP/CMSIS/startup/per-variant linker scripts → Task 1. ✓
- Shared clock (internal osc→GCLK0), SysTick 1 ms tick, busy-loop delays → Task 4. ✓
- GPIO `(group,bit)` helpers, masked field writes → Task 3 (common.h), used throughout. ✓
- FLASH/PROGMEM neutralized → Task 3 (common.h). ✓
- `c_toolchain_flags()` contract + objcopy/.bin/.hex/size → Tasks 2,3; verified Task 14. ✓
- make-targets entries → Task 3. ✓
- Build-only ceiling, AVR + host regression, simavr out of scope → Tasks 10,14. ✓
- Backplane as future harness, CI deferred → spec only (not tasks), per design. ✓

**Placeholder scan:** `<part>` / `<pic32cm_pl10_part>` / `<part flash/sram sizes>` and the final `F_CPU` are **device facts deliberately pinned in Task 1/Task 4** from the datasheet — flagged as such, not silent TODOs. No "implement later" steps; every code step shows code.

**Type consistency:** `asdf_cols_t` on both scanners; `asdf_keycode_t` on `send_code`; pin helpers `(uint8_t g, uint8_t b)` used uniformly; `ROW_MASK`/`COL_MASK`/`ASCII_MASK`/`ASCII_SHIFT` names stable; `arch_delay_us`/`asdf_arch_tick`/`asdf_arch_common_*` declared once in `asdf_arch_pic32cm_common.h` and defined once in `asdf_arch_pic32cm_common.c` (no duplicate symbols across the two variant `.c` files). ✓

**Known executor caveats (verify against vendored headers, not memory):** exact GCLK/oscillator field names (`GCLK_GENCTRL_SRC_OSC48M`, `GCLK_SYNCBUSY_GENCTRL0`, `PORT_PINCFG_INEN`, `OUTTGL`) must be confirmed against the PL10 part header — names may differ from the SAM C20 spelling. Whether the DFP `system_*.c` `SystemInit()` already sets the core clock (Task 4). The busy-loop cycle constant (Task 4) is approximate; acceptable under the build-only ceiling. The 2560/328p open-drain polarity (Tasks 9, 13) must be matched line-for-line against each reference. Confirm `ASDF_MAX_COLS` and `ASDF_DEFAULT_DATA_POLARITY` names/locations in the core config headers.
</content>
