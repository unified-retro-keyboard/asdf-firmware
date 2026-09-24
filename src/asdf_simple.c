// -*- mode: C; tab-width: 2 ; indent-tabs-mode: nil -*-
/**
 * @file asdf_simple.c
 *
 * The simple wrapper (see asdf_simple.h). It owns one set of hardware and one
 * keyboard, and defines the tick interrupt, so it cannot be linked with
 * main.c, which owns its own.
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

#include <stdint.h>
#include "asdf.h"
#include "asdf_arch.h"
#include "asdf_keyboard.h"
#include "asdf_simple.h"

static asdf_arch_t arch;
static asdf_t keyboard;

// A code taken from the keyboard by asdf_available(), not yet read.
static asdf_keycode_t pending_code;
static uint8_t code_pending;

/**
 * The 1 ms tick interrupt.
 *
 * Counts the tick in the hardware state, for asdf_poll() to collect. It does
 * nothing else; asdf_poll() does the scanning and all other work, so the
 * interrupt stays short.
 */
ASDF_ARCH_TICK_ISR
{
  asdf_arch_count_tick(&arch);
}

/**
 * Set up the hardware and the keyboard.
 *
 * Discards any held code, initializes the architecture (which starts the tick
 * interrupt), then initializes the keyboard on the architecture's platform,
 * selecting the first keymap.
 */
void asdf_begin(void)
{
  code_pending = 0;
  asdf_arch_init(&arch);
  asdf_init(&keyboard, &arch.platform);
}

/**
 * Run the keyboard for the ticks counted since the last call.
 *
 * Collects the elapsed ticks from the hardware state and passes them to the
 * keyboard, which advances its timers and scans the key matrix once. Codes
 * generated are queued on the keyboard for asdf_available().
 */
void asdf_poll(void) { asdf_update(&keyboard, asdf_arch_tick(&arch)); }

/**
 * Report whether a code is ready to read.
 *
 * When no code is held, takes the next ready code from the keyboard and holds
 * it until asdf_read() takes it, so that it can be reported as available
 * without being lost.
 *
 * @return Nonzero if a code is held; 0 if not.
 *
 * Complexity: 2
 */
uint8_t asdf_available(void)
{
  if (!code_pending) {
    code_pending = asdf_next_code(&keyboard, &pending_code);
  }
  return code_pending;
}

/**
 * Take the next code.
 *
 * Releases the held code, first taking one from the keyboard through
 * asdf_available() if none is held.
 *
 * @return The held code, or 0 if none is available.
 *
 * Complexity: 2
 */
asdf_keycode_t asdf_read(void)
{
  if (!asdf_available()) {
    return 0;
  }
  code_pending = 0;
  return pending_code;
}

//-------|---------|---------+---------+---------+---------+---------+---------+
// Above line is 80 columns, and should display completely in the editor.
