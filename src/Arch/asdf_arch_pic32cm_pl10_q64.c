// -*- mode: C; tab-width: 2 ; indent-tabs-mode: nil -*-
/**
 * @file asdf_arch_pic32cm_pl10_q64.c
 *
 * PIC32CM6408PL10064 (64-pin, "2560-class") arch implementation. The scan/read
 * and output logic reproduces asdf_arch_atmega2560.c; only the pin-I/O
 * primitives change (Harmony-style PORT_REGS->GROUP[g].PORT_* registers via the
 * helpers in asdf_arch_pic32cm_common.h). Shared clock/tick/delay live in
 * asdf_arch_pic32cm_common.c.
 *
 * Part of the Unified Keyboard Project ASDF keyboard firmware.
 */

#include <stdbool.h>
#include "asdf_arch.h"
#include "asdf_config.h" // ASDF_DEFAULT_DATA_POLARITY, ASDF_PULSE_DELAY_SHORT_US
#include <stddef.h>
#include "asdf_platform.h"


/**
 * Scans one row of the key matrix.
 *
 * Drives the selected row line low, then reads the columns. Leaves the row
 * lines selecting the row.
 *
 * @param row  Row number to scan.
 * @return The row's columns, one bit per column: 1 = pressed, 0 = released.
 *
 * The 16 row lines (PA00..PA15) are one-hot and active low, written in one
 * masked write. After a settling delay (ASDF_KEYBOARD_ROW_SETTLING_TIME_US)
 * the columns are read. A pressed key pulls its column line low, so the column
 * bits are inverted.
 */
static asdf_cols_t asdf_arch_read_row(uint8_t row)
{
  uint32_t one_hot = (~((uint32_t) 1u << row)) & ROW_MASK;

  PORT_REGS->GROUP[ROW_GROUP].PORT_OUT =
    (PORT_REGS->GROUP[ROW_GROUP].PORT_OUT & ~ROW_MASK) | one_hot;
  arch_delay_us(ASDF_KEYBOARD_ROW_SETTLING_TIME_US);

  return (asdf_cols_t)((~PORT_REGS->GROUP[COL_GROUP].PORT_IN) & COL_MASK);
}

/**
 * Sends a code on the parallel ASCII port.
 *
 * Outputs the code, XORed with the data polarity, on the ASCII port, then
 * pulses the strobe for ASDF_STROBE_LENGTH_US. The data stays on the port
 * until the next code is sent.
 *
 * @param arch  Hardware state, giving the data polarity.
 * @param code  The code to send.
 *
 * The ASCII byte (PA16..PA23) is written with one masked write. OUTTGL toggles
 * the strobe twice (assert, then deassert), reproducing the AVR PINx
 * write-toggle, so the strobe returns to the idle level set by the polarity
 * setter.
 */
static void asdf_arch_send_code(const asdf_arch_t *arch, asdf_keycode_t code)
{
  uint32_t ascii = ((uint32_t)((uint8_t)(code ^ arch->data_polarity)) << ASCII_SHIFT) & ASCII_MASK;

  PORT_REGS->GROUP[ASCII_GROUP].PORT_OUT =
    (PORT_REGS->GROUP[ASCII_GROUP].PORT_OUT & ~ASCII_MASK) | ascii;

  pin_toggle(STROBE_GROUP, STROBE_PIN);
  arch_delay_us(ASDF_STROBE_LENGTH_US);
  pin_toggle(STROBE_GROUP, STROBE_PIN);
}

/**
 * Ignores an output request.
 *
 * Handles outputs that do not exist on this board. No side effects.
 *
 * @param value  Ignored.
 */
static void asdf_arch_null_output(uint8_t value) { (void) value; }

/**
 * Sets the strobe to positive polarity.
 *
 * Makes the strobe pin an output idling LOW, so each strobe is a high pulse.
 */
static void asdf_arch_set_pos_strobe(void)
{
  pin_clear(STROBE_GROUP, STROBE_PIN); // idle low
  pin_dir_out(STROBE_GROUP, STROBE_PIN);
}

/**
 * Sets the strobe to negative polarity.
 *
 * Makes the strobe pin an output idling HIGH, so each strobe is a low pulse.
 */
static void asdf_arch_set_neg_strobe(void)
{
  pin_set(STROBE_GROUP, STROBE_PIN); // idle high
  pin_dir_out(STROBE_GROUP, STROBE_PIN);
}

