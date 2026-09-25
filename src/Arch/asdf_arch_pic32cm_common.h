// -*- mode: C; tab-width: 2 ; indent-tabs-mode: nil -*-
/**
 * @file asdf_arch_pic32cm_common.h
 *
 * Shared ARM (Cortex-M0+ / PIC32CM PL10) mechanics common to both arch
 * variants (pic32cm_pl10_q64 and pic32cm_pl10_dip28): the (group,bit) pin
 * helpers, the FLASH/PROGMEM neutralization, F_CPU, and the prototypes for the
 * shared clock/tick/delay implementation in asdf_arch_pic32cm_common.c.
 *
 * Each variant header includes this file; the part-specific device header is
 * selected by pic32c.h from the -D__PIC32CM6408PL100NN__ macro the CMake branch
 * passes.
 *
 * Part of the Unified Keyboard Project ASDF keyboard firmware.
 *
 * @copyright Copyright 2026 David Fenyes. MIT License; see LICENSE.
 */
// SPDX-License-Identifier: MIT

#if !defined(ASDF_ARCH_PIC32CM_COMMON_H)
#define ASDF_ARCH_PIC32CM_COMMON_H

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "pic32c.h" // selects the part header from -D__PIC32CM6408PL100NN__
#include "asdf_platform.h"

// Core clock. asdf_arch_common_clock_init() switches the internal
// high-frequency oscillator (OSCHF) from its 4 MHz reset default to its 24 MHz
// maximum (OSCCTRL_OSCHFCTRL FRQSEL_24M). OSCHF feeds GCLK0 -> MCLK (CPUDIV=1,
// the reset value) -> the Cortex-M0+ core, so the core runs at 24 MHz. The PL10
// flash is single-cycle across its frequency range (the NVMCTRL has no
// wait-state field), so no flash configuration is needed. All timing derives
// from F_CPU.
#define F_CPU 24000000UL

// Cortex-M flash is directly addressable: neutralize the AVR PROGMEM macros so
// keymap tables are read as plain arrays.
#define FLASH
#define FLASH_READ(a) (*(a))
#define FLASH_READ_PTR(a) (*(a))
#define FLASH_MEMCPY(dst, src, n) memcpy((dst), (src), (n))
#define FLASH_READ_MATRIX_ELEMENT(matrix, row, col) ((matrix)[(row)][(col)])
#define FLASH_STRING(s) (s)

// (group, bit) pin helpers over the PIC32CM PORT peripheral, using the
// Harmony-style register names (PORT_REGS->GROUP[g].PORT_*). GROUP[0]=PA,
// GROUP[1]=PB. The AVR analogue of set_bit/clear_bit.

/**
 * Drives an output pin high.
 *
 * Sets the pin's output latch through the OUTSET register, leaving the other
 * pins of the group unchanged.
 *
 * @param g  Port group (0 = PA, 1 = PB).
 * @param b  Bit (pin) number within the group.
 */
static inline void pin_set(uint8_t g, uint8_t b) { PORT_REGS->GROUP[g].PORT_OUTSET = ((uint32_t) 1u << b); }
/**
 * Drives an output pin low.
 *
 * Clears the pin's output latch through the OUTCLR register, leaving the other
 * pins of the group unchanged.
 *
 * @param g  Port group (0 = PA, 1 = PB).
 * @param b  Bit (pin) number within the group.
 */
static inline void pin_clear(uint8_t g, uint8_t b) { PORT_REGS->GROUP[g].PORT_OUTCLR = ((uint32_t) 1u << b); }
/**
 * Toggles an output pin.
 *
 * Inverts the pin's output latch through the OUTTGL register, leaving the
 * other pins of the group unchanged.
 *
 * @param g  Port group (0 = PA, 1 = PB).
 * @param b  Bit (pin) number within the group.
 */
static inline void pin_toggle(uint8_t g, uint8_t b) { PORT_REGS->GROUP[g].PORT_OUTTGL = ((uint32_t) 1u << b); }
/**
 * Reads the level of a pin.
 *
 * The pin's input buffer must be enabled (see pin_dir_in()). No side effects.
 *
 * @param g  Port group (0 = PA, 1 = PB).
 * @param b  Bit (pin) number within the group.
 * @return true if the pin reads high; false if it reads low.
 */
