// -*- mode: C; tab-width: 2 ; indent-tabs-mode: nil -*-
//
// Unified Keyboard Project
// ASDF keyboard firmware
//
// asdf_keymaps.c
//
// Key lookup in the current keymap, and keymap selection. See asdf_keymaps.h.
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

// The key lookup functions operate on the caller's asdf_keymap_state_t, and
// the keymap selection functions, which reset and configure the whole
// keyboard, on the caller's asdf_t. The public contracts are in
// asdf_keymaps.h.

/**
 * Set the key matrix used for one modifier state.
 *
 * Called when a keymap descriptor is applied. Stores the matrix and its
 * dimensions in the keymap state; on failure the state is unchanged.
 *
 * @param keymap          Keymap state to modify.
 * @param matrix          Key matrix, rows x cols in flash, or NULL for none.
 * @param modifier_index  Modifier state the matrix is used for.
 * @param num_rows        Rows in @p matrix, at most ASDF_MAX_ROWS.
 * @param num_cols        Columns in @p matrix, at most ASDF_MAX_COLS.
 * @return 1 if the matrix was set; 0 if @p modifier_index, @p num_rows, or
 *         @p num_cols is out of range.
 *
 * Complexity: 4
 */
uint8_t asdf_keymaps_add_map_r(asdf_keymap_state_t *keymap, const asdf_key_t *matrix,
                               modifier_index_t modifier_index, uint8_t num_rows,
                               uint8_t num_cols) {
    if ((modifier_index < ASDF_MOD_NUM_MODIFIERS) && (num_rows <= ASDF_MAX_ROWS) &&
        (num_cols <= ASDF_MAX_COLS)) {
        keymap->maps[modifier_index].matrix = matrix;
        keymap->maps[modifier_index].rows = num_rows;
        keymap->maps[modifier_index].cols = num_cols;
        return 1;
    }
    return 0;
}

/**
 * Return the number of rows in the matrix for a modifier state.
 *
 * No side effects.
 *
 * @param keymap          Keymap state to query.
 * @param modifier_index  Modifier state.
 * @return Rows of that matrix; 0 if @p modifier_index is out of range.
 *
 * Complexity: 2
 */
uint8_t asdf_keymaps_num_rows_r(const asdf_keymap_state_t *keymap,
                                modifier_index_t modifier_index) {
    return (modifier_index < ASDF_MOD_NUM_MODIFIERS) ? keymap->maps[modifier_index].rows : 0;
}

/**
 * Return the number of columns in the matrix for a modifier state.
 *
 * No side effects.
 *
 * @param keymap          Keymap state to query.
 * @param modifier_index  Modifier state.
 * @return Columns of that matrix; 0 if @p modifier_index is out of range.
 *
 * Complexity: 2
 */
uint8_t asdf_keymaps_num_cols_r(const asdf_keymap_state_t *keymap,
                                modifier_index_t modifier_index) {
    return (modifier_index < ASDF_MOD_NUM_MODIFIERS) ? keymap->maps[modifier_index].cols : 0;
}

/**
 * Look up the key at a matrix position.
 *
 * No side effects.
 *
 * @param keymap          Keymap state to query.
 * @param row             Key row.
 * @param col             Key column.
 * @param modifier_index  Modifier state (a modifier_index_t).
 * @return The key at @p row, @p col in the matrix for @p modifier_index;
 *         KEY_NOTHING if @p modifier_index, @p row, or @p col is out of range,
 *         or no matrix is set.
 *
 * The key is copied out of flash with FLASH_MEMCPY, since the matrix is in
 * flash on the AVR targets.
 *
 * Complexity: 5
 */
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

/**
 * Reset the keyboard's keymap-dependent state.
 *
 * Clears the key matrices and virtual outputs, resets the modifiers and repeat
 * state, clears the each-scan action and the error count, and restores the
 * keyboard's base platform.
 *
 * @param kb  Keyboard to reset.
 *
 * Complexity: 2
 */
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
    kb->keymap.errors = 0;
    asdf_install_platform_r(kb, NULL);
}

/**
 * Count a descriptor entry that could not be applied.
 *
 * Increments keymap->errors, saturating at 255.
 *
 * @param keymap  Keymap state whose error count to increase.
 *
 * Complexity: 2
 */
static void asdf_keymaps_count_error(asdf_keymap_state_t *keymap) {
    if (keymap->errors < UINT8_MAX) {
        keymap->errors++;
    }
}