// LEDs are active low (on => drive low), matching the 2560.

/**
 * Turns LED1 on or off.
 *
 * Drives the LED1 pin.
 *
 * @param value  Nonzero turns the LED on; zero turns it off.
 *
 * Complexity: 2
 */
static void asdf_arch_led1_set(uint8_t value)
{
  if (value != 0u) {
    pin_clear(LED1_GROUP, LED1_PIN);
  } else {
    pin_set(LED1_GROUP, LED1_PIN);
  }
}

/**
 * Turns LED2 on or off.
 *
 * Drives the LED2 pin.
 *
 * @param value  Nonzero turns the LED on; zero turns it off.
 *
 * Complexity: 2
 */
static void asdf_arch_led2_set(uint8_t value)
{
  if (value != 0u) {
    pin_clear(LED2_GROUP, LED2_PIN);
  } else {
    pin_set(LED2_GROUP, LED2_PIN);
  }
}

/**
 * Turns LED3 on or off.
 *
 * Drives the LED3 pin.
 *
 * @param value  Nonzero turns the LED on; zero turns it off.
 *
 * Complexity: 2
 */
static void asdf_arch_led3_set(uint8_t value)
{
  if (value != 0u) {
    pin_clear(LED3_GROUP, LED3_PIN);
  } else {
    pin_set(LED3_GROUP, LED3_PIN);
  }
}

/**
 * Drives an OUT pin as a push-pull output.
 *
 * Sets the pin's level and makes it an output.
 *
 * @param g      Port group of the pin.
 * @param b      Bit (pin) number within the group.
 * @param value  Nonzero drives the pin high; zero drives it low.
 *
 * Complexity: 2
 */
static inline void out_set(uint8_t g, uint8_t b, uint8_t value)
{
  if (value != 0u) {
    pin_set(g, b);
  } else {
    pin_clear(g, b);
  }
  pin_dir_out(g, b);
}

// Open-drain variants, matching the 2560 asymmetric sense:
//   open_hi: value true => Hi-Z (released high), false => driven low
//   open_lo: value true => driven high, false => Hi-Z (released)

/**
 * Drives an OUT pin as an open-collector output.
 *
 * Sets the pin's direction, and its level when driven.
 *
 * @param g      Port group of the pin.
 * @param b      Bit (pin) number within the group.
 * @param value  Nonzero releases the pin to hi-z; zero drives it low.
 *
 * A released pin is an input with no pull resistor, so an external pullup
 * sets the high level.
 *
 * Complexity: 2
 */
static inline void out_open_hi(uint8_t g, uint8_t b, uint8_t value)
{
  if (value != 0u) {
    pin_dir_in(g, b);
  }
  else {
    pin_clear(g, b);
    pin_dir_out(g, b);
  }
}

/**
 * Drives an OUT pin as an open-emitter output.
 *
 * Sets the pin's direction, and its level when driven.
 *
 * @param g      Port group of the pin.
 * @param b      Bit (pin) number within the group.
 * @param value  Nonzero drives the pin high; zero releases it to hi-z.
 *
 * A released pin is an input with no pull resistor, so an external pulldown
 * sets the low level.
 *
 * Complexity: 2
 */
static inline void out_open_lo(uint8_t g, uint8_t b, uint8_t value)
{
  if (value != 0u) {
    pin_set(g, b);
    pin_dir_out(g, b);
  }
  else {
    pin_dir_in(g, b);
  }
}

/**
 * Drives OUT1 as a push-pull output.
 *
 * Sets the OUT1 level and makes the pin an output.
 *
 * @param value  Nonzero drives OUT1 high; zero drives it low.
 */
static void asdf_arch_out1_set(uint8_t value) { out_set(OUT1_GROUP, OUT1_PIN, value); }

/**
 * Drives OUT1 as an open-collector output.
 *
 * Sets the OUT1 pin direction, and its level when driven.
 *
 * @param value  Nonzero releases OUT1 to hi-z; zero drives it low.
 */
static void asdf_arch_out1_open_hi_set(uint8_t value) { out_open_hi(OUT1_GROUP, OUT1_PIN, value); }

/**
 * Drives OUT1 as an open-emitter output.
 *
 * Sets the OUT1 pin direction, and its level when driven.
 *
 * @param value  Nonzero drives OUT1 high; zero releases it to hi-z.
 */
static void asdf_arch_out1_open_lo_set(uint8_t value) { out_open_lo(OUT1_GROUP, OUT1_PIN, value); }

