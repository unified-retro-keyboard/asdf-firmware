// -*- mode: C; tab-width: 4 ; indent-tabs-mode: nil -*- 
//
//  Unfified Keyboard Project
//  ASDF keyboard firmware
//
//  asdf_arch_test.h
//
// Emulated hardware for the host unit tests.
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


#if !defined (ASDF_ARCH_H)
#define ASDF_ARCH_H

#include <string.h>

#include <stdint.h>
#include "asdf.h"
#include "asdf_config.h"
#include "asdf_physical.h"
#include "asdf_platform.h"
#include "asdf_virtual.h"



// Pulse detector states, reported by asdf_arch_check_pulse(). Each emulated
// output has a detector that recognizes a pulse as a transition, a pulse delay,
// then a transition back: low, high, delay, low gives PD_ST_PULSE_HIGH_DETECTED.
// A sequence that is not a valid pulse leaves the detector in an error state,
// where it stays until reset.
typedef enum {
              PD_ST_INITIAL_STATE = 0,
              PD_ST_STABLE_LOW = 1,
              PD_ST_STABLE_HIGH = 2,
              PD_ST_TRANSITION_LOW = 3,
              PD_ST_TRANSITION_HIGH = 4,
              PD_ST_PULSE_DELAY_LOW = 5,
              PD_ST_PULSE_DELAY_HIGH = 6,
              PD_ST_PULSE_HIGH_DETECTED = 7,
              PD_ST_PULSE_LOW_DETECTED = 8,
              PD_ST_NUM_VALID_PULSE_STATES = 9, // error states follow
              PD_ST_ERROR_DOUBLE_DELAY = 10,
              PD_ST_ERROR_DOUBLE_SET = 11,
              PD_ST_ERROR_NO_TRANSITION_BEFORE_DELAY = 12, 
              PD_ST_ERROR_NO_TRANSITION_AFTER_DELAY = 13,
              PD_ST_ERROR_DOUBLE_TRANSITION = 14, // fast pulse without delay
              PD_ST_ERROR_PULSE_FROM_INITIAL_STATE = 15,
} pulse_state_t;

#define FLASH
#define FLASH_READ(a) (*(a))
#define FLASH_READ_PTR(a) (*(a))
#define FLASH_MEMCPY(dst, src, n) memcpy((dst), (src), (n))
#define FLASH_READ_MATRIX_ELEMENT(mat,row,col) (mat)[(row)][(col)]
#define FLASH_STRING(s) (s)

// Strobe polarity setters. The emulation records the polarity only; there is no
// strobe pin.

/**
 * Emulates setting the strobe to positive polarity.
 *
 * Records the strobe polarity as positive.
 */
void asdf_arch_set_pos_strobe(void);

/**
 * Emulates setting the strobe to negative polarity.
 *
 * Records the strobe polarity as negative.
 */
void asdf_arch_set_neg_strobe(void);

/**
 * Reports the recorded strobe polarity.
 *
 * No side effects.
 *
 * @return Nonzero if the strobe polarity is positive; 0 if it is negative.
 */
uint8_t asdf_arch_is_strobe_positive(void);

// Output setters. Each records its value in the emulated outputs and advances
// that output's pulse detector.

/**
 * Emulates writing the nonexistent output (PHYSICAL_NO_OUT).
 *
 * Records the value as the output's current value and advances its pulse
 * detector with a set-high (nonzero) or set-low (zero) event.
 *
 * @param value  Value written to the output.
 */
void asdf_arch_null_output(uint8_t value);

/**
 * Emulates setting LED1.
 *
 * Records the value as the output's current value and advances its pulse
 * detector with a set-high (nonzero) or set-low (zero) event.
 *
 * @param value  Value written to the output.
 */
void asdf_arch_led1_set(uint8_t value);

/**
 * Emulates setting LED2.
 *
 * Records the value as the output's current value and advances its pulse
 * detector with a set-high (nonzero) or set-low (zero) event.
 *
 * @param value  Value written to the output.
 */
