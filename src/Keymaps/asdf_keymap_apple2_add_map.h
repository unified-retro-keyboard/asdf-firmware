// -*- mode: C; tab-width: 4 ; indent-tabs-mode: nil -*-
/**
 * @file asdf_keymap_apple2_add_map.h
 *
 * Matrix size, outputs, and arrow key codes shared by the Apple II keymaps.
 * The key matrices are in asdf_keymap_apple2_maps.yaml.
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


#if !defined(ASDF_KEYMAP_APPLE_ADD_MAP_H)
#define ASDF_KEYMAP_APPLE_ADD_MAP_H

#include "asdf_arch.h"
#include "asdf_modifiers.h"

#define ASDF_APPLE2_NUM_ROWS 9 // DIP switches are row 8 (zero-based)
#define ASDF_APPLE2_NUM_COLS 8

#define APPLE_VIRTUAL_RESET VOUT1
#define APPLE_RESET_OUTPUT PHYSICAL_OUT3_OPEN_HI
#define APPLE_RESET_ACTIVE_VALUE 0

#define APPLE_VIRTUAL_CLR_SCR VOUT2
#define APPLE_CLR_SCR_OUTPUT PHYSICAL_OUT1_OPEN_LO
#define APPLE_CLR_SCR_ACTIVE_VALUE 1

#define APPLE_VIRTUAL_POWER_LED VLED1
#define APPLE_POWER_LED PHYSICAL_LED1
#define APPLE_POWER_LED_INIT_VALUE 0 // Toggles when caps lock is activated

#define APPLE_VIRTUAL_DISABLED_LED VLED2
#define APPLE_DISABLED_LED PHYSICAL_LED3
#define APPLE_DISABLED_INIT_VALUE 0


#define APPLE_LEFT_ARROW ASCII_CTRL_H
#define APPLE_RIGHT_ARROW ASCII_CTRL_U

#endif /* !defined (ASDF_KEYMAP_APPLE_ADD_MAP_H) */

//-------|---------|---------+---------+---------+---------+---------+---------+
// Above line is 80 columns, and should display completely in the editor.
