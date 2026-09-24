// -*- mode: C; tab-width: 4 ; indent-tabs-mode: nil -*-
//
//  Unfified Keyboard Project
//  ASDF keyboard firmware
//
//  asdf_modifiers.h
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

#if !defined(ASDF_MODIFIERS_H)
#define ASDF_MODIFIERS_H

#include <stdint.h>

// The active modifiers are used to build an index into a map that determinds
// which modifier map is selected.  The following define the bit position for each modifier.
// For example, if SHIFT and CTRL are active, the modifier index would be
//
// (1 << ASDF_MODIFIERS_SHIFT_POS) | (1 << ASDF_MODIFIERS_CAPS_POS)
// = (1 << 0) | (1 << 1)
// = (1 | 2)
// = 3
//
// So, if CTRL+SHIFT is a special modifier, then modifier_mapping[3] will point
// to the CTRL+SHIFT map. Alternatively, if CTRL+SHIFT is the same as just CTRL,
// then modifier_mapping[3] will point to the CTRL map.
#define ASDF_MODIFIERS_SHIFT_POS 0
#define ASDF_MODIFIERS_CAPS_POS 1
#define ASDF_MODIFIERS_CTRL_POS 2

#define ASDF_MODIFIERS_SHIFT_MASK (1 << ASDF_MODIFIERS_SHIFT_POS)
#define ASDF_MODIFIERS_CAPS_MASK (1 << ASDF_MODIFIERS_CAPS_POS)
#define ASDF_MODIFIERS_CTRL_MASK (1 << ASDF_MODIFIERS_CTRL_POS)

// Define the legal SHIFT and SHIFTLOCK states
typedef enum {
  SHIFT_OFF_ST = 0,
  SHIFT_ON_ST = 1,
  SHIFT_LOCKED_ST = 2,
  SHIFT_BOTH_ST = 3 // Never explicitly set. SHIFT and SHIFTLOCK together.

} shift_state_t;

// Define the legal CAPS and CAPSLOCK states
typedef enum {
  CAPS_OFF_ST = 0,
  CAPS_LOCKED_ST = 1,
} caps_state_t;

// Define the legal CTRL states
typedef enum { CTRL_OFF_ST = 0, CTRL_ON_ST = 1 } ctrl_state_t;

// Define the legal modifier mappings. In this case, we define 4 maps, for PLAIN
// (no modifier), SHIFT, CAPS, and CTRL. When combinations of these modifiers
// are active, then the precedence is determined by the modifier_mapping[]
// array.
typedef enum {
  MOD_PLAIN_MAP = 0,
  MOD_SHIFT_MAP,
  MOD_CAPS_MAP,
  MOD_CTRL_MAP,
  ASDF_MOD_NUM_MODIFIERS
} modifier_index_t;

// State of the modifier keys for one keyboard.
typedef struct {
  uint8_t shift; // shift_state_t: SHIFT and SHIFTLOCK bits
  uint8_t caps;  // caps_state_t
  uint8_t ctrl;  // ctrl_state_t
} asdf_modifier_state_t;

// Instance API: each function operates only on the state passed to it and
// does not drive indicator LEDs. See asdf_modifiers.c.

void asdf_modifiers_init_r(asdf_modifier_state_t *mods);
void asdf_modifier_shift_activate_r(asdf_modifier_state_t *mods);
void asdf_modifier_shiftlock_on_activate_r(asdf_modifier_state_t *mods);
void asdf_modifier_shiftlock_toggle_activate_r(asdf_modifier_state_t *mods);
void asdf_modifier_shift_deactivate_r(asdf_modifier_state_t *mods);
void asdf_modifier_capslock_activate_r(asdf_modifier_state_t *mods);
void asdf_modifier_ctrl_activate_r(asdf_modifier_state_t *mods);
void asdf_modifier_ctrl_deactivate_r(asdf_modifier_state_t *mods);
uint8_t asdf_modifier_shift_locked_r(const asdf_modifier_state_t *mods);
uint8_t asdf_modifier_caps_locked_r(const asdf_modifier_state_t *mods);
modifier_index_t asdf_modifier_index_r(const asdf_modifier_state_t *mods);

#endif // !defined (ASDF_MODIFIERS_H)

//-------|---------|---------+---------+---------+---------+---------+---------+
// Above line is 80 columns, and should display completely in the editor.
