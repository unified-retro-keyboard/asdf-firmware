// -*- mode: C; tab-width: 2 ; indent-tabs-mode: nil -*- 
//
// Unified Keyboard Project
// ASDF keyboard firmware
//
// asdf_keymap_table.c
//
// initialize keymap setup function table
//
// *** This is a special versiof for testing.
// *** Not auto-generated.
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

#include <stdint.h>
#include <stddef.h>
#include "asdf_actions.h"
#include "asdf_keymap_setup.h"
#include "test_asdf_keymap_defs.h"
#include "test_asdf_lib.h"

// Test action table: the built-in actions and the test actions. Every entry is
// first set to asdf_action_nothing (a GCC range designator), then the used
// entries are overridden, so the override and pedantic warnings are disabled
// for the table.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Woverride-init"
#pragma GCC diagnostic ignored "-Wpedantic"
const asdf_action_fn_t FLASH asdf_action_table[ASDF_NUM_ACTION_SLOTS] = {
  [0 ... ASDF_NUM_ACTION_SLOTS - 1] = asdf_action_nothing,
  ASDF_BUILTIN_ACTIONS,
  [ACTION_TEST_HERE_IS] = test_action_here_is,
  [ACTION_TEST_EACH_SCAN] = test_action_each_scan,
};
#pragma GCC diagnostic pop

// Test keymap registry, indexed by keymap number.
static const asdf_keymap_t *const keymap_table[] = {
  [ASDF_TEST_PLAIN_MAP_INDEX] = &test_plain_keymap,
  [ASDF_TEST_CAPS_MAP_INDEX] = &test_caps_keymap,
  [ASDF_TEST2_PLAIN_MAP_INDEX] = &test2_plain_keymap,
  [ASDF_TEST2_CAPS_MAP_INDEX] = &test2_caps_keymap,

  // keymaps for the virtual device tests
  [SINGLE_TESTS_KEYMAP] = &test_vdevs_single_keymap,
  [DOUBLE_ASSIGN_TEST_KEYMAP] = &test_vdevs_double_keymap,
  [TRIPLE_TESTS_KEYMAP] = &test_vdevs_triple_keymap,
  [VCAPS_TEST_KEYMAP] = &test_vdevs_vcaps_keymap,

  // keymaps for the platform and each-scan action tests
  [ASDF_TEST_DEFAULT_SCANNER_MAP] = &test_hooks_default_keymap,
  [ASDF_TEST_ALTERNATE_OUTPUT_MAP] = &test_hooks_alt_platform_keymap,
  [ASDF_TEST_EACH_SCAN_MAP] = &test_hooks_each_scan_keymap,
};

#define NUM_KEYMAP_SLOTS (sizeof(keymap_table) / sizeof(keymap_table[0]))

const asdf_keymap_t *asdf_keymap_descriptor(uint8_t index)
{
  return (index < NUM_KEYMAP_SLOTS) ? keymap_table[index] : NULL;
}

uint8_t asdf_keymap_valid(uint8_t index)
{
  return NULL != asdf_keymap_descriptor(index);
}

//-------|---------|---------+---------+---------+---------+---------+---------+
// Above line is 80 columns, and should display completely in the editor.

