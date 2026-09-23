// -*- mode: C; tab-width: 2 ; indent-tabs-mode: nil -*-
//
// Unified Keyboard Project
// ASDF keyboard firmware
//
// asdf_virtual_outputs.c
//
// This file contains code that maps "virtual" LEDs and outputs referenced by
// the code to actual LEDs and outputs in hardware. This keeps keymap-specific
// details out of the architecture-dependent files, and provides a flexible way
// for the keymap definitions to specify the LED and output functions for
// different keymaps or keyboard layouts.
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
//

#include <stdint.h>
#include "asdf_physical.h"
#include "asdf_virtual.h"
#include "asdf_config.h"
#include "asdf_arch.h"

// A virtual output identifies one element of the tables in
// asdf_virtual_state_t. Each element holds the first in the list of physical
// resources (if any) assigned to the virtual output, and the function applied
// to those resources when the virtual output is activated by a keypress. The
// physical resources are held in the physical state embedded in the virtual
// state.

// Operations applied along a virtual output's list of physical resources.
typedef enum { MAP_TOGGLE, MAP_ON, MAP_OFF } virtual_map_op_t;

// PROCEDURE: virtual_index_valid
// INPUTS: (asdf_virtual_dev_t) device - virtual output to check
// OUTPUTS: returns TRUE (nonzero) if device indexes the virtual tables,
//          including V_NULL.
//
// SCOPE: private
//
// COMPLEXITY: 1
//
static uint8_t virtual_index_valid(asdf_virtual_dev_t device)
{
  return device < ASDF_VIRTUAL_NUM_RESOURCES;
}

// PROCEDURE: valid_virtual_device
// INPUTS: (asdf_virtual_dev_t) device - virtual output to check
// OUTPUTS: returns TRUE (nonzero) if device is an assignable virtual output
//          (not V_NULL).
//
// SCOPE: private
//
// COMPLEXITY: 1
//
static uint8_t valid_virtual_device(asdf_virtual_dev_t device)
{
  return (device > V_NULL && device < ASDF_VIRTUAL_NUM_RESOURCES);
}

// PROCEDURE: virtual_map
// INPUTS: (asdf_physical_state_t *) phys - physical output state
//         (asdf_physical_dev_t) device - first physical resource in the list
//         (virtual_map_op_t) op - operation to apply to each resource
// OUTPUTS: none
//
// DESCRIPTION: Applies op to each physical resource in the list starting at
// device.
//
// SIDE EFFECTS: see DESCRIPTION
//
// SCOPE: private
//
// COMPLEXITY: 4
//
static void virtual_map(asdf_physical_state_t *phys, asdf_physical_dev_t device,
                        virtual_map_op_t op)
{
  while (PHYSICAL_NO_OUT != device) {
    switch (op) {
      case MAP_ON: asdf_physical_on_r(phys, device); break;
      case MAP_OFF: asdf_physical_off_r(phys, device); break;
      case MAP_TOGGLE:
      default: asdf_physical_toggle_r(phys, device); break;
    }
    device = asdf_physical_next_device_r(phys, device);
  }
}

