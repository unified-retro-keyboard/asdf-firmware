// -*- mode: C; tab-width: 2 ; indent-tabs-mode: nil -*-
/**
 * @file asdf_physical.c
 *
 * This file contains code to manage physical resources and serves as an API
 * between the virtual layer and the architecture specific code.
 *
 * Part of the Unified Keyboard Project ASDF keyboard firmware.
 *
 * @copyright Copyright 2019 David F. MIT License; see LICENSE.
 */
// SPDX-License-Identifier: MIT

#include <stdbool.h>
#include <stdint.h>
#include "asdf_physical.h"
#include "asdf_config.h"
#include "asdf_platform.h"


// For each physical output there is a "shadow" register holding the value last
// written, and the platform drives the output itself. The shadow registers let
// toggle and pulse be implemented here, machine-independently, so the platform
// needs only a "set" operation for each output. This is less efficient than
// native toggles, but output events are infrequent and not timing-critical.
//
// Outputs are allocated through the next[] links, a singly linked list per
// owner in one table. next[PHYSICAL_NO_OUT] heads the list of available
// outputs; asdf_physical_allocate() unlinks an output from it and links the
// output in front of a virtual output's list. Every list ends at
// PHYSICAL_NO_OUT. Outputs are never freed individually: asdf_physical_init()
// returns them all to the available list.
//
// The shadow registers, the links, and the platform are held in the caller's
// asdf_physical_state_t. The public contracts are in asdf_physical.h.

/**
 * Check that a device indexes the physical tables.
 *
 * No side effects.
 *
 * @param device  Device to check.
 * @return true if @p device is < ASDF_PHYSICAL_NUM_RESOURCES, including
 *         PHYSICAL_NO_OUT.
 */
static bool physical_index_valid(asdf_physical_dev_t device)
{
  return device < ASDF_PHYSICAL_NUM_RESOURCES;
}

/**
 * Check that a device is a real output.
 *
 * No side effects.
 *
 * @param device  Device to check.
 * @return true if @p device indexes the physical tables and is not
 *         PHYSICAL_NO_OUT.
 *
 * Complexity: 2
 */
static bool valid_physical_device(asdf_physical_dev_t device)
{
  return (device > PHYSICAL_NO_OUT) && (device < ASDF_PHYSICAL_NUM_RESOURCES);
}

/**
 * Drive an output through the platform, if any, and record its shadow value.
 *
 * Calls the platform's set_output operation and updates the shadow value.
 *
 * @param phys    Physical output state.
 * @param device  Output to write; must be < ASDF_PHYSICAL_NUM_RESOURCES.
 * @param value   Value to drive and record.
 *
 * Complexity: 2
 */
static void physical_write(asdf_physical_state_t *phys, asdf_physical_dev_t device, uint8_t value)
{
  const asdf_platform_t *platform = phys->platform;

  if (platform != NULL) {
    platform->set_output(platform->user, device, value);
  }
  phys->shadow[device] = value;
}

/**
 * Drive an output to a value and record it as the shadow value.
 *
 * Drives the output through the platform, if any, and updates the shadow
 * value. Out-of-range outputs are ignored.
 *
 * @param phys          Physical output state.
 * @param physical_out  Output to set.
 * @param value         Value to drive and record.
 *
 * Complexity: 2
 */
void asdf_physical_set(asdf_physical_state_t *phys, asdf_physical_dev_t physical_out,
                         uint8_t value)
{
  if (physical_index_valid(physical_out)) {
    physical_write(phys, physical_out, value);
  }
}

/**
 * Set an output high (1).
 *
 * Side effects as asdf_physical_set().
 *
 * @param phys          Physical output state.
 * @param physical_out  Output to set.
 */
void asdf_physical_on(asdf_physical_state_t *phys, asdf_physical_dev_t physical_out)
{
  asdf_physical_set(phys, physical_out, 1);
}

/**
 * Set an output low (0).
 *
 * Side effects as asdf_physical_set().
 *
 * @param phys          Physical output state.
 * @param physical_out  Output to clear.
 */
void asdf_physical_off(asdf_physical_state_t *phys, asdf_physical_dev_t physical_out)
{
  asdf_physical_set(phys, physical_out, 0);
}

/**
 * Drive an output to the value in its shadow register.
 *
 * Drives the output through the platform, if any; the shadow value is
 * unchanged. Out-of-range outputs are ignored.
 *
 * @param phys          Physical output state.
 * @param physical_out  Output to drive.
 *
 * Complexity: 2
 */
void asdf_physical_assert(asdf_physical_state_t *phys, asdf_physical_dev_t physical_out)
{
  if (physical_index_valid(physical_out)) {
    physical_write(phys, physical_out, phys->shadow[physical_out]);
  }
}

/**
 * Invert an output.
 *
 * Drives the output through the platform, if any, to the logical NOT of its
 * shadow value, and updates the shadow value. Out-of-range outputs are
 * ignored.
 *
 * @param phys          Physical output state.
 * @param physical_out  Output to toggle.
 *
 * Complexity: 2
 */
