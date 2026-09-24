// -*- mode: C; tab-width: 4 ; indent-tabs-mode: nil -*-
//
//  Unfified Keyboard Project
//  ASDF keyboard firmware
//
//  asdf_modifiers.h
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

#if !defined(ASDF_MODIFIERS_H)
#define ASDF_MODIFIERS_H

#include <stdint.h>

// The active modifiers form a bitmap that indexes the precedence table in
// asdf_modifiers.c, which selects the keymap. These are the bit positions. For
// example, if SHIFT and CAPS are active, the bitmap is
//
// (1 << ASDF_MODIFIERS_SHIFT_POS) | (1 << ASDF_MODIFIERS_CAPS_POS)
// = (1 << 0) | (1 << 1)
// = 3
//
// and entry 3 of the table selects the map for that combination (the SHIFT
// map). A combination with its own behavior would name its own map there; a
// combination that behaves like one of its modifiers names that modifier's map.
#define ASDF_MODIFIERS_SHIFT_POS 0
#define ASDF_MODIFIERS_CAPS_POS 1
#define ASDF_MODIFIERS_CTRL_POS 2

#define ASDF_MODIFIERS_SHIFT_MASK (1 << ASDF_MODIFIERS_SHIFT_POS)
#define ASDF_MODIFIERS_CAPS_MASK (1 << ASDF_MODIFIERS_CAPS_POS)
#define ASDF_MODIFIERS_CTRL_MASK (1 << ASDF_MODIFIERS_CTRL_POS)

/**
 * SHIFT and SHIFTLOCK state: bit 0 is SHIFT (held), bit 1 is SHIFTLOCK.
 */
typedef enum {
  SHIFT_OFF_ST = 0,
  SHIFT_ON_ST = 1,
  SHIFT_LOCKED_ST = 2,
  SHIFT_BOTH_ST = 3 // Never explicitly set. SHIFT and SHIFTLOCK together.

} shift_state_t;

/**
 * CAPSLOCK state.
 */
typedef enum {
  CAPS_OFF_ST = 0,
  CAPS_LOCKED_ST = 1,
} caps_state_t;

/**
 * CTRL state.
 */
typedef enum { CTRL_OFF_ST = 0, CTRL_ON_ST = 1 } ctrl_state_t;

/**
 * Keymaps selectable by the modifiers: PLAIN (no modifier), SHIFT, CAPS, and
 * CTRL.
 *
 * When several modifiers are active, the precedence table in asdf_modifiers.c
 * picks one map: CTRL overrides SHIFT and CAPS, and SHIFT overrides CAPS.
 * SHIFTLOCK selects the SHIFT map, the same as SHIFT.
 */
typedef enum {
  MOD_PLAIN_MAP = 0,
  MOD_SHIFT_MAP,
  MOD_CAPS_MAP,
  MOD_CTRL_MAP,
  ASDF_MOD_NUM_MODIFIERS
} modifier_index_t;

/**
 * State of the modifier keys for one keyboard.
 *
 * Each function operates only on the state passed to it and does not drive
 * the indicator LEDs; the caller syncs the SHIFTLOCK and CAPSLOCK LEDs from
 * asdf_modifier_shift_locked_r() and asdf_modifier_caps_locked_r().
 *
 * SHIFT and SHIFTLOCK share the shift field. Pressing SHIFT clears SHIFTLOCK
 * at once (the SHIFT map stays selected while SHIFT is held), and releasing
 * SHIFT clears both. So a SHIFT press and release ends a lock, however it was
 * set; this is the intended way to release a lock set with
 * ACTION_SHIFTLOCK_ON, as on a typewriter. Setting SHIFTLOCK while SHIFT is
 * held gives SHIFT_BOTH_ST, and the lock is then lost when SHIFT is released.
 *
 * CAPSLOCK toggles on each press and ignores release. CTRL is on while held.
 *
 * Invariants, after asdf_modifiers_init_r() and any sequence of operations:
 * - shift is a shift_state_t (0 to 3)
 * - caps is CAPS_OFF_ST or CAPS_LOCKED_ST
 * - ctrl is CTRL_OFF_ST or CTRL_ON_ST
 * - asdf_modifier_index_r() returns a value < ASDF_MOD_NUM_MODIFIERS
 *
 * @code
 * #include "asdf_modifiers.h"
 *
 * asdf_modifier_state_t mods;
 * modifier_index_t map;
 *
 * asdf_modifiers_init_r(&mods);
 * asdf_modifier_shift_activate_r(&mods);    // SHIFT pressed
 * map = asdf_modifier_index_r(&mods);       // MOD_SHIFT_MAP
 * @endcode
 */
