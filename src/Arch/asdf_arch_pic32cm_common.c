// -*- mode: C; tab-width: 2 ; indent-tabs-mode: nil -*-
//
//  Unified Keyboard Project
//  ASDF keyboard firmware
//
//  asdf_arch_pic32cm_common.c
//
// Shared ARM mechanics for the PIC32CM PL10 arch variants: clock init, the
// 1 ms SysTick tick, and busy-loop delays. Bodies are stubbed here and filled
// in Task 4.

#include "asdf_arch_pic32cm_common.h"

void asdf_arch_common_clock_init(void) {}
void asdf_arch_common_tick_init(void) {}
uint8_t asdf_arch_tick(void) { return 0; }
void arch_delay_us(uint16_t us) { (void) us; }
void asdf_arch_delay_ms(uint16_t delay_ms) { (void) delay_ms; }
void asdf_arch_pulse_delay_short(void) {}
void asdf_arch_pulse_delay_long(void) {}
