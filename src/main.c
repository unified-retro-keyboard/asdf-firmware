// -*- mode: C; tab-width: 2 ; indent-tabs-mode: nil -*-
/**
 * @file main.c
 *
 * The main program for a single keyboard: initializes the hardware and the
 * keyboard, then runs the keyboard from the 1 ms tick in a superloop.
 *
 * Part of the Unified Keyboard Project ASDF keyboard firmware.
 *
 * @copyright Copyright 2019 David Fenyes. MIT License; see LICENSE.
 */
// SPDX-License-Identifier: MIT

#include <stdint.h>
#include "asdf.h"
#include "asdf_arch.h"
#include "asdf_keyboard.h"

// The keyboard's hardware, and the keyboard. The board owns both, and the tick
// interrupt below.
static asdf_arch_t hardware;

/**
 * The 1 ms tick interrupt.
 *
 * Counts the tick in the hardware state, for the main loop to collect. It does
 * nothing else; the main loop does the scanning and all other work.
 */
ASDF_ARCH_TICK_ISR
{
  asdf_arch_count_tick(&hardware);
}

/**
 * Run the keyboard firmware.
 *
 * Initializes the hardware and the keyboard, then loops forever. Each pass
 * collects the 1 ms ticks counted by the tick interrupt since the last pass
 * and runs the keyboard for them: it advances the timers by the elapsed ticks,
 * sends up to one buffered code per tick, and scans the key matrix once.
 * Ticks that elapse while a pass is busy (up to 255) are caught up on the next
 * pass, so no time is lost. Nothing blocks. Side effects: drives all of the
 * keyboard's hardware through the architecture's platform.
 *
 * @return Never returns.
 *
 * The loop is an example of how to use the keyboard, not the most efficient
 * use of the hardware. It can be replaced by an RTOS task. The output queues
 * could then become RTOS message queues, or keep their internal buffers with a
 * counting semaphore to signal waiting codes. Keymap actions run in that task,
 * so they must not block or compute heavily; they should pass messages to
 * other tasks to do the work.
 *
 * Complexity: 3
 */
int main(void)
{
  static asdf_t keyboard;

  // initialize the hardware, then the keyboard logic:
  asdf_arch_init(&hardware);
  asdf_init(&keyboard, &hardware.platform);

  for (;;) {
    uint8_t elapsed_ms = asdf_arch_tick(&hardware);

    if (elapsed_ms > 0u) {
      asdf_process(&keyboard, elapsed_ms);
    }
  }
}


//-------|---------|---------+---------+---------+---------+---------+---------+
// Above line is 80 columns, and should display completely in the editor.
