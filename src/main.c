// -*- mode: C; tab-width: 2 ; indent-tabs-mode: nil -*-
//
// Unfified Keyboard Project
// ASDF keyboard firmware
//
// main.c
//
// main program loop.  Initialize hardware.  Schedule key scans and i/o.
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
#include "asdf.h"
#include "asdf_arch.h"

// The keyboard's hardware. The board owns it, and the tick interrupt below.
static asdf_arch_t arch;

// PROCEDURE: tick interrupt
// DESCRIPTION: Occurs every 1 ms. Counts the tick for the hardware; the main
// loop does the scanning and all other work.
ASDF_ARCH_TICK_ISR
{
  asdf_arch_count_tick(&arch);
}

// PROCEDURE: main
// INPUTS: none
// OUTPUTS: none
//
// DESCRIPTION: Initialize hardware, Schedule key scans and i/o
//
// SIDE EFFECTS: See above
//
// NOTES: This code just initializes the hardware, and then loops. The loop
// includes:
//
//     - Collect the 1 ms ticks counted by the timer interrupt since the last
//       pass, and run the keyboard for that many ticks. For each tick, the
//       keyboard advances its timers, sends at most one buffered character,
//       and scans the key matrix. Ticks that elapse while a pass is busy are
//       caught up on the next pass, so no time is lost. Nothing blocks.
//
// This is not the most efficient use of the hardware, but is an example of how
// the keyboard scanner is used. Of course, this loop can be replaced with an
// RTOS process as well. If an RTOS is used, then the output queues can be modified
// to use RTOS message queues instead of an internal buffer, or to keep the
// internal buffer and use a counting semaphore to indicate characters in the
// buffer.
//
// If you are adding special functions when using an RTOS, make sure the
// functions do not block, or do any heavy computation. Rather, they should be
// small and pass messages to appropriate processes to handle the activities.
//
// COMPLEXITY: 2
//
int main(void)
{
  // initialize the hardware, then the keyboard logic:
  asdf_arch_init(&arch);
  asdf_init(&arch.platform);

  while (1) {
    uint8_t elapsed_ms = asdf_arch_tick(&arch);

    if (elapsed_ms) {
      asdf_process(elapsed_ms);
    }
  }
}


//-------|---------|---------+---------+---------+---------+---------+---------+
// Above line is 80 columns, and should display completely in the editor.
