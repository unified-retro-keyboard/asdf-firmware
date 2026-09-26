// -*- mode: C; tab-width: 2 ; indent-tabs-mode: nil -*-
/**
 * @file asdf_arch_test.c
 *
 * Emulated hardware for the host unit tests. Outputs are recorded in arrays,
 * each with a pulse detector, and sent codes are latched for the tests to check.
 *
 * Part of the Unified Keyboard Project ASDF keyboard firmware.
 *
 * @copyright Copyright 2019 David F. MIT License; see LICENSE.
 */
// SPDX-License-Identifier: MIT

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "asdf.h"
#include "asdf_config.h"
#include "asdf_physical.h"
#include "asdf_arch.h"
#include <stddef.h>
#include "asdf_platform.h"

typedef enum {
  PULSE_EVENT_SET_HIGH,
  PULSE_EVENT_SET_LOW,
  PULSE_EVENT_DELAY,
  NUM_PULSE_EVENTS
} pulse_event_t;

// Emulated output values and their pulse detectors, indexed by physical device.
static uint8_t output_values[ASDF_PHYSICAL_NUM_RESOURCES];
static pulse_state_t pulses[ASDF_PHYSICAL_NUM_RESOURCES];

static bool strobe_is_positive;

static asdf_keycode_t code_register;
static bool code_sent;

/**
 * Emulates sending a code.
 *
 * Latches the code in the code register, where a test can check it, and sets
 * the code-sent flag.
 *
 * @param code  The code sent.
 */
void asdf_arch_send_code(asdf_keycode_t code)
{
  code_register = code;
  code_sent = true;
}

/**
 * Collects the last code sent.
 *
 * Clears the code-sent flag.
 *
 * @return The code in the code register: the last code sent.
 */
asdf_keycode_t asdf_arch_get_sent_code(void)
{
  code_sent = false;
  return code_register;
}

/**
 * Reports whether a code is waiting to be collected.
 *
 * No side effects.
 *
 * @return The code-sent flag: TRUE if a code was sent since the last
 *         asdf_arch_get_sent_code() or reset.
 */
bool asdf_arch_was_code_sent(void)
{
  return code_sent;
}


/**
 * Computes a pulse detector's next state.
 *
 * No side effects.
 *
 * @param current_state  The detector's current state.
 * @param event          The event: the output set high, set low, or a delay.
 * @return The next state from the transition table, or current_state if it is
 *         an error state; error states are terminal.
 *
 * Complexity: 2
 */