// PROCEDURE: asdf_virtual_action_r
// INPUTS: (asdf_virtual_state_t *) virt - virtual output state
//         (asdf_virtual_dev_t) virtual_out - which virtual output to modify
//         (asdf_virtual_function_t) function - what function to apply to the
//         virtual output
// OUTPUTS: none
//
// DESCRIPTION: for each physical resource assigned to the virtual output,
// apply the specified function. Invalid virtual outputs are ignored.
//
// SIDE EFFECTS: see DESCRIPTION
//
// NOTES: The virtual output points to a linked list of physical resources.
//
// SCOPE: public
//
// COMPLEXITY: 7
//
void asdf_virtual_action_r(asdf_virtual_state_t *virt, asdf_virtual_dev_t virtual_out,
                           asdf_virtual_function_t function)
{
  if (!virtual_index_valid(virtual_out)) {
    return;
  }

  asdf_physical_state_t *phys = &virt->physical;
  asdf_physical_dev_t device_list = virt->physical_device[virtual_out];

  switch (function) {

    case V_PULSE_LONG: {
      // Start the pulse; asdf_virtual_tick_r() ends it. A pulse already in
      // progress is not restarted.
      if (!virt->pulse_ticks[virtual_out]) {
        virtual_map(phys, device_list, MAP_TOGGLE);
        virt->pulse_ticks[virtual_out] = ASDF_PULSE_DELAY_LONG_MS;
      }
      break;
    }
    case V_PULSE_SHORT: {
      virtual_map(phys, device_list, MAP_TOGGLE);
      asdf_arch_pulse_delay_short();
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
    default: break;
  }
}

// PROCEDURE: asdf_virtual_activate_r
// INPUTS: (asdf_virtual_state_t *) virt - virtual output state
//         (asdf_virtual_dev_t) virtual_out - which virtual output to activate
// OUTPUTS: none
//
// DESCRIPTION: apply the virtual output's assigned function to its physical
// resources. Invalid virtual outputs are ignored.
//
// SIDE EFFECTS: see DESCRIPTION
//
// SCOPE: public
//
// COMPLEXITY: 2
//
void asdf_virtual_activate_r(asdf_virtual_state_t *virt, asdf_virtual_dev_t virtual_out)
{
  if (virtual_index_valid(virtual_out)) {
    asdf_virtual_action_r(virt, virtual_out, virt->function[virtual_out]);
  }
}

// PROCEDURE: asdf_virtual_assign_r
// INPUTS: (asdf_virtual_state_t *) virt - virtual output state
//         (asdf_virtual_dev_t) virtual_out - virtual output to be paired with
//         the physical resource
//         (asdf_physical_dev_t) physical_out to be assigned to the virtual output.
//         (asdf_virtual_function_t) - the function to be applied to the virtual
//              device when activated by a keypress.
//         (uint8_t) initial_value - the initial state of the physical resource.
//
// OUTPUTS: none
//
// DESCRIPTION: map the virtual output specified by virtual_out to
// physical_out, if both arguments are valid. Ignore if not valid.
//
// SIDE EFFECTS: see above.
//
// NOTES: if the virtual output is invalid, or the physical resource is
// invalid, or the physical resource is already assigned, then nothing happens.
//
// SCOPE: public
//
// COMPLEXITY: 3
//
void asdf_virtual_assign_r(asdf_virtual_state_t *virt, asdf_virtual_dev_t virtual_out,
                           asdf_physical_dev_t physical_out, asdf_virtual_function_t function,
                           uint8_t initial_value)
{
  if (valid_virtual_device(virtual_out)) {
    asdf_physical_dev_t tail = virt->physical_device[virtual_out];
    if (asdf_physical_allocate_r(&virt->physical, physical_out, tail, initial_value)) {
      virt->physical_device[virtual_out] = physical_out;
      virt->function[virtual_out] = function;
    }
  }
}

// PROCEDURE: asdf_virtual_init_r
// INPUTS: (asdf_virtual_state_t *) virt - virtual output state
// OUTPUTS: none
//
// DESCRIPTION: Initialize the virtual outputs, with no physical resources
// assigned and no function, and initialize the embedded physical state.
//
// SIDE EFFECTS: see above.
//
// SCOPE: public
//
// COMPLEXITY: 2
//
void asdf_virtual_init_r(asdf_virtual_state_t *virt)
{
  asdf_physical_init_r(&virt->physical);

  for (uint8_t i = 0; i < ASDF_VIRTUAL_NUM_RESOURCES; i++) {
    virt->function[i] = V_NOFUNC;
    virt->physical_device[i] = PHYSICAL_NO_OUT;
    virt->pulse_ticks[i] = 0;
  }
}

// PROCEDURE: asdf_virtual_sync_r
// INPUTS: (asdf_virtual_state_t *) virt - virtual output state
// OUTPUTS: none
//
// DESCRIPTION: Drive every physical output to its shadow value.
//
// SIDE EFFECTS: see above.
//
// SCOPE: public
//
// COMPLEXITY: 2
//
void asdf_virtual_sync_r(asdf_virtual_state_t *virt)
{
  for (uint8_t i = 0; i < ASDF_PHYSICAL_NUM_RESOURCES; i++) {
    asdf_physical_assert_r(&virt->physical, (asdf_physical_dev_t) i);
  }
}


// PROCEDURE: asdf_virtual_tick_r
// INPUTS: (asdf_virtual_state_t *) virt - virtual output state
//         (uint8_t) elapsed - ticks (ms) elapsed since the last call
// OUTPUTS: none
//
// DESCRIPTION: Advances each long pulse in progress. When a pulse's time has
// expired, its physical resources are toggled back.
//
// SIDE EFFECTS: see DESCRIPTION
//
// SCOPE: public
//
// COMPLEXITY: 4
//
void asdf_virtual_tick_r(asdf_virtual_state_t *virt, uint8_t elapsed)
{
  for (uint8_t i = 0; i < ASDF_VIRTUAL_NUM_RESOURCES; i++) {
    if (virt->pulse_ticks[i]) {
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
