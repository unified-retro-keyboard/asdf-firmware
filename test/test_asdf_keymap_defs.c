// -*- mode: C; tab-width: 2 ; indent-tabs-mode: nil -*-
//
// Unified Keyboard Project
// ASDF keyboard firmware
//
// test_asdf_keymap_defs.c
//
// Contains keymap definitions for unit tests
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

#include "test_asdf_keymap_defs.h"
#include "asdf_ascii.h"
#include "asdf_modifiers.h"
#include "asdf_keymaps.h"
#include "asdf_platform.h"
#include "test_asdf_lib.h"
#include "asdf_keymap_setup.h"

static const asdf_keycode_t test_PLAIN_matrix[TEST_NUM_ROWS][TEST_NUM_COLS] = ASDF_TEST_PLAIN_MAP;
static const asdf_keycode_t test_SHIFT_matrix[TEST_NUM_ROWS][TEST_NUM_COLS] = ASDF_TEST_SHIFT_MAP;
static const asdf_keycode_t test_CAPS_matrix[TEST_NUM_ROWS][TEST_NUM_COLS] = ASDF_TEST_CAPS_MAP;
static const asdf_keycode_t test_CTRL_matrix[TEST_NUM_ROWS][TEST_NUM_COLS] = ASDF_TEST_CTRL_MAP;

static const asdf_keycode_t test2_PLAIN_matrix[TEST_NUM_ROWS][TEST_NUM_COLS] = ASDF_TEST2_PLAIN_MAP;
static const asdf_keycode_t test2_SHIFT_matrix[TEST_NUM_ROWS][TEST_NUM_COLS] = ASDF_TEST2_SHIFT_MAP;
static const asdf_keycode_t test2_CAPS_matrix[TEST_NUM_ROWS][TEST_NUM_COLS] = ASDF_TEST2_CAPS_MAP;
static const asdf_keycode_t test2_CTRL_matrix[TEST_NUM_ROWS][TEST_NUM_COLS] = ASDF_TEST2_CTRL_MAP;


// Modifier maps of the "test" and "test2" keymaps, in modifier_index_t order.
#define TEST_MAPS(prefix)                                                                          \
  .maps = { [MOD_PLAIN_MAP] = &prefix##_PLAIN_matrix[0][0],                                        \
            [MOD_SHIFT_MAP] = &prefix##_SHIFT_matrix[0][0],                                        \
            [MOD_CAPS_MAP] = &prefix##_CAPS_matrix[0][0],                                          \
            [MOD_CTRL_MAP] = &prefix##_CTRL_matrix[0][0] },                                        \
  .rows = TEST_NUM_ROWS, .cols = TEST_NUM_COLS

#define NUM_ELEMENTS(array) ((uint8_t) (sizeof(array) / sizeof((array)[0])))

const asdf_keymap_t test_plain_keymap = { TEST_MAPS(test) };
const asdf_keymap_t test_caps_keymap = { TEST_MAPS(test) };
const asdf_keymap_t test2_plain_keymap = { TEST_MAPS(test2) };
const asdf_keymap_t test2_caps_keymap = { TEST_MAPS(test2) };

// Virtual output tests

static const asdf_virtual_initializer_t vdevs_single_outputs[] = {
  { VOUT1, PHYSICAL_OUT1, V_NOFUNC, 0 },      // single assignment
  { VOUT2, PHYSICAL_OUT2, V_TOGGLE, 0 },      // single toggle
  { VOUT3, PHYSICAL_OUT3, V_PULSE_SHORT, 0 }, // single pulse
};

static const asdf_virtual_initializer_t vdevs_double_outputs[] = {
  { VOUT4, PHYSICAL_LED1, V_NOFUNC, 0 }, // first of double assignment attempt
  { VOUT5, PHYSICAL_LED1, V_NOFUNC, 1 }, // second of double assignment attempt
};

static const asdf_virtual_initializer_t vdevs_triple_outputs[] = {
  { VOUT1, PHYSICAL_OUT1, V_TOGGLE, 0 },
  { VOUT1, PHYSICAL_OUT2, V_TOGGLE, 1 },
  { VOUT1, PHYSICAL_OUT3, V_TOGGLE, 0 },
};

static const asdf_virtual_initializer_t vdevs_vcaps_outputs[] = {
  { VCAPS_LED, PHYSICAL_LED1, V_NOFUNC, 0 },
  { VSHIFT_LED, PHYSICAL_LED2, V_NOFUNC, 0 },
  { VOUT2, PHYSICAL_OUT3, V_NOFUNC, 0 },
  { VOUT2, ASDF_PHYSICAL_NUM_RESOURCES, V_NOFUNC, 0 }, // invalid; ignored
};

const asdf_keymap_t test_vdevs_single_keymap = {
  TEST_MAPS(test),
  .num_outputs = NUM_ELEMENTS(vdevs_single_outputs),
  .outputs = vdevs_single_outputs,
};

const asdf_keymap_t test_vdevs_double_keymap = {
  TEST_MAPS(test),
  .num_outputs = NUM_ELEMENTS(vdevs_double_outputs),
  .outputs = vdevs_double_outputs,
};

const asdf_keymap_t test_vdevs_triple_keymap = {
  TEST_MAPS(test),
  .num_outputs = NUM_ELEMENTS(vdevs_triple_outputs),
  .outputs = vdevs_triple_outputs,
};

const asdf_keymap_t test_vdevs_vcaps_keymap = {
  TEST_MAPS(test2),
  .num_outputs = NUM_ELEMENTS(vdevs_vcaps_outputs),
  .outputs = vdevs_vcaps_outputs,
};

// Hook and platform tests

static asdf_cols_t test_platform_read_row(void *user, uint8_t row)
{
  (void) user;
  return test_hook_read_row(row);
}

static void test_platform_send_code(void *user, asdf_keycode_t code)
{
  (void) user;
  test_hook_output(code);
}

const asdf_platform_t test_alt_platform = {
  .user = NULL,
  .read_row = test_platform_read_row,
  .send_code = test_platform_send_code,
};

static const asdf_hook_binding_t each_scan_hooks[] = {
  { ASDF_HOOK_EACH_SCAN, test_hook_each_scan },
};

const asdf_keymap_t test_hooks_default_keymap = { TEST_MAPS(test2) };

const asdf_keymap_t test_hooks_alt_platform_keymap = {
  TEST_MAPS(test2),
  .platform = &test_alt_platform,
};

const asdf_keymap_t test_hooks_each_scan_keymap = {
  TEST_MAPS(test2),
  .num_hooks = NUM_ELEMENTS(each_scan_hooks),
  .hooks = each_scan_hooks,
};


//-------|---------|---------+---------+---------+---------+---------+---------+
// Above line is 80 columns, and should display completely in the editor.
