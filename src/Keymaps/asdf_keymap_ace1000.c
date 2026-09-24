// -*- mode: C; tab-width: 2 ; indent-tabs-mode: nil -*-
//
// Unified Keyboard Project
// ASDF keyboard firmware
//
// asdf_keymap_ace1000.c
//
// The keymap descriptor, outputs, and ID message for the Franklin ACE 1000
// replacement keyboard. The key matrices are in asdf_keymap_ace1000_maps.yaml.
// https://github.com/ryucats/Franklin-ACE-1000-Keyboard
//
// Copyright 2023 Chris RYU
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

#include "asdf.h"
#include "asdf_arch.h"
#include "asdf_keymaps.h"
#include "asdf_virtual.h"
#include "asdf_modifiers.h"
#include "asdf_print.h"
#include "asdf_keymap_ace1000.h"
#include "asdf_keymap_ace1000_add_map.h"
#include "asdf_keymap_apple2_add_map.h"
#include "asdf_keymap_ace1000_maps.h"

// Printed by the KEYMAP_ID key.
static const char FLASH ace1000_id_message[] = "[Keymap: ace1000]\n";

static const asdf_virtual_initializer_t FLASH ace1000_outputs[] = {
  // Assign power LED to virtual power LED, and initialize to ON
  { ACE1000_VIRTUAL_POWER_LED, ACE1000_POWER_LED, V_NOFUNC, ACE1000_POWER_LED_INIT_VALUE },

  // Assign CAPS LED to virtual CAPS LED, and initialize to the INIT value, to
  // match the initial CAPSLOCK state. The capslock state code will alter the
  // virtual LED according to the state.
  { VCAPS_LED, ACE1000_CAPS_LED, V_NOFUNC, ACE1000_CAPS_LED_INIT_VALUE },

  // assign RESET output to the virtual RESET output, configure to produce a
  // short pulse when activated
  { ACE1000_VIRTUAL_RESET, ACE1000_RESET_OUTPUT, V_PULSE_SHORT, !ACE1000_RESET_ACTIVE_VALUE },

  // assign the CLRSCR output to the virtual CLRSCR output, configure to produce
  // a long pulse when activated
  { ACE1000_VIRTUAL_CLR_SCR, ACE1000_CLR_SCR_OUT, V_PULSE_LONG, !ACE1000_CLR_SCR_ACTIVE_VALUE },
};

// The ACE 1000 starts with caps lock on.
const asdf_keymap_t FLASH ace1000_keymap = {
  .maps = { [MOD_PLAIN_MAP] = &ace1000_plain_matrix[0][0],
            [MOD_SHIFT_MAP] = &ace1000_shift_matrix[0][0],
            [MOD_CAPS_MAP] = &ace1000_caps_matrix[0][0],
            [MOD_CTRL_MAP] = &ace1000_ctrl_matrix[0][0] },
  .rows = ACE1000_NUM_ROWS,
  .cols = ACE1000_NUM_COLS,
  .print_delay_ms = ASDF_ACE1000_PRINT_SPEED,
  .flags = ASDF_KEYMAP_CAPS_ON,
  .id_message = ace1000_id_message,
  .num_outputs = ASDF_NUM_ELEMENTS(ace1000_outputs),
  .outputs = ace1000_outputs,
};


//-------|---------|---------+---------+---------+---------+---------+---------+
// Above line is 80 columns, and should display completely in the editor.