/**
 * Drives OUT2 as a push-pull output.
 *
 * Sets the OUT2 level and makes the pin an output.
 *
 * @param value  Nonzero drives OUT2 high; zero drives it low.
 */
static void asdf_arch_out2_set(uint8_t value) { out_set(OUT2_GROUP, OUT2_PIN, value); }

/**
 * Drives OUT2 as an open-collector output.
 *
 * Sets the OUT2 pin direction, and its level when driven.
 *
 * @param value  Nonzero releases OUT2 to hi-z; zero drives it low.
 */
static void asdf_arch_out2_open_hi_set(uint8_t value) { out_open_hi(OUT2_GROUP, OUT2_PIN, value); }

/**
 * Drives OUT2 as an open-emitter output.
 *
 * Sets the OUT2 pin direction, and its level when driven.
 *
 * @param value  Nonzero drives OUT2 high; zero releases it to hi-z.
 */
static void asdf_arch_out2_open_lo_set(uint8_t value) { out_open_lo(OUT2_GROUP, OUT2_PIN, value); }

/**
 * Drives OUT3 as a push-pull output.
 *
 * Sets the OUT3 level and makes the pin an output.
 *
 * @param value  Nonzero drives OUT3 high; zero drives it low.
 */
static void asdf_arch_out3_set(uint8_t value) { out_set(OUT3_GROUP, OUT3_PIN, value); }

/**
 * Drives OUT3 as an open-collector output.
 *
 * Sets the OUT3 pin direction, and its level when driven.
 *
 * @param value  Nonzero releases OUT3 to hi-z; zero drives it low.
 */
static void asdf_arch_out3_open_hi_set(uint8_t value) { out_open_hi(OUT3_GROUP, OUT3_PIN, value); }

/**
 * Drives OUT3 as an open-emitter output.
 *
 * Sets the OUT3 pin direction, and its level when driven.
 *
 * @param value  Nonzero drives OUT3 high; zero releases it to hi-z.
 */
static void asdf_arch_out3_open_lo_set(uint8_t value) { out_open_lo(OUT3_GROUP, OUT3_PIN, value); }

/**
 * Sets up the clock, the tick timer and the pins.
 *
 * Brings up the shared clock and SysTick tick, which starts the tick
 * interrupt; makes the rows, the ASCII byte, the LEDs and OUT1-3 outputs, and
 * the columns inputs.
 *
 * Complexity: 2
 */
static void asdf_arch_init_hardware(void)
{
  asdf_arch_common_clock_init();
  asdf_arch_common_tick_init();

  // Rows and the ASCII byte are outputs; columns are inputs.
  PORT_REGS->GROUP[ROW_GROUP].PORT_DIRSET = ROW_MASK;
  PORT_REGS->GROUP[ASCII_GROUP].PORT_DIRSET = ASCII_MASK;
  for (uint8_t b = 0; b < ASDF_MAX_COLS; b++) {
    pin_dir_in(COL_GROUP, b);
  }

  // LEDs and OUT1-3 are outputs (their setters assume an output direction).
  pin_dir_out(LED1_GROUP, LED1_PIN);
  pin_dir_out(LED2_GROUP, LED2_PIN);
  pin_dir_out(LED3_GROUP, LED3_PIN);
  pin_dir_out(OUT1_GROUP, OUT1_PIN);
  pin_dir_out(OUT2_GROUP, OUT2_PIN);
  pin_dir_out(OUT3_GROUP, OUT3_PIN);
}

// Output handlers, indexed by physical output.
typedef void (*asdf_arch_output_handler_t)(uint8_t value);


/**
 * Drives a physical output through its handler.
 *
 * Calls the output's handler, which drives the pin. Invalid outputs are
 * ignored.
 *
 * @param output  The physical output to drive.
 * @param value   The value to drive it to.
 *
 * Complexity: 2
 */
static void asdf_arch_set_output(asdf_physical_dev_t output, uint8_t value)
{
  // Output handlers, indexed by physical output.
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

  if (output < ASDF_PHYSICAL_NUM_RESOURCES) {
    output_handlers[output](value);
  }
}

/**
 * Returns the ASCII output to its default state.
 *
 * Sets the data polarity and strobe polarity to their defaults, and sets the
 * ASCII output to idle (the code 0 XORed with the data polarity).
 *
 * @param arch  Hardware state to reset.
 *
 * Complexity: 2
 */
