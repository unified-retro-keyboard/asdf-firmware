// -*- mode: C; tab-width: 2 ; indent-tabs-mode: nil -*-
//
// Unified Keyboard Project
// ASDF keyboard firmware
//
// asdf_keymaps.c
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

#include "asdf_keymaps.h"
#include "asdf.h"
#include "asdf_arch.h"
#include "asdf_config.h"
#include "asdf_keyboard.h"
#include "asdf_keymap_setup.h"
#include "asdf_modifiers.h"
#include "asdf_platform.h"
#include "asdf_repeat.h"
#include "asdf_virtual.h"
#include <stddef.h>
#include <stdint.h>

// The keymap functions operate on the caller's state: the key lookup
// functions on an asdf_keymap_state_t, and the keymap selection functions,
// which reset and configure the whole keyboard, on an asdf_t.

// PROCEDURE: asdf_keymaps_add_map_r
// INPUTS: (asdf_keymap_state_t *) keymap - keymap state
//         (const asdf_key_t *) matrix - key matrix (in flash), or NULL
//         (modifier_index_t) modifier_index - the modifier state it is used for
//         (uint8_t) num_rows, num_cols - dimensions of the matrix
// OUTPUTS: none
//
// DESCRIPTION: Called when a keymap descriptor is applied. Sets the key
// matrix used for one modifier state.
//
// NOTES: If the modifier index, num_rows, or num_cols are not valid then no
// action is performed.
//
// SCOPE: public
//
// COMPLEXITY: 2
//
void asdf_keymaps_add_map_r(asdf_keymap_state_t *keymap, const asdf_key_t *matrix,
                            modifier_index_t modifier_index, uint8_t num_rows,
                            uint8_t num_cols) {
    if ((modifier_index < ASDF_MOD_NUM_MODIFIERS) && (num_rows <= ASDF_MAX_ROWS) &&
        (num_cols <= ASDF_MAX_COLS)) {
        keymap->maps[modifier_index].matrix = matrix;
        keymap->maps[modifier_index].rows = num_rows;
        keymap->maps[modifier_index].cols = num_cols;
    }
}

// PROCEDURE: asdf_keymaps_num_rows_r, asdf_keymaps_num_cols_r
// INPUTS: (const asdf_keymap_state_t *) keymap - keymap state
//         (modifier_index_t) modifier_index - modifier state
// OUTPUTS: number of rows (columns) of the matrix for that modifier state, or 0
//          if the modifier index is invalid
//
// SCOPE: public
//
// COMPLEXITY: 2
//
uint8_t asdf_keymaps_num_rows_r(const asdf_keymap_state_t *keymap,
                                modifier_index_t modifier_index) {
    return (modifier_index < ASDF_MOD_NUM_MODIFIERS) ? keymap->maps[modifier_index].rows : 0;
}

uint8_t asdf_keymaps_num_cols_r(const asdf_keymap_state_t *keymap,
                                modifier_index_t modifier_index) {
    return (modifier_index < ASDF_MOD_NUM_MODIFIERS) ? keymap->maps[modifier_index].cols : 0;
}

// PROCEDURE: asdf_keymaps_get_code_r
// INPUTS: (const asdf_keymap_state_t *) keymap - keymap state
//         (uint8_t) row, col - key position
//         (uint8_t) modifier_index - modifier state
// OUTPUTS: returns the key at that position in the matrix for that modifier
//          state, or a key that does nothing if the modifier index, row, or
//          column is out of range, or no matrix is set.
//
// NOTES: The key is copied out of flash.
//
// SCOPE: public
//
// COMPLEXITY: 3
//
asdf_key_t asdf_keymaps_get_key_r(const asdf_keymap_state_t *keymap, uint8_t row, uint8_t col,
                                  uint8_t modifier_index) {
    asdf_key_t key = KEY_NOTHING(0);

    if (modifier_index < ASDF_MOD_NUM_MODIFIERS) {
        const asdf_keycode_map_t *map = &keymap->maps[modifier_index];
        if (map->matrix && row < map->rows && col < map->cols) {
            FLASH_MEMCPY(&key, &map->matrix[row * map->cols + col], sizeof(key));
        }
    }
    return key;
}

// PROCEDURE: asdf_keymaps_reset_r
// INPUTS: (asdf_t *) kb - keyboard
// OUTPUTS: none
//
// DESCRIPTION: Reset the keyboard's keymap-dependent state:
//              - Clear all keycode mapping matrices.
//              - Clear all virtual devices
//              - Reset modifier and repeat state.
//              - Clear the each-scan action.
//              - Restore the keyboard's base platform.
//
// SIDE EFFECTS: see DESCRIPTION
//
// SCOPE: private
//
// COMPLEXITY: 2
//
static void asdf_keymaps_reset_r(asdf_t *kb) {
    for (uint8_t i = 0; i < ASDF_MOD_NUM_MODIFIERS; i++) {
        asdf_keymaps_add_map_r(&kb->keymap, NULL, (modifier_index_t)i, 0, 0);
    }

    asdf_virtual_init_r(&kb->outputs, kb->base_platform);

    // Reset modifiers and repeat state, so each keymap starts from a known
    // state regardless of the keymap it replaces.
    asdf_modifiers_init_r(&kb->modifiers);
    asdf_repeat_init_r(&kb->repeat);

    kb->keymap.each_scan = ACTION_NOTHING;
    asdf_install_platform_r(kb, NULL);
}

