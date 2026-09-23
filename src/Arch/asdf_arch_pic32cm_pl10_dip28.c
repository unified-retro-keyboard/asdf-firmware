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
#include "asdf_config.h" // ASDF_DEFAULT_DATA_POLARITY, ASDF_PULSE_DELAY_SHORT_US
#include <stddef.h>
#include "asdf_platform.h"


// PROCEDURE: asdf_arch_read_row
// Output the encoded row to the 74LS138, then read the columns serially from
// the shift register: LOAD (COLMODE low + clock pulse), back to SHIFT, then one
// inverted bit per clock pulse for ASDF_MAX_COLS columns.
static asdf_cols_t asdf_arch_read_row(uint8_t row)
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
static void asdf_arch_send_code(const asdf_arch_t *arch, asdf_keycode_t code)
{
  uint32_t ascii = ((uint32_t)((uint8_t)(code ^ arch->data_polarity)) << ASCII_SHIFT) & ASCII_MASK;

  PORT_REGS->GROUP[ASCII_GROUP].PORT_OUT =
    (PORT_REGS->GROUP[ASCII_GROUP].PORT_OUT & ~ASCII_MASK) | ascii;

  pin_toggle(STROBE_GROUP, STROBE_PIN);
  arch_delay_us(ASDF_STROBE_LENGTH_US);
  pin_toggle(STROBE_GROUP, STROBE_PIN);
}

static void asdf_arch_null_output(uint8_t value) { (void) value; }

static void asdf_arch_set_pos_strobe(void)
{
  pin_clear(STROBE_GROUP, STROBE_PIN); // idle low
  pin_dir_out(STROBE_GROUP, STROBE_PIN);
}

static void asdf_arch_set_neg_strobe(void)
{
  pin_set(STROBE_GROUP, STROBE_PIN); // idle high
  pin_dir_out(STROBE_GROUP, STROBE_PIN);
}

// LEDs are active low (on => drive low), matching the 328P.
static void asdf_arch_led1_set(uint8_t value)
{
  value ? pin_clear(LED1_GROUP, LED1_PIN) : pin_set(LED1_GROUP, LED1_PIN);
}
static void asdf_arch_led2_set(uint8_t value)
{
  value ? pin_clear(LED2_GROUP, LED2_PIN) : pin_set(LED2_GROUP, LED2_PIN);
}
static void asdf_arch_led3_set(uint8_t value)
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

static void asdf_arch_out1_set(uint8_t value) { out_set(OUT1_GROUP, OUT1_PIN, value); }
static void asdf_arch_out1_open_hi_set(uint8_t value) { out_open_hi(OUT1_GROUP, OUT1_PIN, value); }
static void asdf_arch_out1_open_lo_set(uint8_t value) { out_open_lo(OUT1_GROUP, OUT1_PIN, value); }
static void asdf_arch_out2_set(uint8_t value) { out_set(OUT2_GROUP, OUT2_PIN, value); }
static void asdf_arch_out2_open_hi_set(uint8_t value) { out_open_hi(OUT2_GROUP, OUT2_PIN, value); }
static void asdf_arch_out2_open_lo_set(uint8_t value) { out_open_lo(OUT2_GROUP, OUT2_PIN, value); }
static void asdf_arch_out3_set(uint8_t value) { out_set(OUT3_GROUP, OUT3_PIN, value); }
static void asdf_arch_out3_open_hi_set(uint8_t value) { out_open_hi(OUT3_GROUP, OUT3_PIN, value); }
static void asdf_arch_out3_open_lo_set(uint8_t value) { out_open_lo(OUT3_GROUP, OUT3_PIN, value); }

// PROCEDURE: asdf_arch_init_hardware
static void asdf_arch_init_hardware(void)
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

}

// PROCEDURE: asdf_arch_set_output
// Drive an output through its handler. Invalid outputs are ignored.
typedef void (*asdf_arch_output_handler_t)(uint8_t);

