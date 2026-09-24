// -*- mode: C; tab-width: 2 ; indent-tabs-mode: nil -*-
//
// Unified Keyboard Project
// ASDF keyboard firmware
//
// asdf_simple.c
//
// The simple wrapper (see asdf_simple.h). It owns one set of hardware and one
// keyboard, and defines the tick interrupt, so it cannot be linked with
// main.c, which owns its own.
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

// PROCEDURE: tick interrupt
// DESCRIPTION: Occurs every 1 ms. Counts the tick; asdf_poll() does the work.
ASDF_ARCH_TICK_ISR
{
  asdf_arch_count_tick(&arch);
}

void asdf_begin(void)
{
  code_pending = 0;
  asdf_arch_init(&arch);
  asdf_init_r(&keyboard, &arch.platform);
}

void asdf_poll(void) { asdf_update_r(&keyboard, asdf_arch_tick(&arch)); }

// Takes the next code from the keyboard, if one is ready, and holds it until
// it is read, so that it can be reported as available without being lost.
uint8_t asdf_available(void)
{
  if (!code_pending) {
    code_pending = asdf_next_code_r(&keyboard, &pending_code);
  }
  return code_pending;
}

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