static inline bool pin_read(uint8_t g, uint8_t b)
{
  return ((PORT_REGS->GROUP[g].PORT_IN >> b) & 1u) != 0u;
}
/**
 * Makes a pin an output.
 *
 * Sets the pin's direction bit through the DIRSET register; the pin then
 * drives its output latch.
 *
 * @param g  Port group (0 = PA, 1 = PB).
 * @param b  Bit (pin) number within the group.
 */
static inline void pin_dir_out(uint8_t g, uint8_t b) { PORT_REGS->GROUP[g].PORT_DIRSET = ((uint32_t) 1u << b); }
/**
 * Makes a pin an input.
 *
 * Clears the pin's direction bit through the DIRCLR register and sets its pin
 * configuration to input buffer enabled, with no pull resistor. Overwrites the
 * pin's whole PINCFG register.
 *
 * @param g  Port group (0 = PA, 1 = PB).
 * @param b  Bit (pin) number within the group.
 */
static inline void pin_dir_in(uint8_t g, uint8_t b)
{
  PORT_REGS->GROUP[g].PORT_DIRCLR = ((uint32_t) 1u << b);
  PORT_REGS->GROUP[g].PORT_PINCFG[b] = PORT_PINCFG_INEN_Msk;
}

// State of one keyboard's hardware. The embedded platform passes this state to
// each of its operations. The board layer (main.c) owns the state and the tick
// interrupt vector (ASDF_ARCH_TICK_ISR), which calls asdf_arch_count_tick().
typedef struct {
  asdf_platform_t platform; // the keyboard's platform; its user pointer is this state
  volatile uint8_t ticks;   // ticks counted by the interrupt, not yet collected
  uint8_t data_polarity;    // XORed with each code sent
} asdf_arch_t;

// The tick interrupt: SysTick, every 1 ms. The application defines the
// interrupt handler with this macro; defining it overrides the weak
// SysTick_Handler in the DFP startup file.
#define ASDF_ARCH_TICK_ISR void SysTick_Handler(void)

/**
 * Counts one elapsed 1 ms tick.
 *
 * The count saturates at 255, so a long stall cannot wrap it. Call only from
 * the tick interrupt. Increments the tick count in arch.
 *
 * @param arch  Hardware state of the keyboard whose tick is counted.
 */
static inline void asdf_arch_count_tick(asdf_arch_t *arch)
{
  if (arch->ticks < 0xFFu) {
    arch->ticks++;
  }
}

// Shared mechanics (asdf_arch_pic32cm_common.c).

/**
 * Sets the core clock to 24 MHz (F_CPU).
 *
 * Switches the internal high-frequency oscillator to 24 MHz and returns once
 * it has locked at the new frequency. Call once, early in the variant's
 * asdf_arch_init(), before anything that depends on F_CPU.
 */
void asdf_arch_common_clock_init(void);

/**
 * Starts the 1 ms tick timer.
 *
 * Configures SysTick to interrupt every 1 ms and enables its interrupt, so the
 * tick interrupt (ASDF_ARCH_TICK_ISR) starts running.
 */
void asdf_arch_common_tick_init(void);

/**
 * Busy-waits for approximately the given number of microseconds.
 *
 * The delay is approximate and assumes the core runs at F_CPU. No side
 * effects.
 *
 * @param us  Delay in microseconds.
 */
void arch_delay_us(uint16_t us);

/**
 * Collects the ticks counted since the last call.
 *
 * Shared by both variants. Reads and clears the tick count as one step with
 * respect to the tick interrupt, so no tick is lost. Clears the tick count in
 * arch.
 *
 * @param arch  Hardware state of the keyboard.
 * @return The number of 1 ms ticks since the last call, saturating at 255.
 */
uint8_t asdf_arch_tick(asdf_arch_t *arch);
#endif /* !defined (ASDF_ARCH_PIC32CM_COMMON_H) */