static const asdf_arch_output_handler_t output_handlers[ASDF_PHYSICAL_NUM_RESOURCES] = {
  [PHYSICAL_NO_OUT] = &asdf_arch_null_output,
  [PHYSICAL_OUT1] = &asdf_arch_out1_set,
  [PHYSICAL_OUT2] = &asdf_arch_out2_set,
  [PHYSICAL_OUT3] = &asdf_arch_out3_set,
  [PHYSICAL_OUT1_OPEN_HI] = &asdf_arch_out1_open_hi_set,
  [PHYSICAL_OUT2_OPEN_HI] = &asdf_arch_out2_open_hi_set,
  [PHYSICAL_OUT3_OPEN_HI] = &asdf_arch_out3_open_hi_set,
  [PHYSICAL_OUT1_OPEN_LO] = &asdf_arch_out1_open_lo_set,
  [PHYSICAL_OUT2_OPEN_LO] = &asdf_arch_out2_open_lo_set,
  [PHYSICAL_OUT3_OPEN_LO] = &asdf_arch_out3_open_lo_set,
  [PHYSICAL_LED1] = &asdf_arch_led1_set,
  [PHYSICAL_LED2] = &asdf_arch_led2_set,
  [PHYSICAL_LED3] = &asdf_arch_led3_set,
};

static void asdf_arch_set_output(asdf_physical_dev_t output, uint8_t value)
{
  if (output < ASDF_PHYSICAL_NUM_RESOURCES) {
    output_handlers[output](value);
  }
}

// PROCEDURE: asdf_arch_reset
// Return the data and strobe polarity to their defaults, and idle the ASCII
// output.
static void asdf_arch_reset(asdf_arch_t *arch)
{
  arch->data_polarity = ASDF_DEFAULT_DATA_POLARITY;
  PORT_REGS->GROUP[ASCII_GROUP].PORT_OUT =
    (PORT_REGS->GROUP[ASCII_GROUP].PORT_OUT & ~ASCII_MASK)
    | (((uint32_t) arch->data_polarity << ASCII_SHIFT) & ASCII_MASK);

  if (ASDF_DEFAULT_STROBE_POLARITY == ASDF_POSITIVE_POLARITY) {
    asdf_arch_set_pos_strobe();
  }
  else {
    asdf_arch_set_neg_strobe();
  }
}

// PROCEDURE: arch_platform_*
// Adapt the architecture's operations to the typed platform interface. The
// platform context pointer is the keyboard's asdf_arch_t.
static asdf_cols_t arch_platform_read_row(void *user, uint8_t row)
{
  (void) user;
  return asdf_arch_read_row(row);
}

static void arch_platform_send_code(void *user, asdf_keycode_t code)
{
  asdf_arch_send_code(user, code);
}

static void arch_platform_set_output(void *user, asdf_physical_dev_t output, uint8_t value)
{
  (void) user;
  asdf_arch_set_output(output, value);
}

static void arch_platform_set_strobe_polarity(void *user, uint8_t positive)
{
  (void) user;
  if (positive) {
    asdf_arch_set_pos_strobe();
  }
  else {
    asdf_arch_set_neg_strobe();
  }
}

static void arch_platform_pulse_delay_short(void *user)
{
  (void) user;
  arch_delay_us(ASDF_PULSE_DELAY_SHORT_US);
}

static void arch_platform_reset(void *user) { asdf_arch_reset(user); }

// PROCEDURE: asdf_arch_init
// Set up the platform embedded in arch and the hardware, with the default data
// and strobe polarity, and start the tick interrupt.
void asdf_arch_init(asdf_arch_t *arch)
{
  arch->platform.user = arch;
  arch->platform.read_row = arch_platform_read_row;
  arch->platform.send_code = arch_platform_send_code;
  arch->platform.set_output = arch_platform_set_output;
  arch->platform.set_strobe_polarity = arch_platform_set_strobe_polarity;
  arch->platform.pulse_delay_short = arch_platform_pulse_delay_short;
  arch->platform.reset = arch_platform_reset;
  arch->ticks = 0;

  asdf_arch_init_hardware();
  asdf_arch_reset(arch);
}
