// -*- mode: C; tab-width: 2 ; indent-tabs-mode: nil -*-
/**
 * @file asdf_arch_atmega328p.c
 *
 * Architecture-dependent code for the ATmega328P: register setup, I/O, and the
 * tick timer. Pin assignments are in asdf_arch_atmega328p.h.
 *
 * Part of the Unified Keyboard Project ASDF keyboard firmware.
 *
 * @copyright Copyright 2019 David Fenyes. GNU General Public License
 * version 3 or later; see the license notice below.
 */
//
// This program is free software: you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free Software
// Foundation, either version 3 of the License, or (at your option) any later
// version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
// details.
//
// You should have received a copy of the GNU General Public License along with
// this program. If not, see <https://www.gnu.org/licenses/>.


#include "asdf_arch.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#include <util/delay.h>
#include <stdint.h>
#include "asdf.h"
#include "asdf_config.h"
#include <stddef.h>
#include "asdf_platform.h"

/**
 * Sets one bit of an I/O port register.
 *
 * Sets the bit in the register; no other side effects.
 *
 * @param port  Pointer to the 8-bit port register.
 * @param bit   Bit position to set.
 *
 * Declared inline; it is inlined only within this module, so it is also
 * declared static.
 */
static inline void set_bit(volatile uint8_t *port, uint8_t bit)
{
  *port |= (uint8_t)(1u << bit);
}

/**
 * Clears one bit of an I/O port register.
 *
 * Clears the bit in the register; no other side effects.
 *
 * @param port  Pointer to the 8-bit port register.
 * @param bit   Bit position to clear.
 *
 * Declared inline; it is inlined only within this module, so it is also
 * declared static.
 */
static inline void clear_bit(volatile uint8_t *port, uint8_t bit)
{
  *port &= (uint8_t) ~(1u << bit);
}

/**
 * Configures timer 0 from one combined config word.
 *
 * Writes timer 0's control registers A and B and its interrupt mask register.
 *
 * @param bits  Config word holding the TCCR0A, TCCR0B and TIMSK0 values at the
 *              TMR0A_POS, TMR0B_POS and TMR0IMSK_POS offsets.
 *
 * Building all the fields in one word lets a setting whose bits span more than
 * one register, such as the waveform mode, be expressed as a single value. The
 * timer is stopped first and TCCR0B, which selects the clock and so starts the
 * timer, is written last.
 */
static void arch_timer0_config(uint32_t bits)
{
  TCCR0B = 0; // first turn off timer.
  TCCR0A = (bits >> TMR0A_POS) & 0xff;
  TIMSK0 = (bits >> TMR0IMSK_POS) & 0xff;
  TCCR0B = (bits >> TMR0B_POS) & 0xff; // Set the mode (and turn on timer) last
}

/**
 * Sets up the 1 ms tick timer.
 *
 * Configures timer 0 and enables its compare match A interrupt.
 *
 * Timer 0 runs in CTC mode, interrupting on compare match A. The compare
 * register is set before the timer is enabled, so the first period is correct.
 */
static void asdf_arch_tick_timer_init(void)
{
  // set compare register first, so timer can operate correctly as soon as it is
  // enabled.
  OCR0A = TICK_COUNT;

  // CTC mode, prescaler 64: 8 MHz / 64 = 125 kHz, so a TOP of TICK_COUNT (124)
  // gives a period of exactly 1 ms.
  arch_timer0_config(TIMER0_WFM_CTC | TIMER0_DIV64 | TIMER0_INT_ON_COMA);
}

/**
 * Collects the ticks counted since the last call.
 *
 * Clears the tick count, with interrupts masked.
 *
 * @param arch  Hardware state of the keyboard.
 * @return The number of 1 ms ticks since the last call, saturating at 255.
 */
uint8_t asdf_arch_tick(asdf_arch_t *arch)
{
  uint8_t retval;

  // read and clear as one step, so a tick counted between them is not lost
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
  {
    retval = arch->ticks;
    arch->ticks = 0;
  }
  return retval;
}

/**
 * Sets the system clock prescaler.
 *
 * Writes CLKPR so the system clock runs undivided, at F_CPU.
 */
static void asdf_arch_init_clock(void)
{
  CLKPR = (CLKPCE | SYSCLK_DIV1);
}

/**
 * Makes the LED pins outputs.
 *
 * Sets the LED data direction bits. The LED levels are not set here; the
 * keymap sets them.
 */
