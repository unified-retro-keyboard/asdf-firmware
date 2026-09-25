// -*- mode: C; tab-width: 4 ; indent-tabs-mode: nil -*-
/**
 * @file asdf_virtual.h
 *
 * Definitions and prototypes for virtual LED and virtual output management.
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
//

#if !defined(ASDF_VIRTUAL_H)
#define ASDF_VIRTUAL_H

#include <stdbool.h>
#include "asdf_physical.h"

/**
 * Virtual outputs: the LEDs and output lines the keyboard code refers to.
 *
 * Keymap initializers map each virtual output to physical outputs, so the
 * code does not depend on the hardware. V_NULL is never assigned.
 */
typedef enum {
  V_NULL,
  VOUT1,
  VOUT2,
  VOUT3,
  VOUT4,
  VOUT5,
  VOUT6,
  VLED1,
  VLED2,
  VLED3,
  VCAPS_LED,
  VSHIFT_LED,
  ASDF_VIRTUAL_NUM_RESOURCES
} asdf_virtual_dev_t;


/**
 * Functions applied to the physical outputs of a virtual output.
 *
 * V_SET_HI and V_SET_LO drive the outputs high or low. V_TOGGLE inverts them.
 * V_PULSE_SHORT inverts them for ASDF_PULSE_DELAY_SHORT_US, blocking the
 * caller. V_PULSE_LONG inverts them for ASDF_PULSE_DELAY_LONG_MS ticks, ended
 * by asdf_virtual_tick(). V_NOFUNC does nothing.
 */
typedef enum {
  V_NOFUNC,
  V_SET_HI,
  V_SET_LO,
  V_PULSE_SHORT,
  V_PULSE_LONG,
  V_TOGGLE,
  ASDF_VIRTUAL_NUM_FUNCTIONS
} asdf_virtual_function_t;

/**
 * One keymap assignment of a physical output to a virtual output.
 *
 * Each keymap specifies an array of these, applied with
 * asdf_virtual_assign().
 */
/**
 * The idle value of an output whose active value is @p active_value (0u or
 * 1u): the initial value of an output that pulses to its active value.
 */
#define ASDF_IDLE_VALUE(active_value) (((active_value) == 0u) ? 1u : 0u)

typedef struct {
  asdf_virtual_dev_t virtual_device;
  asdf_physical_dev_t physical_device;
  asdf_virtual_function_t function;
  uint8_t initial_value;
} asdf_virtual_initializer_t;

/**
 * Changeable state of the virtual outputs of one keyboard, including the
 * physical outputs they drive.
 *
 * Each virtual output heads a list of physical outputs (see
 * asdf_physical_state_t), and an operation on the virtual output applies to
 * every physical output in its list. Each function operates only on the state
 * passed to it. Virtual outputs >= ASDF_VIRTUAL_NUM_RESOURCES are ignored.
 *
 * Invariants, after asdf_virtual_init() and any sequence of asdf_virtual_*
 * operations:
 * - every physical output other than PHYSICAL_NO_OUT is on the available list
 *   or on exactly one virtual output's list
 * - physical_device[V_NULL] is PHYSICAL_NO_OUT
 * - function[v] is V_NOFUNC whenever physical_device[v] is PHYSICAL_NO_OUT
 * - pulse_ticks[v] <= ASDF_PULSE_DELAY_LONG_MS
 *
 * @code
 * #include "asdf_platform.h"
 * #include "asdf_virtual.h"
 *
 * const asdf_platform_t *platform = ...; // from the application
 * asdf_virtual_state_t outputs;
 *
 * asdf_virtual_init(&outputs, platform);
 * asdf_virtual_assign(&outputs, VLED1, PHYSICAL_LED1, V_TOGGLE, 0);
 * asdf_virtual_sync(&outputs);            // LED1 is driven off (0)
 * asdf_virtual_activate(&outputs, VLED1); // LED1 toggles on (1)
 * @endcode
 */
typedef struct {
  asdf_physical_dev_t physical_device[ASDF_VIRTUAL_NUM_RESOURCES]; ///< head of each output's list
  asdf_virtual_function_t function[ASDF_VIRTUAL_NUM_RESOURCES];    ///< applied on activation
  uint8_t pulse_ticks[ASDF_VIRTUAL_NUM_RESOURCES]; ///< ticks left in a long pulse (0 = none)
  asdf_physical_state_t physical;
} asdf_virtual_state_t;

