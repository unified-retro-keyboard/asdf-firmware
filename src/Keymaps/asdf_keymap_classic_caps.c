// -*- mode: C; tab-width: 2 ; indent-tabs-mode: nil -*-
/**
 * @file asdf_keymap_classic_caps.c
 *
 * The keymap descriptor, outputs, and ID message for the ALL CAPS "classic"
 * ADM 3A style keymap. The key matrices are in asdf_keymap_classic_maps.yaml.
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

#include "asdf.h"
#include "asdf_arch.h"
#include "asdf_keymaps.h"
#include "asdf_print.h"
#include "asdf_virtual.h"
#include "asdf_modifiers.h"
#include "asdf_keymap_classic.h"
#include "asdf_keymap_classic_caps.h"
#include "asdf_keymap_classic_maps.h"


// Printed by the KEYMAP_ID key.
static const char FLASH classic_caps_id_message[] = "[Keymap: classic]\n";

static const asdf_virtual_initializer_t FLASH classic_caps_outputs[] = {
  // Assign power LED to virtual power LED, and initialize to ON
  { CLASSIC_VIRTUAL_POWER_LED, CLASSIC_POWER_LED, V_NOFUNC, CLASSIC_POWER_LED_INIT_VALUE },

  // Because the virtual power LED never changes, also assign the CAPSLOCK
  // physical LED to the virtual Power LED, and initialize to OFF (or can change
  // to ON depending on preference)
  { CLASSIC_VIRTUAL_POWER_LED, CLASSIC_CAPS_LED, V_NOFUNC, CLASSIC_CAPS_LED_INIT_VALUE },

  // assign RESET output to the virtual RESET output, configure to produce a
  // short pulse when activated
  { CLASSIC_VIRTUAL_RESET, CLASSIC_RESET_OUTPUT, V_PULSE_SHORT,
    ASDF_IDLE_VALUE(CLASSIC_RESET_ACTIVE_VALUE) },

  // assign the CLRSCR output to the virtual CLRSCR output, configure to produce
  // a long pulse when activated
  { CLASSIC_VIRTUAL_CLR_SCR, CLASSIC_CLR_SCR_OUT, V_PULSE_LONG,
    ASDF_IDLE_VALUE(CLASSIC_CLR_SCR_ACTIVE_VALUE) },
};

// For the ALL CAPS keymap, the "plain" mode is the same as "all caps" mode.
const asdf_keymap_t FLASH classic_caps_keymap = {
  .maps = { [MOD_PLAIN_MAP] = &classic_caps_matrix[0][0],
            [MOD_SHIFT_MAP] = &classic_shift_matrix[0][0],
            [MOD_CAPS_MAP] = &classic_caps_matrix[0][0],
            [MOD_CTRL_MAP] = &classic_ctrl_matrix[0][0] },
  .rows = CLASSIC_NUM_ROWS,
  .cols = CLASSIC_NUM_COLS,
  .print_delay_ms = ASDF_CLASSIC_PRINT_SPEED,
  .id_message = classic_caps_id_message,
  .num_outputs = ASDF_NUM_ELEMENTS(classic_caps_outputs),
  .outputs = classic_caps_outputs,
};


//-------|---------|---------+---------+---------+---------+---------+---------+
// Above line is 80 columns, and should display completely in the editor.
