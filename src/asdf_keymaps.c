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
#include "asdf_hook.h"
#include "asdf_keymap_setup.h"
#include "asdf_modifiers.h"
#include "asdf_platform.h"
#include "asdf_repeat.h"
#include "asdf_virtual.h"
#include <stddef.h>
#include <stdint.h>

// The keymap array contains the keymap information for each modifier state
// This structure is populated using the asdf_keymaps_add_map() function.
static asdf_keycode_map_t keymaps[ASDF_MOD_NUM_MODIFIERS] = {};

// The current keymap index.  This is stored so bitwise operators on the ke
// index can be performed.
static uint8_t current_keyboard_index;

// PROCEDURE: asdf_keymaps_add_map
// INPUTS: (asdf_keycode_t *) - pointer to the keycode matrix to add in to map
//         (uint8_t) modifier_index - the modifier value for the keycode matrix
//         being added
//         (uint8_t) rows - number of rows in the keymap
//         (uint8_t) cols - number of columns in the keymap
//
// OUTPUTS: none
//
// DESCRIPTION: Called by keymap building modules. This routine adds a keymap to
// the current setup function into the keymap setup array.
//
// SIDE EFFECTS:
//
// NOTES: If the keymap modifier index, num_rows, or num_cols are not valid then
// no action is performed.
//
// SCOPE: public
//
// COMPLEXITY: 1
void asdf_keymaps_add_map(const asdf_keycode_t *matrix,
                          modifier_index_t modifier_index, uint8_t num_rows,
                          uint8_t num_cols) {
    if ((modifier_index < ASDF_MOD_NUM_MODIFIERS) &&
        (num_rows <= ASDF_MAX_ROWS) && (num_cols <= ASDF_MAX_COLS)) {
        keymaps[modifier_index].matrix = (asdf_keycode_t *)matrix;
        keymaps[modifier_index].rows = num_rows;
        keymaps[modifier_index].cols = num_cols;
    }
}

// PROCEDURE: asdf_keymaps_num_rows
// INPUTS: none
// OUTPUTS: uint8_t - returns number of rows in keymap for current modifier
// state
//
// DESCRIPTION: See OUTPUTS
//
// SIDE EFFECTS: none
//
// NOTES:
//
// SCOPE: publice
//
// COMPLEXITY: 1
//
uint8_t asdf_keymaps_num_rows(void) {
    return keymaps[asdf_modifier_index()].rows;
}

// PROCEDURE: asdf_keymaps_num_cols
// INPUTS: none
// OUTPUTS: uint8_t - returns number of columns in keymap for current modifier
// state
//
// DESCRIPTION: See OUTPUTS
//
// SIDE EFFECTS: none
//
// NOTES:
//
// SCOPE: publice
//
// COMPLEXITY: 1
//
uint8_t asdf_keymaps_num_cols(void) {
    return keymaps[asdf_modifier_index()].cols;
}

// PROCEDURE: asdf_keymaps_reset
// INPUTS: none
// OUTPUTS: none
//
// DESCRIPTION: Reset keymaps to initial state:
//              - Clear all keycode mapping matrices.
//              - Clear all virtual devices
//              - Reset modifier and repeat state.
//              - Reset all hooks to default state.
//              - Restore the architecture's platform.
//
// SIDE EFFECTS: see DESCRIPTION
//
// SCOPE: private
//
// COMPLEXITY: 2
//
static void asdf_keymaps_reset(void) {
    // Clear all keycode mappings
    for (uint8_t i = 0; i < ASDF_MOD_NUM_MODIFIERS; i++) {
        asdf_keymaps_add_map((const asdf_keycode_t *)NULL, (modifier_index_t)i,
                             0, 0);
    }

    // Clear virtual devices
    asdf_virtual_init();

    // Reset modifiers and repeat state, so each keymap starts from a known
    // state regardless of the keymap it replaces.
    asdf_modifiers_init();
    asdf_repeat_init();

    // Reset hooks, and restore the architecture's platform
    asdf_hook_init();
    asdf_install_platform(NULL);
}

