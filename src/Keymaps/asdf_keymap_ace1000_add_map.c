// -*- mode: C; tab-width: 2 ; indent-tabs-mode: nil -*-
/**
 * @file asdf_keymap_ace1000_add_map.c
 *
 * Keyboard test action for the Franklin ACE 1000 replacement keyboard. The key
 * matrices are in asdf_keymap_ace1000_maps.yaml.
 * https://github.com/ryucats/Franklin-ACE-1000-Keyboard
 *
 * Part of the Unified Keyboard Project ASDF keyboard firmware.
 *
 * @copyright Copyright 2023 Chris RYU. GNU General Public License version 3
 * or later, pending relicensing; see LICENSES/GPL-3.0-or-later.txt.
 */
// Copyright 2019 David F.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <stdint.h>
#include "asdf.h"
#include "asdf_arch.h"
#include "asdf_keymap_actions.h"
#include "asdf_print.h"

/**
 * Type the keyboard test program for the Franklin ACE 1000.
 *
 * Queues one numbered program line, ending in CR, on the system message
 * output. When run, the program prints the code of each key pressed, until
 * CTRL-C is pressed. A keymap-provided key action.
 *
 * @param kb     Keyboard to type on.
 * @param param  Ignored.
 */
void ace1000_keyboard_test(asdf_t *kb, uint8_t param)
{
  (void) param;
  asdf_print(kb,
             "10GETA$(0):A=ASC(A$(0)):A$(1)=\"CTL+\"+CHR$(A + 64):?\"'\";A$(A<32);\"' = "
             "\";A:IFA<>3GOTO10\r");
}

//-------|---------|---------+---------+---------+---------+---------+---------+
// Above line is 80 columns, and should display completely in the editor.
