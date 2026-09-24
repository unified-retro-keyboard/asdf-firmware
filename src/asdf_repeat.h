// -*- mode: C; tab-width: 2 ; indent-tabs-mode: nil -*-
//
// Unfified Keyboard Project
// ASDF keyboard firmware
//
// asdf_repeat.h
//
// Copyright 2019 David Fenyes
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

// The repeat "mode" is the current repeat rate: the value loaded into the
// repeat timer when in repeat mode (autorepeat or repeat key) and a key is
// pressed.
//
// If the base mode is REPEAT_OFF, then key repetition only occurs when the
// REPEAT key is pressed along with a "code" key.
//
// If the base mode is REPEAT_AUTO, then key repetition occurs when the REPEAT
// key is pressed with a "code" key, or when a "code" key is held for the
// autorepeat delay.
//
// The base mode is never REPEAT_ON, which would instantly repeat any key when
// pressed.
typedef enum {
  REPEAT_OFF = 0,                        // no repeat
  REPEAT_ON = ASDF_REPEAT_TIME_MS,       // currently repeating
  REPEAT_AUTO = ASDF_AUTOREPEAT_TIME_MS, // wait for autorepeat delay, then start repeating
} asdf_repeat_mode_t;

// State of one repeat state machine.
typedef struct {
  asdf_repeat_mode_t mode;      // current repeat mode
  asdf_repeat_mode_t base_mode; // mode restored when a key or REPEAT is released
  uint16_t timer;               // counts down to the next repeat event
} asdf_repeat_state_t;

// Instance API: each function operates only on the state passed to it.

void asdf_repeat_init_r(asdf_repeat_state_t *repeat);
void asdf_repeat_reset_count_r(asdf_repeat_state_t *repeat);
void asdf_repeat_auto_off_r(asdf_repeat_state_t *repeat);
void asdf_repeat_auto_on_r(asdf_repeat_state_t *repeat);
uint8_t asdf_repeat_is_autorepeat_enabled_r(const asdf_repeat_state_t *repeat);
void asdf_repeat_activate_r(asdf_repeat_state_t *repeat);
void asdf_repeat_deactivate_r(asdf_repeat_state_t *repeat);

// PROCEDURE: asdf_repeat_r
// INPUTS: (asdf_repeat_state_t *) repeat - repeat state to operate on
// OUTPUTS: returns TRUE (nonzero) when the current key should repeat
// DESCRIPTION: Advances the repeat timer by one tick.
uint8_t asdf_repeat_r(asdf_repeat_state_t *repeat);

// PROCEDURE: asdf_repeat_advance_r
// INPUTS: (asdf_repeat_state_t *) repeat, (uint8_t) elapsed - ticks elapsed
// OUTPUTS: returns TRUE (nonzero) when the current key should repeat
// DESCRIPTION: Advances the repeat timer by elapsed ticks.
uint8_t asdf_repeat_advance_r(asdf_repeat_state_t *repeat, uint8_t elapsed);

#endif // !defined (ASDF_REPEAT_H)

//-------|---------|---------+---------+---------+---------+---------+---------+
// Above line is 80 columns, and should display completely in the editor.
