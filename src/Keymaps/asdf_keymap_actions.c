// -*- mode: C; tab-width: 2 ; indent-tabs-mode: nil -*-
/**
 * @file asdf_keymap_actions.c
 *
 * The action table for the production keymaps: the built-in actions and the
 * keymap-provided actions (asdf_keymap_actions.h).
 *
 * Part of the Unified Keyboard Project ASDF keyboard firmware.
 *
 * @copyright Copyright 2019 David Fenyes. MIT License; see LICENSE.
 */
// SPDX-License-Identifier: MIT

#include "asdf_arch.h"
#include "asdf_actions.h"
#include "asdf_keymap_actions.h"

// Every entry is first set to asdf_action_nothing (a GCC range designator),
// then the used entries are overridden, so the override warning, and the
// pedantic warning for the range designator, are disabled for the table.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Woverride-init"
#pragma GCC diagnostic ignored "-Wpedantic"
const asdf_action_fn_t FLASH asdf_action_table[ASDF_NUM_ACTION_SLOTS] = {
  [0 ... ASDF_NUM_ACTION_SLOTS - 1] = &asdf_action_nothing,
  ASDF_BUILTIN_ACTIONS,
  [ACTION_APPLESOFT_KEYBOARD_TEST] = &applesoft_keyboard_test,
  [ACTION_ACE1000_KEYBOARD_TEST] = &ace1000_keyboard_test,
};
#pragma GCC diagnostic pop

//-------|---------|---------+---------+---------+---------+---------+---------+
// Above line is 80 columns, and should display completely in the editor.
