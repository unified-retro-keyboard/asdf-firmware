// -*- mode: C; tab-width: 2 ; indent-tabs-mode: nil -*-
/**
 * @file asdf_keymap_apple2_add_map.c
 *
 * Keyboard test action for the apple2 keymaps. The key matrices are in
 * asdf_keymap_apple2_maps.yaml.
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

#include <stdint.h>
#include "asdf.h"
#include "asdf_arch.h"
#include "asdf_keymap_actions.h"
#include "asdf_print.h"

/**
 * Type an Applesoft BASIC keyboard test program.
 *
 * Queues one numbered program line, ending in CR, on the system message
 * output. When run, the program prints the code of each key pressed, until
 * CTRL-C is pressed. A keymap-provided key action.
 *
 * @param kb     Keyboard to type on.
 * @param param  Ignored.
 */
void applesoft_keyboard_test(asdf_t *kb, uint8_t param)
{
  (void) param;
  asdf_print(kb, "10GETA$(0):A=ASC(A$(0)):A$(1)=\"CTL+\"+CHR$(A + 64):?\"'\";A$(A<32);\"' = \";A:IFA<>3GOTO10\r");
}

//-------|---------|---------+---------+---------+---------+---------+---------+
// Above line is 80 columns, and should display completely in the editor.