static pulse_state_t pulse_detect(pulse_state_t current_state, pulse_event_t event)
{
  // Pulse detector transitions: a pulse is a transition, a pulse delay, then a
  // transition back. A delay without a preceding transition, a second delay, or
  // no transition after the delay is an error. Error states have no row here.
  static const pulse_state_t
    pulse_transition_table[PD_ST_NUM_VALID_PULSE_STATES][NUM_PULSE_EVENTS] =
    {
     [PD_ST_INITIAL_STATE] =
     {
      [PULSE_EVENT_SET_HIGH] = PD_ST_STABLE_HIGH,
      [PULSE_EVENT_SET_LOW] = PD_ST_STABLE_LOW,
      [PULSE_EVENT_DELAY] = PD_ST_ERROR_PULSE_FROM_INITIAL_STATE,
     },
     [PD_ST_STABLE_LOW] =
     {
      [PULSE_EVENT_SET_HIGH] = PD_ST_TRANSITION_HIGH,
      [PULSE_EVENT_SET_LOW] = PD_ST_STABLE_LOW,
      [PULSE_EVENT_DELAY] = PD_ST_ERROR_NO_TRANSITION_BEFORE_DELAY,
     },
     [PD_ST_STABLE_HIGH] =
     {
      [PULSE_EVENT_SET_HIGH] = PD_ST_STABLE_HIGH,
      [PULSE_EVENT_SET_LOW] = PD_ST_TRANSITION_LOW,
      [PULSE_EVENT_DELAY] = PD_ST_ERROR_NO_TRANSITION_BEFORE_DELAY,
     },
     [PD_ST_TRANSITION_LOW] =
     {
      [PULSE_EVENT_SET_HIGH] = PD_ST_TRANSITION_HIGH,
      [PULSE_EVENT_SET_LOW] = PD_ST_STABLE_LOW,
      [PULSE_EVENT_DELAY] = PD_ST_PULSE_DELAY_LOW,
     },
     [PD_ST_TRANSITION_HIGH] =
     {
      [PULSE_EVENT_SET_HIGH] = PD_ST_STABLE_HIGH,
      [PULSE_EVENT_SET_LOW] = PD_ST_TRANSITION_LOW,
      [PULSE_EVENT_DELAY] = PD_ST_PULSE_DELAY_HIGH,
     },
     [PD_ST_PULSE_DELAY_LOW] =
     {
      [PULSE_EVENT_SET_HIGH] = PD_ST_PULSE_LOW_DETECTED,
      [PULSE_EVENT_SET_LOW] = PD_ST_ERROR_NO_TRANSITION_AFTER_DELAY,
      [PULSE_EVENT_DELAY] = PD_ST_ERROR_DOUBLE_DELAY,
     },
     [PD_ST_PULSE_DELAY_HIGH] = 
     {
      [PULSE_EVENT_SET_HIGH] = PD_ST_ERROR_NO_TRANSITION_AFTER_DELAY,
      [PULSE_EVENT_SET_LOW] = PD_ST_PULSE_HIGH_DETECTED,
      [PULSE_EVENT_DELAY] = PD_ST_ERROR_DOUBLE_DELAY,
     },
     [PD_ST_PULSE_HIGH_DETECTED] = 
     {
      [PULSE_EVENT_SET_HIGH] = PD_ST_TRANSITION_HIGH,
      [PULSE_EVENT_SET_LOW] = PD_ST_STABLE_LOW,
      [PULSE_EVENT_DELAY] = PD_ST_ERROR_NO_TRANSITION_BEFORE_DELAY
     },
     [PD_ST_PULSE_LOW_DETECTED] = 
     {
      [PULSE_EVENT_SET_HIGH] = PD_ST_STABLE_HIGH,
      [PULSE_EVENT_SET_LOW] = PD_ST_TRANSITION_LOW,
      [PULSE_EVENT_DELAY] = PD_ST_ERROR_NO_TRANSITION_BEFORE_DELAY
     },
    };

  pulse_state_t next_state = current_state;

  // advance state if current state is valid (not an error state)
  if (current_state < PD_ST_NUM_VALID_PULSE_STATES) {
    next_state = pulse_transition_table[current_state][event];
  }
  return next_state;
}


/**
 * Records an output's value and advances its pulse detector.
 *
 * Stores the value in the emulated outputs and advances the output's pulse
 * detector with a set-high (nonzero) or set-low (zero) event.
 *
 * @param output_dev  The output to set; must be a valid device.
 * @param value       The value written to the output.
 *
 * Complexity: 2
 */
static void record_output(asdf_physical_dev_t output_dev, uint8_t value)
{
  pulse_event_t pulse_event = (value != 0u) ? PULSE_EVENT_SET_HIGH : PULSE_EVENT_SET_LOW;

  output_values[output_dev] = value;
  pulses[output_dev] = pulse_detect(pulses[output_dev], pulse_event);
}

/**
 * Emulates writing the nonexistent output.
 *
 * Records the value on PHYSICAL_NO_OUT and advances its pulse detector.
 *
 * @param value  Value written to the output.
 */
void asdf_arch_null_output(uint8_t value)
{
  record_output(PHYSICAL_NO_OUT, value);
}


/**
 * Emulates setting LED1.
 *
 * Records the value on PHYSICAL_LED1 and advances its pulse detector.
 *
 * @param value  Value written to the output.
 */
void asdf_arch_led1_set(uint8_t value)
{
  record_output(PHYSICAL_LED1, value);
}

/**
 * Emulates setting LED2.
 *
 * Records the value on PHYSICAL_LED2 and advances its pulse detector.
 *
 * @param value  Value written to the output.
 */
void asdf_arch_led2_set(uint8_t value)
{
  record_output(PHYSICAL_LED2, value);
}

/**
 * Emulates setting LED3.
 *
 * Records the value on PHYSICAL_LED3 and advances its pulse detector.
 *
 * @param value  Value written to the output.
 */
void asdf_arch_led3_set(uint8_t value)
{
  record_output(PHYSICAL_LED3, value);
}

/**
 * Emulates setting OUT1 as a push-pull output.
 *
 * Records the value on PHYSICAL_OUT1 and advances its pulse detector.
 *
 * @param value  Value written to the output.
 */
void asdf_arch_out1_set(uint8_t value)
{
  record_output(PHYSICAL_OUT1, value);
}

/**
 * Emulates setting OUT1 as an open-collector output.
 *
 * Records the value on PHYSICAL_OUT1_OPEN_HI and advances its pulse detector.
 *
 * @param value  Value written to the output.
 */
void asdf_arch_out1_open_hi_set(uint8_t value)
{
  record_output(PHYSICAL_OUT1_OPEN_HI, value);
}

