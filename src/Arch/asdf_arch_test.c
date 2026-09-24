// -*- mode: C; tab-width: 2 ; indent-tabs-mode: nil -*-
//
// Unfified Keyboard Project
// ASDF keyboard firmware
//
// asdf_arch.c
//
// This file contains all the architecture dependent code, including register
// setup, I/O, timers, etc.
//
// Copyright 2019 David Fenyes
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


// Wiring Information:
// Chip: {Microcontroller type and version}
//
// Example:
// PIN          NAME      FUNCTION
// 14-19,9,10   PORTB     COLUMN inputs (1 bit per column)
// 23-25        PORTC0-2  ROW outputs (row number)
// 27           PORTC4

#include <stdio.h>
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

static pulse_state_t pulse_transition_table[PD_ST_NUM_VALID_PULSE_STATES][NUM_PULSE_EVENTS] =
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

static uint8_t outputs[ASDF_PHYSICAL_NUM_RESOURCES];
static pulse_state_t pulses[ASDF_PHYSICAL_NUM_RESOURCES];

static uint8_t strobe_is_positive;

static asdf_keycode_t code_register;
static uint8_t code_sent;

// PROCEDURE: asdf_arch_send_code
// INPUTS: asdf_keycode_t code
// OUTPUTS: none
// DESCRIPTION: emulates sending a code, by copying code to a register that can
// be tested.
//
// SIDE_EFFECTS: sets code_sent variable and alters code_register
//
// NOTES:
//
// SCOPE: public
//
// COMPLEXITY: 1
//
void asdf_arch_send_code(asdf_keycode_t code)
{
  code_register = code;
  code_sent = 1;
}

// PROCEDURE: asdf_arch_get_sent_code
// INPUTS: none
// OUTPUTS: returns type (asdf_keycode_t) in register and zeros the register.
// DESCRIPTION: faciliates test of sending a code, by reporting contents of the
// "sent register".  Resets code_sent variable.
// 
// SIDE_EFFECTS: alters code_sent variable
//
// NOTES:
//
// SCOPE: public
//
// COMPLEXITY: 1
//
asdf_keycode_t asdf_arch_get_sent_code(void)
{
  code_sent = 0;
  return code_register;
}

// PROCEDURE: asdf_arch_was_code_sent
// INPUTS: none
// OUTPUTS: returns TRUE if a code was sent, FALSE otherwise.
// DESCRIPTION: returns status of code_sent variable.
//
// SIDE_EFFECTS: sets code_sent variable and alters code_register
//
// NOTES:
//
// SCOPE: public
//
// COMPLEXITY: 1
//
uint8_t asdf_arch_was_code_sent(void){
  return code_sent;
}


// PROCEDURE: pulse_detect
// INPUTS: (pulse_state_t) current_state
//         (pulse_event_t) event - current input to the state machine
// OUTPUTS: returns new pulse detector state for the output.
//
// DESCRIPTION: Given an output and a new value for the output, calculate the
// next state of the pulse detector for the output.
//
// SIDE EFFECTS: none.
//
// NOTES:
//
// SCOPE: private
//
// COMPLEXITY: 2
//
static pulse_state_t pulse_detect(pulse_state_t current_state, pulse_event_t event)
{
  pulse_state_t next_state = current_state;

  // advance state if current state is valid (not an error state)
  if (current_state < PD_ST_NUM_VALID_PULSE_STATES) {
    next_state = pulse_transition_table[current_state][event];
  }
  return next_state;
}


// PROCEDURE: set_output
// INPUTS: (asdf_physical_dev_t) output_dev - the output to set
//         (uint8_t) value - value to assert on the output
// OUTPUTS: none
//
// DESCRIPTION: Given an output and a new value for the output, set the output
// and advance the pulse detector state for the output.
//
// SIDE EFFECTS: see above.
//
// NOTES:
//
// SCOPE: private
//
// COMPLEXITY: 1
//
static void set_output(asdf_physical_dev_t output_dev, uint8_t value)
{
  pulse_event_t pulse_event = value ? PULSE_EVENT_SET_HIGH : PULSE_EVENT_SET_LOW;

  outputs[output_dev] = value;
  pulses[output_dev] = pulse_detect(pulses[output_dev], pulse_event);
}

