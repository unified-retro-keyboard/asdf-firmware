// -*- mode: C; tab-width: 2 ; indent-tabs-mode: nil -*-
//
//  Unified Keyboard Project
//  ASDF keyboard firmware
//
//  asdf_arch_pic32cm_pl10_q64.c
//
// PIC32CM6408PL10064 (64-pin, "2560-class") arch implementation. Stubbed
// here (Task 3); peripheral logic filled in Tasks 5-9. Shared clock/tick/delay
// live in asdf_arch_pic32cm_common.c.

#include "asdf_arch.h"

void asdf_arch_init(void) {}

asdf_cols_t asdf_arch_read_row(uint8_t row)
{
  (void) row;
  return 0;
}

void asdf_arch_send_code(asdf_keycode_t code) { (void) code; }

void asdf_arch_set_pos_strobe(void) {}
void asdf_arch_set_neg_strobe(void) {}
void asdf_arch_null_output(uint8_t value) { (void) value; }
void asdf_arch_led1_set(uint8_t value) { (void) value; }
void asdf_arch_led2_set(uint8_t value) { (void) value; }
void asdf_arch_led3_set(uint8_t value) { (void) value; }
void asdf_arch_out1_set(uint8_t value) { (void) value; }
void asdf_arch_out1_open_hi_set(uint8_t value) { (void) value; }
void asdf_arch_out1_open_lo_set(uint8_t value) { (void) value; }
void asdf_arch_out2_set(uint8_t value) { (void) value; }
void asdf_arch_out2_open_hi_set(uint8_t value) { (void) value; }
void asdf_arch_out2_open_lo_set(uint8_t value) { (void) value; }
void asdf_arch_out3_set(uint8_t value) { (void) value; }
void asdf_arch_out3_open_hi_set(uint8_t value) { (void) value; }
void asdf_arch_out3_open_lo_set(uint8_t value) { (void) value; }
