// -*- mode: C; tab-width: 2 ; indent-tabs-mode: nil -*-
/**
 * @file asdf_virtual.c
 *
 * This file contains code that maps "virtual" LEDs and outputs referenced by
 * the code to actual LEDs and outputs in hardware. This keeps keymap-specific
 * details out of the architecture-dependent files, and provides a flexible way
 * for the keymap definitions to specify the LED and output functions for
 * different keymaps or keyboard layouts.
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

#include <stdbool.h>
#include <stdint.h>
#include "asdf_physical.h"
#include "asdf_virtual.h"
#include "asdf_config.h"
#include "asdf_platform.h"

// A virtual output indexes the tables in asdf_virtual_state_t. Each entry holds
// the head of the virtual output's list of physical outputs (if any) and the
// function applied to those outputs when the virtual output is activated by a
// keypress. The lists themselves are linked through the next[] table of the
// physical state embedded in the virtual state. Assigning a physical output
// allocates it from the physical available list and prepends it to the
// virtual output's list, passing the current head as the tail. The public
// contracts are in asdf_virtual.h.

/** Operations applied along a virtual output's list of physical outputs. */
typedef enum { MAP_TOGGLE, MAP_ON, MAP_OFF } virtual_map_op_t;

/**
 * Check that a virtual output indexes the virtual tables.
 *
 * No side effects.
 *
 * @param device  Virtual output to check.
 * @return true if @p device is < ASDF_VIRTUAL_NUM_RESOURCES, including
 *         V_NULL.
 */
static bool virtual_index_valid(asdf_virtual_dev_t device)
{
  return device < ASDF_VIRTUAL_NUM_RESOURCES;
}

/**
 * Check that a virtual output can be assigned.
 *
 * No side effects.
 *
 * @param device  Virtual output to check.
 * @return true if @p device indexes the virtual tables and is not V_NULL.
 *
 * Complexity: 2
 */
static bool valid_virtual_device(asdf_virtual_dev_t device)
{
  return (device > V_NULL) && (device < ASDF_VIRTUAL_NUM_RESOURCES);
}

/**
 * Apply an operation to each physical output in a list.
 *
 * Drives each output in the list through the platform and updates its shadow
 * value.
 *
 * @param phys    Physical output state.
 * @param device  First physical output in the list; PHYSICAL_NO_OUT for an
 *                empty list.
 * @param op      Operation to apply.
 *
 * The list is followed through asdf_physical_next_device() until it reaches
 * PHYSICAL_NO_OUT.
 *
 * Complexity: 3
 */
static void virtual_map(asdf_physical_state_t *phys, asdf_physical_dev_t device,
                        virtual_map_op_t op)
{
  asdf_physical_dev_t out = device;

  while (PHYSICAL_NO_OUT != out) {
    switch (op) {
      case MAP_ON: asdf_physical_on(phys, out); break;
      case MAP_OFF: asdf_physical_off(phys, out); break;
      case MAP_TOGGLE:
      default: asdf_physical_toggle(phys, out); break;
    }
    out = asdf_physical_next_device(phys, out);
  }
}

/**
 * Apply a function to every physical output assigned to a virtual output.
 *
 * Drives the physical outputs through the platform and updates their shadow
 * values. V_PULSE_LONG also records the pulse in @p virt; V_PULSE_SHORT
 * blocks for the short pulse width. Out-of-range virtual outputs are ignored.
 *
 * @param virt         Virtual output state.
 * @param virtual_out  Virtual output to act on.
 * @param function     Function to apply.
 *
 * The virtual output heads a linked list of physical outputs, walked by
 * virtual_map(). A long pulse is ended by asdf_virtual_tick().
 *
 * Complexity: 4
 */
void asdf_virtual_action(asdf_virtual_state_t *virt, asdf_virtual_dev_t virtual_out,
                           asdf_virtual_function_t function)
{
  if (!virtual_index_valid(virtual_out)) {
    return;
  }

  asdf_physical_state_t *phys = &virt->physical;
  asdf_physical_dev_t device_list = virt->physical_device[virtual_out];

  switch (function) {

    case V_PULSE_LONG: {
      // Start the pulse; asdf_virtual_tick() ends it. A pulse already in
      // progress is not restarted.
      if (virt->pulse_ticks[virtual_out] == 0u) {
        virtual_map(phys, device_list, MAP_TOGGLE);
        virt->pulse_ticks[virtual_out] = ASDF_PULSE_DELAY_LONG_MS;
      }
      break;
    }
    case V_PULSE_SHORT: {
      virtual_map(phys, device_list, MAP_TOGGLE);
      asdf_physical_pulse_delay_short(phys);
      virtual_map(phys, device_list, MAP_TOGGLE);
      break;
    }
    case V_TOGGLE: {
      virtual_map(phys, device_list, MAP_TOGGLE);
      break;
    }
    case V_SET_HI: {
      virtual_map(phys, device_list, MAP_ON);
      break;
    }
    case V_SET_LO: {
      virtual_map(phys, device_list, MAP_OFF);
      break;
    }
    case V_NOFUNC:
    default:
      // no function: nothing to drive
      break;
  }
}

