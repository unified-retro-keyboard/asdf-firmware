// -*- mode: C; tab-width: 4 ; indent-tabs-mode: nil -*-
/**
 * @file asdf_keymaps.h
 *
 * Keymap descriptors, and the keymap state of a keyboard: key lookup in the
 * current keymap, and keymap selection.
 *
 * Part of the Unified Keyboard Project ASDF keyboard firmware.
 *
 * @copyright Copyright 2019 David Fenyes. GNU General Public License
 * version 3 or later; see the license notice below.
 */
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
#include "asdf_actions.h"
#include "asdf_virtual.h"
#include "asdf_physical.h"
#include "asdf_modifiers.h"
#include "asdf_platform.h"

/**
 * Keymap select bit masks, for asdf_keymaps_request_bit().
 *
 * ASDF_KEYMAP_BIT_n is the mask of bit n of the requested keymap number. The
 * keymap select actions (KEY_MAPSEL(n), usually on DIP switches) set and clear
 * these bits, so the switch positions select the keymap.
 */
#define ASDF_KEYMAP_BIT_0 1
#define ASDF_KEYMAP_BIT_1 2
#define ASDF_KEYMAP_BIT_2 4
#define ASDF_KEYMAP_BIT_3 8

/**
 * One key matrix in use, for one modifier state.
 *
 * The matrix is rows x cols keys in row-major order, stored in flash. Each
 * keymap can have its own row and column count.
 */
typedef struct {
  const asdf_key_t *matrix;
  uint8_t rows;
  uint8_t cols;
} asdf_keycode_map_t;

/** Number of elements in an array, for the descriptor table counts. */
#define ASDF_NUM_ELEMENTS(array) ((uint8_t)(sizeof(array) / sizeof((array)[0])))

/** Flags in asdf_keymap_t.flags. */
#define ASDF_KEYMAP_CAPS_ON 0x01         // start with CAPSLOCK on
#define ASDF_KEYMAP_NEGATIVE_STROBE 0x02 // start with negative output strobe

/**
 * A keymap descriptor.
 *
 * Each keymap is described by one immutable descriptor, stored in flash, which
 * the keymap registry (asdf_keymap_setup.h) lists by index. Selecting a keymap
 * applies its descriptor, in field order: the modifier maps, the message print
 * delay, the each-scan action, the virtual output assignments, the flags, and
 * the platform. The ID message is printed by the KEYMAP_ID key action.
 *
 * Every modifier map must be set. A map that is NULL or larger than
 * ASDF_MAX_ROWS x ASDF_MAX_COLS, and an output assignment that is invalid or
 * conflicts with an earlier one, is skipped and counted in
 * asdf_keymap_state_t.errors. Fields left out of the initializer are zero: no
 * each-scan action (ACTION_NOTHING), no flags, and the architecture's platform.
 *
 * @code
 * #include "asdf_keymaps.h"
 *
 * static const asdf_key_t FLASH tiny_plain[2][2] = {
 *   { KEY_SEND('a'), KEY_SEND('b') },
 *   { KEY_SHIFT(0), KEY_KEYMAP_ID(0) },
 * };
 * static const asdf_key_t FLASH tiny_shift[2][2] = {
 *   { KEY_SEND('A'), KEY_SEND('B') },
 *   { KEY_SHIFT(0), KEY_KEYMAP_ID(0) },
 * };
 * static const char FLASH tiny_id_message[] = "[Keymap: tiny]\n";
 * static const asdf_virtual_initializer_t FLASH tiny_outputs[] = {
 *   { VCAPS_LED, PHYSICAL_LED1, V_NOFUNC, 0 },
 * };
 *
 * const asdf_keymap_t FLASH tiny_keymap = {
 *   .maps = { [MOD_PLAIN_MAP] = &tiny_plain[0][0],
 *             [MOD_SHIFT_MAP] = &tiny_shift[0][0],
 *             [MOD_CAPS_MAP] = &tiny_shift[0][0],
 *             [MOD_CTRL_MAP] = &tiny_plain[0][0] },
 *   .rows = 2,
 *   .cols = 2,
 *   .id_message = tiny_id_message,
 *   .num_outputs = ASDF_NUM_ELEMENTS(tiny_outputs),
 *   .outputs = tiny_outputs,
 * };
 * @endcode
 */
typedef struct {
  // key matrix for each modifier state (indexed by modifier_index_t), each
  // rows x cols, stored in flash
  const asdf_key_t *maps[ASDF_MOD_NUM_MODIFIERS];
  uint8_t rows;
  uint8_t cols;
  uint8_t print_delay_ms; ///< delay between system message characters
  uint8_t flags;          ///< ASDF_KEYMAP_* flags
  uint8_t each_scan;      ///< action at the start of each scan (param 0), or ACTION_NOTHING
  uint8_t num_outputs;
  const char *id_message;                    ///< flash string, or NULL
  const asdf_virtual_initializer_t *outputs; ///< num_outputs entries, in flash
  const asdf_platform_t *platform;           ///< NULL for the architecture's platform
} asdf_keymap_t;

/**
 * Keymap state of one keyboard: the matrices in use for each modifier state,
 * the each-scan action, and the current and requested keymap numbers.
 *
 * Invariants, after asdf_keymaps_init() has selected a keymap:
 * - current changes only to a keymap that exists (asdf_keymap_valid()); it is
 *   valid once asdf_keymaps_init() has run, provided any keymap exists
 * - maps and each_scan hold the current keymap's descriptor; a modifier map
 *   the descriptor could not supply is empty (NULL, 0 x 0), so every lookup in
 *   it returns a key that does nothing
 * - every maps entry has rows <= ASDF_MAX_ROWS and cols <= ASDF_MAX_COLS
 * - errors counts the rejected entries of the current keymap's descriptor; it
 *   is cleared on each keymap switch and saturates at 255
 * - requested may name a keymap that does not exist; such a request is ignored
 */