/**
 * Initialize the virtual outputs, with no physical outputs assigned.
 *
 * Every virtual output gets no physical outputs, function V_NOFUNC, and no
 * pulse in progress, and the embedded physical state is initialized (see
 * asdf_physical_init()). Writes only @p virt; no output is driven.
 *
 * @param virt      State to initialize.
 * @param platform  Platform that drives the physical outputs; NULL to track
 *                  shadow values only.
 */
void asdf_virtual_init(asdf_virtual_state_t *virt, const struct asdf_platform *platform);

/**
 * Apply a function to every physical output assigned to a virtual output.
 *
 * V_PULSE_SHORT waits for the short pulse width through the platform before
 * returning. V_PULSE_LONG starts a pulse that asdf_virtual_tick() ends; if
 * a long pulse is already in progress on @p virtual_out, it is not restarted.
 * A virtual output with no physical outputs is left unchanged.
 *
 * Drives the physical outputs through the platform and updates their shadow
 * values. V_PULSE_LONG also records the pulse in @p virt.
 *
 * @param virt         Virtual output state.
 * @param virtual_out  Virtual output to act on; out-of-range values are
 *                     ignored.
 * @param function     Function to apply.
 */
void asdf_virtual_action(asdf_virtual_state_t *virt, asdf_virtual_dev_t virtual_out,
                           asdf_virtual_function_t function);

/**
 * Apply a virtual output's assigned function to its physical outputs.
 *
 * Equivalent to asdf_virtual_action() with the function given at
 * assignment, with the same side effects. A virtual output never assigned has
 * function V_NOFUNC and does nothing.
 *
 * @param virt         Virtual output state.
 * @param virtual_out  Virtual output to activate; out-of-range values are
 *                     ignored.
 */
void asdf_virtual_activate(asdf_virtual_state_t *virt, asdf_virtual_dev_t virtual_out);

/**
 * Assign a physical output to a virtual output.
 *
 * The physical output is added to the front of the virtual output's list, so
 * a virtual output can drive several physical outputs. @p function replaces
 * the virtual output's function for its whole list. @p initial_value is
 * recorded as the physical output's shadow value but not driven;
 * asdf_virtual_sync() drives it once all assignments are made.
 *
 * On success, takes the physical output off the available list and updates
 * the virtual output's list and function. No output is driven.
 *
 * @param virt           Virtual output state.
 * @param virtual_out    Virtual output to assign to.
 * @param physical_out   Physical output to assign.
 * @param function       Function applied when the virtual output is
 *                       activated.
 * @param initial_value  Initial value of the physical output.
 * @return true if assigned; false if @p virtual_out is V_NULL or out of range,
 *         or if @p physical_out is PHYSICAL_NO_OUT, out of range, or already
 *         assigned. On failure the state is unchanged.
 */
bool asdf_virtual_assign(asdf_virtual_state_t *virt, asdf_virtual_dev_t virtual_out,
                         asdf_physical_dev_t physical_out, asdf_virtual_function_t function,
                         uint8_t initial_value);

/**
 * Drive every physical output, assigned or not, to its shadow value.
 *
 * Call once after the keymap's assignments to apply their initial values.
 * Drives the outputs through the platform; @p virt is unchanged.
 *
 * @param virt  Virtual output state.
 */
void asdf_virtual_sync(asdf_virtual_state_t *virt);

/**
 * Advance the long pulses in progress, ending those whose time has expired.
 *
 * A pulse ends when the ticks passed since it started reach
 * ASDF_PULSE_DELAY_LONG_MS; its physical outputs are then inverted back.
 *
 * Updates the pulse counts in @p virt and drives the outputs of ended pulses
 * through the platform.
 *
 * @param virt     Virtual output state.
 * @param elapsed  Ticks (ms) elapsed since the last call.
 */
void asdf_virtual_tick(asdf_virtual_state_t *virt, uint8_t elapsed);

#endif /* !defined (ASDF_VIRTUAL_H) */

//-------|---------|---------+---------+---------+---------+---------+---------+
// Above line is 80 columns, and should display completely in the editor.