// PROCEDURE: asdf_arch_null_output
// INPUTS: (uint8_t) value - ignored
// OUTPUTS: none
//
// DESCRIPTION: Does nothing.
//
// SIDE EFFECTS: See above.
//
// NOTES:
//
// SCOPE: public
//
// COMPLEXITY: 2
//
void asdf_arch_null_output(uint8_t value)
{
  set_output(PHYSICAL_NO_OUT, value);
}


// PROCEDURE: asdf_arch_led1_set
// INPUTS: (uint8_t) value
// OUTPUTS: none
//
// DESCRIPTION: If value is true, turn on LED1.  If value is false, turn off LED1
//
// SIDE EFFECTS: See above.
//
// SCOPE: public
//
// COMPLEXITY: 1
//
void asdf_arch_led1_set(uint8_t value)
{
  set_output(PHYSICAL_LED1, value);
}

// PROCEDURE: asdf_arch_led2_set
// INPUTS: (uint8_t) value
// OUTPUTS: none
//
// DESCRIPTION: If value is true, turn on LED2.  If value is false, turn off LED2
//
// SIDE EFFECTS: See above.
//
// SCOPE: public
//
// COMPLEXITY: 1
//
void asdf_arch_led2_set(uint8_t value)
{
  set_output(PHYSICAL_LED2, value);
}

// PROCEDURE: asdf_arch_led3_set
// INPUTS: (uint8_t) value
// OUTPUTS: none
//
// DESCRIPTION: If value is true, turn on LED3.  If value is false, turn off LED3
//
// SIDE EFFECTS: See above.
//
// SCOPE: public
//
// COMPLEXITY: 1
//
void asdf_arch_led3_set(uint8_t value)
{
  set_output(PHYSICAL_LED3, value);
}

// PROCEDURE: asdf_arch_out1_set
// INPUTS: (uint8_t) value
// OUTPUTS: none
//
// DESCRIPTION: Sets the OUT1 bit if value is true, and clear OUT1 if value is false.
//
// SIDE EFFECTS: See above.
//
// NOTES:
//
// SCOPE: public
//
// COMPLEXITY: 1
//
void asdf_arch_out1_set(uint8_t value)
{
  set_output(PHYSICAL_OUT1, value);
}

// PROCEDURE: asdf_arch_out1_open_hi_set
// INPUTS: (uint8_t) value
// OUTPUTS: none
//
// DESCRIPTION: Emulates setting the OUT1 bit to hi-z if value is true, and low
// if value is false. For testing, set PHYSICAL_OUT1_OPEN_HI to the value.
//
// SIDE EFFECTS: See above.
//
// NOTES:
//
// SCOPE: public
//
// COMPLEXITY: 1
//
void asdf_arch_out1_open_hi_set(uint8_t value)
{set_output(PHYSICAL_OUT1_OPEN_HI, value);
}

// PROCEDURE: asdf_arch_out1_open_lo_set
// INPUTS: (uint8_t) value
// OUTPUTS: none
//
// DESCRIPTION: Emulates setting the OUT1 bit to high if value is true, and hi-z
// if value is false. For testing, set PHYSICAL_OUT1_OPEN_LO to the value.
//
// SIDE EFFECTS: See above.
//
// NOTES:
//
// SCOPE: public
//
// COMPLEXITY: 1
//
void asdf_arch_out1_open_lo_set(uint8_t value)
{set_output(PHYSICAL_OUT1_OPEN_HI, value);
}

// PROCEDURE: asdf_arch_out2_set
// INPUTS: (uint8_t) value
// OUTPUTS: none
//
// DESCRIPTION: Sets the OUT2 bit if value is true, and clear OUT2 if value is false.
//
// SIDE EFFECTS: See above.
//
// SCOPE: public
//
// COMPLEXITY: 2
//
void asdf_arch_out2_set(uint8_t value)
{
  set_output(PHYSICAL_OUT2, value);
}


// PROCEDURE: asdf_arch_out2_open_hi_set
// INPUTS: (uint8_t) value
// OUTPUTS: none
//
// DESCRIPTION: Emulates setting the OUT2 bit to hi-z if value is true, and low
// if value is false. For testing, set PHYSICAL_OUT2_OPEN_HI to the value.
//
// SIDE EFFECTS: See above.
//
// NOTES:
//
// SCOPE: public
//
// COMPLEXITY: 1
//
void asdf_arch_out2_open_hi_set(uint8_t value)
{set_output(PHYSICAL_OUT1_OPEN_HI, value);
}

