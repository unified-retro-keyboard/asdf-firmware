// -*- mode: C; tab-width: 2 ; indent-tabs-mode: nil -*-
//
// Unfified Keyboard Project
// ASDF keyboard firmware
//
// asdf_repeat.c
//
// This file contains the key repeat logic.
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

#include <stdint.h>
#include "asdf_repeat.h"

// This module keeps track of a single repeat event per repeat state object,
// which makes sense for a single keyboard, since only one key can repeat at any
// given time. All state is held in the caller's asdf_repeat_state_t. The
// public contracts, the repeat modes, and the state machine are described in
// asdf_repeat.h.

/**
 * Initialize the repeat state machine.
 *
 * Sets the base mode and mode to ASDF_DEFAULT_REPEAT_STATE and loads the timer
 * for that mode. Writes only @p repeat.
 *
 * @param repeat  Repeat state to initialize.
 */
void asdf_repeat_init_r(asdf_repeat_state_t *repeat)
{
  repeat->mode = repeat->base_mode = ASDF_DEFAULT_REPEAT_STATE;
  repeat->timer = (uint16_t) repeat->mode;
}

/**
 * Reset the repeat timer for the current key, to begin a new repeat cycle.
 *
 * Reloads the timer with the countdown for the current mode (no repeat,
 * autorepeat, normal repeat). Writes only @p repeat.
 *
 * @param repeat  Repeat state to update.
 */
void asdf_repeat_reset_count_r(asdf_repeat_state_t *repeat)
{
  repeat->timer = (uint16_t) repeat->mode;
}

/**
 * Turn autorepeat off by setting the base mode to REPEAT_OFF.
 *
 * If a key is repeating under REPEAT_ON, the new behavior takes effect when
 * REPEAT is released; otherwise the mode and timer change at once. Writes only
 * @p repeat. Can be bound to a key or DIP switch.
 *
 * @param repeat  Repeat state to update.
 *
 * Complexity: 2
 */
void asdf_repeat_auto_off_r(asdf_repeat_state_t *repeat)
{
  repeat->base_mode = REPEAT_OFF;
  if (REPEAT_ON != repeat->mode) {
    repeat->timer = repeat->mode = repeat->base_mode;
  }
}

/**
 * Turn autorepeat on by setting the base mode to REPEAT_AUTO.
 *
 * If a key is repeating under REPEAT_ON, the new behavior takes effect when
 * REPEAT is released; otherwise the mode and timer change at once. Writes only
 * @p repeat. Can be bound to a key or DIP switch.
 *
 * @param repeat  Repeat state to update.
 *
 * Complexity: 2
 */
void asdf_repeat_auto_on_r(asdf_repeat_state_t *repeat)
{
  repeat->base_mode = REPEAT_AUTO;
  if (REPEAT_ON != repeat->mode) {
    repeat->timer = repeat->mode = repeat->base_mode;
  }
}

/**
 * Switch the repeat state machine to REPEAT_ON, when REPEAT is pressed.
 *
 * Writes only @p repeat.
 *
 * @param repeat  Repeat state to update.
 *
 * The timer is left alone when it is already due within the repeat interval
 * and a mode other than REPEAT_OFF is running, to avoid visible stutter. In
 * that case the mode is left unchanged too: if it was REPEAT_AUTO, it stays
 * REPEAT_AUTO while REPEAT is held.
 *
 * Complexity: 3
 */
void asdf_repeat_activate_r(asdf_repeat_state_t *repeat)
{
  if (repeat->timer > REPEAT_ON || REPEAT_OFF == repeat->mode) {
    repeat->timer = repeat->mode = REPEAT_ON;
  }
}

/**
 * Restore the base mode and restart the timer, when REPEAT is released.
 *
 * Writes only @p repeat.
 *
 * @param repeat  Repeat state to update.
 *
 * Releasing REPEAT disrupts any repeat timing. If a key is still held and
 * autorepeat is enabled, the autorepeat delay starts over; otherwise repeat
 * stops.
 */
void asdf_repeat_deactivate_r(asdf_repeat_state_t *repeat)
{
  repeat->timer = repeat->mode = repeat->base_mode;
}

/**
 * Report whether autorepeat is enabled.
 *
 * No side effects.
 *
 * @param repeat  Repeat state to query.
 * @return 1 if the base mode is REPEAT_AUTO, else 0.
 */
uint8_t asdf_repeat_is_autorepeat_enabled_r(const asdf_repeat_state_t *repeat)
{
  return (repeat->base_mode == REPEAT_AUTO);
}

/**
 * Advance the repeat timer by one tick.
 *
 * Counts down and may reload the timer in @p repeat.
 *
 * @param repeat  Repeat state to update.
 * @return 1 when the timer expires and the last code should be repeated to the
 *         output, else 0.
 */
uint8_t asdf_repeat_r(asdf_repeat_state_t *repeat)
{
  return asdf_repeat_advance_r(repeat, 1);
}

/**
 * Advance the repeat timer by @p elapsed ticks.
 *
 * When the timer expires, the key repeats once and the timer reloads with the
 * repeat interval (REPEAT_ON). Counts down and may reload the timer in
 * @p repeat.
 *
 * @param repeat   Repeat state to update.
 * @param elapsed  Ticks (ms) since the last call.
 * @return 1 when the timer expires and the current key should repeat; 0 when
 *         the timer is stopped (REPEAT_OFF) or has not yet expired.
 *
 * A zero timer means REPEAT_OFF: the timer does not run. Time beyond expiry is
 * not carried over into the next interval.
 *
 * Complexity: 3
 */
uint8_t asdf_repeat_advance_r(asdf_repeat_state_t *repeat, uint8_t elapsed)
{
  if (!repeat->timer) {
    return 0;
  }
  if (repeat->timer > elapsed) {
    repeat->timer -= elapsed;
    return 0;
  }
  repeat->timer = REPEAT_ON;
  return 1;
}

//-------|---------|---------+---------+---------+---------+---------+---------+
// Above line is 80 columns, and should display completely in the editor.
