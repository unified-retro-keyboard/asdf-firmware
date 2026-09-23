// -*- mode: C; tab-width: 4 ; indent-tabs-mode: nil -*-
//
// Unfified Keyboard Project
// ASDF keyboard firmware
//
// asdf_virtual.h
//
// Definitions and prototypes for virtual LED and virtual output management.
//
// Copyright 2019 David Fenyes
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

#include "asdf_physical.h"

// These are "virtual" output identifiers that can be mapped to the real outputs using
// keymap initializer commands.
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


typedef enum {
  V_NOFUNC,
  V_SET_HI,
  V_SET_LO,
  V_PULSE_SHORT,
  V_PULSE_LONG,
  V_TOGGLE,
  ASDF_VIRTUAL_NUM_FUNCTIONS
} asdf_virtual_function_t;

// Each keymap specifies an array of initializer structs to configure virtual
// devices, specifying the mapped real device and initial value.
typedef struct {
  asdf_virtual_dev_t virtual_device;
  asdf_physical_dev_t physical_device;
  asdf_virtual_function_t function;
  uint8_t initial_value;
} asdf_virtual_initializer_t;

// Changeable state of the virtual outputs of one keyboard, including the
// physical outputs they drive.
typedef struct {
  asdf_physical_dev_t physical_device[ASDF_VIRTUAL_NUM_RESOURCES]; // head of each output's list
  asdf_virtual_function_t function[ASDF_VIRTUAL_NUM_RESOURCES];    // applied on activation
  asdf_physical_state_t physical;
} asdf_virtual_state_t;

// Instance API: each function operates only on the state passed to it.
// Invalid virtual outputs are ignored. See asdf_virtual.c.

void asdf_virtual_init_r(asdf_virtual_state_t *virt);
void asdf_virtual_action_r(asdf_virtual_state_t *virt, asdf_virtual_dev_t virtual_out,
                           asdf_virtual_function_t function);
void asdf_virtual_activate_r(asdf_virtual_state_t *virt, asdf_virtual_dev_t virtual_out);
void asdf_virtual_assign_r(asdf_virtual_state_t *virt, asdf_virtual_dev_t virtual_out,
                           asdf_physical_dev_t physical_out, asdf_virtual_function_t function,
                           uint8_t initial_value);
void asdf_virtual_sync_r(asdf_virtual_state_t *virt);

// Single-keyboard API, operating on the default virtual output state
// (asdf_compat.c). The single-keyboard physical output functions operate on
// the physical state embedded in it.

// PROCEDURE: asdf_virtual_action
// DESCRIPTION: apply function to the physical resources of virtual_out.
void asdf_virtual_action(asdf_virtual_dev_t virtual_out, asdf_virtual_function_t function);

// PROCEDURE: asdf_virtual_activate
// DESCRIPTION: apply virtual_out's assigned function to its physical resources.
void asdf_virtual_activate(asdf_virtual_dev_t virtual_out);

// PROCEDURE: asdf_virtual_assign
// DESCRIPTION: assign physical_out to virtual_out, with a function and an
// initial value.
void asdf_virtual_assign(asdf_virtual_dev_t virtual_out, asdf_physical_dev_t physical_out,
                         asdf_virtual_function_t function, uint8_t initial_value);

// PROCEDURE: asdf_virtual_init
// DESCRIPTION: initialize the virtual and physical outputs.
void asdf_virtual_init(void);

// PROCEDURE: asdf_virtual_sync
// DESCRIPTION: drive every physical output to its shadow value.
void asdf_virtual_sync(void);

#endif /* !defined (ASDF_VIRTUAL_H) */

//-------|---------|---------+---------+---------+---------+---------+---------+
// Above line is 80 columns, and should display completely in the editor.