void asdf_arch_led2_set(uint8_t value);

/**
 * Emulates setting LED3.
 *
 * Records the value as the output's current value and advances its pulse
 * detector with a set-high (nonzero) or set-low (zero) event.
 *
 * @param value  Value written to the output.
 */
void asdf_arch_led3_set(uint8_t value);

/**
 * Emulates setting OUT1 as a push-pull output.
 *
 * Records the value as the output's current value and advances its pulse
 * detector with a set-high (nonzero) or set-low (zero) event.
 *
 * @param value  Value written to the output.
 */
void asdf_arch_out1_set(uint8_t value);

/**
 * Emulates setting OUT1 as an open-collector output.
 *
 * Records the value as the output's current value and advances its pulse
 * detector with a set-high (nonzero) or set-low (zero) event.
 *
 * @param value  Value written to the output.
 */
void asdf_arch_out1_open_hi_set(uint8_t value);

/**
 * Emulates setting OUT1 as an open-emitter output.
 *
 * Records the value as the output's current value and advances its pulse
 * detector with a set-high (nonzero) or set-low (zero) event.
 *
 * @param value  Value written to the output.
 */
void asdf_arch_out1_open_lo_set(uint8_t value);

/**
 * Emulates setting OUT2 as a push-pull output.
 *
 * Records the value as the output's current value and advances its pulse
 * detector with a set-high (nonzero) or set-low (zero) event.
 *
 * @param value  Value written to the output.
 */
void asdf_arch_out2_set(uint8_t value);

/**
 * Emulates setting OUT2 as an open-collector output.
 *
 * Records the value as the output's current value and advances its pulse
 * detector with a set-high (nonzero) or set-low (zero) event.
 *
 * @param value  Value written to the output.
 */
void asdf_arch_out2_open_hi_set(uint8_t value);

/**
 * Emulates setting OUT2 as an open-emitter output.
 *
 * Records the value as the output's current value and advances its pulse
 * detector with a set-high (nonzero) or set-low (zero) event.
 *
 * @param value  Value written to the output.
 */
void asdf_arch_out2_open_lo_set(uint8_t value);

/**
 * Emulates setting OUT3 as a push-pull output.
 *
 * Records the value as the output's current value and advances its pulse
 * detector with a set-high (nonzero) or set-low (zero) event.
 *
 * @param value  Value written to the output.
 */
void asdf_arch_out3_set(uint8_t value);

/**
 * Emulates setting OUT3 as an open-collector output.
 *
 * Records the value as the output's current value and advances its pulse
 * detector with a set-high (nonzero) or set-low (zero) event.
 *
 * @param value  Value written to the output.
 */
void asdf_arch_out3_open_hi_set(uint8_t value);

/**
 * Emulates setting OUT3 as an open-emitter output.
 *
 * Records the value as the output's current value and advances its pulse
 * detector with a set-high (nonzero) or set-low (zero) event.
 *
 * @param value  Value written to the output.
 */
void asdf_arch_out3_open_lo_set(uint8_t value);

/**
 * Reports the value of an emulated output.
 *
 * No side effects.
 *
 * @param device  The physical output to check; must be a valid device.
 * @return The last value written to the output, or 0 if none has been
 *         written since the last reset.
 */
uint8_t asdf_arch_check_output(asdf_physical_dev_t device);

/**
 * Reports the pulse detector state of an emulated output.
 *
 * No side effects.
 *
 * @param device  The physical output to check; must be a valid device.
 * @return The detector state, as a pulse_state_t: PD_ST_PULSE_HIGH_DETECTED
 *         or PD_ST_PULSE_LOW_DETECTED after a complete pulse; a PD_ST_ERROR_*
 *         state if the output sequence was not a valid pulse; otherwise the
 *         intermediate state reached so far.
 */
uint8_t asdf_arch_check_pulse(asdf_physical_dev_t device);

/**
 * Emulates the short pulse delay.
 *
 * Advances every output's pulse detector with a delay event.
 */
void asdf_arch_pulse_delay_short(void);