typedef struct {
  asdf_keycode_map_t maps[ASDF_MOD_NUM_MODIFIERS];
  uint8_t each_scan; ///< action at the start of each scan, or ACTION_NOTHING
  uint8_t errors;    ///< descriptor entries of the current keymap that could not
                     // be applied (see asdf_keymaps_apply); saturates
  uint8_t current;   ///< keymap now in use
  uint8_t requested; ///< keymap requested by the keymap select actions
} asdf_keymap_state_t;

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
 */
uint8_t asdf_keymaps_add_map(asdf_keymap_state_t *keymap, const asdf_key_t *matrix,
                               modifier_index_t modifier_index, uint8_t num_rows,
                               uint8_t num_cols);

/**
 * Number of rows in the matrix for a modifier state.
 *
 * No side effects.
 *
 * @param keymap          Keymap state to query.
 * @param modifier_index  Modifier state.
 * @return Rows of that matrix; 0 if @p modifier_index is out of range.
 */
uint8_t asdf_keymaps_num_rows(const asdf_keymap_state_t *keymap,
                                modifier_index_t modifier_index);

/**
 * Number of columns in the matrix for a modifier state.
 *
 * No side effects.
 *
 * @param keymap          Keymap state to query.
 * @param modifier_index  Modifier state.
 * @return Columns of that matrix; 0 if @p modifier_index is out of range.
 */
uint8_t asdf_keymaps_num_cols(const asdf_keymap_state_t *keymap,
                                modifier_index_t modifier_index);

/**
 * Look up the key at a matrix position, copied out of flash.
 *
 * No side effects.
 *
 * @param keymap          Keymap state to query.
 * @param row             Key row.
 * @param col             Key column.
 * @param modifier_index  Modifier state (a modifier_index_t).
 * @return The key at @p row, @p col in the matrix for @p modifier_index; a key
 *         that does nothing (KEY_NOTHING) if @p modifier_index, @p row, or
 *         @p col is out of range, or no matrix is set.
 */
asdf_key_t asdf_keymaps_get_key(const asdf_keymap_state_t *keymap, uint8_t row, uint8_t col,
                                  uint8_t modifier_index);

/**
 * Set or clear bits of the requested keymap number.
 *
 * Called by the keymap select actions (DIP switches). Changes only the
 * requested keymap number; the request takes effect at the end of the scan, in
 * asdf_keymaps_apply_request().
 *
 * @param keymap  Keymap state to modify.
 * @param bit     Mask of the bits to change, normally one ASDF_KEYMAP_BIT_n.
 * @param set     Nonzero to set the bits, 0 to clear them.
 */
void asdf_keymaps_request_bit(asdf_keymap_state_t *keymap, uint8_t bit, uint8_t set);

/**
 * Select the first keymap: the lowest-numbered keymap that exists.
 *
 * Called by asdf_init(). Resets and configures the keyboard as
 * asdf_keymaps_switch() does. With no keymaps at all, the keyboard is left
 * reset, with no key matrices and keymap number 0.
 *
 * @param kb  Keyboard to configure.
 */
void asdf_keymaps_init(asdf_t *kb);

/**
 * Switch to a keymap, resetting and reconfiguring the keyboard.
 *
 * If the keymap exists, this:
 * 1. records it as the current (and requested) keymap;
 * 2. clears the key matrices, virtual outputs, modifiers, repeat state,
 *    each-scan action, and error count, restores the base platform, and
 *    returns the platform's outputs to their defaults, undoing any settings
 *    of the previous keymap;
 * 3. applies the keymap's descriptor (see asdf_keymap_t);
 * 4. re-applies the configuration actions of held switches (keymap select,
 *    strobe polarity, autorepeat select); other held keys, such as a held
 *    SHIFT, are not re-activated;
 * 5. applies the initial virtual output values to the hardware.
 *
 * The switch happens even if @p index is already the current keymap. If the
 * keymap does not exist, nothing changes.
 *
 * @param kb     Keyboard to configure.
 * @param index  Keymap number.
 */
void asdf_keymaps_switch(asdf_t *kb, uint8_t index);

/**
 * Switch to a keymap if it differs from the current keymap.
 *
 * Side effects as for asdf_keymaps_switch(). Does nothing if @p index is
 * the current keymap; a keymap that does not exist is ignored.
 *
 * @param kb     Keyboard to configure.
 * @param index  Keymap number.
 */
void asdf_keymaps_select(asdf_t *kb, uint8_t index);

/**
 * Switch to the requested keymap if it differs from the current keymap.
 *
 * Called at the end of each scan, so a keymap never changes part-way through a
 * scan, and DIP switch bits that change in the same scan select the final
 * keymap directly rather than passing through intermediate keymaps. Side
 * effects as for asdf_keymaps_switch(). A request for a keymap that does not
 * exist is ignored.
 *
 * @param kb  Keyboard to configure.
 */
void asdf_keymaps_apply_request(asdf_t *kb);

#endif /* !defined (ASDF_KEYMAPS_H) */

//-------|---------|---------+---------+---------+---------+---------+---------+
// Above line is 80 columns, and should display completely in the editor.