/**
 * Configure the keyboard from a keymap descriptor.
 *
 * Applies the fields in the order documented for asdf_keymap_t: modifier
 * maps, print delay, each-scan action, virtual output assignments, flags, and
 * platform. A modifier map that is missing or too large, and a virtual output
 * assignment that is invalid or conflicts with an earlier one, is skipped and
 * counted in keymap.errors.
 *
 * @param kb      Keyboard to configure, freshly reset.
 * @param keymap  Keymap descriptor, in flash.
 *
 * The descriptor and its output table are copied out of flash one element at
 * a time, so only one element is held in RAM.
 *
 * Complexity: 9
 */
static void asdf_keymaps_apply_r(asdf_t *kb, const asdf_keymap_t *keymap) {
    asdf_keymap_t k;
    FLASH_MEMCPY(&k, keymap, sizeof(k));

    for (uint8_t m = 0; m < ASDF_MOD_NUM_MODIFIERS; m++) {
        if (!k.maps[m] ||
            !asdf_keymaps_add_map_r(&kb->keymap, k.maps[m], (modifier_index_t)m, k.rows, k.cols)) {
            asdf_keymaps_count_error(&kb->keymap);
        }
    }

    kb->print_delay_ms = k.print_delay_ms;

    kb->keymap.each_scan = k.each_scan;

    for (uint8_t i = 0; i < k.num_outputs; i++) {
        asdf_virtual_initializer_t out;
        FLASH_MEMCPY(&out, &k.outputs[i], sizeof(out));
        if (!asdf_virtual_assign_r(&kb->outputs, out.virtual_device, out.physical_device,
                                   out.function, out.initial_value)) {
            asdf_keymaps_count_error(&kb->keymap);
        }
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

/**
 * Switch to a keymap, resetting and reconfiguring the keyboard.
 *
 * If the keymap exists: records it as the current (and requested) keymap;
 * resets the keymap-dependent state and returns the platform's outputs to
 * their defaults, undoing the previous keymap's settings; applies the
 * descriptor; re-applies the configuration actions of held switches (keymap
 * select, strobe polarity, autorepeat select), but not other held keys such
 * as SHIFT; and applies the initial virtual output values to the hardware. If
 * the keymap does not exist, nothing changes.
 *
 * @param kb     Keyboard to configure.
 * @param index  Keymap number.
 *
 * Complexity: 2
 */
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

/**
 * Switch to a keymap if it differs from the current keymap.
 *
 * Side effects as for asdf_keymaps_switch_r(); does nothing if @p index is
 * the current keymap or does not exist.
 *
 * @param kb     Keyboard to configure.
 * @param index  Keymap number.
 *
 * Complexity: 2
 */
void asdf_keymaps_select_r(asdf_t *kb, uint8_t index) {
    if (index != kb->keymap.current) {
        asdf_keymaps_switch_r(kb, index);
    }
}

/**
 * Select keymap 0.
 *
 * Switches to keymap 0, configuring the keyboard; does nothing if keymap 0
 * does not exist.
 *
 * @param kb  Keyboard to configure.
 */
void asdf_keymaps_init_r(asdf_t *kb) { asdf_keymaps_switch_r(kb, 0); }

/**
 * Set or clear bits of the requested keymap number.
 *
 * Called by the keymap select actions (DIP switches). Changes only
 * keymap->requested; asdf_keymaps_apply_request_r() applies the request at
 * the end of the scan.
 *
 * @param keymap  Keymap state to modify.
 * @param bit     Mask of the bits to change, normally one ASDF_KEYMAP_BIT_n.
 * @param set     Nonzero to set the bits, 0 to clear them.
 *
 * Complexity: 2
 */
void asdf_keymaps_request_bit_r(asdf_keymap_state_t *keymap, uint8_t bit, uint8_t set) {
    if (set) {
        keymap->requested |= bit;
    } else {
        keymap->requested &= (uint8_t)~bit;
    }
}

/**
 * Switch to the requested keymap if it differs from the current keymap.
 *
 * Called at the end of each scan, so a keymap never changes part-way through
 * a scan, and DIP switch bits that change in the same scan select the final
 * keymap directly rather than passing through intermediate keymaps. Side
 * effects as for asdf_keymaps_switch_r(); a request for a keymap that does
 * not exist is ignored.
 *
 * @param kb  Keyboard to configure.
 */
void asdf_keymaps_apply_request_r(asdf_t *kb) {
    asdf_keymaps_select_r(kb, kb->keymap.requested);
}


//-------|---------|---------+---------+---------+---------+---------+---------+
// Above line is 80 columns, and should display completely in the editor.