/**
 * Emulates setting OUT1 as an open-emitter output.
 *
 * Records the value on PHYSICAL_OUT1_OPEN_LO and advances its pulse detector.
 *
 * @param value  Value written to the output.
 */
void asdf_arch_out1_open_lo_set(uint8_t value)
{
  record_output(PHYSICAL_OUT1_OPEN_LO, value);
}

/**
 * Emulates setting OUT2 as a push-pull output.
 *
 * Records the value on PHYSICAL_OUT2 and advances its pulse detector.
 *
 * @param value  Value written to the output.
 */
void asdf_arch_out2_set(uint8_t value)
{
  record_output(PHYSICAL_OUT2, value);
}


/**
 * Emulates setting OUT2 as an open-collector output.
 *
 * Records the value on PHYSICAL_OUT2_OPEN_HI and advances its pulse detector.
 *
 * @param value  Value written to the output.
 */
void asdf_arch_out2_open_hi_set(uint8_t value)
{
  record_output(PHYSICAL_OUT2_OPEN_HI, value);
}

/**
 * Emulates setting OUT2 as an open-emitter output.
 *
 * Records the value on PHYSICAL_OUT2_OPEN_LO and advances its pulse detector.
 *
 * @param value  Value written to the output.
 */
void asdf_arch_out2_open_lo_set(uint8_t value)
{
  record_output(PHYSICAL_OUT2_OPEN_LO, value);
}

/**
 * Emulates setting OUT3 as a push-pull output.
 *
 * Records the value on PHYSICAL_OUT3 and advances its pulse detector.
 *
 * @param value  Value written to the output.
 */
void asdf_arch_out3_set(uint8_t value)
{
  record_output(PHYSICAL_OUT3, value);
}

/**
 * Emulates setting OUT3 as an open-collector output.
 *
 * Records the value on PHYSICAL_OUT3_OPEN_HI and advances its pulse detector.
 *
 * @param value  Value written to the output.
 */
void asdf_arch_out3_open_hi_set(uint8_t value)
{
  record_output(PHYSICAL_OUT3_OPEN_HI, value);
}

/**
 * Emulates setting OUT3 as an open-emitter output.
 *
 * Records the value on PHYSICAL_OUT3_OPEN_LO and advances its pulse detector.
 *
 * @param value  Value written to the output.
 */
void asdf_arch_out3_open_lo_set(uint8_t value)
{
  record_output(PHYSICAL_OUT3_OPEN_LO, value);
}

/**
 * Reports the value of an emulated output.
 *
 * No side effects.
 *
 * @param device  The physical output to check; must be a valid device.
 * @return The last value written to the output.
 */
uint8_t asdf_arch_check_output(asdf_physical_dev_t device)
{
  return output_values[device];
}

/**
 * Reports the pulse detector state of an emulated output.
 *
 * No side effects.
 *
 * @param device  The physical output to check; must be a valid device.
 * @return The output's pulse detector state, as a pulse_state_t.
 */
uint8_t asdf_arch_check_pulse(asdf_physical_dev_t device)
{
  return (uint8_t) pulses[device];
}

/**
 * Emulates a delay.
 *
 * Advances every output's pulse detector with a delay event.
 *
 * Complexity: 2
 */
static void asdf_arch_pulse_delay(void)
{
  for (uint8_t i = 0u; i < (uint8_t) ASDF_PHYSICAL_NUM_RESOURCES; i++) {
    pulses[i] = pulse_detect(pulses[i], PULSE_EVENT_DELAY);
  }
}

/**
 * Emulates setting the strobe to positive polarity.
 *
 * Records the strobe polarity as positive.
 */
void asdf_arch_set_pos_strobe(void)
{
  strobe_is_positive = true;
}

/**
 * Emulates setting the strobe to negative polarity.
 *
 * Records the strobe polarity as negative.
 */
void asdf_arch_set_neg_strobe(void)
{
  strobe_is_positive = false;
}

/**
 * Reports the recorded strobe polarity.
 *
 * No side effects.
 *
 * @return true if the strobe polarity is positive; false if it is negative.
 */
bool asdf_arch_is_strobe_positive(void)
{
  return strobe_is_positive;
}


/**
 * Resets the emulated hardware.
 *
 * Sets every output to 0 with its pulse detector in PD_ST_INITIAL_STATE,
 * clears the code-sent flag, and sets negative strobe polarity.
 *
 * Complexity: 2
 */