// PROCEDURE: asdf_keymaps_apply
// INPUTS: (const asdf_keymap_t *) keymap - keymap descriptor, in flash
// OUTPUTS: none
//
// DESCRIPTION: Configures the keyboard from a keymap descriptor, in the order
// documented for asdf_keymap_t: modifier maps, print delay, hook bindings,
// virtual output assignments, flags, platform, and finally the transitional
// setup function.
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
static void asdf_keymaps_apply(const asdf_keymap_t *keymap) {
    asdf_keymap_t k;
    FLASH_MEMCPY(&k, keymap, sizeof(k));

    for (uint8_t m = 0; m < ASDF_MOD_NUM_MODIFIERS; m++) {
        asdf_keymaps_add_map(k.maps[m], (modifier_index_t)m, k.rows, k.cols);
    }

    asdf_set_print_delay(k.print_delay_ms);

    for (uint8_t i = 0; i < k.num_hooks; i++) {
        asdf_hook_binding_t binding;
        FLASH_MEMCPY(&binding, &k.hooks[i], sizeof(binding));
        asdf_hook_assign(binding.hook, binding.function);
    }

    for (uint8_t i = 0; i < k.num_outputs; i++) {
        asdf_virtual_initializer_t out;
        FLASH_MEMCPY(&out, &k.outputs[i], sizeof(out));
        asdf_virtual_assign(out.virtual_device, out.physical_device, out.function,
                            out.initial_value);
    }

    if (k.flags & ASDF_KEYMAP_CAPS_ON) {
        asdf_modifier_capslock_activate();
    }
    if (k.flags & ASDF_KEYMAP_NEGATIVE_STROBE) {
        asdf_arch_set_neg_strobe();
    }

    if (k.platform) {
        asdf_install_platform(k.platform);
    }

    if (k.setup) {
        k.setup();
    }
}

// PROCEDURE: asdf_keymaps_switch
// INPUTS: (uint8_t) index - index of the keymap number to switch to
// OUTPUTS: none
//
// DESCRIPTION: accepts a index value.
// 1) assign the value to the global (to the module) current_keyboard_index
// variable
// 2) execute the architecture-dependent init routine, and reset keymaps,
// virtual devices, modifiers, repeat state, and hooks, to undo any settings
// from the previous keymap
// 3) apply the keymap descriptor.
// 4) Re-apply the configuration actions of held switches (DIP switch keymap
// select, strobe polarity, autorepeat select). Other held keys, such as a held
// SHIFT, are not re-activated.
// 5) Apply any initial virtual outputs to the hardware by calling
// asdf_virtual_sync()
//
// SIDE EFFECTS: See DESCRIPTION
//
// NOTES: If the requested index is not valid, then no action is performed.
//
// SCOPE: public
//
// COMPLEXITY: 2
//
void asdf_keymaps_switch(uint8_t index) {
    // we set the current keymap index in order to track the state of the DIP
    // switches, but only switch to a valid map

    if (asdf_keymap_valid(index)) {
        current_keyboard_index = index;

        asdf_arch_init();
        asdf_keymaps_reset();

        asdf_keymaps_apply(asdf_keymap_descriptor(index));

        // Re-apply DIP/configuration switches; other held keys are not
        // re-activated.
        asdf_apply_configuration();

        // Apply the initial virtual device settings
        asdf_virtual_sync();
    }
}

// PROCEDURE: asdf_keymaps_select
// INPUTS: (uint8_t) index - index of the keymap number to select
// OUTPUTS: none
//
// DESCRIPTION: accepts a index value. If the keyboard is different from the
// currently active keyboard, then attempt to switch to the new keyboard index
//
// SIDE EFFECTS: See DESCRIPTION
//
// NOTES:
//
// SCOPE: public
//
// COMPLEXITY: 2
//
void asdf_keymaps_select(uint8_t index) {

    if (index != current_keyboard_index) {
        asdf_keymaps_switch(index);
    }
}

// PROCEDURE: asdf_keymaps_init
// INPUTS: none
// OUTPUTS: none
//
// DESCRIPTION: initialize the keymap list. Called at startup. This function
// switches to keyboard 0. The switch routine cleans up the keyboard data
// structures.
//
// SIDE EFFECTS: See DESCRIPTION
//
// SCOPE: public
//
// COMPLEXITY: 1
//
void asdf_keymaps_init(void) { asdf_keymaps_switch(0); }

// PROCEDURE: asdf_keymaps_map_select_0_clear
// INPUTS: none
// OUTPUTS: none
//
// DESCRIPTION: called when map select 0 switch is open. Clears the 0 bit in the
// keymap index.
//
// SIDE EFFECTS: changes selected keymap
//
// NOTES:
//
// SCOPE: public
//
// COMPLEXITY: 1
//
void asdf_keymaps_map_select_0_clear(void) {
    asdf_keymaps_select(current_keyboard_index & (~ASDF_KEYMAP_BIT_0));
}

// PROCEDURE: asdf_keymaps_map_select_0_set
// INPUTS: none
// OUTPUTS: none
//
// DESCRIPTION: called when map select 0 switch is closed. Sets the 0 bit in the
// keymap index.
//
// SIDE EFFECTS: changes selected keymap
//
// NOTES:
//
// SCOPE: public
//
// COMPLEXITY: 1
//
void asdf_keymaps_map_select_0_set(void) {
    asdf_keymaps_select(current_keyboard_index | ASDF_KEYMAP_BIT_0);
}