// PROCEDURE: asdf_arch_out2_open_lo_set
// INPUTS: (uint8_t) value
// OUTPUTS: none
//
// DESCRIPTION: Emulates setting the OUT2 bit to high if value is true, and hi-z
// if value is false. For testing, set PHYSICAL_OUT2_OPEN_LO to the value.
//
// SIDE EFFECTS: See above.
//
// NOTES:
//
// SCOPE: public
//
// COMPLEXITY: 1
//
void asdf_arch_out2_open_lo_set(uint8_t value)
{set_output(PHYSICAL_OUT1_OPEN_HI, value);
}

// PROCEDURE: asdf_arch_out3_set
// INPUTS: (uint8_t) value
// OUTPUTS: none
//
// DESCRIPTION: Sets the OUT3 bit if value is true, and clear OUT3 if value is false.
//
// SIDE EFFECTS: See above.
//
// NOTES:
//
// SCOPE: public
//
// COMPLEXITY: 1
//
void asdf_arch_out3_set(uint8_t value)
{
  set_output(PHYSICAL_OUT3, value);
}

// PROCEDURE: asdf_arch_out3_open_hi_set
// INPUTS: (uint8_t) value
// OUTPUTS: none
//
// DESCRIPTION: Emulates setting the OUT3 bit to hi-z if value is true, and low
// if value is false. For testing, set PHYSICAL_OUT3_OPEN_HI to the value.
//
// SIDE EFFECTS: See above.
//
// NOTES:
//
// SCOPE: public
//
// COMPLEXITY: 1
//
void asdf_arch_out3_open_hi_set(uint8_t value)
{set_output(PHYSICAL_OUT1_OPEN_HI, value);
}

// PROCEDURE: asdf_arch_out3_open_lo_set
// INPUTS: (uint8_t) value
// OUTPUTS: none
//
// DESCRIPTION: Emulates setting the OUT3 bit to high if value is true, and hi-z
// if value is false. For testing, set PHYSICAL_OUT3_OPEN_LO to the value.
//
// SIDE EFFECTS: See above.
//
// NOTES:
//
// SCOPE: public
//
// COMPLEXITY: 1
//
void asdf_arch_out3_open_lo_set(uint8_t value)
{set_output(PHYSICAL_OUT1_OPEN_HI, value);
}

// PROCEDURE: asdf_arch_check_output
// INPUTS:(asdf_physical_dev_t) device - which device to check
// OUTPUTS: the value of the device setting.
//
// DESCRIPTION: For a given physical device, return the current setting (true or false)
//
// SIDE EFFECTS: none
//
// NOTES:
//
// SCOPE: public
//
// COMPLEXITY: 1
//
uint8_t asdf_arch_check_output(asdf_physical_dev_t device)
{
  return outputs[device];
}

// PROCEDURE: asdf_arch_check_pulse
// INPUTS:(asdf_physical_dev_t) device - which device to check
// OUTPUTS: the value of the device pulse detector
//
// DESCRIPTION: For a given physical device, return the state of the pulse detector
//
// SIDE EFFECTS: none
//
// NOTES:
//
// SCOPE: public
//
// COMPLEXITY: 1
//
uint8_t asdf_arch_check_pulse(asdf_physical_dev_t device)
{
  return pulses[device];
}

// PROCEDURE: asdf_arch_pulse_delay
// INPUTS: none
// OUTPUTS: none
//
// DESCRIPTION: Emulates a delay by advancing the pulse detector state machine
// for each output.
//
// SIDE EFFECTS: see above.
//
// NOTES: Set ASDF_PULSE_DELAY_US in asdf_config.h
//
// SCOPE: private
//
// COMPLEXITY: 2
//
void asdf_arch_pulse_delay(void)
{
  for (uint8_t i = 0; i < ASDF_PHYSICAL_NUM_RESOURCES; i++) {
    pulses[i] = pulse_detect(pulses[i], PULSE_EVENT_DELAY);
  }
}

// PROCEDURE: asdf_arch_pulse_delay_short
// INPUTS: none
// OUTPUTS: none
//
// DESCRIPTION: Emulates a short delay by advancing the pulse detector state machine
// for each output.
//
// SIDE EFFECTS: see above.
//
// NOTES: Set ASDF_PULSE_DELAY_US in asdf_config.h
//
// SCOPE: public
//
// COMPLEXITY: 1
//
void asdf_arch_pulse_delay_short(void)
{
  asdf_arch_pulse_delay();
}

