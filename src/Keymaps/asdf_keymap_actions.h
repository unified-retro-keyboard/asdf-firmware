// -*- mode: C; tab-width: 2 ; indent-tabs-mode: nil -*-
/**
 * @file asdf_keymap_actions.h
 *
 * Keymap-provided key actions: their action numbers, from ASDF_KEYMAP_ACTIONS
 * on, and their functions. The action table (asdf_keymap_actions.c) maps each
 * number to its function. Keys name these actions in the YAML key matrices,
 * by their KEY_ macros below, for example KEY_APPLESOFT_KEYBOARD_TEST.
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

#if !defined(ASDF_KEYMAP_ACTIONS_H)
#define ASDF_KEYMAP_ACTIONS_H

#include <stdint.h>
#include "asdf.h"
#include "asdf_actions.h"

#define ACTION_APPLESOFT_KEYBOARD_TEST (ASDF_KEYMAP_ACTIONS + 0)
#define ACTION_ACE1000_KEYBOARD_TEST (ASDF_KEYMAP_ACTIONS + 1)

// Keys for the keymap-provided actions, run on press.
#define KEY_APPLESOFT_KEYBOARD_TEST(unused)                                                        \
  ASDF_KEY(ACTION_APPLESOFT_KEYBOARD_TEST, 0, ACTION_NOTHING, 0)
#define KEY_ACE1000_KEYBOARD_TEST(unused)                                                          \
  ASDF_KEY(ACTION_ACE1000_KEYBOARD_TEST, 0, ACTION_NOTHING, 0)

/**
 * Type an Applesoft BASIC keyboard test program.
 *
 * Queues one numbered program line, ending in CR, on the system message
 * output. When run, the program prints the code of each key pressed, until
 * CTRL-C is pressed. Bound to a key in the apple2 and classic keymaps.
 *
 * @param kb     Keyboard to type on.
 * @param param  Ignored.
 */
void applesoft_keyboard_test(asdf_t *kb, uint8_t param);

/**
 * Type the keyboard test program for the Franklin ACE 1000.
 *
 * Queues the same program line as applesoft_keyboard_test(). No key is bound
 * to it.
 *
 * @param kb     Keyboard to type on.
 * @param param  Ignored.
 */
void ace1000_keyboard_test(asdf_t *kb, uint8_t param);

#endif /* !defined(ASDF_KEYMAP_ACTIONS_H) */

//-------|---------|---------+---------+---------+---------+---------+---------+
// Above line is 80 columns, and should display completely in the editor.