static void asdf_arch_init_leds(void)
{
  set_bit(&ASDF_LED1_DDR, ASDF_LED1_BIT);
  set_bit(&ASDF_LED2_DDR, ASDF_LED2_BIT);
  set_bit(&ASDF_LED3_DDR, ASDF_LED3_BIT);
}


/**
 * Turns LED1 on or off.
 *
 * Drives the LED1 port bit.
 *
 * @param value  Nonzero turns the LED on; zero turns it off.
 *
 * The LED1 pin drives the LED cathode directly, so clearing the bit pulls the
 * cathode low and lights the LED.
 *
 * Complexity: 2
 */
static void asdf_arch_led1_set(uint8_t value)
{
  if (value) {
    clear_bit(&ASDF_LED1_PORT, ASDF_LED1_BIT);
  }
  else {
    set_bit(&ASDF_LED1_PORT, ASDF_LED1_BIT);
  }
}

/**
 * Turns LED2 on or off.
 *
 * Drives the LED2 port bit.
 *
 * @param value  Nonzero turns the LED on; zero turns it off.
 *
 * The LED2 pin drives the LED through an inverting buffer, so setting the bit
 * pulls the cathode low and lights the LED.
 *
 * Complexity: 2
 */
static void asdf_arch_led2_set(uint8_t value)
{
  if (value) {
    set_bit(&ASDF_LED2_PORT, ASDF_LED2_BIT);
  }
  else {
    clear_bit(&ASDF_LED2_PORT, ASDF_LED2_BIT);
  }
}

/**
 * Turns LED3 on or off.
 *
 * Drives the LED3 port bit.
 *
 * @param value  Nonzero turns the LED on; zero turns it off.
 *
 * The LED3 pin drives the LED through an inverting buffer, so setting the bit
 * pulls the cathode low and lights the LED.
 *
 * Complexity: 2
 */
static void asdf_arch_led3_set(uint8_t value)
{
  if (value) {
    set_bit(&ASDF_LED3_PORT, ASDF_LED3_BIT);
  }
  else {
    clear_bit(&ASDF_LED3_PORT, ASDF_LED3_BIT);
  }
}

// OUTn setters. Push-pull: drive the level. open_hi (open collector): true
// releases the pin to hi-z, false drives it low. open_lo (open emitter): true
// drives it high, false releases it to hi-z.

/**
 * Drives OUT1 as a push-pull output.
 *
 * Sets the OUT1 level and makes the pin an output.
 *
 * @param value  Nonzero drives OUT1 high; zero drives it low.
 *
 * Complexity: 2
 */
static void asdf_arch_out1_set(uint8_t value)
{
  if (value) {
    set_bit(&ASDF_OUT1_PORT, ASDF_OUT1_BIT);
  }
  else {
    clear_bit(&ASDF_OUT1_PORT, ASDF_OUT1_BIT);
  }
  set_bit(&ASDF_OUT1_DDR, ASDF_OUT1_BIT);
}

/**
 * Drives OUT1 as an open-collector output.
 *
 * Sets the OUT1 data direction and port bits.
 *
 * @param value  Nonzero releases OUT1 to hi-z; zero drives it low.
 *
 * When released, the pin is an input with its weak pullup enabled.
 *
 * Complexity: 2
 */
static void asdf_arch_out1_open_hi_set(uint8_t value)
{
  if (value) {
    clear_bit(&ASDF_OUT1_DDR, ASDF_OUT1_BIT);
    set_bit(&ASDF_OUT1_PORT, ASDF_OUT1_BIT);
  }
  else {
    clear_bit(&ASDF_OUT1_PORT, ASDF_OUT1_BIT);
    set_bit(&ASDF_OUT1_DDR, ASDF_OUT1_BIT);
  }
}

/**
 * Drives OUT1 as an open-emitter output.
 *
 * Sets the OUT1 data direction and port bits.
 *
 * @param value  Nonzero drives OUT1 high; zero releases it to hi-z.
 *
 * Complexity: 2
 */
static void asdf_arch_out1_open_lo_set(uint8_t value)
{
  if (value) {
    set_bit(&ASDF_OUT1_PORT, ASDF_OUT1_BIT);
    set_bit(&ASDF_OUT1_DDR, ASDF_OUT1_BIT);
  }
  else {
    clear_bit(&ASDF_OUT1_DDR, ASDF_OUT1_BIT);
    clear_bit(&ASDF_OUT1_PORT, ASDF_OUT1_BIT);
  }
}

