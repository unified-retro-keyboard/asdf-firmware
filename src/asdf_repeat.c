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
// given time. All state is held in the caller's asdf_repeat_state_t; see
// asdf_repeat.h for the meaning of each repeat mode.

// PROCEDURE: asdf_repeat_init_r
// INPUTS: (asdf_repeat_state_t *) repeat - repeat state to operate on
// OUTPUTS: none
//
// DESCRIPTION: Initialize the repeat state state machine
//
// SIDE EFFECTS: see DESCRIPTION
//
// COMPLEXITY: 1
//
void asdf_repeat_init_r(asdf_repeat_state_t *repeat)
{
  repeat->mode = repeat->base_mode = ASDF_DEFAULT_REPEAT_STATE;
  repeat->timer = (uint16_t) repeat->mode;
}

// PROCEDURE: asdf_repeat_reset_count_r
// INPUTS: (asdf_repeat_state_t *) repeat - repeat state to operate on
// OUTPUTS: none
//
// DESCRIPTION: resets the repeat counter for the current key, to begin a new
// repeat cycle. The counter is reset to the countdown value for the current
// state (no repeat, autorepeat, normal repeat).
//
// SIDE EFFECTS: see DESCRIPTION
//
// SCOPE: Public
//
// COMPLEXITY: 1
//
void asdf_repeat_reset_count_r(asdf_repeat_state_t *repeat)
{
  repeat->timer = (uint16_t) repeat->mode;
}

// PROCEDURE: asdf_repeat_auto_off_r
// INPUTS: (asdf_repeat_state_t *) repeat - repeat state to operate on
// OUTPUTS: none
//
// DESCRIPTION: Turns Autorepeat mode off by setting the base state to
// autorepeat. If key is repeating, then the new behavior will be realized after
// the repeat key is released. This function can be bound to a key or DIP switch
// to turn autorepeat off.
//
// SIDE EFFECTS: See DESCRIPTION
//
// SCOPE: public
//
// COMPLEXITY: 1
//
void asdf_repeat_auto_off_r(asdf_repeat_state_t *repeat)
{
  repeat->base_mode = REPEAT_OFF;
  if (REPEAT_ON != repeat->mode) {
    repeat->timer = repeat->mode = repeat->base_mode;
  }
}

// PROCEDURE: asdf_repeat_auto_on_r
// INPUTS: (asdf_repeat_state_t *) repeat - repeat state to operate on
// OUTPUTS: none
//
// DESCRIPTION: Turns Autorepeat mode on by setting the base state to
// autorepeat. If key is repeating, then the new behavior will be realized after
// the repeat key is released.  This function can be bound to a key or DIP switch
// to turn autorepeat on.
//
// SIDE EFFECTS: see above
//
// SCOPE: public
//
// COMPLEXITY: 1
//
void asdf_repeat_auto_on_r(asdf_repeat_state_t *repeat)
{
  repeat->base_mode = REPEAT_AUTO;
  if (REPEAT_ON != repeat->mode) {
    repeat->timer = repeat->mode = repeat->base_mode;
  }
}

// PROCEDURE: asdf_repeat_activate_r
// INPUTS: (asdf_repeat_state_t *) repeat - repeat state to operate on
// OUTPUTS: none
//
// DESCRIPTION: set repeat state machine to repeat mode. Called when REPEAT key
// is pressed.
//
// SIDE EFFECTS: see DESCRIPTION
//
// NOTES: If a key is pressed and the key timer is less than or equal to the
// normal repeat rate, then don't change the timing, to avoid appearance of
// stuttering.
//
// COMPLEXITY: 1
//
void asdf_repeat_activate_r(asdf_repeat_state_t *repeat)
{
  if (repeat->timer > REPEAT_ON || REPEAT_OFF == repeat->mode) {
    repeat->timer = repeat->mode = REPEAT_ON;
  }
}

// PROCEDURE: asdf_repeat_deactivate_r
// INPUTS: (asdf_repeat_state_t *) repeat - repeat state to operate on
// OUTPUTS: none
//
// DESCRIPTION: Reset repeat state to default state. Called when REPEAT
// key is released.
//
// SIDE EFFECTS: See DESCRIPTION
//
// NOTES: Releasing repeat disrupts any repeat timers. If a key is still held
// down after REPEAT is released, and autorepeat mode is enabled, then restart
// the autorepeat timer. Otherwise disable repeat.
//
// SCOPE: Public
//
// COMPLEXITY: 1
//
void asdf_repeat_deactivate_r(asdf_repeat_state_t *repeat)
{
  repeat->timer = repeat->mode = repeat->base_mode;
}

// PROCEDURE: asdf_repeat_is_autorepeat_enabled_r
// INPUTS: (const asdf_repeat_state_t *) repeat - repeat state to query
// OUTPUTS: returns TRUE (nonzero) if the base mode is autorepeat
//
// SCOPE: public
//
// COMPLEXITY: 1
//
uint8_t asdf_repeat_is_autorepeat_enabled_r(const asdf_repeat_state_t *repeat)
{
  return (repeat->base_mode == REPEAT_AUTO);
}

// PROCEDURE: asdf_repeat_r
// INPUTS: (asdf_repeat_state_t *) repeat - repeat state to operate on
// OUTPUTS: none
//
// DESCRIPTION: counts down the repeat timer (if activated) and returns a TRUE
// value when the counter times out. A true values indicates that the last value
// should be repeated to the output.
//
// SIDE EFFECTS: none
//
// NOTES: The repeat->timer is only decremented if it is nonzero.
//
// SCOPE: Public
//
// COMPLEXITY: 2
//
uint8_t asdf_repeat_r(asdf_repeat_state_t *repeat)
{
  return asdf_repeat_advance_r(repeat, 1);
}

// PROCEDURE: asdf_repeat_advance_r
// INPUTS: (asdf_repeat_state_t *) repeat - repeat state to operate on
//         (uint8_t) elapsed - ticks elapsed
// OUTPUTS: returns TRUE (nonzero) when the current key should repeat
//
// DESCRIPTION: Advances the repeat timer by elapsed ticks. When it expires, the
// key repeats once and the timer is reloaded for the repeat interval.
//
// NOTES: The timer only runs while it is nonzero (REPEAT_OFF stops it).
//
// SCOPE: public
//
// COMPLEXITY: 3
//
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
