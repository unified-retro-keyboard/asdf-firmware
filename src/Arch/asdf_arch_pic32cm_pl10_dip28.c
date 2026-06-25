// -*- mode: C; tab-width: 2 ; indent-tabs-mode: nil -*-
//
//  Unified Keyboard Project
//  ASDF keyboard firmware
//
//  asdf_arch_pic32cm_pl10_dip28.c
//
// PIC32CM6408PL10028 (SPDIP-28, "328p-class") arch implementation. The
// scan/read and output logic reproduces asdf_arch_atmega328p.c: a 4-bit encoded
// row value to an external 74LS138, and a serial shift-register column read.
// Only the pin-I/O primitives change (PORT_REGS helpers in
// asdf_arch_pic32cm_common.h). Shared clock/tick/delay live in
// asdf_arch_pic32cm_common.c.

#include "asdf_arch.h"
#include "asdf_config.h" // ASDF_DEFAULT_DATA_POLARITY

static uint8_t data_polarity = ASDF_DEFAULT_DATA_POLARITY;

// PROCEDURE: asdf_arch_init
void asdf_arch_init(void)
{
  asdf_arch_common_clock_init();
  asdf_arch_common_tick_init();

  // Encoded row-select lines and the ASCII byte are outputs.
  PORT_REGS->GROUP[ROW_GROUP].PORT_DIRSET = ROW_MASK;
  PORT_REGS->GROUP[ASCII_GROUP].PORT_DIRSET = ASCII_MASK;

  // Column control: COLCLK output idle low, COLMODE output idle high (SHIFT),
  // COL serial data line input.
  pin_clear(COLCLK_GROUP, COLCLK_PIN);
  pin_dir_out(COLCLK_GROUP, COLCLK_PIN);
  pin_set(COLMODE_GROUP, COLMODE_PIN);
  pin_dir_out(COLMODE_GROUP, COLMODE_PIN);
  pin_dir_in(COL_GROUP, COL_PIN);

  // LEDs and OUT1-3 outputs.
  pin_dir_out(LED1_GROUP, LED1_PIN);
  pin_dir_out(LED2_GROUP, LED2_PIN);
  pin_dir_out(LED3_GROUP, LED3_PIN);
  pin_dir_out(OUT1_GROUP, OUT1_PIN);
  pin_dir_out(OUT2_GROUP, OUT2_PIN);
  pin_dir_out(OUT3_GROUP, OUT3_PIN);

  asdf_arch_set_pos_strobe();
}

// PROCEDURE: asdf_arch_read_row
// Output the encoded row to the 74LS138, then read the columns serially from
// the shift register: LOAD (COLMODE low + clock pulse), back to SHIFT, then one
// inverted bit per clock pulse for ASDF_MAX_COLS columns.
asdf_cols_t asdf_arch_read_row(uint8_t row)
{
  asdf_cols_t cols = 0;

  PORT_REGS->GROUP[ROW_GROUP].PORT_OUT =
    (PORT_REGS->GROUP[ROW_GROUP].PORT_OUT & ~ROW_MASK) | (((uint32_t) row << ROW_SHIFT) & ROW_MASK);

  // LOAD the parallel column inputs into the shift register.
  pin_clear(COLMODE_GROUP, COLMODE_PIN);
  pin_set(COLCLK_GROUP, COLCLK_PIN);
  pin_clear(COLCLK_GROUP, COLCLK_PIN);
  pin_set(COLMODE_GROUP, COLMODE_PIN);

  for (uint8_t i = 0; i < ASDF_MAX_COLS; i++) {
    // invert: a pressed key reads low on the serial data line
    cols |= (asdf_cols_t)((uint8_t)(!pin_read(COL_GROUP, COL_PIN)) << i);
    pin_set(COLCLK_GROUP, COLCLK_PIN);
    pin_clear(COLCLK_GROUP, COLCLK_PIN);
  }
  return cols;
}