// PROCEDURE: asdf_keymaps_apply_r
// INPUTS: (asdf_t *) kb - keyboard
//         (const asdf_keymap_t *) keymap - keymap descriptor, in flash
// OUTPUTS: none
//
// DESCRIPTION: Configures the keyboard from a keymap descriptor, in the order
// documented for asdf_keymap_t: modifier maps, print delay, each-scan action,
// virtual output assignments, flags, and platform.
//
// SIDE EFFECTS: see DESCRIPTION
//
// NOTES: The descriptor and its tables are copied out of flash one element at
// a time, so only one element is held in RAM.
//
// SCOPE: private
//
// COMPLEXITY: 6
//
static void asdf_keymaps_apply_r(asdf_t *kb, const asdf_keymap_t *keymap) {
    asdf_keymap_t k;
    FLASH_MEMCPY(&k, keymap, sizeof(k));

    for (uint8_t m = 0; m < ASDF_MOD_NUM_MODIFIERS; m++) {
        asdf_keymaps_add_map_r(&kb->keymap, k.maps[m], (modifier_index_t)m, k.rows, k.cols);
    }

    kb->print_delay_ms = k.print_delay_ms;

    kb->keymap.each_scan = k.each_scan;

    for (uint8_t i = 0; i < k.num_outputs; i++) {
        asdf_virtual_initializer_t out;
        FLASH_MEMCPY(&out, &k.outputs[i], sizeof(out));
        asdf_virtual_assign_r(&kb->outputs, out.virtual_device, out.physical_device,
                              out.function, out.initial_value);
    }

    if (k.flags & ASDF_KEYMAP_CAPS_ON) {
        asdf_modifier_capslock_activate_r(&kb->modifiers);
        asdf_sync_lock_leds_r(kb);
    }
    if (k.flags & ASDF_KEYMAP_NEGATIVE_STROBE) {
        asdf_set_strobe_polarity_r(kb, 0);
    }

    if (k.platform) {
        asdf_install_platform_r(kb, k.platform);
    }
}

// PROCEDURE: asdf_keymaps_switch_r
// INPUTS: (asdf_t *) kb - keyboard
//         (uint8_t) index - index of the keymap number to switch to
// OUTPUTS: none
//
// DESCRIPTION: If the keymap exists:
// 1) record it as the current (and requested) keymap
// 2) reset keymaps, virtual devices, modifiers, repeat state, each-scan action, and
// platform, and return the platform's output configuration to its defaults, to
// undo any settings from the previous keymap
// 3) apply the keymap descriptor.
// 4) Re-apply the configuration actions of held switches (DIP switch keymap
// select, strobe polarity, autorepeat select). Other held keys, such as a held
// SHIFT, are not re-activated.
// 5) Apply any initial virtual outputs to the hardware.
//
// SIDE EFFECTS: See DESCRIPTION
//
// NOTES: If the requested index is not valid, then no action is performed.
//
// SCOPE: public
//
// COMPLEXITY: 2
//
void asdf_keymaps_switch_r(asdf_t *kb, uint8_t index) {
    if (asdf_keymap_valid(index)) {
        kb->keymap.current = index;
        kb->keymap.requested = index;

        asdf_keymaps_reset_r(kb);
        kb->platform->reset(kb->platform->user);

        asdf_keymaps_apply_r(kb, asdf_keymap_descriptor(index));

        asdf_apply_configuration_r(kb);
        asdf_virtual_sync_r(&kb->outputs);
    }
}

// PROCEDURE: asdf_keymaps_select_r
// INPUTS: (asdf_t *) kb - keyboard
//         (uint8_t) index - index of the keymap number to select
// OUTPUTS: none
//
// DESCRIPTION: Switch to the keymap if it differs from the current keymap.
//
// SCOPE: public
//
// COMPLEXITY: 2
//
void asdf_keymaps_select_r(asdf_t *kb, uint8_t index) {
    if (index != kb->keymap.current) {
        asdf_keymaps_switch_r(kb, index);
    }
}

// PROCEDURE: asdf_keymaps_init_r
// INPUTS: (asdf_t *) kb - keyboard
// OUTPUTS: none
//
// DESCRIPTION: Select keymap 0.
//
// SCOPE: public
//
// COMPLEXITY: 1
//
void asdf_keymaps_init_r(asdf_t *kb) { asdf_keymaps_switch_r(kb, 0); }

// PROCEDURE: asdf_keymaps_request_bit_r
// INPUTS: (asdf_keymap_state_t *) keymap - keymap state
//         (uint8_t) bit - ASDF_KEYMAP_BIT_n mask of the keymap select bit
//         (uint8_t) set - TRUE (nonzero) to set the bit, FALSE (0) to clear it
// OUTPUTS: none
//
// DESCRIPTION: Called by the keymap select actions (DIP switches). Sets or
// clears one bit of the requested keymap number. The request is applied at the
// end of the scan by asdf_keymaps_apply_request_r().
//
// SCOPE: public
//
// COMPLEXITY: 2
//
void asdf_keymaps_request_bit_r(asdf_keymap_state_t *keymap, uint8_t bit, uint8_t set) {
    if (set) {
        keymap->requested |= bit;
    } else {
        keymap->requested &= (uint8_t)~bit;
    }
}

// PROCEDURE: asdf_keymaps_apply_request_r
// INPUTS: (asdf_t *) kb - keyboard
// OUTPUTS: none
//
// DESCRIPTION: If the keymap select actions have requested a different keymap,
// switch to it. Called at the end of each scan, so that a keymap never changes
// part-way through a scan, and DIP switch bits that change in the same scan
// select the final keymap directly rather than passing through intermediate
// keymaps. A request for a keymap that does not exist is ignored.
//
// SIDE EFFECTS: see DESCRIPTION
//
// SCOPE: public
//
// COMPLEXITY: 1
//
void asdf_keymaps_apply_request_r(asdf_t *kb) {
    asdf_keymaps_select_r(kb, kb->keymap.requested);
}


//-------|---------|---------+---------+---------+---------+---------+---------+
// Above line is 80 columns, and should display completely in the editor.
