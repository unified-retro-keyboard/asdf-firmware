// -*- mode: C; tab-width: 4 ; indent-tabs-mode: nil -*-
/**
 * @file asdf_keymap_apple2_caps.h
 *
 * Print delay for the ALL CAPS Apple II keymap.
 *
 * Part of the Unified Keyboard Project ASDF keyboard firmware.
 *
 * @copyright Copyright 2019 David Fenyes. MIT License; see LICENSE.
 */
// SPDX-License-Identifier: MIT


// The Apple 2 ASCII keyboard keymaps come in two variants:
//
// 1) An Upper/Lowercase variant. This variant moves the @ and ^ symbols from
//    the P and N keys to the REPEAT key, since the P and N keys need to reserve
//    the shifted value for the uppercase characters. Also, the "Power" key
//    doubles as a caps-lock key.
//
// 2) A CAPS only keyboard, following the standard Apple II conventions and
//    layout, with ^ above the N key and @ above the P key, and a functioning
//    REPEAT key.
//
// For both variants, CTRL+RESET is required for a system reset.

#if !defined(ASDF_KEYMAP_DEFS_APPLE2_CAPS_H)
#define ASDF_KEYMAP_DEFS_APPLE2_CAPS_H

#include "asdf_arch.h"
#include "asdf_keymaps.h"

/** The Apple 2 CAPS-only keymap's descriptor, in flash. */
extern const asdf_keymap_t FLASH apple2_caps_keymap;

#endif /* !defined (ASDF_KEYMAP_DEFS_APPLE2_CAPS_H) */

//-------|---------|---------+---------+---------+---------+---------+---------+
// Above line is 80 columns, and should display completely in the editor.
