// -*-mode : C;
// tab - width : 2; indent-tabs-mode: nil -*-
//
//  Unfified Keyboard Project
//  ASDF keyboard firmware
//
//  asdf_modifiers.c
//
// This file contains the logic for the modifier keys. Any new modifiers or
// special handling goes here.
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

#include <stdint.h>
#include "asdf_modifiers.h"

// All modifier state is held in the caller's asdf_modifier_state_t. These
// functions change only that state; driving the SHIFTLOCK and CAPSLOCK
// indicator LEDs is left to the caller (see asdf_sync_lock_leds_r), using
// asdf_modifier_shift_locked_r() and asdf_modifier_caps_locked_r().

// This table maps the active modifiers (a bitmap of ASDF_MODIFIERS_*_MASK) to
// the keymap used, and so defines the precedence of combined modifiers.
static const modifier_index_t modifier_mapping[] = { MOD_PLAIN_MAP, // 0x00: no modifiers
                                                     MOD_SHIFT_MAP, // 0x01: only SHIFT active
                                                     MOD_CAPS_MAP,  // 0x02: only CAPS active
                                                     MOD_SHIFT_MAP, // 0x03: CAPS and SHIFT active
                                                     MOD_CTRL_MAP,  // CTRL overrides SHIFT and CAPS
                                                     MOD_CTRL_MAP,  MOD_CTRL_MAP, MOD_CTRL_MAP };

// PROCEDURE: asdf_modifiers_init_r
// INPUTS: (asdf_modifier_state_t *) mods - modifier state to operate on
// OUTPUTS: none
//
// DESCRIPTION: Sets SHIFT, SHIFTLOCK, CAPS, and CTRL to OFF.
//
// SIDE EFFECTS: see DESCRIPTION
//
// COMPLEXITY: 1
//
void asdf_modifiers_init_r(asdf_modifier_state_t *mods)
{
  mods->shift = SHIFT_OFF_ST;
  mods->caps = CAPS_OFF_ST;
  mods->ctrl = CTRL_OFF_ST;
}

// PROCEDURE: asdf_modifier_shift_activate_r
// INPUTS: (asdf_modifier_state_t *) mods - modifier state to operate on
// OUTPUTS: none
//
// DESCRIPTION: Sets SHIFT to ON. This also clears SHIFTLOCK: a SHIFT press
// after SHIFTLOCK ends the lock when SHIFT is released.
//
// SIDE EFFECTS: see DESCRIPTION
//
// COMPLEXITY: 1
//
void asdf_modifier_shift_activate_r(asdf_modifier_state_t *mods)
{
  mods->shift = SHIFT_ON_ST;
}

// PROCEDURE: asdf_modifier_shiftlock_on_activate_r
// INPUTS: (asdf_modifier_state_t *) mods - modifier state to operate on
// OUTPUTS: none
//
// DESCRIPTION: Sets SHIFTLOCK to ON.
//
// SIDE EFFECTS: see DESCRIPTION
//
// COMPLEXITY: 1
//
void asdf_modifier_shiftlock_on_activate_r(asdf_modifier_state_t *mods)
{
  mods->shift |= SHIFT_LOCKED_ST;
}

// PROCEDURE: asdf_modifier_shiftlock_toggle_activate_r
// INPUTS: (asdf_modifier_state_t *) mods - modifier state to operate on
// OUTPUTS: none
//
// DESCRIPTION: Toggles SHIFTLOCK.
//
// SIDE EFFECTS: see DESCRIPTION
//
// COMPLEXITY: 1
//
void asdf_modifier_shiftlock_toggle_activate_r(asdf_modifier_state_t *mods)
{
  mods->shift ^= SHIFT_LOCKED_ST;
}

// PROCEDURE: asdf_modifier_shift_deactivate_r
// INPUTS: (asdf_modifier_state_t *) mods - modifier state to operate on
// OUTPUTS: none
//
// DESCRIPTION: Sets SHIFT and SHIFTLOCK to OFF.
//
// SIDE EFFECTS: see DESCRIPTION
//
// COMPLEXITY: 1
//
void asdf_modifier_shift_deactivate_r(asdf_modifier_state_t *mods)
{
  mods->shift = SHIFT_OFF_ST;
}

// PROCEDURE: asdf_modifier_capslock_activate_r
// INPUTS: (asdf_modifier_state_t *) mods - modifier state to operate on
// OUTPUTS: none
//
// DESCRIPTION: Toggles CAPSLOCK.
//
// SIDE EFFECTS: see DESCRIPTION
//
// COMPLEXITY: 1
//
void asdf_modifier_capslock_activate_r(asdf_modifier_state_t *mods)
{
  mods->caps ^= CAPS_LOCKED_ST;
}

// PROCEDURE: asdf_modifier_ctrl_activate_r
// INPUTS: (asdf_modifier_state_t *) mods - modifier state to operate on
// OUTPUTS: none
//
// DESCRIPTION: Sets CTRL to ON.
//
// SIDE EFFECTS: see DESCRIPTION
//
// COMPLEXITY: 1
//
void asdf_modifier_ctrl_activate_r(asdf_modifier_state_t *mods)
{
  mods->ctrl = CTRL_ON_ST;
}

// PROCEDURE: asdf_modifier_ctrl_deactivate_r
// INPUTS: (asdf_modifier_state_t *) mods - modifier state to operate on
// OUTPUTS: none
//
// DESCRIPTION: Sets CTRL to OFF.
//
// SIDE EFFECTS: see DESCRIPTION
//
// COMPLEXITY: 1
//
void asdf_modifier_ctrl_deactivate_r(asdf_modifier_state_t *mods)
{
  mods->ctrl = CTRL_OFF_ST;
}

// PROCEDURE: asdf_modifier_shift_locked_r
// INPUTS: (const asdf_modifier_state_t *) mods - modifier state to query
// OUTPUTS: returns TRUE (nonzero) if SHIFTLOCK is on
//
// COMPLEXITY: 1
//
uint8_t asdf_modifier_shift_locked_r(const asdf_modifier_state_t *mods)
{
  return (mods->shift & SHIFT_LOCKED_ST) != 0;
}

// PROCEDURE: asdf_modifier_caps_locked_r
// INPUTS: (const asdf_modifier_state_t *) mods - modifier state to query
// OUTPUTS: returns TRUE (nonzero) if CAPSLOCK is on
//
// COMPLEXITY: 1
//
uint8_t asdf_modifier_caps_locked_r(const asdf_modifier_state_t *mods)
{
  return mods->caps != CAPS_OFF_ST;
}

// PROCEDURE: asdf_modifier_index_r
// INPUTS: (const asdf_modifier_state_t *) mods - modifier state to query
// OUTPUTS: returns the keymap index for the active modifiers
//
// DESCRIPTION: See OUTPUTS. Precedence among combined modifiers is set by
// modifier_mapping[].
//
// COMPLEXITY: 4
//
modifier_index_t asdf_modifier_index_r(const asdf_modifier_state_t *mods)
{
  uint8_t active_modifiers = 0;

  if (mods->shift) {
    active_modifiers |= ASDF_MODIFIERS_SHIFT_MASK;
  }
  if (mods->caps) {
    active_modifiers |= ASDF_MODIFIERS_CAPS_MASK;
  }
  if (mods->ctrl) {
    active_modifiers |= ASDF_MODIFIERS_CTRL_MASK;
  }

  return modifier_mapping[active_modifiers];
}


//-------|---------|---------+---------+---------+---------+---------+---------+
// Above line is 80 columns, and should display completely in the editor.
