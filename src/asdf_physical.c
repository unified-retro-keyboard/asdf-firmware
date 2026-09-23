// -*- mode: C; tab-width: 2 ; indent-tabs-mode: nil -*-
//
// Unified Keyboard Project
// ASDF keyboard firmware
//
// asdf_physical.c
//
// This file contains code to manage physical resources and serves as an API
// between the virtual layer and the architecture specific code.
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
#include "asdf_config.h"
#include "asdf_platform.h"


// For each physical resource, there is a "shadow" register for the output
// value, and the platform drives the output itself.
//
// For line outputs, the shadow register permits machine independent
// implementations of the toggle and pulse functions to be implemented in this
// module, requiring only a "set" operation for each physical resource in the
// platform. This implementation is not as efficient, but the timing is not
// critical, and the events are so infrequent that the benefits of the
// refactoring far outweigh any performance penalty.
//
// The shadow registers, the allocation links, and the platform are held in the
// caller's asdf_physical_state_t.

// PROCEDURE: physical_index_valid
// INPUTS: (asdf_physical_dev_t) device - device to check
// OUTPUTS: returns TRUE (nonzero) if device indexes the physical tables,
//          including PHYSICAL_NO_OUT.
//
// SCOPE: private
//
// COMPLEXITY: 1
//
static uint8_t physical_index_valid(asdf_physical_dev_t device)
{
  return device < ASDF_PHYSICAL_NUM_RESOURCES;
}

// PROCEDURE: valid_physical_device
// INPUTS: (asdf_physical_dev_t) device - device to check
// OUTPUTS: returns TRUE (nonzero) if device is a real output (not
//          PHYSICAL_NO_OUT).
//
// SCOPE: private
//
// COMPLEXITY: 1
//
static uint8_t valid_physical_device(asdf_physical_dev_t device)
{
  return (device > PHYSICAL_NO_OUT && device < ASDF_PHYSICAL_NUM_RESOURCES);
}

// PROCEDURE: physical_write
// INPUTS: (asdf_physical_state_t *) phys, (asdf_physical_dev_t) device (valid
//         index), (uint8_t) value
// OUTPUTS: none
//
// DESCRIPTION: Drives the output through the platform, if any, and records
// the value in the shadow register.
//
// SCOPE: private
//
// COMPLEXITY: 2
//
static void physical_write(asdf_physical_state_t *phys, asdf_physical_dev_t device, uint8_t value)
{
  const asdf_platform_t *platform = phys->platform;

  if (platform) {
    platform->set_output(platform->user, device, value);
  }
  phys->shadow[device] = value;
}

// PROCEDURE: asdf_physical_set_r
// INPUTS: (asdf_physical_state_t *) phys - physical output state
//         (asdf_physical_dev_t) physical_out - the physical resource to be set.
//         (uint8_t) value - the value to set the resource to.
// OUTPUTS: none
//
// DESCRIPTION: If the physical resource is valid, set it to value.
//
// SIDE EFFECTS: see above.
//
// SCOPE: public
//
// COMPLEXITY: 2
//
void asdf_physical_set_r(asdf_physical_state_t *phys, asdf_physical_dev_t physical_out,
                         uint8_t value)
{
  if (physical_index_valid(physical_out)) {
    physical_write(phys, physical_out, value);
  }
}

// PROCEDURE: asdf_physical_on_r
// INPUTS: (asdf_physical_state_t *) phys, (asdf_physical_dev_t) physical_out
// OUTPUTS: none
//
// DESCRIPTION: If the physical resource is valid, set to high.
//
// SCOPE: public
//
// COMPLEXITY: 1
//
void asdf_physical_on_r(asdf_physical_state_t *phys, asdf_physical_dev_t physical_out)
{
  asdf_physical_set_r(phys, physical_out, 1);
}

// PROCEDURE: asdf_physical_off_r
// INPUTS: (asdf_physical_state_t *) phys, (asdf_physical_dev_t) physical_out
// OUTPUTS: none
//
// DESCRIPTION: If the physical resource is valid, set to low.
//
// SCOPE: public
//
// COMPLEXITY: 1
//
void asdf_physical_off_r(asdf_physical_state_t *phys, asdf_physical_dev_t physical_out)
{
  asdf_physical_set_r(phys, physical_out, 0);
}

// PROCEDURE: asdf_physical_assert_r
// INPUTS: (asdf_physical_state_t *) phys, (asdf_physical_dev_t) physical_out
// OUTPUTS: none
//
// DESCRIPTION: If the physical resource is valid, drive the output to the
// value in its shadow register.
//
// SCOPE: public
//
// COMPLEXITY: 2
//
void asdf_physical_assert_r(asdf_physical_state_t *phys, asdf_physical_dev_t physical_out)
{
  if (physical_index_valid(physical_out)) {
    physical_write(phys, physical_out, phys->shadow[physical_out]);
  }
}

// PROCEDURE: asdf_physical_toggle_r
// INPUTS: (asdf_physical_state_t *) phys, (asdf_physical_dev_t) physical_out
// OUTPUTS: none
//
// DESCRIPTION: If the physical resource is valid, toggle its value.
//
// SCOPE: public
//
// COMPLEXITY: 2
//
void asdf_physical_toggle_r(asdf_physical_state_t *phys, asdf_physical_dev_t physical_out)
{
  if (physical_index_valid(physical_out)) {
    physical_write(phys, physical_out, !phys->shadow[physical_out]);
  }
}