void asdf_physical_toggle(asdf_physical_state_t *phys, asdf_physical_dev_t physical_out)
{
  if (physical_out < ASDF_PHYSICAL_NUM_RESOURCES) {
    uint8_t toggled = (uint8_t) ((phys->shadow[physical_out] != 0u) ? 0u : 1u);
    physical_write(phys, physical_out, toggled);
  }
}

/**
 * Find the element before a device on the available list.
 *
 * No side effects.
 *
 * @param phys    Physical output state.
 * @param device  Device to look for.
 * @return The element before @p device on the available list (PHYSICAL_NO_OUT
 *         if it is first); ASDF_PHYSICAL_NUM_RESOURCES if @p device is not on
 *         the list, that is, already allocated.
 *
 * Walks the available list from its head, PHYSICAL_NO_OUT.
 *
 * Complexity: 4
 */
static asdf_physical_dev_t physical_device_predecessor(const asdf_physical_state_t *phys,
                                                       asdf_physical_dev_t device)
{
  asdf_physical_dev_t current_out = PHYSICAL_NO_OUT;
  asdf_physical_dev_t next_out = phys->next[current_out];

  while ((next_out != PHYSICAL_NO_OUT) && (next_out != device)) {
    current_out = next_out;
    next_out = phys->next[current_out]; //lint !e661 next[] holds only valid indexes (asdf_physical_state_t invariant)
  }

  return (PHYSICAL_NO_OUT == next_out) ? ASDF_PHYSICAL_NUM_RESOURCES : current_out;
}

/**
 * Return the output after a device in its list.
 *
 * No side effects.
 *
 * @param phys    Physical output state.
 * @param device  Current output in a virtual output's list.
 * @return The next output in the list; PHYSICAL_NO_OUT at the end of the list
 *         or if @p device is out of range.
 *
 * Complexity: 2
 */
asdf_physical_dev_t asdf_physical_next_device(const asdf_physical_state_t *phys,
                                                asdf_physical_dev_t device)
{
  return (device < ASDF_PHYSICAL_NUM_RESOURCES) ? phys->next[device] : PHYSICAL_NO_OUT;
}

/**
 * Take an output off the available list and link a list after it.
 *
 * Checks that the output is valid and available. If so, removes it from the
 * available list, links @p tail after it, and records its initial shadow
 * value. No output is driven.
 *
 * @param phys           Physical output state.
 * @param physical_out   Output to allocate.
 * @param tail           List to link after @p physical_out.
 * @param initial_value  Initial shadow value of @p physical_out.
 * @return true if allocated; false if either device is invalid or @p
 *         physical_out is already allocated, with the state unchanged.
 *
 * The shadow values are driven to the outputs only after all the assignments
 * have been made.
 *
 * Complexity: 4
 */
bool asdf_physical_allocate(asdf_physical_state_t *phys, asdf_physical_dev_t physical_out,
                            asdf_physical_dev_t tail, uint8_t initial_value)
{
  if (!valid_physical_device(physical_out) || !physical_index_valid(tail)) {
    return false;
  }

  asdf_physical_dev_t predecessor = physical_device_predecessor(phys, physical_out);
  if (ASDF_PHYSICAL_NUM_RESOURCES == predecessor) {
    return false;
  }

  // Remove from the available list.
  phys->next[predecessor] = phys->next[physical_out];

  // tack the tail on to the physical resource
  phys->next[physical_out] = tail;
  phys->shadow[physical_out] = initial_value;
  return true;
}

/**
 * Wait for the width of a short pulse on the outputs, through the platform.
 *
 * Blocks the caller for the pulse width; returns at once if the state has no
 * platform. No other side effects.
 *
 * @param phys  Physical output state.
 *
 * Complexity: 2
 */
void asdf_physical_pulse_delay_short(const asdf_physical_state_t *phys)
{
  const asdf_platform_t *platform = phys->platform;

  if (platform != NULL) {
    platform->pulse_delay_short(platform->user);
  }
}

/**
 * Initialize the shadow registers and the available list, and record the
 * platform.
 *
 * Sets every shadow value to ASDF_VIRTUAL_OUT_DEFAULT_VALUE and links every
 * output into the available list in order, headed by PHYSICAL_NO_OUT. Writes
 * only @p phys; no output is driven.
 *
 * @param phys      State to initialize.
 * @param platform  Platform that drives the outputs; NULL to track shadow
 *                  values only.
 *
 * Complexity: 2
 */
void asdf_physical_init(asdf_physical_state_t *phys, const struct asdf_platform *platform)
{
  phys->platform = platform;

  for (uint8_t i = 0u; i < (uint8_t) ASDF_PHYSICAL_NUM_RESOURCES; i++) {
    uint8_t next = i + 1u;
    phys->shadow[i] = ASDF_VIRTUAL_OUT_DEFAULT_VALUE;
    phys->next[i] = (asdf_physical_dev_t) next; //lint !e9030 D15
  }

  // The last element is left pointing beyond the end of the table by the loop
  // above. Terminate the list at PHYSICAL_NO_OUT.
  phys->next[(uint8_t) ASDF_PHYSICAL_NUM_RESOURCES - 1u] = PHYSICAL_NO_OUT;
}


//-------|---------|---------+---------+---------+---------+---------+---------+
// Above line is 80 columns, and should display completely in the editor.
