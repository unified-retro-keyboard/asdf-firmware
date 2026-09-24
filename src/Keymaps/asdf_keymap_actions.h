// -*- mode: C; tab-width: 2 ; indent-tabs-mode: nil -*-
//
// Unified Keyboard Project
// ASDF keyboard firmware
//
// asdf_keymap_actions.h
//
// Keymap-provided key actions: their action numbers, from ASDF_KEYMAP_ACTIONS
// on, and their functions. The action table (asdf_keymap_actions.c) maps each
// number to its function. Keys name these actions in the YAML key matrices,
// by their KEY_ macros below, for example KEY_APPLESOFT_KEYBOARD_TEST.
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

// Types an Applesoft BASIC program that prints the code of each key pressed
// (asdf_keymap_apple2_add_map.c).
void applesoft_keyboard_test(asdf_t *kb, uint8_t param);

// The same keyboard test program, for the ACE 1000
// (asdf_keymap_ace1000_add_map.c). No key is bound to it.
void ace1000_keyboard_test(asdf_t *kb, uint8_t param);

#endif /* !defined(ASDF_KEYMAP_ACTIONS_H) */

//-------|---------|---------+---------+---------+---------+---------+---------+
// Above line is 80 columns, and should display completely in the editor.