// PROCEDURE: physical_device_predecessor
// INPUTS: (const asdf_physical_state_t *) phys, (asdf_physical_dev_t) device
// OUTPUTS: If device is on the available list, returns the element before it
//          in the list. If device is already allocated, returns
//          ASDF_PHYSICAL_NUM_RESOURCES.
//
// DESCRIPTION: iterates through the linked list of available devices, which
// starts at PHYSICAL_NO_OUT.
//
// SIDE EFFECTS: none
//
// SCOPE: private
//
// COMPLEXITY: 3
//
static asdf_physical_dev_t physical_device_predecessor(const asdf_physical_state_t *phys,
                                                       asdf_physical_dev_t device)
{
  asdf_physical_dev_t current_out = PHYSICAL_NO_OUT;
  asdf_physical_dev_t next_out = phys->next[current_out];

  while (next_out != PHYSICAL_NO_OUT && next_out != device) {
    current_out = next_out;
    next_out = phys->next[current_out];
  }

  return (PHYSICAL_NO_OUT == next_out) ? ASDF_PHYSICAL_NUM_RESOURCES : current_out;
}

// PROCEDURE: asdf_physical_next_device_r
// INPUTS: (const asdf_physical_state_t *) phys
//         (asdf_physical_dev_t) device - the current physical resource attached
//         to the virtual output being operated on
// OUTPUTS: (asdf_physical_dev_t) returns the next physical resource assigned to
//          the virtual output, or PHYSICAL_NO_OUT if device is invalid.
//
// SIDE EFFECTS: None.
//
// SCOPE: public
//
// COMPLEXITY: 2
//
asdf_physical_dev_t asdf_physical_next_device_r(const asdf_physical_state_t *phys,
                                                asdf_physical_dev_t device)
{
  return physical_index_valid(device) ? phys->next[device] : PHYSICAL_NO_OUT;
}

// PROCEDURE: asdf_physical_allocate_r
//
// INPUTS: (asdf_physical_state_t *) phys
//         (asdf_physical_dev_t) physical_out - the desired physical resource to allocate.
//         (asdf_physical_dev_t) tail - the list of physical resources to tack on
//         to the requested resource, if available.
//         (uint8_t) initial_value - initial shadow value of the resource
//
// OUTPUTS: returns TRUE if the allocation is succesful, FALSE (0) otherwise.
//
// DESCRIPTION: Check that the requested physical resource is valid and
// available. If so, then remove the resource from the available list, link
// the tail to it, and assign an initial shadow value, then return TRUE (1).
// Return FALSE (0) if allocation was not successful.
//
// SIDE EFFECTS: see above.
//
// NOTES: The shadow values are asserted to the outputs only after all the
// assignments have been performed.
//
// SCOPE: public
//
// COMPLEXITY: 2
//
uint8_t asdf_physical_allocate_r(asdf_physical_state_t *phys, asdf_physical_dev_t physical_out,
                                 asdf_physical_dev_t tail, uint8_t initial_value)
{
  if (!valid_physical_device(physical_out) || !physical_index_valid(tail)) {
    return 0;
  }

  asdf_physical_dev_t predecessor = physical_device_predecessor(phys, physical_out);
  if (ASDF_PHYSICAL_NUM_RESOURCES == predecessor) {
    return 0;
  }

  // remove from available list:
  phys->next[predecessor] = phys->next[physical_out];

  // tack the tail on to the physical resource
  phys->next[physical_out] = tail;
  phys->shadow[physical_out] = initial_value;
  return 1;
}

// PROCEDURE: asdf_physical_pulse_delay_short_r
// INPUTS: (const asdf_physical_state_t *) phys
// OUTPUTS: none
//
// DESCRIPTION: Waits for the width of a short pulse on the outputs, through
// the platform, if any.
//
// SCOPE: public
//
// COMPLEXITY: 2
//
void asdf_physical_pulse_delay_short_r(const asdf_physical_state_t *phys)
{
  const asdf_platform_t *platform = phys->platform;

  if (platform) {
    platform->pulse_delay_short(platform->user);
  }
}

// PROCEDURE: asdf_physical_init_r
// INPUTS: (asdf_physical_state_t *) phys
//         (const asdf_platform_t *) platform - drives the outputs; NULL to
//         track shadow values only
// OUTPUTS: none
//
// DESCRIPTION: Initialize the shadow registers to the default value, place
// every device on the available list, and record the platform.
//
// SIDE EFFECTS: see above
//
// SCOPE: public
//
// COMPLEXITY: 2
//
void asdf_physical_init_r(asdf_physical_state_t *phys, const asdf_platform_t *platform)
{
  phys->platform = platform;

  for (uint8_t i = 0; i < ASDF_PHYSICAL_NUM_RESOURCES; i++) {
    phys->shadow[i] = ASDF_VIRTUAL_OUT_DEFAULT_VALUE;
    phys->next[i] = (asdf_physical_dev_t) (i + 1);
  }

  // The last element is left pointing beyond the end of the table by the loop
  // above. Terminate the list at PHYSICAL_NO_OUT.
  phys->next[ASDF_PHYSICAL_NUM_RESOURCES - 1] = PHYSICAL_NO_OUT;
}


//-------|---------|---------+---------+---------+---------+---------+---------+
// Above line is 80 columns, and should display completely in the editor.