void asdf_arch_test_reset(void)
{
  for (uint8_t i = 0u; i < (uint8_t) ASDF_PHYSICAL_NUM_RESOURCES; i++) {
    output_values[i] = 0;
    pulses[i] = PD_ST_INITIAL_STATE;
  }

  // initially, no keycodes have been sent via asdf_arch_send_code:
  code_sent = false;
  strobe_is_positive = false;
}


/**
 * Reads one row of the default emulated key matrix, which has no keys pressed.
 *
 * Tests that press keys define their own asdf_arch_read_row(), which replaces
 * this weak definition. No side effects.
 *
 * @param row  Row number to read; ignored.
 * @return 0 (no keys pressed).
 */
__attribute__((weak)) asdf_cols_t asdf_arch_read_row(uint8_t row)
{
  (void) row;
  return 0;
}

// Platform operations (see asdf_platform.h). They adapt the emulated hardware
// to the typed platform interface. There is one set of emulated hardware, so
// the user pointer is unused.

/**
 * Platform operation: reads one row of the emulated key matrix.
 *
 * Has the side effects, if any, of the test's asdf_arch_read_row().
 *
 * @param user  Platform context; unused.
 * @param row   Row number to read.
 * @return The row's columns, one bit per column, with 1 = pressed.
 */
static asdf_cols_t arch_platform_read_row(void *user, uint8_t row)
{
  (void) user;
  return asdf_arch_read_row(row);
}

/**
 * Platform operation: emulates sending a code.
 *
 * Latches the code and sets the code-sent flag.
 *
 * @param user  Platform context; unused.
 * @param code  The code sent.
 */
static void arch_platform_send_code(void *user, asdf_keycode_t code)
{
  (void) user;
  asdf_arch_send_code(code);
}

/**
 * Platform operation: emulates driving a physical output.
 *
 * Records the value on the output and advances its pulse detector. Invalid
 * outputs are ignored.
 *
 * @param user    Platform context; unused.
 * @param output  The physical output to drive.
 * @param value   The value written to it.
 *
 * Complexity: 2
 */
static void arch_platform_set_output(void *user, asdf_physical_dev_t output, uint8_t value)
{
  (void) user;
  if (output < ASDF_PHYSICAL_NUM_RESOURCES) {
    record_output(output, value);
  }
}

/**
 * Platform operation: emulates setting the strobe polarity.
 *
 * Records the strobe polarity.
 *
 * @param user      Platform context; unused.
 * @param positive  True for positive polarity; zero for negative.
 */
static void arch_platform_set_strobe_polarity(void *user, bool positive)
{
  (void) user;
  strobe_is_positive = positive;
}

/**
 * Platform operation: emulates the short pulse delay.
 *
 * Advances every output's pulse detector with a delay event.
 *
 * @param user  Platform context; unused.
 */
static void arch_platform_pulse_delay_short(void *user)
{
  (void) user;
  asdf_arch_pulse_delay();
}

/**
 * Platform operation: resets the emulated hardware.
 *
 * Does the same as asdf_arch_test_reset().
 *
 * @param user  Platform context; unused.
 */
static void arch_platform_reset(void *user)
{
  (void) user;
  asdf_arch_test_reset();
}

const asdf_platform_t asdf_arch_platform = {
  .user = NULL,
  .read_row = &arch_platform_read_row,
  .send_code = &arch_platform_send_code,
  .set_output = &arch_platform_set_output,
  .set_strobe_polarity = &arch_platform_set_strobe_polarity,
  .pulse_delay_short = &arch_platform_pulse_delay_short,
  .reset = &arch_platform_reset,
};

/**
 * Resets the emulated hardware and sets up arch.
 *
 * Resets the shared emulation, copies asdf_arch_platform into arch, and clears
 * the tick count.
 *
 * @param arch  Hardware state to initialize.
 *
 * There is no timer: tests count ticks by calling the tick interrupt handler.
 */
void asdf_arch_init(asdf_arch_t *arch)
{
  asdf_arch_test_reset();
  arch->platform = asdf_arch_platform;
  arch->ticks = 0;
}

/**
 * Collects the ticks counted since the last call.
 *
 * Clears the tick count in arch.
 *
 * @param arch  Hardware state of the keyboard.
 * @return The number of ticks counted since the last call, saturating at 255.
 *
 * The host tests are single-threaded, so the read and clear need no masking.
 */
uint8_t asdf_arch_tick(asdf_arch_t *arch)
{
  uint8_t ticks = arch->ticks;
  arch->ticks = 0;
  return ticks;
}

//-------|---------|---------+---------+---------+---------+---------+---------+
// Above line is 80 columns, and should display completely in the editor.
//