/**
 * Drives OUT2 as a push-pull output.
 *
 * Sets the OUT2 level and makes the pin an output.
 *
 * @param value  Nonzero drives OUT2 high; zero drives it low.
 *
 * OUT2 is inverted by the 7404 buffer, so clearing the bit sets the output
 * high. OUT2 cannot be high impedance.
 *
 * Complexity: 2
 */
static void asdf_arch_out2_set(uint8_t value)
{
  if (value) {
    clear_bit(&ASDF_OUT2_PORT, ASDF_OUT2_BIT);
  }
  else {
    set_bit(&ASDF_OUT2_PORT, ASDF_OUT2_BIT);
  }
  set_bit(&ASDF_OUT2_DDR, ASDF_OUT2_BIT);
}

/**
 * Ignores an output request.
 *
 * Handles outputs that do not exist on this board. No side effects.
 *
 * @param value  Ignored.
 */
static void asdf_arch_null_output(uint8_t value)
{
  (void) value;
}

/**
 * Ignores a request to drive OUT2 as an open-collector output.
 *
 * No side effects.
 *
 * @param value  Ignored.
 *
 * OUT2 is buffered, so its open-collector and open-emitter modes are not
 * supported on the ATmega328P ASCII interface.
 */
static void asdf_arch_out2_open_hi_set(uint8_t value)
{
  (void) value;
}

/**
 * Ignores a request to drive OUT2 as an open-emitter output.
 *
 * No side effects.
 *
 * @param value  Ignored.
 *
 * OUT2 is buffered, so its open-emitter mode is not supported on the
 * ATmega328P ASCII interface.
 */
static void asdf_arch_out2_open_lo_set(uint8_t value)
{
  (void) value;
}

/**
 * Drives OUT3 as a push-pull output.
 *
 * Sets the OUT3 level and makes the pin an output.
 *
 * @param value  Nonzero drives OUT3 high; zero drives it low.
 *
 * Complexity: 2
 */
static void asdf_arch_out3_set(uint8_t value)
{
  if (value) {
    set_bit(&ASDF_OUT3_PORT, ASDF_OUT3_BIT);
  }
  else {
    clear_bit(&ASDF_OUT3_PORT, ASDF_OUT3_BIT);
  }
  set_bit(&ASDF_OUT3_DDR, ASDF_OUT3_BIT);
}

/**
 * Drives OUT3 as an open-collector output.
 *
 * Sets the OUT3 data direction and port bits.
 *
 * @param value  Nonzero releases OUT3 to hi-z; zero drives it low.
 *
 * When released, the pin is an input with its weak pullup enabled.
 *
 * Complexity: 2
 */
static void asdf_arch_out3_open_hi_set(uint8_t value)
{
  if (value) {
    clear_bit(&ASDF_OUT3_DDR, ASDF_OUT3_BIT);
    set_bit(&ASDF_OUT3_PORT, ASDF_OUT3_BIT);
  }
  else {
    clear_bit(&ASDF_OUT3_PORT, ASDF_OUT3_BIT);
    set_bit(&ASDF_OUT3_DDR, ASDF_OUT3_BIT);
  }
}

/**
 * Drives OUT3 as an open-emitter output.
 *
 * Sets the OUT3 data direction and port bits.
 *
 * @param value  Nonzero drives OUT3 high; zero releases it to hi-z.
 *
 * Complexity: 2
 */
static void asdf_arch_out3_open_lo_set(uint8_t value)
{
  if (value) {
    set_bit(&ASDF_OUT3_PORT, ASDF_OUT3_BIT);
    set_bit(&ASDF_OUT3_DDR, ASDF_OUT3_BIT);
  }
  else {
    clear_bit(&ASDF_OUT3_DDR, ASDF_OUT3_BIT);
    clear_bit(&ASDF_OUT3_PORT, ASDF_OUT3_BIT);
  }
}

/**
 * Sets the strobe to positive polarity.
 *
 * Makes the strobe pin an output idling LOW, so each strobe is a high pulse.
 */
static void asdf_arch_set_pos_strobe(void)
{
  clear_bit(&ASDF_STROBE_PORT, ASDF_STROBE_BIT);
  set_bit(&ASDF_STROBE_DDR, ASDF_STROBE_BIT);
}