// PROCEDURE: asdf_keymaps_map_select_1_clear
// INPUTS: none
// OUTPUTS: none
//
// DESCRIPTION: called when map select 1 switch is open. Clears the 1 bit in the
// keymap index.
//
// SIDE EFFECTS: changes selected keymap
//
// NOTES:
//
// SCOPE: public
//
// COMPLEXITY: 1
//
void asdf_keymaps_map_select_1_clear(void) {
    asdf_keymaps_select(current_keyboard_index & (~ASDF_KEYMAP_BIT_1));
}

// PROCEDURE: asdf_keymaps_map_select_1_set
// INPUTS: none
// OUTPUTS: none
//
// DESCRIPTION: called when map select 1 switch is closed. Sets the 1 bit in the
// keymap index.
//
// SIDE EFFECTS: changes selected keymap
//
// NOTES:
//
// SCOPE: public
//
// COMPLEXITY: 1
//
void asdf_keymaps_map_select_1_set(void) {
    asdf_keymaps_select(current_keyboard_index | ASDF_KEYMAP_BIT_1);
}

// PROCEDURE: asdf_keymaps_map_select_2_clear
// INPUTS: none
// OUTPUTS: none
//
// DESCRIPTION: called when map select 2 switch is open. Clears the 2 bit in the
// keymap index.
//
// SIDE EFFECTS: changes selected keymap
//
// NOTES:
//
// SCOPE: public
//
// COMPLEXITY: 1
//
void asdf_keymaps_map_select_2_clear(void) {
    asdf_keymaps_select(current_keyboard_index & (~ASDF_KEYMAP_BIT_2));
}

// PROCEDURE: asdf_keymaps_map_select_2_set
// INPUTS: none
// OUTPUTS: none
//
// DESCRIPTION: called when map select 2 switch is closed. Sets the 2 bit in the
// keymap index.
//
// SIDE EFFECTS: changes selected keymap
//
// NOTES:
//
// SCOPE: public
//
// COMPLEXITY: 1
//
void asdf_keymaps_map_select_2_set(void) {
    asdf_keymaps_select(current_keyboard_index | ASDF_KEYMAP_BIT_2);
}

// PROCEDURE: asdf_keymaps_map_select_3_clear
// INPUTS: none
// OUTPUTS: none
//
// DESCRIPTION: called when map select 3 switch is open. Clears the 3 bit in the
// keymap index.
//
// SIDE EFFECTS: changes selected keymap
//
// NOTES:
//
// SCOPE: public
//
// COMPLEXITY: 1
//
void asdf_keymaps_map_select_3_clear(void) {
    asdf_keymaps_select(current_keyboard_index & (~ASDF_KEYMAP_BIT_3));
}

// PROCEDURE: asdf_keymaps_map_select_3_set
// INPUTS: none
// OUTPUTS: none
//
// DESCRIPTION: called when map select 3 switch is closed. Sets the 3 bit in the
// keymap index.
//
// SIDE EFFECTS: changes selected keymap
//
// NOTES:
//
// SCOPE: public
//
// COMPLEXITY: 1
//
void asdf_keymaps_map_select_3_set(void) {
    asdf_keymaps_select(current_keyboard_index | ASDF_KEYMAP_BIT_3);
}

// PROCEDURE: asdf_keymaps_get_code
// INPUTS: row, col: row and column of key that has been pressed
//         modifiers_index: index into the keymap array, based on modifier state
// OUTPUTS: returns a key code.
//
// DESCRIPTION: Given a key row and column, and an index based on modifier
// state, return the appropriate keycode.
//
// SIDE EFFECTS: none
//
// SCOPE: public
//
// NOTES: Since the first value in each row is the physical row number, we add 1
// to the column number to read the code for a given row and column number.
//
// COMPLEXITY: 1
//
asdf_keycode_t asdf_keymaps_get_code(const uint8_t row, const uint8_t col,
                                     const uint8_t modifier_index) {
    uint8_t num_cols = keymaps[modifier_index].cols;
    asdf_keycode_t keycode = 0;

    if (keymaps[modifier_index].cols && keymaps[modifier_index].rows) {
        asdf_keycode_t(*keycode_matrix)[num_cols] =
            (void *)(keymaps[modifier_index].matrix);
        keycode = FLASH_READ_MATRIX_ELEMENT(keycode_matrix, row, col);
    }
    return keycode;
}

//-------|---------|---------+---------+---------+---------+---------+---------+
// Above line is 80 columns, and should display completely in the editor.