// PROCEDURE: asdf_arch_send_code
// Output the polarity-adjusted byte and pulse the strobe (OUTTGL twice).
void asdf_arch_send_code(asdf_keycode_t code)
{
  uint32_t ascii = ((uint32_t)((uint8_t)(code ^ data_polarity)) << ASCII_SHIFT) & ASCII_MASK;

  PORT_REGS->GROUP[ASCII_GROUP].PORT_OUT =
    (PORT_REGS->GROUP[ASCII_GROUP].PORT_OUT & ~ASCII_MASK) | ascii;

  pin_toggle(STROBE_GROUP, STROBE_PIN);
  arch_delay_us(ASDF_STROBE_LENGTH_US);
  pin_toggle(STROBE_GROUP, STROBE_PIN);
}

void asdf_arch_null_output(uint8_t value) { (void) value; }

void asdf_arch_set_pos_strobe(void)
{
  pin_clear(STROBE_GROUP, STROBE_PIN); // idle low
  pin_dir_out(STROBE_GROUP, STROBE_PIN);
}

void asdf_arch_set_neg_strobe(void)
{
  pin_set(STROBE_GROUP, STROBE_PIN); // idle high
  pin_dir_out(STROBE_GROUP, STROBE_PIN);
}

// LEDs are active low (on => drive low), matching the 328P.
void asdf_arch_led1_set(uint8_t value)
{
  value ? pin_clear(LED1_GROUP, LED1_PIN) : pin_set(LED1_GROUP, LED1_PIN);
}
void asdf_arch_led2_set(uint8_t value)
{
  value ? pin_clear(LED2_GROUP, LED2_PIN) : pin_set(LED2_GROUP, LED2_PIN);
}
void asdf_arch_led3_set(uint8_t value)
{
  value ? pin_clear(LED3_GROUP, LED3_PIN) : pin_set(LED3_GROUP, LED3_PIN);
}

// Push-pull OUT setters: drive the level and ensure output direction.
static inline void out_set(uint8_t g, uint8_t b, uint8_t value)
{
  value ? pin_set(g, b) : pin_clear(g, b);
  pin_dir_out(g, b);
}

// Open-drain variants, matching the 328P asymmetric sense:
//   open_hi: value true => Hi-Z (released high), false => driven low
//   open_lo: value true => driven high, false => Hi-Z (released)
static inline void out_open_hi(uint8_t g, uint8_t b, uint8_t value)
{
  if (value) {
    pin_dir_in(g, b);
  }
  else {
    pin_clear(g, b);
    pin_dir_out(g, b);
  }
}
static inline void out_open_lo(uint8_t g, uint8_t b, uint8_t value)
{
  if (value) {
    pin_set(g, b);
    pin_dir_out(g, b);
  }
  else {
    pin_dir_in(g, b);
  }
}

void asdf_arch_out1_set(uint8_t value) { out_set(OUT1_GROUP, OUT1_PIN, value); }
void asdf_arch_out1_open_hi_set(uint8_t value) { out_open_hi(OUT1_GROUP, OUT1_PIN, value); }
void asdf_arch_out1_open_lo_set(uint8_t value) { out_open_lo(OUT1_GROUP, OUT1_PIN, value); }
void asdf_arch_out2_set(uint8_t value) { out_set(OUT2_GROUP, OUT2_PIN, value); }
void asdf_arch_out2_open_hi_set(uint8_t value) { out_open_hi(OUT2_GROUP, OUT2_PIN, value); }
void asdf_arch_out2_open_lo_set(uint8_t value) { out_open_lo(OUT2_GROUP, OUT2_PIN, value); }
void asdf_arch_out3_set(uint8_t value) { out_set(OUT3_GROUP, OUT3_PIN, value); }
void asdf_arch_out3_open_hi_set(uint8_t value) { out_open_hi(OUT3_GROUP, OUT3_PIN, value); }
void asdf_arch_out3_open_lo_set(uint8_t value) { out_open_lo(OUT3_GROUP, OUT3_PIN, value); }
