// -*- mode: C; tab-width: 2 ; indent-tabs-mode: nil -*-
//
//  Unified Keyboard Project
//  ASDF keyboard firmware
//
//  asdf_arch_pic32cm_common.c
//
// Shared ARM mechanics for the PIC32CM PL10 arch variants: clock, the 1 ms
// SysTick tick, and busy-loop delays. The matrix-scan and output logic that
// differs between the 64-pin (2560-class) and SPDIP-28 (328p-class) variants
// lives in their respective asdf_arch_pic32cm_pl10_*.c files.

#include "asdf_arch_pic32cm_common.h"
#include "asdf_config.h" // ASDF_PULSE_DELAY_SHORT_US / ASDF_PULSE_DELAY_LONG_MS

// PROCEDURE: asdf_arch_common_clock_init
// Switch OSCHF from its 4 MHz reset default to 24 MHz (its maximum). GCLK0
// already sources OSCHF and MCLK CPUDIV is DIV1 out of reset, so the core clock
// follows to 24 MHz. The PL10 flash is single-cycle (the NVMCTRL exposes no
// wait-state field), so no flash configuration is required. The OSCHF frequency
// calibration is applied by hardware for the selected FRQSEL.
void asdf_arch_common_clock_init(void)
{
  OSCCTRL_REGS->OSCCTRL_OSCHFCTRL = (OSCCTRL_REGS->OSCCTRL_OSCHFCTRL & ~OSCCTRL_OSCHFCTRL_FRQSEL_Msk)
                                    | OSCCTRL_OSCHFCTRL_FRQSEL(OSCCTRL_OSCHFCTRL_FRQSEL_24M_Val);

  // Wait for OSCHF to re-lock at the new frequency before relying on the clock.
  while ((OSCCTRL_REGS->OSCCTRL_STATUS & OSCCTRL_STATUS_OSCHFRDY_Msk) == 0u) {
  }
}

static volatile uint8_t tick = 0;

// SysTick interrupt: fires once per millisecond, sets the tick flag. Overrides
// the weak SysTick_Handler in the DFP startup file.
void SysTick_Handler(void) { tick = 1; }

// PROCEDURE: asdf_arch_common_tick_init
// Configure SysTick for a 1 ms period off the core clock.
void asdf_arch_common_tick_init(void) { (void) SysTick_Config(F_CPU / 1000u); }

// PROCEDURE: asdf_arch_tick
// Returns 1 once per elapsed millisecond, 0 otherwise (drains the flag).
uint8_t asdf_arch_tick(void)
{
  uint8_t result = tick;
  tick = 0;
  return result;
}

// PROCEDURE: arch_delay_us
// Cortex-M0+ has no DWT cycle counter, so use a calibrated busy-loop. The loop
// body is roughly 4 cycles; F_CPU/1e6 gives cycles per microsecond.
void arch_delay_us(uint16_t us)
{
  volatile uint32_t loops = ((uint32_t) us * (F_CPU / 1000000u)) / 4u;

  while (loops--) {
    __asm volatile("nop");
  }
}

// PROCEDURE: asdf_arch_delay_ms
void asdf_arch_delay_ms(uint16_t delay_ms)
{
  while (delay_ms--) {
    arch_delay_us(1000);
  }
}

// PROCEDURE: asdf_arch_pulse_delay_short
void asdf_arch_pulse_delay_short(void) { arch_delay_us(ASDF_PULSE_DELAY_SHORT_US); }

// PROCEDURE: asdf_arch_pulse_delay_long
void asdf_arch_pulse_delay_long(void) { asdf_arch_delay_ms(ASDF_PULSE_DELAY_LONG_MS); }
