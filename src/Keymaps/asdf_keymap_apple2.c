// -*- mode: C; tab-width: 2 ; indent-tabs-mode: nil -*-
/**
 * @file asdf_keymap_apple2.c
 *
 * The keymap descriptor, outputs, and ID message for the upper/lowercase
 * Apple II keymap. The key matrices are in asdf_keymap_apple2_maps.yaml.
 *
 * Part of the Unified Keyboard Project ASDF keyboard firmware.
 *
 * @copyright Copyright 2019 David Fenyes. MIT License; see LICENSE.
 */
// SPDX-License-Identifier: MIT


#include "asdf_print.h"
#include "asdf_keymaps.h"
#include "asdf_virtual.h"
#include "asdf_modifiers.h"
#include "asdf_keymap_apple2_add_map.h"
#include "asdf_keymap_apple2.h"
#include "asdf_keymap_apple2_maps.h"


// Printed by the KEYMAP_ID key.
static const char FLASH apple2_id_message[] = "[Keymap: Apple 2 (u/l case)]";

static const asdf_virtual_initializer_t FLASH apple2_outputs[] = {
  // Attach the physical POWER LED as the CAPS LED. Assign no triggered
  // function, and initialize to initial state of the CAPS logic. The CAPS LED
  // will be controlled by the state of the CAPSLOCK logic.
  { VCAPS_LED, APPLE_POWER_LED, V_NOFUNC, APPLE_POWER_LED_INIT_VALUE },

  // Assign CAPS LED to off (disabled)
  { APPLE_VIRTUAL_DISABLED_LED, APPLE_DISABLED_LED, V_NOFUNC, APPLE_DISABLED_INIT_VALUE },

  // assign RESET output to the virtual RESET output, configure to produce a
  // short pulse when activated
  { APPLE_VIRTUAL_RESET, APPLE_RESET_OUTPUT, V_PULSE_SHORT,
    ASDF_IDLE_VALUE(APPLE_RESET_ACTIVE_VALUE) },

  // assign the CLRSCR output to the virtual CLRSCR output, configure to produce
  // a long pulse when activated
  { APPLE_VIRTUAL_CLR_SCR, APPLE_CLR_SCR_OUTPUT, V_PULSE_LONG,
    ASDF_IDLE_VALUE(APPLE_CLR_SCR_ACTIVE_VALUE) },
};

// For Apple 2, start with CAPS active.
const asdf_keymap_t FLASH apple2_keymap = {
  .maps = { [MOD_PLAIN_MAP] = &apple_plain_matrix[0][0],
            [MOD_SHIFT_MAP] = &apple_shift_matrix[0][0],
            [MOD_CAPS_MAP] = &apple_caps_matrix[0][0],
            [MOD_CTRL_MAP] = &apple_ctrl_matrix[0][0] },
  .rows = ASDF_APPLE2_NUM_ROWS,
  .cols = ASDF_APPLE2_NUM_COLS,
  .print_delay_ms = APPLE2_PRINT_DELAY,
  .flags = ASDF_KEYMAP_CAPS_ON,
  .id_message = apple2_id_message,
  .num_outputs = ASDF_NUM_ELEMENTS(apple2_outputs),
  .outputs = apple2_outputs,
};


//-------|---------|---------+---------+---------+---------+---------+---------+
// Above line is 80 columns, and should display completely in the editor.

