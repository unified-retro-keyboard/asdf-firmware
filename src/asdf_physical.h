// -*- mode: C; tab-width: 4 ; indent-tabs-mode: nil -*-
//
// Unfified Keyboard Project
// ASDF keyboard firmware
//
// asdf_physical.h
//
// Definitions and prototypes for physical LED and virtual output resources.
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

#if !defined(ASDF_PHYSICAL_H)
#define ASDF_PHYSICAL_H

#include <stdint.h>

// The asdf_virtual_real_dev_t enumerates real outputs that can be assigned to
// virtual outputs via the asdf_virtual_assign() function. The name is a bit
// confusing, containing virtual and real. The "virtual" part refers to the
// module and the "real_dev" part attempts to clarify that these are the
// hardware outputs implemented by the architecture-specific module.
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

// Changeable state of the physical outputs of one keyboard: the value last
// written to each output, and the links of the available list and of each
// virtual output's list of physical outputs.
typedef struct {
  uint8_t shadow[ASDF_PHYSICAL_NUM_RESOURCES];
  asdf_physical_dev_t next[ASDF_PHYSICAL_NUM_RESOURCES];
} asdf_physical_state_t;

// Instance API: each function operates only on the state passed to it.
// Invalid devices are ignored. See asdf_physical.c.

void asdf_physical_init_r(asdf_physical_state_t *phys);
void asdf_physical_set_r(asdf_physical_state_t *phys, asdf_physical_dev_t physical_out,
                         uint8_t value);
void asdf_physical_on_r(asdf_physical_state_t *phys, asdf_physical_dev_t physical_out);
void asdf_physical_off_r(asdf_physical_state_t *phys, asdf_physical_dev_t physical_out);
void asdf_physical_assert_r(asdf_physical_state_t *phys, asdf_physical_dev_t physical_out);
void asdf_physical_toggle_r(asdf_physical_state_t *phys, asdf_physical_dev_t physical_out);
asdf_physical_dev_t asdf_physical_next_device_r(const asdf_physical_state_t *phys,
                                                asdf_physical_dev_t device);
uint8_t asdf_physical_allocate_r(asdf_physical_state_t *phys, asdf_physical_dev_t physical_out,
                                 asdf_physical_dev_t tail, uint8_t initial_value);

// Single-keyboard API, operating on the default physical output state
// (asdf_compat.c).

void asdf_physical_init(void);
void asdf_physical_set(asdf_physical_dev_t physical_out, uint8_t value);
void asdf_physical_on(asdf_physical_dev_t physical_out);
void asdf_physical_off(asdf_physical_dev_t physical_out);
void asdf_physical_assert(asdf_physical_dev_t physical_out);
void asdf_physical_toggle(asdf_physical_dev_t physical_out);
asdf_physical_dev_t asdf_physical_next_device(asdf_physical_dev_t device);
uint8_t asdf_physical_allocate(asdf_physical_dev_t physical_out, asdf_physical_dev_t tail,
                               uint8_t initial_value);

#endif /* !defined (ASDF_PHYSICAL_H) */

//-------|---------|---------+---------+---------+---------+---------+---------+
// Above line is 80 columns, and should display completely in the editor.
