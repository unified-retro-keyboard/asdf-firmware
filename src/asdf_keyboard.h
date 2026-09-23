// -*- mode: C; tab-width: 2 ; indent-tabs-mode: nil -*-
//
// Unified Keyboard Project
// ASDF keyboard firmware
//
// asdf_keyboard.h
//
// The keyboard object. An asdf_t holds all of the changeable state of one
// keyboard, so any number of keyboards can be run independently. The "_r"
// functions operate on the keyboard passed to them. The argument-less
// functions in asdf.h operate on one default keyboard (asdf_compat.c).
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
//

#if !defined(ASDF_KEYBOARD_H)
#define ASDF_KEYBOARD_H

#include <stdint.h>
#include "asdf.h"
#include "asdf_config.h"
#include "asdf_hook.h"
#include "asdf_keymaps.h"
#include "asdf_modifiers.h"
#include "asdf_platform.h"
#include "asdf_repeat.h"
#include "asdf_ring.h"
#include "asdf_virtual.h"

struct asdf_keyboard {
  // Key matrix scanner
  asdf_cols_t stable_rows[ASDF_MAX_ROWS];                    // debounced key state
  uint8_t debounce[ASDF_MAX_ROWS][ASDF_MAX_COLS];            // scans left to debounce
  asdf_keycode_t last_key;                                   // last code-producing key
  uint8_t last_key_row;                                      // ... and its position
  uint8_t last_key_col;

  // Output queues. System messages have priority over typed keycodes.
  asdf_ring_t keycodes;
  asdf_ring_t messages;
  asdf_keycode_t keycode_storage[ASDF_KEYCODE_BUFFER_SIZE];
  asdf_keycode_t message_storage[ASDF_MESSAGE_BUFFER_SIZE];
  uint8_t print_delay_ms; // delay after each system message character
  uint8_t output_wait_ms; // ticks before the next code may be output

  asdf_keymap_state_t keymap;
  asdf_hook_state_t hooks;
  asdf_modifier_state_t modifiers;
  asdf_repeat_state_t repeat;
  asdf_virtual_state_t outputs;

  const asdf_platform_t *base_platform; // platform given to asdf_init_r()
  const asdf_platform_t *platform;      // platform in use (a keymap may override)
};

// PROCEDURE: asdf_init_r
// INPUTS: (asdf_t *) kb - keyboard to initialize
//         (const asdf_platform_t *) platform - its hardware
// OUTPUTS: none
// DESCRIPTION: Initializes every part of the keyboard, with no keys pressed and
// empty queues, and selects keymap 0. May be called again to reset the
// keyboard.
void asdf_init_r(asdf_t *kb, const asdf_platform_t *platform);

// PROCEDURE: asdf_keyscan_r
// DESCRIPTION: Scans the key matrix once, debouncing and acting on key changes,
// and applies any keymap change requested during the scan.
void asdf_keyscan_r(asdf_t *kb);

// PROCEDURE: asdf_process_r
// INPUTS: (asdf_t *) kb, (uint16_t) elapsed_ms - ticks elapsed since the last
//         call
// DESCRIPTION: Runs the keyboard for the elapsed ticks: advance the timers,
// send up to one code per tick, and scan the key matrix once with debounce and
// repeat advanced by the elapsed ticks. Never blocks.
void asdf_process_r(asdf_t *kb, uint16_t elapsed_ms);

// PROCEDURE: asdf_tick_r
// INPUTS: (asdf_t *) kb, (uint8_t) elapsed_ms - ticks elapsed
// DESCRIPTION: Advances the keyboard's timers (message pacing and long output
// pulses) without scanning or sending.
void asdf_tick_r(asdf_t *kb, uint8_t elapsed_ms);

// PROCEDURE: asdf_next_code_r
// OUTPUTS: the next code to send (system messages first), or ASDF_INVALID_CODE
// if none is queued or output is paused after a system message character.
asdf_keycode_t asdf_next_code_r(asdf_t *kb);

// PROCEDURE: asdf_send_code_r
// DESCRIPTION: sends a code to the host through the keyboard's platform.
void asdf_send_code_r(asdf_t *kb, asdf_keycode_t code);

// PROCEDURE: asdf_put_code_r
// OUTPUTS: returns TRUE (nonzero) if queued, FALSE (0) if the queue was full
// DESCRIPTION: queues a typed keycode.
uint8_t asdf_put_code_r(asdf_t *kb, asdf_keycode_t code);

// PROCEDURE: asdf_putc_r
// OUTPUTS: returns c, or EOF if the message queue had no room for it
// DESCRIPTION: queues a system message character; a newline is queued as CR
// LF, as a unit.
int asdf_putc_r(asdf_t *kb, char c);

// PROCEDURE: asdf_install_platform_r
// DESCRIPTION: selects the platform the keyboard is scanned and sent through;
// NULL restores the keyboard's base platform.
void asdf_install_platform_r(asdf_t *kb, const asdf_platform_t *platform);

// PROCEDURE: asdf_apply_configuration_r
// DESCRIPTION: after a keymap switch, re-applies the configuration actions of
// held switches (see asdf_apply_configuration in asdf.h).
void asdf_apply_configuration_r(asdf_t *kb);

// PROCEDURE: asdf_sync_lock_leds_r
// DESCRIPTION: drives the SHIFTLOCK (VSHIFT_LED) and CAPSLOCK (VCAPS_LED)
// indicators from the keyboard's modifier state.
void asdf_sync_lock_leds_r(asdf_t *kb);

#endif /* !defined (ASDF_KEYBOARD_H) */

//-------|---------|---------+---------+---------+---------+---------+---------+
// Above line is 80 columns, and should display completely in the editor.