/**
 * Reads one row of the emulated key matrix.
 *
 * The default (weak) definition reports no keys pressed; tests that press keys
 * define their own. The default has no side effects.
 *
 * @param row  Row number to read.
 * @return The row's columns, one bit per column, with 1 = pressed.
 */
asdf_cols_t asdf_arch_read_row(uint8_t row);

/**
 * Emulates sending a code.
 *
 * Latches the code for asdf_arch_get_sent_code() and sets the code-sent flag.
 *
 * @param code  The code sent.
 */
void asdf_arch_send_code(asdf_keycode_t);

/**
 * Collects the last code sent.
 *
 * Clears the code-sent flag.
 *
 * @return The last code sent through asdf_arch_send_code(). If no code has
 *         been sent since the last reset, the value is stale.
 */
asdf_keycode_t asdf_arch_get_sent_code(void);

/**
 * Reports whether a code is waiting to be collected.
 *
 * No side effects.
 *
 * @return TRUE if a code was sent since the last asdf_arch_get_sent_code() or
 *         reset; FALSE otherwise.
 */
uint8_t asdf_arch_was_code_sent(void);

/**
 * Resets the emulated hardware.
 *
 * Sets every output to 0 with its pulse detector in PD_ST_INITIAL_STATE,
 * clears the code-sent flag, and sets negative strobe polarity. The
 * platform's reset operation does the same.
 */
void asdf_arch_test_reset(void);

// The platform of the emulated hardware. Its operations act on the emulation
// above, which is shared by every keyboard that uses this platform; tests that
// need independent hardware for each keyboard use fake_platform_t instead.
extern const asdf_platform_t asdf_arch_platform;

// The DIP switch row of the production keymaps, as on the firmware targets.
#define ASDF_ARCH_DIPSWITCH_ROW 8

// The instance API of the firmware adapters, over the emulated hardware, so
// that code written for them (such as the simple wrapper) runs on the host.
// asdf_arch_test_tick_isr() stands in for the tick interrupt.
typedef struct {
  asdf_platform_t platform; // a copy of asdf_arch_platform
  volatile uint8_t ticks;   // ticks counted by the interrupt, not yet collected
} asdf_arch_t;

// Stands in for the firmware targets' tick interrupt vector: the application
// defines the handler with this macro, and tests call it to count a tick.
#define ASDF_ARCH_TICK_ISR void asdf_arch_test_tick_isr(void)

/**
 * Emulated tick interrupt handler, defined by the application through
 * ASDF_ARCH_TICK_ISR.
 *
 * Tests call it to emulate one 1 ms tick; the handler counts the tick with
 * asdf_arch_count_tick().
 */
void asdf_arch_test_tick_isr(void);

/**
 * Counts one elapsed tick.
 *
 * The count saturates at 255, so a long stall cannot wrap it. Call only from
 * the tick interrupt handler. Increments the tick count in arch.
 *
 * @param arch  Hardware state of the keyboard whose tick is counted.
 */
static inline void asdf_arch_count_tick(asdf_arch_t *arch)
{
  if (arch->ticks < UINT8_MAX) {
    arch->ticks++;
  }
}

/**
 * Resets the emulated hardware and sets up arch.
 *
 * Resets the shared emulation (see asdf_arch_test_reset()), copies
 * asdf_arch_platform into arch, and clears the tick count. There is no timer;
 * ticks are counted only when a test calls the tick interrupt handler.
 *
 * @param arch  Hardware state to initialize.
 */
void asdf_arch_init(asdf_arch_t *arch);

/**
 * Collects the ticks counted since the last call.
 *
 * Clears the tick count in arch.
 *
 * @param arch  Hardware state of the keyboard.
 * @return The number of ticks counted since the last call, saturating at 255.
 */
uint8_t asdf_arch_tick(asdf_arch_t *arch);


#endif // !defined (ASDF_ARCH_H)

//-------|---------|---------+---------+---------+---------+---------+---------+
// Above line is 80 columns, and should display completely in the editor.