typedef struct {
  uint8_t shift; // shift_state_t: SHIFT and SHIFTLOCK bits
  uint8_t caps;  // caps_state_t
  uint8_t ctrl;  // ctrl_state_t
} asdf_modifier_state_t;

/**
 * Set SHIFT, SHIFTLOCK, CAPSLOCK, and CTRL to off.
 *
 * Call once before any other operation on @p mods. Writes only @p mods.
 *
 * @param mods  Modifier state to initialize.
 */
void asdf_modifiers_init_r(asdf_modifier_state_t *mods);

/**
 * SHIFT pressed: set SHIFT on and clear SHIFTLOCK.
 *
 * Clearing SHIFTLOCK here means a SHIFT press and release ends a lock. Writes
 * only @p mods.
 *
 * @param mods  Modifier state to update.
 */
void asdf_modifier_shift_activate_r(asdf_modifier_state_t *mods);

/**
 * SHIFTLOCK pressed (ACTION_SHIFTLOCK_ON): set SHIFTLOCK on.
 *
 * SHIFT is unchanged; if SHIFT is held, the state becomes SHIFT_BOTH_ST and
 * the lock ends when SHIFT is released. Writes only @p mods.
 *
 * @param mods  Modifier state to update.
 */
void asdf_modifier_shiftlock_on_activate_r(asdf_modifier_state_t *mods);

/**
 * SHIFTLOCK pressed (ACTION_SHIFTLOCK_TOGGLE): toggle SHIFTLOCK.
 *
 * SHIFT is unchanged. Writes only @p mods.
 *
 * @param mods  Modifier state to update.
 */
void asdf_modifier_shiftlock_toggle_activate_r(asdf_modifier_state_t *mods);

/**
 * SHIFT released: set both SHIFT and SHIFTLOCK off.
 *
 * Also ends a lock set while SHIFT was held. Writes only @p mods.
 *
 * @param mods  Modifier state to update.
 */
void asdf_modifier_shift_deactivate_r(asdf_modifier_state_t *mods);

/**
 * CAPSLOCK pressed: toggle CAPSLOCK.
 *
 * There is no matching release operation; CAPSLOCK ignores release. Writes
 * only @p mods.
 *
 * @param mods  Modifier state to update.
 */
void asdf_modifier_capslock_activate_r(asdf_modifier_state_t *mods);

/**
 * CTRL pressed: set CTRL on.
 *
 * Writes only @p mods.
 *
 * @param mods  Modifier state to update.
 */
void asdf_modifier_ctrl_activate_r(asdf_modifier_state_t *mods);

/**
 * CTRL released: set CTRL off.
 *
 * Writes only @p mods.
 *
 * @param mods  Modifier state to update.
 */
void asdf_modifier_ctrl_deactivate_r(asdf_modifier_state_t *mods);

/**
 * Whether SHIFTLOCK is on, for driving the SHIFTLOCK LED.
 *
 * True in both SHIFT_LOCKED_ST and SHIFT_BOTH_ST. No side effects.
 *
 * @param mods  Modifier state to query.
 * @return 1 if SHIFTLOCK is on, else 0.
 */
uint8_t asdf_modifier_shift_locked_r(const asdf_modifier_state_t *mods);

/**
 * Whether CAPSLOCK is on, for driving the CAPSLOCK LED.
 *
 * No side effects.
 *
 * @param mods  Modifier state to query.
 * @return 1 if CAPSLOCK is on, else 0.
 */
uint8_t asdf_modifier_caps_locked_r(const asdf_modifier_state_t *mods);

/**
 * Keymap selected by the active modifiers.
 *
 * SHIFT and SHIFTLOCK both count as SHIFT. Combinations resolve by the
 * precedence in modifier_index_t: CTRL overrides SHIFT and CAPS, and SHIFT
 * overrides CAPS. No side effects.
 *
 * @param mods  Modifier state to query.
 * @return The keymap index, always < ASDF_MOD_NUM_MODIFIERS: MOD_CTRL_MAP if
 *         CTRL is on; else MOD_SHIFT_MAP if SHIFT or SHIFTLOCK is on; else
 *         MOD_CAPS_MAP if CAPSLOCK is on; else MOD_PLAIN_MAP.
 */
modifier_index_t asdf_modifier_index_r(const asdf_modifier_state_t *mods);

#endif // !defined (ASDF_MODIFIERS_H)

//-------|---------|---------+---------+---------+---------+---------+---------+
// Above line is 80 columns, and should display completely in the editor.
