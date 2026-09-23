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
#include "pic32c.h" // selects the part header from -D__PIC32CM6408PL100NN__

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

// Shared mechanics (asdf_arch_pic32cm_common.c).
void asdf_arch_common_clock_init(void);
void asdf_arch_common_tick_init(void);
void arch_delay_us(uint16_t us);

// Shared public-API functions, identical for both variants (also declared in
// the variant header, which mirrors the AVR reference header).
uint8_t asdf_arch_tick(void);
void asdf_arch_delay_ms(uint16_t delay_ms);
void asdf_arch_pulse_delay_short(void);
void asdf_arch_pulse_delay_long(void);

#endif /* !defined (ASDF_ARCH_PIC32CM_COMMON_H) */