/**
 * Sets the strobe to negative polarity.
 *
 * Makes the strobe pin an output idling HIGH, so each strobe is a low pulse.
 */
static void asdf_arch_set_neg_strobe(void)
{
  set_bit(&ASDF_STROBE_PORT, ASDF_STROBE_BIT);
  set_bit(&ASDF_STROBE_DDR, ASDF_STROBE_BIT);
}

/**
 * Sets up the parallel ASCII output port.
 *
 * Makes the whole ASCII port an output and sets it to its idle value.
 *
 * @param data_polarity  Idle output value: the code 0 XORed with the data
 *                       polarity.
 */
static void asdf_arch_init_ascii_output(uint8_t data_polarity)
{

  // set all outputs
  ASDF_ASCII_PORT = data_polarity;
  ASDF_ASCII_DDR = ALL_OUTPUTS;
}

/**
 * Sets up the column shift register control lines.
 *
 * Makes COLCLK an output idling LOW, COLMODE an output idling HIGH (shift), and
 * COL an input with no weak pullup.
 *
 * COLMODE is LOW for load and HIGH for shift; the load or shift occurs when
 * COLCLK goes high.
 */
static void asdf_arch_init_column_control(void)
{

  // COLCLK is output and initialized LOW
  clear_bit(&ASDF_COLCLK_PORT, ASDF_COLCLK_BIT);
  set_bit(&ASDF_COLCLK_DDR, ASDF_COLCLK_BIT);

  // COLMODE is output and initialized HIGH
  set_bit(&ASDF_COLMODE_PORT, ASDF_COLMODE_BIT);
  set_bit(&ASDF_COLMODE_DDR, ASDF_COLMODE_BIT);

  // COL is input, no weak pullup.
  clear_bit(&ASDF_COL_DDR, ASDF_COL_BIT);
  clear_bit(&ASDF_COL_PORT, ASDF_COL_BIT);
}

/**
 * Sets up the row-select outputs.
 *
 * Makes the row-select lines outputs, initially selecting row 0. The lines
 * carry the encoded row number to the external decoder that drives the key
 * matrix row being scanned.
 */
static void asdf_arch_init_row_outputs(void)
{
  ASDF_ROW_PORT &= (uint8_t) ~ASDF_ROW_MASK;
  ASDF_ROW_DDR |= ASDF_ROW_MASK;
}



/**
 * Scans one row of the key matrix.
 *
 * Outputs the row number on the ROW port, then reads the columns of that row.
 * Leaves the ROW port selecting the row and the shift register in SHIFT mode.
 *
 * @param row  Row number to scan.
 * @return The row's columns, one bit per column: 1 = pressed, 0 = released.
 *
 * The columns are loaded in parallel into the shift register, then clocked in
 * serially, one bit per column. The keymap represents a pressed key as 1, but a
 * pressed key pulls its column line low, so each bit is inverted as it is read.
 *
 * Complexity: 2
 */
static asdf_cols_t asdf_arch_read_row(uint8_t row)
{
  asdf_cols_t cols = 0;

  // first, output the new row value:
  ASDF_ROW_PORT = (uint8_t) ((ASDF_ROW_PORT & ~ASDF_ROW_MASK)
                             | ((row & ASDF_ROW_MASK) << ASDF_ROW_OFFSET));


  // read in the columns.  Set LOAD mode and pulse clock.
  clear_bit(&ASDF_COLMODE_PORT, ASDF_COLMODE_BIT);

  set_bit(&ASDF_COLCLK_PORT, ASDF_COLCLK_BIT);
  clear_bit(&ASDF_COLCLK_PORT, ASDF_COLCLK_BIT);

  // set back to SHIFT mode
  set_bit(&ASDF_COLMODE_PORT, ASDF_COLMODE_BIT);

  // After the load operation, the LSB is already at the output pin, so each bit
  // is read before the clock pulse that shifts in the next.
  for (uint8_t i = 0; i < ASDF_MAX_COLS; i++) {

    // invert: a pressed key reads low
    cols |= (asdf_cols_t)(((((uint8_t) ~ASDF_COL_PIN) >> ASDF_COL_BIT) & 1u) << i);

    set_bit(&ASDF_COLCLK_PORT, ASDF_COLCLK_BIT);
    clear_bit(&ASDF_COLCLK_PORT, ASDF_COLCLK_BIT);
  }

  return cols;
}


