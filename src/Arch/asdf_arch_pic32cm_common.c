// -*- mode: C; tab-width: 2 ; indent-tabs-mode: nil -*-
/**
 * @file asdf_arch_pic32cm_common.c
 *
 * Shared ARM mechanics for the PIC32CM PL10 arch variants: clock, the 1 ms
 * SysTick tick timer and tick collection, and busy-loop delays. The board
 * layer (main.c) defines the SysTick interrupt handler. The matrix-scan and
 * output logic that differs between the 64-pin (2560-class) and SPDIP-28
 * (328p-class) variants lives in their asdf_arch_pic32cm_pl10_*.c files.
 *
 * Part of the Unified Keyboard Project ASDF keyboard firmware.
 *
 * @copyright Copyright 2026 David F. MIT License; see LICENSE.
 */
// SPDX-License-Identifier: MIT

#include "asdf_arch_pic32cm_common.h"

/**
 * Sets the core clock to 24 MHz.
 *
 * Switches the internal high-frequency oscillator (OSCHF) to 24 MHz and waits
 * for it to lock.
 *
 * OSCHF comes out of reset at 4 MHz; 24 MHz is its maximum. GCLK0 already
 * sources OSCHF and MCLK CPUDIV is DIV1 out of reset, so the core clock
 * follows to 24 MHz. The PL10 flash is single-cycle (the NVMCTRL exposes no
 * wait-state field), so no flash configuration is required. The OSCHF
 * frequency calibration is applied by hardware for the selected FRQSEL.
 *
 * Complexity: 2
 */
void asdf_arch_common_clock_init(void)
{
  OSCCTRL_REGS->OSCCTRL_OSCHFCTRL =
    (OSCCTRL_REGS->OSCCTRL_OSCHFCTRL & ~OSCCTRL_OSCHFCTRL_FRQSEL_Msk)
    | OSCCTRL_OSCHFCTRL_FRQSEL(OSCCTRL_OSCHFCTRL_FRQSEL_24M_Val);

  // Wait for OSCHF to re-lock at the new frequency before relying on the clock.
  while ((OSCCTRL_REGS->OSCCTRL_STATUS & OSCCTRL_STATUS_OSCHFRDY_Msk) == 0u) {
  }
}

/**
 * Starts the 1 ms SysTick tick timer.
 *
 * Configures SysTick and enables its interrupt.
 *
 * SysTick counts the core clock, so a reload of F_CPU / 1000 gives a 1 ms
 * period. The reload fits SysTick's 24-bit counter, so SysTick_Config()
 * cannot fail and its result is ignored.
 */
void asdf_arch_common_tick_init(void)
{
  (void) SysTick_Config(F_CPU / 1000u);
}

/**
 * Collects the ticks counted since the last call.
 *
 * Clears the tick count, with interrupts masked.
 *
 * @param arch  Hardware state of the keyboard.
 * @return The number of 1 ms ticks since the last call, saturating at 255.
 *
 * The read and clear happen with interrupts masked, so a tick counted between
 * them is not lost. PRIMASK is saved and restored rather than unconditionally
 * re-enabling interrupts, so the call is safe with interrupts already masked.
 */
uint8_t asdf_arch_tick(asdf_arch_t *arch)
{
  uint32_t primask = __get_PRIMASK();
  __disable_irq();
  uint8_t result = arch->ticks;
  arch->ticks = 0;
  __set_PRIMASK(primask);
  return result;
}

/**
 * Busy-waits for approximately the given number of microseconds.
 *
 * No side effects.
 *
 * @param us  Delay in microseconds.
 *
 * Cortex-M0+ has no DWT cycle counter, so this is a calibrated busy-loop. The
 * loop body is roughly 4 cycles; F_CPU/1e6 gives cycles per microsecond. The
 * loop counter is volatile so the compiler cannot remove the loop.
 *
 * Complexity: 2
 */
void arch_delay_us(uint16_t us)
{
  volatile uint32_t loops = ((uint32_t) us * (F_CPU / 1000000u)) / 4u;

  while (loops > 0u) {
    __NOP();
    loops--;
  }
}
