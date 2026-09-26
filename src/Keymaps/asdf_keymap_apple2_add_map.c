// -*- mode: C; tab-width: 2 ; indent-tabs-mode: nil -*-
/**
 * @file asdf_keymap_apple2_add_map.c
 *
 * Keyboard test action for the apple2 keymaps. The key matrices are in
 * asdf_keymap_apple2_maps.yaml.
 *
 * Part of the Unified Keyboard Project ASDF keyboard firmware.
 *
 * @copyright Copyright 2019 David F. MIT License; see LICENSE.
 */
// SPDX-License-Identifier: MIT

#include <stdint.h>
#include "asdf.h"
#include "asdf_arch.h"
#include "asdf_keymap_actions.h"
#include "asdf_print.h"

/**
 * Type an Applesoft BASIC keyboard test program.
 *
 * Queues one numbered program line, ending in CR, on the system message
 * output. When run, the program prints the code of each key pressed, until
 * CTRL-C is pressed. A keymap-provided key action.
 *
 * @param kb     Keyboard to type on.
 * @param param  Ignored.
 */
void applesoft_keyboard_test(asdf_t *kb, uint8_t param)
{
  (void) param;
  asdf_print(kb,
             "10GETA$(0):A=ASC(A$(0)):A$(1)=\"CTL+\"+CHR$(A + 64):?\"'\";A$(A<32);\"' = "
             "\";A:IFA<>3GOTO10\r");
}

//-------|---------|---------+---------+---------+---------+---------+---------+
// Above line is 80 columns, and should display completely in the editor.
