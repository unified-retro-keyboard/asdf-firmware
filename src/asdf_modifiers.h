// -*- mode: C; tab-width: 4 ; indent-tabs-mode: nil -*-
/**
 * @file asdf_modifiers.h
 *
 * Part of the Unified Keyboard Project ASDF keyboard firmware.
 *
 * @copyright Copyright 2019 David Fenyes. MIT License; see LICENSE.
 */
// SPDX-License-Identifier: MIT

#if !defined(ASDF_MODIFIERS_H)
#define ASDF_MODIFIERS_H

#include <stdbool.h>
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
#define ASDF_MODIFIERS_SHIFT_POS 0u
#define ASDF_MODIFIERS_CAPS_POS 1u
#define ASDF_MODIFIERS_CTRL_POS 2u

#define ASDF_MODIFIERS_SHIFT_MASK (1U << ASDF_MODIFIERS_SHIFT_POS)
#define ASDF_MODIFIERS_CAPS_MASK (1U << ASDF_MODIFIERS_CAPS_POS)
#define ASDF_MODIFIERS_CTRL_MASK (1U << ASDF_MODIFIERS_CTRL_POS)

/** SHIFT and SHIFTLOCK state bits, in asdf_modifier_state_t.shift. */
#define SHIFT_OFF_ST 0u
#define SHIFT_ON_ST 1u     ///< SHIFT held
#define SHIFT_LOCKED_ST 2u ///< SHIFTLOCK on
#define SHIFT_BOTH_ST 3u   ///< SHIFT and SHIFTLOCK together; never set explicitly

/** CAPSLOCK state, in asdf_modifier_state_t.caps. */
#define CAPS_OFF_ST 0u
#define CAPS_LOCKED_ST 1u

/** CTRL state, in asdf_modifier_state_t.ctrl. */
#define CTRL_OFF_ST 0u
#define CTRL_ON_ST 1u

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
 * asdf_modifier_shift_locked() and asdf_modifier_caps_locked().
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
 * Invariants, after asdf_modifiers_init() and any sequence of operations:
 * - shift is a combination of SHIFT_*_ST bits (0 to 3)
 * - caps is CAPS_OFF_ST or CAPS_LOCKED_ST
 * - ctrl is CTRL_OFF_ST or CTRL_ON_ST
 * - asdf_modifier_index() returns a value < ASDF_MOD_NUM_MODIFIERS
 *
 * @code
 * #include "asdf_modifiers.h"
 *
 * asdf_modifier_state_t mods;
 * modifier_index_t map;
 *
 * asdf_modifiers_init(&mods);
 * asdf_modifier_shift_activate(&mods);    // SHIFT pressed
 * map = asdf_modifier_index(&mods);       // MOD_SHIFT_MAP
 * @endcode
 */
typedef struct {
  uint8_t shift; ///< SHIFT_*_ST: SHIFT and SHIFTLOCK bits
  uint8_t caps;  ///< CAPS_*_ST
  uint8_t ctrl;  ///< CTRL_*_ST
} asdf_modifier_state_t;

/**
 * Set SHIFT, SHIFTLOCK, CAPSLOCK, and CTRL to off.
 *
 * Call once before any other operation on @p mods. Writes only @p mods.
 *
 * @param mods  Modifier state to initialize.
 */
void asdf_modifiers_init(asdf_modifier_state_t *mods);

/**
 * SHIFT pressed: set SHIFT on and clear SHIFTLOCK.
 *
 * Clearing SHIFTLOCK here means a SHIFT press and release ends a lock. Writes
 * only @p mods.
 *
 * @param mods  Modifier state to update.
 */
void asdf_modifier_shift_activate(asdf_modifier_state_t *mods);

/**
 * SHIFTLOCK pressed (ACTION_SHIFTLOCK_ON): set SHIFTLOCK on.
 *
 * SHIFT is unchanged; if SHIFT is held, the state becomes SHIFT_BOTH_ST and
 * the lock ends when SHIFT is released. Writes only @p mods.
 *
 * @param mods  Modifier state to update.
 */
void asdf_modifier_shiftlock_on_activate(asdf_modifier_state_t *mods);

/**
 * SHIFTLOCK pressed (ACTION_SHIFTLOCK_TOGGLE): toggle SHIFTLOCK.
 *
 * SHIFT is unchanged. Writes only @p mods.
 *
 * @param mods  Modifier state to update.
 */
void asdf_modifier_shiftlock_toggle_activate(asdf_modifier_state_t *mods);

/**
 * SHIFT released: set both SHIFT and SHIFTLOCK off.
 *
 * Also ends a lock set while SHIFT was held. Writes only @p mods.
 *
 * @param mods  Modifier state to update.
 */
void asdf_modifier_shift_deactivate(asdf_modifier_state_t *mods);

/**
 * CAPSLOCK pressed: toggle CAPSLOCK.
 *
 * There is no matching release operation; CAPSLOCK ignores release. Writes
 * only @p mods.
 *
 * @param mods  Modifier state to update.
 */
void asdf_modifier_capslock_activate(asdf_modifier_state_t *mods);

/**
 * CTRL pressed: set CTRL on.
 *
 * Writes only @p mods.
 *
 * @param mods  Modifier state to update.
 */
void asdf_modifier_ctrl_activate(asdf_modifier_state_t *mods);

/**
 * CTRL released: set CTRL off.
 *
 * Writes only @p mods.
 *
 * @param mods  Modifier state to update.
 */
void asdf_modifier_ctrl_deactivate(asdf_modifier_state_t *mods);

/**
 * Whether SHIFTLOCK is on, for driving the SHIFTLOCK LED.
 *
 * True in both SHIFT_LOCKED_ST and SHIFT_BOTH_ST. No side effects.
 *
 * @param mods  Modifier state to query.
 * @return true if SHIFTLOCK is on, else false.
 */
bool asdf_modifier_shift_locked(const asdf_modifier_state_t *mods);

/**
 * Whether CAPSLOCK is on, for driving the CAPSLOCK LED.
 *
 * No side effects.
 *
 * @param mods  Modifier state to query.
 * @return true if CAPSLOCK is on, else false.
 */
bool asdf_modifier_caps_locked(const asdf_modifier_state_t *mods);

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
modifier_index_t asdf_modifier_index(const asdf_modifier_state_t *mods);

#endif // !defined (ASDF_MODIFIERS_H)

//-------|---------|---------+---------+---------+---------+---------+---------+
// Above line is 80 columns, and should display completely in the editor.
