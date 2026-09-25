// -*- mode: C; tab-width: 4 ; indent-tabs-mode: nil -*-
/**
 * @file asdf_keymap_apple2_caps.h
 *
 * Print delay for the ALL CAPS Apple II keymap.
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


// The Apple 2 ASCII keyboard keymaps come in two variants:
//
// 1) An Upper/Lowercase variant. This variant moves the @ and ^ symbols from
//    the P and N keys to the REPEAT key, since the P and N keys need to reserve
//    the shifted value for the uppercase characters. Also, the "Power" key
//    doubles as a caps-lock key.
//
// 2) A CAPS only keyboard, following the standard Apple II conventions and
//    layout, with ^ above the N key and @ above the P key, and a functioning
//    REPEAT key.
//
// For both variants, CTRL+RESET is required for a system reset.

#if !defined(ASDF_KEYMAP_DEFS_APPLE2_CAPS_H)
#define ASDF_KEYMAP_DEFS_APPLE2_CAPS_H

#include "asdf_arch.h"
#include "asdf_keymaps.h"

/** The Apple 2 CAPS-only keymap's descriptor, in flash. */
extern const asdf_keymap_t FLASH apple2_caps_keymap;

#endif /* !defined (ASDF_KEYMAP_DEFS_APPLE2_CAPS_H) */

//-------|---------|---------+---------+---------+---------+---------+---------+
// Above line is 80 columns, and should display completely in the editor.