/**
 * Sends a code on the parallel ASCII port.
 *
 * Outputs the code, XORed with the data polarity, on the ASCII port, then
 * pulses the strobe for ASDF_STROBE_LENGTH_US. The data stays on the port
 * until the next code is sent. This routine could be replaced with a UART, I2C,
 * USB or other output mechanism.
 *
 * @param arch  Hardware state, giving the data polarity.
 * @param code  The 7-bit ASCII code to send.
 *
 * The strobe is toggled rather than set, so it returns to the idle level set
 * by the strobe polarity.
 */
static void asdf_arch_send_code(const asdf_arch_t *arch, asdf_keycode_t code)
{
  ASDF_ASCII_PORT = (code ^ arch->data_polarity);


  // Writing 1 to a PIN register bit toggles the output, so two toggles pulse
  // the strobe and return it to the idle level set by the strobe polarity.
  set_bit(&ASDF_STROBE_PIN, ASDF_STROBE_BIT);

  _delay_us(ASDF_STROBE_LENGTH_US);

  set_bit(&ASDF_STROBE_PIN, ASDF_STROBE_BIT);
}

// Output handlers, indexed by physical output, kept in flash.
typedef void (*asdf_arch_output_handler_t)(uint8_t);

static const asdf_arch_output_handler_t FLASH output_handlers[ASDF_PHYSICAL_NUM_RESOURCES] = {
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
  if (output < ASDF_PHYSICAL_NUM_RESOURCES) {
    // copied out of flash, rather than cast from a data pointer
    asdf_arch_output_handler_t handler;
    FLASH_MEMCPY(&handler, &output_handlers[output], sizeof(handler));
    handler(value);
  }
}

/**
 * Returns the ASCII output to its default state.
 *
 * Sets the data polarity and strobe polarity to their defaults, and sets the
 * ASCII output port to idle.
 *
 * @param arch  Hardware state to reset.
 *
 * Complexity: 2
 */
static void asdf_arch_reset(asdf_arch_t *arch)
{
  arch->data_polarity = ASDF_DEFAULT_DATA_POLARITY;
  asdf_arch_init_ascii_output(arch->data_polarity);

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
 * Selects the row on the ROW port.
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
 * @param positive  Nonzero for a positive strobe (idles low); zero for a
 *                  negative strobe (idles high).
 *
 * Complexity: 2
 */
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
  _delay_us(ASDF_PULSE_DELAY_SHORT_US);
}

/**
 * Platform operation: returns the ASCII output to its default state.
 *
 * Restores the default data and strobe polarity and idles the ASCII port.
 *
 * @param user  Platform context: the keyboard's asdf_arch_t.
 */
static void arch_platform_reset(void *user)
{
  asdf_arch_reset(user);
}

/**
 * Sets up the keyboard hardware and the platform embedded in arch.
 *
 * Sets up the clock, the tick timer, the ASCII output, LEDs, row outputs and
 * column control lines; fills in the platform operations; clears the tick
 * count; and enables interrupts, starting the tick interrupt.
 *
 * @param arch  Hardware state to initialize.
 *
 * Interrupts are disabled during setup, so the tick interrupt cannot run on a
 * half-initialized state.
 */
void asdf_arch_init(asdf_arch_t *arch)
{
  // disable interrupts:
  cli();

  arch->platform.user = arch;
  arch->platform.read_row = arch_platform_read_row;
  arch->platform.send_code = arch_platform_send_code;
  arch->platform.set_output = arch_platform_set_output;
  arch->platform.set_strobe_polarity = arch_platform_set_strobe_polarity;
  arch->platform.pulse_delay_short = arch_platform_pulse_delay_short;
  arch->platform.reset = arch_platform_reset;

  // clear the tick count;
  arch->ticks = 0;

  // set up timers for 1 msec intervals
  asdf_arch_init_clock();
  asdf_arch_tick_timer_init();

  // set up the ASCII output port, and the data and strobe polarity
  asdf_arch_reset(arch);

  asdf_arch_init_leds();

  // set up row output port
  asdf_arch_init_row_outputs();

  // set up column control lines
  asdf_arch_init_column_control();

  // enable interrupts:
  sei();
}

//-------|---------|---------+---------+---------+---------+---------+---------+
// Above line is 80 columns, and should display completely in the editor.
//