// PROCEDURE: asdf_arch_pos_strobe
// INPUTS: none
// OUTPUTS: none
// DESCRIPTION: Initialize strobe output to positive polarity. Initial state is
// LOW
void asdf_arch_set_pos_strobe(void)
{
  strobe_is_positive = 1;
}

// PROCEDURE: asdf_arch_neg_strobe
// INPUTS: none
// OUTPUTS: none
// DESCRIPTION: Initialize strobe output
void asdf_arch_set_neg_strobe(void)
{
  strobe_is_positive = 0;
}

uint8_t asdf_arch_is_strobe_positive(void)
{
  return strobe_is_positive;
}


// PROCEDURE: asdf_arch_test_reset
// INPUTS: none
// OUTPUTS: none
//
// DESCRIPTION: sets up all the hardware for the keyboard
//
// SIDE EFFECTS: see DESCRIPTION
//
// SCOPE: public
//
// COMPLEXITY: 1
//
void asdf_arch_test_reset(void)
{
  for (uint8_t i = 0; i < ASDF_PHYSICAL_NUM_RESOURCES; i++) {
    outputs[i] = 0;
    pulses[i] = PD_ST_INITIAL_STATE;
  }

  // initially, no keycodes have been sent via asdf_arch_send_code:
  code_sent = 0;
  strobe_is_positive = 0;
}


// PROCEDURE: asdf_arch_read_row
// INPUTS: (uint8_t) row - row to read
// OUTPUTS: returns 0 (no keys pressed)
//
// DESCRIPTION: Default fake key matrix with no keys pressed. Tests that press
// keys define their own asdf_arch_read_row(), which replaces this weak
// definition.
//
// SCOPE: public
//
// COMPLEXITY: 1
//
__attribute__((weak)) asdf_cols_t asdf_arch_read_row(uint8_t row)
{
  (void) row;
  return 0;
}

// PROCEDURE: arch_platform_*
// DESCRIPTION: adapt the emulated hardware to the typed platform interface.
// This test architecture has one set of hardware, so the platform context
// pointer is unused.
static asdf_cols_t arch_platform_read_row(void *user, uint8_t row)
{
  (void) user;
  return asdf_arch_read_row(row);
}

static void arch_platform_send_code(void *user, asdf_keycode_t code)
{
  (void) user;
  asdf_arch_send_code(code);
}

static void arch_platform_set_output(void *user, asdf_physical_dev_t output, uint8_t value)
{
  (void) user;
  if (output < ASDF_PHYSICAL_NUM_RESOURCES) {
    set_output(output, value);
  }
}

static void arch_platform_set_strobe_polarity(void *user, uint8_t positive)
{
  (void) user;
  strobe_is_positive = positive ? 1 : 0;
}

static void arch_platform_pulse_delay_short(void *user)
{
  (void) user;
  asdf_arch_pulse_delay_short();
}

static void arch_platform_reset(void *user)
{
  (void) user;
  asdf_arch_test_reset();
}

const asdf_platform_t asdf_arch_platform = {
  .user = NULL,
  .read_row = arch_platform_read_row,
  .send_code = arch_platform_send_code,
  .set_output = arch_platform_set_output,
  .set_strobe_polarity = arch_platform_set_strobe_polarity,
  .pulse_delay_short = arch_platform_pulse_delay_short,
  .reset = arch_platform_reset,
};

// PROCEDURE: asdf_arch_init, asdf_arch_tick
// DESCRIPTION: the firmware adapters' instance API over the emulated hardware
// (see asdf_arch_test.h). There is no timer: tests count ticks by calling the
// tick interrupt handler.
void asdf_arch_init(asdf_arch_t *arch)
{
  asdf_arch_test_reset();
  arch->platform = asdf_arch_platform;
  arch->ticks = 0;
}

uint8_t asdf_arch_tick(asdf_arch_t *arch)
{
  uint8_t ticks = arch->ticks;
  arch->ticks = 0;
  return ticks;
}

//-------|---------|---------+---------+---------+---------+---------+---------+
// Above line is 80 columns, and should display completely in the editor.
//
