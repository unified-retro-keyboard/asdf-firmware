// -*- mode: C; tab-width: 4 ; indent-tabs-mode: nil -*-
/**
 * @file asdf_keymap_ace1000.h
 *
 * Matrix size, outputs, and print delay for the Franklin ACE 1000 replacement
 * keyboard keymap.
 * https://github.com/ryucats/Franklin-ACE-1000-Keyboard
 *
 * Part of the Unified Keyboard Project ASDF keyboard firmware.
 *
 * @copyright Copyright 2023 Chris RYU. MIT License; see LICENSE.
 */
// Copyright 2019 David Fenyes
// SPDX-License-Identifier: MIT


#if !defined(ASDF_KEYMAP_DEFS_ACE1000_H)
#define ASDF_KEYMAP_DEFS_ACE1000_H

#include "asdf_arch.h"
#include "asdf_keymaps.h"


// The size of this keymap's matrices, at most ASDF_MAX_ROWS by ASDF_MAX_COLS.
// Keys not given in the YAML matrices do nothing.

#define ACE1000_NUM_ROWS 10 // DIP switches are row 8 (zero based)
#define ACE1000_NUM_COLS 8


#define ACE1000_VIRTUAL_RESET VOUT1
#define ACE1000_RESET_OUTPUT PHYSICAL_OUT3_OPEN_HI
#define ACE1000_RESET_ACTIVE_VALUE 0u

#define ACE1000_VIRTUAL_CLR_SCR VOUT2
#define ACE1000_CLR_SCR_OUT PHYSICAL_OUT1_OPEN_LO
#define ACE1000_CLR_SCR_ACTIVE_VALUE 1u

#define ACE1000_VIRTUAL_POWER_LED VLED1
#define ACE1000_POWER_LED PHYSICAL_LED1
#define ACE1000_POWER_LED_INIT_VALUE 1u

#define ACE1000_CAPS_LED PHYSICAL_LED3
#define ACE1000_CAPS_LED_INIT_VALUE 0u


#define ASDF_ACE1000_PRINT_SPEED 40

/** The Franklin ACE 1000 keymap's descriptor, in flash. */
extern const asdf_keymap_t FLASH ace1000_keymap;

#endif /* !defined (ASDF_KEYMAP_DEFS_ACE1000_H) */

//-------|---------|---------+---------+---------+---------+---------+---------+
// Above line is 80 columns, and should display completely in the editor.
