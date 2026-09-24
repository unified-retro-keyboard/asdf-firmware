// -*- mode: C; tab-width: 4 ; indent-tabs-mode: nil -*-
/**
 * @file asdf.h
 *
 * Basic types and sizes shared by all of the firmware.
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

#if !defined(ASDF_H)
#define ASDF_H

#include <stdio.h>
#include <stdint.h>

/**
 * A value meaning "no code". Every 8-bit value is a valid code, so this lies
 * outside their range.
 */
#define ASDF_INVALID_CODE 0x100

// Maximum key matrix size. ASDF_MAX_COLS must fit in asdf_cols_t.
#define ASDF_MAX_COLS 8
#define ASDF_MAX_ROWS 16

/**
 * The state of one row of the key matrix, one bit per column.
 *
 * 8 columns per row, for efficiency on 8-bit machines. For 16 columns per row,
 * change this to uint16_t and increase ASDF_MAX_COLS to 16.
 */
typedef uint8_t asdf_cols_t;

/**
 * A code sent to the host: any 8-bit value.
 *
 * Key actions are separate from codes (see asdf_actions.h).
 */
typedef uint8_t asdf_keycode_t;

/** One keyboard: all of its changeable state (defined in asdf_keyboard.h). */
typedef struct asdf_keyboard asdf_t;


#endif // !defined (ASDF_H)


//-------|---------|---------+---------+---------+---------+---------+---------+
// Above line is 80 columns, and should display completely in the editor.
