// -*- mode: C; tab-width: 2 ; indent-tabs-mode: nil -*-
//
// Unified Keyboard Project
// ASDF keyboard firmware
//
// asdf_keymap_apple2_caps.c
//
// set up keymaps for ALL CAPS Apple II keyboards
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


#include "asdf_print.h"
#include "asdf_keymaps.h"
#include "asdf_virtual.h"
#include "asdf_modifiers.h"
#include "asdf_keymap_apple2_add_map.h"
#include "asdf_keymap_apple2_caps.h"

void apple2_caps_id_message(void) {
  asdf_print("[Keymap: Apple 2 CAPS]");
}

static const asdf_hook_binding_t FLASH apple2_caps_hooks[] = {
  { APPLESOFT_KEYBOARD_TEST, applesoft_keyboard_test },
  { APPLE2_CAPS_ID_MESSAGE, apple2_caps_id_message },
};

static const asdf_virtual_initializer_t FLASH apple2_caps_outputs[] = {
  // Turn the POWER LED on and don't assign to any function
  { APPLE_VIRTUAL_POWER_LED, APPLE_POWER_LED, V_NOFUNC, APPLE_POWER_LED_INIT_VALUE },

  // Assign CAPS LED to off (disabled)
  { APPLE_VIRTUAL_DISABLED_LED, APPLE_DISABLED_LED, V_NOFUNC, APPLE_DISABLED_INIT_VALUE },

  // assign RESET output to the virtual RESET output, configure to produce a
  // short pulse when activated
  { APPLE_VIRTUAL_RESET, APPLE_RESET_OUTPUT, V_PULSE_SHORT, !APPLE_RESET_ACTIVE_VALUE },

  // assign the CLRSCR output to the virtual CLRSCR output, configure to produce
  // a long pulse when activated
  { APPLE_VIRTUAL_CLR_SCR, APPLE_CLR_SCR_OUTPUT, V_PULSE_LONG, !APPLE_CLR_SCR_ACTIVE_VALUE },
};

// The CAPS-only layout: "plain" mode is the same as "caps" mode.
const asdf_keymap_t FLASH apple2_caps_keymap = {
  .maps = { [MOD_PLAIN_MAP] = &apple_caps_matrix[0][0],
            [MOD_SHIFT_MAP] = &apple_caps_shift_matrix[0][0],
            [MOD_CAPS_MAP] = &apple_caps_matrix[0][0],
            [MOD_CTRL_MAP] = &apple_ctrl_matrix[0][0] },
  .rows = ASDF_APPLE2_NUM_ROWS,
  .cols = ASDF_APPLE2_NUM_COLS,
  .print_delay_ms = APPLE2_PRINT_DELAY,
  .num_hooks = ASDF_NUM_ELEMENTS(apple2_caps_hooks),
  .hooks = apple2_caps_hooks,
  .num_outputs = ASDF_NUM_ELEMENTS(apple2_caps_outputs),
  .outputs = apple2_caps_outputs,
};


//-------|---------|---------+---------+---------+---------+---------+---------+
// Above line is 80 columns, and should display completely in the editor.
