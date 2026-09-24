// -*- mode: C; tab-width: 2 ; indent-tabs-mode: nil -*-
//
// Unified Keyboard Project
// ASDF keyboard firmware
//
// asdf_simple.h
//
// The simple wrapper: the keyboard firmware for a single keyboard, in four
// calls. It owns the hardware state, the keyboard, and the tick interrupt, so
// the application needs no knowledge of keyboard objects or platforms. For
// example:
//
//   asdf_begin();
//   while (1) {
//     asdf_poll();
//     while (asdf_available()) {
//       send_somewhere(asdf_read());
//     }
//   }
//
// The application delivers the codes itself (USB, serial, a parallel port, and
// so on). The keyboard still scans the matrix and drives its LEDs and outputs
// through the architecture's platform. An application needing more than one
// keyboard, or its own platform, uses asdf_t and the _r functions directly
// (asdf_keyboard.h), as main.c does.
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

#if !defined(ASDF_SIMPLE_H)
#define ASDF_SIMPLE_H

#include <stdint.h>
#include "asdf.h"

// PROCEDURE: asdf_begin
// DESCRIPTION: Sets up the hardware and the keyboard, and selects keymap 0.
void asdf_begin(void);

// PROCEDURE: asdf_poll
// DESCRIPTION: Runs the keyboard for the time since the last call: advances its
// timers and scans the key matrix. Call it often, at least once per
// millisecond tick for best timing. Never blocks.
void asdf_poll(void);

// PROCEDURE: asdf_available
// OUTPUTS: returns TRUE (nonzero) if a code is ready to read.
// NOTES: After each system message character, the next code is held back for
// the keymap's print delay, for hosts that cannot take characters quickly.
uint8_t asdf_available(void);

// PROCEDURE: asdf_read
// OUTPUTS: returns the next code, or 0 if none is available (check
//          asdf_available() first; 0 is also a valid code).
asdf_keycode_t asdf_read(void);

#endif /* !defined(ASDF_SIMPLE_H) */

//-------|---------|---------+---------+---------+---------+---------+---------+
// Above line is 80 columns, and should display completely in the editor.
