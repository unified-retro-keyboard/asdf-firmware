// -*- mode: C; tab-width: 4 ; indent-tabs-mode: nil -*-
//
//  Unfified Keyboard Project
//  ASDF keyboard firmware
//
//  asdf_keymaps.h
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

// While there is nothing preventing a standard keyboard from having both a
// "Shift Lock" key and a "Caps Lock" key, usually only one will be present. For
// testing, both must be present to test their functionality.

#if !defined(TEST_ASDF_KEYMAP_DEFS_H)
#define TEST_ASDF_KEYMAP_DEFS_H

// The test key matrices are generated from test_keymaps.yaml into
// test_keymaps.h.

#define TEST_NUM_ROWS 9
#define TEST_NUM_COLS 8

// Marker codes, sent by row 0 column 0 of each test matrix, identifying the
// matrix.
#define PLAIN_MATRIX_1 0xE1
#define CAPS_MATRIX_1 0xE2
#define SHIFT_MATRIX_1 0xE3
#define CTRL_MATRIX_1 0xE4
#define PLAIN_MATRIX_2 0xE5
#define CAPS_MATRIX_2 0xE6
#define SHIFT_MATRIX_2 0xE7
#define CTRL_MATRIX_2 0xE8

// Test actions, in the test action table (asdf_keymap_table.c).
#define ACTION_TEST_HERE_IS (ASDF_KEYMAP_ACTIONS + 0)   // counts calls (test_asdf_lib.c)
#define ACTION_TEST_EACH_SCAN (ASDF_KEYMAP_ACTIONS + 1) // counts scans (test_asdf_lib.c)
#define KEY_TEST_HERE_IS(unused) ASDF_KEY(ACTION_TEST_HERE_IS, 0, ACTION_NOTHING, 0)

#define ASDF_TEST_PLAIN_MAP_INDEX 0
#define ASDF_TEST_CAPS_MAP_INDEX 1
#define ASDF_TEST2_PLAIN_MAP_INDEX 2
#define ASDF_TEST2_CAPS_MAP_INDEX 3


// keymap assignments for the virtual device tests
#define SINGLE_TESTS_KEYMAP 4
#define DOUBLE_ASSIGN_TEST_KEYMAP 5
#define TRIPLE_TESTS_KEYMAP 6
#define VCAPS_TEST_KEYMAP 7
#define VSHIFT_TEST_KEYMAP VCAPS_TEST_KEYMAP

// keymap assignments for the platform and each-scan action tests
#define ASDF_TEST_DEFAULT_SCANNER_MAP 8
#define ASDF_TEST_ALTERNATE_SCANNER_MAP 9
#define ASDF_TEST_ALTERNATE_OUTPUT_MAP 9
#define ASDF_TEST_EACH_SCAN_MAP 10

// number of test keymap slots (0 through ASDF_TEST_EACH_SCAN_MAP)
#define ASDF_NUM_KEYMAPS 11

#include "asdf_keymaps.h"
#include "asdf_platform.h"

// Test keymap descriptors, registered in asdf_keymap_table.c.
extern const asdf_keymap_t test_plain_keymap;
extern const asdf_keymap_t test_caps_keymap;
extern const asdf_keymap_t test2_plain_keymap;
extern const asdf_keymap_t test2_caps_keymap;
extern const asdf_keymap_t test_vdevs_single_keymap;
extern const asdf_keymap_t test_vdevs_double_keymap;
extern const asdf_keymap_t test_vdevs_triple_keymap;
extern const asdf_keymap_t test_vdevs_vcaps_keymap;
extern const asdf_keymap_t test_hooks_default_keymap;
extern const asdf_keymap_t test_hooks_alt_platform_keymap;
extern const asdf_keymap_t test_hooks_each_scan_keymap;

// Platform of test_hooks_alt_platform_keymap, reading rows with
// test_hook_read_row() and sending codes with test_hook_output().
extern const asdf_platform_t test_alt_platform;
void setup_test_hooks_each_scan(void);




#endif /* !defined (TEST_ASDF_KEYMAP_DEFS_H) */
//-------|---------|---------+---------+---------+---------+---------+---------+
// Above line is 80 columns, and should display completely in the editor.
