// -*- mode: C; tab-width: 2 ; indent-tabs-mode: nil -*-
/**
 * @file asdf_repeat.h
 *
 * Part of the Unified Keyboard Project ASDF keyboard firmware.
 *
 * @copyright Copyright 2019 David Fenyes. GNU General Public License
 * version 3 or later; see the license notice below.
 */
//
// This program is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation, either version 3 of the License, or (at your option) any
// later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
// details.
//
// You should have received a copy of the GNU General Public License along with
// this program. If not, see <https://www.gnu.org/licenses/>.
//
//

#if !defined(ASDF_REPEAT_H)
#define ASDF_REPEAT_H

#include <stdint.h>
#include "asdf_config.h"

/**
 * Repeat mode. Each value is the timer reload for that mode, in ticks (1 ms).
 *
 * The base mode is the mode restored when REPEAT is released:
 * - REPEAT_OFF: a key repeats only while REPEAT is held with it.
 * - REPEAT_AUTO: a key also repeats after it is held for the autorepeat delay.
 *
 * The base mode is never REPEAT_ON, which would repeat every key at once.
 */
typedef enum {
  REPEAT_OFF = 0,                        ///< no repeat
  REPEAT_ON = ASDF_REPEAT_TIME_MS,       ///< currently repeating
  REPEAT_AUTO = ASDF_AUTOREPEAT_TIME_MS, ///< wait for autorepeat delay, then start repeating
} asdf_repeat_mode_t;

/**
 * State of one repeat state machine, which times the one key that is
 * repeating.
 *
 * Each function operates only on the state passed to it. The caller resets
 * the timer with asdf_repeat_reset_count() when a new key becomes the
 * repeating key, then advances it with asdf_repeat_advance() while that key
 * is held. When the timer expires the key repeats, and the timer reloads with
 * the repeat interval (REPEAT_ON), so a held key repeats first after the
 * current mode's delay and then at the repeat rate.
 *
 * Pressing REPEAT (asdf_repeat_activate()) switches to REPEAT_ON;
 * releasing it (asdf_repeat_deactivate()) restores the base mode and
 * restarts the timer. Changing the base mode while REPEAT_ON is in effect takes
 * effect when REPEAT is released.
 *
 * Invariants, after asdf_repeat_init() and any sequence of operations:
 * - timer is 0 exactly when mode is REPEAT_OFF
 * - base_mode is REPEAT_OFF or REPEAT_AUTO, when ASDF_DEFAULT_REPEAT_STATE
 *   is one of those
 * - asdf_repeat_advance() reports at most one repeat per call
 *
 * @code
 * #include "asdf_repeat.h"
 *
 * asdf_repeat_state_t repeat;
 *
 * asdf_repeat_init(&repeat);        // base mode ASDF_DEFAULT_REPEAT_STATE
 * asdf_repeat_reset_count(&repeat); // a new key is pressed
 *
 * // on each scan while the key is held, with elapsed_ms since the last scan:
 * if (asdf_repeat_advance(&repeat, elapsed_ms)) {
 *     // send the key's code again
 * }
 * @endcode
 */
typedef struct {
  asdf_repeat_mode_t mode;      ///< current repeat mode
  asdf_repeat_mode_t base_mode; ///< mode restored when a key or REPEAT is released
  uint16_t timer;               ///< counts down to the next repeat event
} asdf_repeat_state_t;

/**
 * Set the base mode and mode to ASDF_DEFAULT_REPEAT_STATE and load the timer.
 *
 * Call once before any other operation on @p repeat. Writes only @p repeat.
 *
 * @param repeat  Repeat state to initialize.
 */
void asdf_repeat_init(asdf_repeat_state_t *repeat);

/**
 * Start a new repeat cycle for a newly pressed key.
 *
 * Reloads the timer with the current mode's delay: none for REPEAT_OFF, the
 * autorepeat delay for REPEAT_AUTO, the repeat interval for REPEAT_ON. The
 * mode is unchanged. Writes only @p repeat.
 *
 * @param repeat  Repeat state to update.
 */
void asdf_repeat_reset_count(asdf_repeat_state_t *repeat);

/**
 * Turn autorepeat off by setting the base mode to REPEAT_OFF.
 *
 * Unless REPEAT_ON is in effect, the mode changes at once and the timer
 * stops; otherwise the change takes effect when REPEAT is released. Can be
 * bound to a key or DIP switch. Writes only @p repeat.
 *
 * @param repeat  Repeat state to update.
 */
void asdf_repeat_auto_off(asdf_repeat_state_t *repeat);

/**
 * Turn autorepeat on by setting the base mode to REPEAT_AUTO.
 *
 * Unless REPEAT_ON is in effect, the mode changes at once and the timer
 * restarts with the autorepeat delay; otherwise the change takes effect when
 * REPEAT is released. Can be bound to a key or DIP switch. Writes only
 * @p repeat.
 *
 * @param repeat  Repeat state to update.
 */
void asdf_repeat_auto_on(asdf_repeat_state_t *repeat);

/**
 * Whether autorepeat is enabled.
 *
 * No side effects.
 *
 * @param repeat  Repeat state to query.
 * @return 1 if the base mode is REPEAT_AUTO, else 0.
 */
uint8_t asdf_repeat_is_autorepeat_enabled(const asdf_repeat_state_t *repeat);

/**
 * REPEAT pressed: switch to REPEAT_ON.
 *
 * The mode becomes REPEAT_ON. The timer reloads with the repeat interval,
 * unless it is already due within that interval (a key that is already
 * repeating keeps its timing, so it does not stutter). Writes only @p repeat.
 *
 * @param repeat  Repeat state to update.
 */
void asdf_repeat_activate(asdf_repeat_state_t *repeat);

/**
 * REPEAT released: restore the base mode and restart the timer with it.
 *
 * A key still held then waits the autorepeat delay if autorepeat is enabled,
 * and otherwise stops repeating. Writes only @p repeat.
 *
 * @param repeat  Repeat state to update.
 */
void asdf_repeat_deactivate(asdf_repeat_state_t *repeat);

/**
 * Advance the repeat timer by one tick; same as asdf_repeat_advance() with
 * an @p elapsed of 1.
 *
 * Counts down and may reload the timer in @p repeat.
 *
 * @param repeat  Repeat state to update.
 * @return 1 when the current key should repeat, else 0.
 */
uint8_t asdf_repeat(asdf_repeat_state_t *repeat);

/**
 * Advance the repeat timer by @p elapsed ticks.
 *
 * When the timer expires, the key repeats once and the timer reloads with the
 * repeat interval (REPEAT_ON). Time beyond expiry is not carried over. The
 * timer does not run in REPEAT_OFF. Counts down and may reload the timer in
 * @p repeat.
 *
 * @param repeat   Repeat state to update.
 * @param elapsed  Ticks (ms) since the last call.
 * @return 1 when the current key should repeat, else 0.
 */
uint8_t asdf_repeat_advance(asdf_repeat_state_t *repeat, uint8_t elapsed);

#endif // !defined (ASDF_REPEAT_H)

//-------|---------|---------+---------+---------+---------+---------+---------+
// Above line is 80 columns, and should display completely in the editor.
