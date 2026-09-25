// -*- mode: C; tab-width: 2 ; indent-tabs-mode: nil -*-
/**
 * @file asdf_keymap_actions.c
 *
 * The action table for the production keymaps: the built-in actions and the
 * keymap-provided actions (asdf_keymap_actions.h).
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

#include "asdf_arch.h"
#include "asdf_actions.h"
#include "asdf_keymap_actions.h"

// Every entry is first set to asdf_action_nothing (a GCC range designator),
// then the used entries are overridden, so the override warning, and the
// pedantic warning for the range designator, are disabled for the table.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Woverride-init"
#pragma GCC diagnostic ignored "-Wpedantic"
const asdf_action_fn_t FLASH asdf_action_table[ASDF_NUM_ACTION_SLOTS] = {
  [0 ... ASDF_NUM_ACTION_SLOTS - 1] = &asdf_action_nothing,
  ASDF_BUILTIN_ACTIONS,
  [ACTION_APPLESOFT_KEYBOARD_TEST] = &applesoft_keyboard_test,
  [ACTION_ACE1000_KEYBOARD_TEST] = &ace1000_keyboard_test,
};
#pragma GCC diagnostic pop

//-------|---------|---------+---------+---------+---------+---------+---------+
// Above line is 80 columns, and should display completely in the editor.