static void asdf_arch_reset(asdf_arch_t *arch)
{
  arch->data_polarity = ASDF_DEFAULT_DATA_POLARITY;
  PORT_REGS->GROUP[ASCII_GROUP].PORT_OUT =
    (PORT_REGS->GROUP[ASCII_GROUP].PORT_OUT & ~ASCII_MASK)
    | (((uint32_t) arch->data_polarity << ASCII_SHIFT) & ASCII_MASK);

  if (ASDF_DEFAULT_STROBE_POLARITY == ASDF_POSITIVE_POLARITY) { //lint !e506 !e774 build-time configuration
    asdf_arch_set_pos_strobe();
  }
  else {
    asdf_arch_set_neg_strobe();
  }
}

// Platform operations (see asdf_platform.h). They adapt the architecture's
// operations to the typed platform interface. The user pointer is the
// keyboard's asdf_arch_t.

/**
 * Platform operation: scans one row of the key matrix.
 *
 * Selects the row on the row outputs.
 *
 * @param user  Platform context (the keyboard's asdf_arch_t); unused.
 * @param row   Row number to scan.
 * @return The row's columns, one bit per column: 1 = pressed.
 */
static asdf_cols_t arch_platform_read_row(void *user, uint8_t row)
{
  (void) user;
  return asdf_arch_read_row(row);
}

/**
 * Platform operation: sends a code on the parallel ASCII port.
 *
 * Drives the ASCII port and pulses the strobe.
 *
 * @param user  Platform context: the keyboard's asdf_arch_t.
 * @param code  The code to send.
 */
static void arch_platform_send_code(void *user, asdf_keycode_t code)
{
  asdf_arch_send_code(user, code);
}

/**
 * Platform operation: drives a physical output.
 *
 * Drives the output's pin; invalid outputs are ignored.
 *
 * @param user    Platform context (the keyboard's asdf_arch_t); unused.
 * @param output  The physical output to drive.
 * @param value   The value to drive it to.
 */
static void arch_platform_set_output(void *user, asdf_physical_dev_t output, uint8_t value)
{
  (void) user;
  asdf_arch_set_output(output, value);
}

/**
 * Platform operation: sets the strobe polarity.
 *
 * Sets the strobe pin to the idle level of the chosen polarity.
 *
 * @param user      Platform context (the keyboard's asdf_arch_t); unused.
 * @param positive  True for a positive strobe (idles low); false for a
 *                  negative strobe (idles high).
 *
 * Complexity: 2
 */
static void arch_platform_set_strobe_polarity(void *user, bool positive)
{
  (void) user;
  if (positive) {
    asdf_arch_set_pos_strobe();
  }
  else {
    asdf_arch_set_neg_strobe();
  }
}

/**
 * Platform operation: waits for a short output pulse.
 *
 * Busy-waits for ASDF_PULSE_DELAY_SHORT_US. No side effects.
 *
 * @param user  Platform context (the keyboard's asdf_arch_t); unused.
 */
static void arch_platform_pulse_delay_short(void *user)
{
  (void) user;
  arch_delay_us(ASDF_PULSE_DELAY_SHORT_US);
}

/**
 * Platform operation: returns the ASCII output to its default state.
 *
 * Restores the default data and strobe polarity and idles the ASCII port.
 *
 * @param user  Platform context: the keyboard's asdf_arch_t.
 */
static void arch_platform_reset(void *user) { asdf_arch_reset(user); }

/**
 * Sets up the keyboard hardware and the platform embedded in arch.
 *
 * Fills in the platform operations and clears the tick count, then sets up the
 * clock, the tick timer and the pins, which starts the tick interrupt, and
 * sets the default data and strobe polarity.
 *
 * @param arch  Hardware state to initialize.
 *
 * The platform and tick count are set before the tick interrupt starts, so the
 * interrupt never sees an uninitialized state.
 */
void asdf_arch_init(asdf_arch_t *arch)
{
  arch->platform.user = arch;
  arch->platform.read_row = &arch_platform_read_row;
  arch->platform.send_code = &arch_platform_send_code;
  arch->platform.set_output = &arch_platform_set_output;
  arch->platform.set_strobe_polarity = &arch_platform_set_strobe_polarity;
  arch->platform.pulse_delay_short = &arch_platform_pulse_delay_short;
  arch->platform.reset = &arch_platform_reset;
  arch->ticks = 0;

  asdf_arch_init_hardware();
  asdf_arch_reset(arch);
}