/**
 * Apply a virtual output's assigned function to its physical outputs.
 *
 * Side effects as asdf_virtual_action(). Out-of-range virtual outputs are
 * ignored.
 *
 * @param virt         Virtual output state.
 * @param virtual_out  Virtual output to activate.
 *
 * Complexity: 2
 */
void asdf_virtual_activate(asdf_virtual_state_t *virt, asdf_virtual_dev_t virtual_out)
{
  if (virtual_index_valid(virtual_out)) {
    asdf_virtual_action(virt, virtual_out, virt->function[virtual_out]);
  }
}

/**
 * Assign a physical output to a virtual output.
 *
 * On success, allocates the physical output from the available list, prepends
 * it to the virtual output's list, records @p initial_value as its shadow
 * value, and sets the virtual output's function. No output is driven.
 *
 * @param virt           Virtual output state.
 * @param virtual_out    Virtual output to assign to.
 * @param physical_out   Physical output to assign.
 * @param function       Function applied when the virtual output is
 *                       activated.
 * @param initial_value  Initial shadow value of the physical output.
 * @return true if assigned; false if either output is invalid or @p
 *         physical_out is already assigned, with the state unchanged.
 *
 * The shadow values are driven to the outputs by asdf_virtual_sync() only
 * after all assignments are made.
 *
 * Complexity: 3
 */
bool asdf_virtual_assign(asdf_virtual_state_t *virt, asdf_virtual_dev_t virtual_out,
                         asdf_physical_dev_t physical_out, asdf_virtual_function_t function,
                         uint8_t initial_value)
{
  if (valid_virtual_device(virtual_out)) {
    asdf_physical_dev_t tail = virt->physical_device[virtual_out];
    if (asdf_physical_allocate(&virt->physical, physical_out, tail, initial_value)) {
      virt->physical_device[virtual_out] = physical_out;
      virt->function[virtual_out] = function;
      return true;
    }
  }
  return false;
}

/**
 * Initialize the virtual outputs, with no physical outputs assigned.
 *
 * Initializes the embedded physical state, then gives every virtual output an
 * empty list, function V_NOFUNC, and no pulse in progress. Writes only
 * @p virt; no output is driven.
 *
 * @param virt      State to initialize.
 * @param platform  Platform that drives the physical outputs, or NULL.
 *
 * Complexity: 2
 */
void asdf_virtual_init(asdf_virtual_state_t *virt, const struct asdf_platform *platform)
{
  asdf_physical_init(&virt->physical, platform);

  for (uint8_t i = 0u; i < (uint8_t) ASDF_VIRTUAL_NUM_RESOURCES; i++) {
    virt->function[i] = V_NOFUNC;
    virt->physical_device[i] = PHYSICAL_NO_OUT;
    virt->pulse_ticks[i] = 0;
  }
}

/**
 * Drive every physical output to its shadow value.
 *
 * Drives the outputs through the platform; @p virt is unchanged.
 *
 * @param virt  Virtual output state.
 *
 * Complexity: 2
 */
void asdf_virtual_sync(asdf_virtual_state_t *virt)
{
  for (uint8_t i = 0u; i < (uint8_t) ASDF_PHYSICAL_NUM_RESOURCES; i++) {
    asdf_physical_assert(&virt->physical, (asdf_physical_dev_t) i); //lint !e9030 D15
  }
}


/**
 * Advance each long pulse in progress, ending those whose time has expired.
 *
 * Decrements the pulse counts in @p virt. When a pulse's time has expired,
 * its physical outputs are toggled back through the platform.
 *
 * @param virt     Virtual output state.
 * @param elapsed  Ticks (ms) elapsed since the last call.
 *
 * Complexity: 4
 */
void asdf_virtual_tick(asdf_virtual_state_t *virt, uint8_t elapsed)
{
  for (uint8_t i = 0u; i < (uint8_t) ASDF_VIRTUAL_NUM_RESOURCES; i++) {
    if (virt->pulse_ticks[i] > 0u) {
      if (elapsed >= virt->pulse_ticks[i]) {
        virt->pulse_ticks[i] = 0;
        virtual_map(&virt->physical, virt->physical_device[i], MAP_TOGGLE);
      } else {
        virt->pulse_ticks[i] -= elapsed;
      }
    }
  }
}

//-------|---------|---------+---------+---------+---------+---------+---------+
// Above line is 80 columns, and should display completely in the editor.
