// -*- mode: C; tab-width: 4 ; indent-tabs-mode: nil -*-
/**
 * @file asdf_physical.h
 *
 * Definitions and prototypes for physical LED and virtual output resources.
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

#if !defined(ASDF_PHYSICAL_H)
#define ASDF_PHYSICAL_H

#include <stdint.h>

/**
 * The physical outputs the hardware can provide, which keymaps assign to
 * virtual outputs with asdf_virtual_assign().
 *
 * PHYSICAL_NO_OUT is not an output: it ends each list of physical outputs and
 * is never assigned. Each architecture adapter drives the outputs it has and
 * ignores the rest.
 */
typedef enum {
  PHYSICAL_NO_OUT = 0,
  PHYSICAL_OUT1,
  PHYSICAL_OUT1_OPEN_HI,
  PHYSICAL_OUT1_OPEN_LO,
  PHYSICAL_OUT2,
  PHYSICAL_OUT2_OPEN_HI,
  PHYSICAL_OUT2_OPEN_LO,
  PHYSICAL_OUT3,
  PHYSICAL_OUT3_OPEN_HI,
  PHYSICAL_OUT3_OPEN_LO,
  PHYSICAL_LED1,
  PHYSICAL_LED2,
  PHYSICAL_LED3,
  ASDF_PHYSICAL_NUM_RESOURCES
} asdf_physical_dev_t;

struct asdf_platform; // asdf_platform.h

/**
 * Changeable state of the physical outputs of one keyboard.
 *
 * shadow[] holds the value last written to each output. next[] links the
 * outputs into singly linked lists: the available list, headed by
 * next[PHYSICAL_NO_OUT], and one list per virtual output, whose head the
 * virtual state holds. Each list ends at PHYSICAL_NO_OUT. The platform drives
 * the outputs; with no platform, only the shadow values change.
 *
 * Each function operates only on the state passed to it.
 *
 * Invariants, after asdf_physical_init() and any sequence of operations:
 * - every next[] value is < ASDF_PHYSICAL_NUM_RESOURCES
 * - PHYSICAL_NO_OUT is never on the available list and is never allocated
 * - an output leaves the available list only through
 *   asdf_physical_allocate(), and returns to it only through
 *   asdf_physical_init()
 */
typedef struct {
  uint8_t shadow[ASDF_PHYSICAL_NUM_RESOURCES];
  asdf_physical_dev_t next[ASDF_PHYSICAL_NUM_RESOURCES];
  const struct asdf_platform *platform; ///< set by the owning keyboard
} asdf_physical_state_t;

/**
 * Initialize the physical outputs: all available, none driven.
 *
 * Sets every shadow value to ASDF_VIRTUAL_OUT_DEFAULT_VALUE and places every
 * output except PHYSICAL_NO_OUT on the available list, returning any
 * allocated outputs to it. Writes only @p phys; the outputs themselves are not
 * driven.
 *
 * @param phys      State to initialize.
 * @param platform  Platform that drives the outputs; NULL to track shadow
 *                  values only.
 */
void asdf_physical_init(asdf_physical_state_t *phys, const struct asdf_platform *platform);

/**
 * Drive an output to a value and record it as the output's shadow value.
 *
 * Drives the output through the platform, if any, and updates the shadow
 * value.
 *
 * @param phys          Physical output state.
 * @param physical_out  Output to set; values >= ASDF_PHYSICAL_NUM_RESOURCES
 *                      are ignored.
 * @param value         Value to drive and record, stored as given.
 */
void asdf_physical_set(asdf_physical_state_t *phys, asdf_physical_dev_t physical_out,
                         uint8_t value);

/**
 * Set an output high (1), as asdf_physical_set().
 *
 * Drives the output through the platform, if any, and updates the shadow
 * value. Out-of-range outputs are ignored.
 *
 * @param phys          Physical output state.
 * @param physical_out  Output to set.
 */
void asdf_physical_on(asdf_physical_state_t *phys, asdf_physical_dev_t physical_out);

/**
 * Set an output low (0), as asdf_physical_set().
 *
 * Drives the output through the platform, if any, and updates the shadow
 * value. Out-of-range outputs are ignored.
 *
 * @param phys          Physical output state.
 * @param physical_out  Output to clear.
 */
void asdf_physical_off(asdf_physical_state_t *phys, asdf_physical_dev_t physical_out);

/**
 * Drive an output to its shadow value.
 *
 * Used to apply values recorded while the output was not being driven, such
 * as initial values set by asdf_physical_allocate(). Drives the output
 * through the platform, if any; the shadow value is unchanged.
 *
 * @param phys          Physical output state.
 * @param physical_out  Output to drive; values >= ASDF_PHYSICAL_NUM_RESOURCES
 *                      are ignored.
 */
void asdf_physical_assert(asdf_physical_state_t *phys, asdf_physical_dev_t physical_out);

/**
 * Invert an output: drive it to the logical NOT of its shadow value.
 *
 * The toggle is computed from the shadow value, so the platform needs only a
 * set operation. Drives the output through the platform, if any, and updates
 * the shadow value.
 *
 * @param phys          Physical output state.
 * @param physical_out  Output to toggle; values >= ASDF_PHYSICAL_NUM_RESOURCES
 *                      are ignored.
 */
void asdf_physical_toggle(asdf_physical_state_t *phys, asdf_physical_dev_t physical_out);

/**
 * Follow one link of the list that contains an output.
 *
 * No side effects.
 *
 * @param phys    Physical output state.
 * @param device  Current output in a virtual output's list.
 * @return The output after @p device in its list; PHYSICAL_NO_OUT at the end
 *         of the list or if @p device is >= ASDF_PHYSICAL_NUM_RESOURCES. For
 *         PHYSICAL_NO_OUT, returns the head of the available list.
 */
asdf_physical_dev_t asdf_physical_next_device(const asdf_physical_state_t *phys,
                                                asdf_physical_dev_t device);

/**
 * Take an output off the available list and prepend it to a list.
 *
 * On success, the output's next link is set to @p tail and its shadow value
 * to @p initial_value. The output is not driven; asdf_physical_assert()
 * applies the value once all assignments are made.
 *
 * @p tail is not checked beyond its range: the caller passes PHYSICAL_NO_OUT
 * or the head of a list of allocated outputs.
 *
 * On success, updates the links and the shadow value in @p phys. No output is
 * driven.
 *
 * @param phys           Physical output state.
 * @param physical_out   Output to allocate.
 * @param tail           List to link after @p physical_out.
 * @param initial_value  Shadow value to record for @p physical_out.
 * @return 1 if allocated; 0 if @p physical_out is PHYSICAL_NO_OUT, out of
 *         range, or already allocated, or if @p tail is out of range. On
 *         failure the state is unchanged.
 */
uint8_t asdf_physical_allocate(asdf_physical_state_t *phys, asdf_physical_dev_t physical_out,
                                 asdf_physical_dev_t tail, uint8_t initial_value);

/**
 * Wait for the width of a short output pulse, through the platform.
 *
 * Returns at once if the state has no platform. Blocks the caller for the
 * pulse width; no other side effects.
 *
 * @param phys  Physical output state.
 */
void asdf_physical_pulse_delay_short(const asdf_physical_state_t *phys);

#endif /* !defined (ASDF_PHYSICAL_H) */

//-------|---------|---------+---------+---------+---------+---------+---------+
// Above line is 80 columns, and should display completely in the editor.
