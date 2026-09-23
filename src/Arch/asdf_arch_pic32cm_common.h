// -*- mode: C; tab-width: 2 ; indent-tabs-mode: nil -*-
//
//  Unified Keyboard Project
//  ASDF keyboard firmware
//
//  asdf_arch_pic32cm_common.h
//
// Shared ARM (Cortex-M0+ / PIC32CM PL10) mechanics common to both arch
// variants (pic32cm_pl10_q64 and pic32cm_pl10_dip28): the (group,bit) pin
// helpers, the FLASH/PROGMEM neutralization, F_CPU, and the prototypes for the
// shared clock/tick/delay implementation in asdf_arch_pic32cm_common.c.
//
// Each variant header includes this file; the part-specific device header is
// selected by pic32c.h from the -D__PIC32CM6408PL100NN__ macro the CMake branch
// passes.

#if !defined(ASDF_ARCH_PIC32CM_COMMON_H)
#define ASDF_ARCH_PIC32CM_COMMON_H

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
static inline void pin_set(uint8_t g, uint8_t b) { PORT_REGS->GROUP[g].PORT_OUTSET = (1u << b); }
static inline void pin_clear(uint8_t g, uint8_t b) { PORT_REGS->GROUP[g].PORT_OUTCLR = (1u << b); }
static inline void pin_toggle(uint8_t g, uint8_t b) { PORT_REGS->GROUP[g].PORT_OUTTGL = (1u << b); }
static inline uint8_t pin_read(uint8_t g, uint8_t b)
{
  return (uint8_t)((PORT_REGS->GROUP[g].PORT_IN >> b) & 1u);
}
static inline void pin_dir_out(uint8_t g, uint8_t b) { PORT_REGS->GROUP[g].PORT_DIRSET = (1u << b); }
static inline void pin_dir_in(uint8_t g, uint8_t b)
{
  PORT_REGS->GROUP[g].PORT_DIRCLR = (1u << b);
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

// The tick interrupt: SysTick, every 1 ms. Defining it overrides the weak
// SysTick_Handler in the DFP startup file.
#define ASDF_ARCH_TICK_ISR void SysTick_Handler(void)

// PROCEDURE: asdf_arch_count_tick
// Counts one elapsed tick, saturating so a long stall cannot wrap the count.
// Called only from the tick interrupt.
static inline void asdf_arch_count_tick(asdf_arch_t *arch)
{
  if (arch->ticks < UINT8_MAX) {
    arch->ticks++;
  }
}

// Shared mechanics (asdf_arch_pic32cm_common.c).
void asdf_arch_common_clock_init(void);
void asdf_arch_common_tick_init(void);
void arch_delay_us(uint16_t us);

// Shared public-API functions, identical for both variants (also declared in
// the variant header, which mirrors the AVR reference header).
uint8_t asdf_arch_tick(asdf_arch_t *arch);
void SysTick_Handler(void);
#endif /* !defined (ASDF_ARCH_PIC32CM_COMMON_H) */
