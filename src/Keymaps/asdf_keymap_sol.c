// -*- mode: C; tab-width: 2 ; indent-tabs-mode: nil -*-
//
// Unified Keyboard Project
// ASDF keyboard firmware
//
// asdf_keymap_sol.c
//
// The keymap descriptor, outputs, and ID message for the Sol-20 keymap. The
// key matrices are in asdf_keymap_sol_maps.yaml.
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

#include "asdf_arch.h"
#include "asdf_keymap_sol.h"
#include "asdf_keymap_sol_maps.h"
#include "asdf.h"
#include "asdf_ascii.h"
#include "asdf_modifiers.h"
#include "asdf_keymaps.h"
#include "asdf_print.h"


#define SOL_PRINT_DELAY 40 // msec

// Printed by the KEYMAP_ID key.
static const char FLASH sol_id_message[] = "[Keybd: Sol-20]";

static const asdf_virtual_initializer_t FLASH sol_outputs[] = {
  // Set up the ALL CAPS LED, default = off
  { VCAPS_LED, SOL_KBD_LED_UPPERCASE, V_NOFUNC, SOL_KBD_LED_OFF },

  // Set up the SHIFT LED, default = off
  { VSHIFT_LED, SOL_KBD_LED_SHIFTLOCK, V_NOFUNC, SOL_KBD_LED_OFF },

  // Set up the LOCAL LED and output, default LED=OFF, TTL output HIGH. Both LED
  // and TTL out are bound to the same virtual device.
  { SOL_KBD_VLOCAL, SOL_KBD_TTLOUT_LOCAL, V_TOGGLE, SOL_KBD_TTL_HIGH },
  { SOL_KBD_VLOCAL, SOL_KBD_LED_LOCAL, V_TOGGLE, SOL_KBD_LED_OFF },

  // Set up the RESET output, produce a short pulse when activated. Default
  // output HIGH
  { SOL_KBD_VRESET, SOL_KBD_TTLOUT_RESET, V_PULSE_SHORT, SOL_KBD_TTL_HIGH },

  // Set up the BREAK output, produce a long pulse when activated, default
  // output high
  { SOL_KBD_VBREAK, SOL_KBD_TTLOUT_BREAK, V_PULSE_LONG, SOL_KBD_TTL_HIGH },
};

// Start in ALL CAPS mode, to emulate the original keyboard, with a negative
// strobe.
const asdf_keymap_t FLASH sol_keymap = {
  .maps = { [MOD_PLAIN_MAP] = &sol_plain_map[0][0],
            [MOD_SHIFT_MAP] = &sol_shift_map[0][0],
            [MOD_CAPS_MAP] = &sol_caps_map[0][0],
            [MOD_CTRL_MAP] = &sol_ctrl_map[0][0] },
  .rows = ASDF_SOL_NUM_ROWS,
  .cols = ASDF_SOL_NUM_COLS,
  .print_delay_ms = SOL_PRINT_DELAY,
  .flags = ASDF_KEYMAP_CAPS_ON | ASDF_KEYMAP_NEGATIVE_STROBE,
  .id_message = sol_id_message,
  .num_outputs = ASDF_NUM_ELEMENTS(sol_outputs),
  .outputs = sol_outputs,
};


//-------|---------|---------+---------+---------+---------+---------+---------+
// Above line is 80 columns, and should display completely in the editor.
