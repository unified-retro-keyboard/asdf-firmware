// -*- mode: C; tab-width: 4 ; indent-tabs-mode: nil -*-
/**
 * @file asdf_keymap_classic.h
 *
 * Matrix size, outputs, and print delay for the "classic" keymaps.
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


#if !defined(ASDF_KEYMAP_DEFS_CLASSIC_H)
#define ASDF_KEYMAP_DEFS_CLASSIC_H

#include "asdf_arch.h"
#include "asdf_keymaps.h"


// The size of this keymap's matrices, at most ASDF_MAX_ROWS by ASDF_MAX_COLS.
// Keys not given in the YAML matrices do nothing.

#define CLASSIC_NUM_ROWS 9 // DIP switches are row 8 (zero based)
#define CLASSIC_NUM_COLS 8


#define CLASSIC_VIRTUAL_RESET VOUT1
#define CLASSIC_RESET_OUTPUT PHYSICAL_OUT3_OPEN_HI
#define CLASSIC_RESET_ACTIVE_VALUE 0u

#define CLASSIC_VIRTUAL_CLR_SCR VOUT2
#define CLASSIC_CLR_SCR_OUT PHYSICAL_OUT1_OPEN_LO
#define CLASSIC_CLR_SCR_ACTIVE_VALUE 1u

#define CLASSIC_VIRTUAL_POWER_LED VLED1
#define CLASSIC_POWER_LED PHYSICAL_LED1
#define CLASSIC_POWER_LED_INIT_VALUE 1u

#define CLASSIC_CAPS_LED PHYSICAL_LED3
#define CLASSIC_CAPS_LED_INIT_VALUE 0u


#define ASDF_CLASSIC_PRINT_SPEED 40

/** The "classic" ADM 3A style keymap's descriptor, in flash. */
extern const asdf_keymap_t FLASH classic_keymap;

#endif /* !defined (ASDF_KEYMAP_DEFS_CLASSIC_H) */

//-------|---------|---------+---------+---------+---------+---------+---------+
// Above line is 80 columns, and should display completely in the editor.
