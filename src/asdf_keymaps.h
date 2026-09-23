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

#if !defined(ASDF_KEYMAPS_H)
#define ASDF_KEYMAPS_H

#include "asdf.h"
#include "asdf_hook.h"
#include "asdf_virtual.h"
#include "asdf_physical.h"
#include "asdf_modifiers.h"
#include "asdf_platform.h"

// Define the bit position of each keymap DIP switch. The DIP switch values at
// each bit position can be used to select the current keymap. This requires the
// DIP switches to be mapped to the asdf_keymaps_select_X_set() and
// asdf_keymaps_select_X_clear() functions in each keymap.
#define ASDF_KEYMAP_BIT_0 1
#define ASDF_KEYMAP_BIT_1 2
#define ASDF_KEYMAP_BIT_2 4
#define ASDF_KEYMAP_BIT_3 8

// define the struct for each keymap matrix in the keymap array. One per
// modifier state. Each keymap can have it's own row and column count.
typedef struct {
  const asdf_keycode_t *matrix;
  uint8_t rows;
  uint8_t cols;
} asdf_keycode_map_t;

// Number of elements in an array, for the descriptor table counts.
#define ASDF_NUM_ELEMENTS(array) ((uint8_t)(sizeof(array) / sizeof((array)[0])))

// Binds a function to a hook for the duration of a keymap.
typedef struct {
  asdf_hook_id_t hook;
  asdf_hook_function_t function;
} asdf_hook_binding_t;

// Flags in asdf_keymap_t.flags
#define ASDF_KEYMAP_CAPS_ON 0x01         // start with CAPSLOCK on
#define ASDF_KEYMAP_NEGATIVE_STROBE 0x02 // start with negative output strobe

// A keymap descriptor. Each keymap is described by one immutable descriptor,
// stored in flash, which the keymap registry (asdf_keymap_setup.h) lists by
// index. Selecting a keymap applies its descriptor, in field order: the
// modifier maps, the message print delay, the hook bindings, the virtual
// output assignments, the flags, and the platform.
typedef struct {
  // keycode matrix for each modifier state (indexed by modifier_index_t), each
  // rows x cols, stored in flash
  const asdf_keycode_t *maps[ASDF_MOD_NUM_MODIFIERS];
  uint8_t rows;
  uint8_t cols;
  uint8_t print_delay_ms; // delay between system message characters
  uint8_t flags;          // ASDF_KEYMAP_* flags
  uint8_t num_hooks;
  uint8_t num_outputs;
  const asdf_hook_binding_t *hooks;           // num_hooks entries, in flash
  const asdf_virtual_initializer_t *outputs;  // num_outputs entries, in flash
  const asdf_platform_t *platform;            // NULL for the architecture's platform
} asdf_keymap_t;

// Keymap state of one keyboard: the matrices in use for each modifier state,
// and the current and requested keymap numbers.
typedef struct {
  asdf_keycode_map_t maps[ASDF_MOD_NUM_MODIFIERS];
  uint8_t current;   // keymap now in use
  uint8_t requested; // keymap requested by the keymap select actions
} asdf_keymap_state_t;

// Instance API on keymap state. See asdf_keymaps.c.

void asdf_keymaps_add_map_r(asdf_keymap_state_t *keymap, const asdf_keycode_t *matrix,
                            modifier_index_t modifier_index, uint8_t num_rows,
                            uint8_t num_cols);
uint8_t asdf_keymaps_num_rows_r(const asdf_keymap_state_t *keymap,
                                modifier_index_t modifier_index);
uint8_t asdf_keymaps_num_cols_r(const asdf_keymap_state_t *keymap,
                                modifier_index_t modifier_index);
asdf_keycode_t asdf_keymaps_get_code_r(const asdf_keymap_state_t *keymap, uint8_t row,
                                       uint8_t col, uint8_t modifier_index);
void asdf_keymaps_request_bit_r(asdf_keymap_state_t *keymap, uint8_t bit, uint8_t set);

// Instance API on a whole keyboard, since selecting a keymap resets and
// configures the keyboard. See asdf_keymaps.c.

void asdf_keymaps_init_r(asdf_t *kb);
void asdf_keymaps_switch_r(asdf_t *kb, uint8_t index);
void asdf_keymaps_select_r(asdf_t *kb, uint8_t index);
void asdf_keymaps_apply_request_r(asdf_t *kb);

// Single-keyboard API, operating on the default keyboard (asdf_compat.c).

// PROCEDURE: asdf_keymaps_add_map
// DESCRIPTION: Sets the keycode matrix used for one modifier state.
void asdf_keymaps_add_map(const asdf_keycode_t *matrix, modifier_index_t modifier_index,
                          uint8_t num_rows, uint8_t num_cols);

// PROCEDURE: asdf_keymaps_apply_request
// DESCRIPTION: Switch to the keymap requested by the keymap select actions, if
// it differs from the current keymap and exists. Called at the end of each scan.
void asdf_keymaps_apply_request(void);

// PROCEDURE: asdf_keymaps_num_rows, asdf_keymaps_num_cols
// OUTPUTS: number of rows (columns) in the keymap for the current modifier state
uint8_t asdf_keymaps_num_rows(void);
uint8_t asdf_keymaps_num_cols(void);

// PROCEDURE: asdf_keymaps_select
// INPUTS: (uint8_t) index - index of the keymap number to select
// DESCRIPTION: Switch to the keymap if it exists and differs from the current
// keymap.
void asdf_keymaps_select(uint8_t index);

// PROCEDURE: asdf_keymaps_map_select_N_set, asdf_keymaps_map_select_N_clear
// DESCRIPTION: Set or clear bit N of the requested keymap number (DIP switch
// actions). The request is applied at the end of the scan.
void asdf_keymaps_map_select_0_clear(void);
void asdf_keymaps_map_select_0_set(void);
void asdf_keymaps_map_select_1_clear(void);
void asdf_keymaps_map_select_1_set(void);
void asdf_keymaps_map_select_2_clear(void);
void asdf_keymaps_map_select_2_set(void);
void asdf_keymaps_map_select_3_clear(void);
void asdf_keymaps_map_select_3_set(void);

// PROCEDURE: asdf_keymaps_init
// DESCRIPTION: Select keymap 0.
void asdf_keymaps_init(void);

// PROCEDURE: asdf_keymaps_get_code
// INPUTS: (uint8_t) row, col - key position; (uint8_t) modifier_index
// OUTPUTS: the keycode at that position for that modifier state, or
// ACTION_NOTHING if any index is out of range.
asdf_keycode_t asdf_keymaps_get_code(uint8_t row, uint8_t col, uint8_t modifier_index);

#endif /* !defined (ASDF_KEYMAPS_H) */

//-------|---------|---------+---------+---------+---------+---------+---------+
// Above line is 80 columns, and should display completely in the editor.
