// -*- mode: C; tab-width: 4 ; indent-tabs-mode: nil -*-
//
// Unfified Keyboard Project
// ASDF keyboard firmware
//
// asdf_keymap_classic_add_map.h
//
// defines keymap matrices for classic layouts
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


// To use this ascii for a new keymap, edit the keymaps definitions as
// desired. The keymaps are organized from row 0, counting upward, and each row
// includes the columns from 0-NUM_COLS.
//

#if !defined(ASDF_KEYMAP_CLASSIC_ADD_MAP_H)
#define ASDF_KEYMAP_CLASSIC_ADD_MAP_H

#include "asdf_arch.h"
#include "asdf_keymap_classic.h"

// Keycode matrices, one per modifier state, used by the classic keymap
// descriptors.
typedef asdf_keycode_t classic_keycode_matrix_t[CLASSIC_NUM_ROWS][CLASSIC_NUM_COLS];

extern const FLASH classic_keycode_matrix_t classic_plain_matrix;
extern const FLASH classic_keycode_matrix_t classic_shift_matrix;
extern const FLASH classic_keycode_matrix_t classic_caps_matrix;
extern const FLASH classic_keycode_matrix_t classic_ctrl_matrix;

#endif /* !defined (ASDF_KEYMAP_CLASSIC_ADD_MAP_H) */

//-------|---------|---------+---------+---------+---------+---------+---------+
// Above line is 80 columns, and should display completely in the editor.
