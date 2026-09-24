// -*- mode: C; tab-width: 2 ; indent-tabs-mode: nil -*-
/**
 * @file asdf_simple.h
 *
 * The keyboard firmware for a single keyboard, in four calls.
 *
 * The wrapper owns the hardware state, the keyboard, and the tick interrupt,
 * so the application needs no knowledge of keyboard objects or platforms. The
 * application delivers the codes itself (USB, serial, a parallel port, and so
 * on); the keyboard still scans the matrix and drives its LEDs and outputs
 * through the architecture's platform.
 *
 * Because it defines the tick interrupt, the wrapper cannot be linked with
 * main.c. An application needing more than one keyboard, or its own platform,
 * uses asdf_t and the functions in asdf_keyboard.h directly, as main.c
 * does.
 *
 * @code
 * #include "asdf_simple.h"
 *
 * asdf_begin();
 * while (1) {
 *     asdf_poll();
 *     while (asdf_available()) {
 *         send_somewhere(asdf_read());
 *     }
 * }
 * @endcode
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
//

#if !defined(ASDF_SIMPLE_H)
#define ASDF_SIMPLE_H

#include <stdint.h>
#include "asdf.h"

/**
 * Set up the hardware and the keyboard.
 *
 * Initializes the architecture (including the 1 ms tick interrupt), then the
 * keyboard, with empty output queues and keymap 0 selected. Discards any code
 * held by asdf_available(). Call once before any other function here; calling
 * it again resets the keyboard. Drives the keyboard's outputs and LEDs to
 * their initial values through the platform.
 */
void asdf_begin(void);

/**
 * Run the keyboard for the time since the last call.
 *
 * Advances the keyboard's timers by the elapsed 1 ms ticks and scans the key
 * matrix once; codes generated are queued for asdf_available() and
 * asdf_read(). Never blocks. Call it at least once per tick for best timing;
 * slower calls lose no time, since the elapsed ticks (up to 255) are caught up
 * on the next call. Key actions may also drive the keyboard's outputs and LEDs
 * through the platform.
 */
void asdf_poll(void);

/**
 * Report whether a code is ready to read.
 *
 * After each system message character, the next code is held back for the
 * keymap's print delay, for hosts that cannot take characters quickly; the
 * delay counts down in asdf_poll(). Once this returns nonzero, the code stays
 * available until asdf_read() takes it. Side effects: when no code is held,
 * takes the next ready code from the keyboard and holds it for asdf_read().
 *
 * @return Nonzero if a code is ready; 0 if not.
 */
uint8_t asdf_available(void);

/**
 * Take the next code.
 *
 * Removes the code from the wrapper, taking it from the keyboard first if
 * asdf_available() has not already done so.
 *
 * @return The next code, or 0 if none is available. Since 0 is also a valid
 *         code, check asdf_available() first.
 */
asdf_keycode_t asdf_read(void);

#endif /* !defined(ASDF_SIMPLE_H) */

//-------|---------|---------+---------+---------+---------+---------+---------+
// Above line is 80 columns, and should display completely in the editor.
