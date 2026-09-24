// -*- mode: C; tab-width: 4 ; indent-tabs-mode: nil -*-
/**
 * @file asdf_keymap_sol.h
 *
 * Matrix size, outputs, and special key codes for the Sol-20 keymap.
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


#if !defined(ASDF_KEYMAP_SOL_H)
#define ASDF_KEYMAP_SOL_H
#include "asdf.h"

// The size of this keymap's matrices, at most ASDF_MAX_ROWS by ASDF_MAX_COLS.
// Keys not given in the YAML matrices do nothing.

#define ASDF_SOL_NUM_ROWS 13 // DIP switches are row 8
#define ASDF_SOL_NUM_COLS 8


// Key matrix for the Sol-20 keyboard on the ASCII controller
//
// Col->   0          1          2          3          4          5          6          7
// Row 0   Lt Ctrl    ShiftLock  A          S          D          F          G(alpha)   H
// Row 1   J          K          L          ;(semi)    :(colon)   DEL        Repeat     Rt Ctrl
// Row 2   UpperCase  Lt Shift   Z          X          C          V          B(alpha)   N
// Row 3   M          ,(comma)   .(period)  /(slash)   Rt Shift   ModeSelect
// Row 4   ESC        1          2          3          4          5          6(six)     7
// Row 5   8(eight)   9          0(zero)    -(Dash)    ^(Caret)   [          \          ]
// Row 6   BREAK      TAB        Q          W          E          R          T          Y
// Row 7   U          I          O(alpha)   P          @(at)      Return     LineFeed   Load
// Row 9   LOCAL      UpArrow    LtArrow    Spacebar   RtArrow    DnArrow    Home       Clear
// Row 10  NP-Minus   NP-7       NP-Times   NP-8       NP-Divide  NP-9       (none)     (none)
// Row 11  NP-4       NP-1       NP-5       NP-2       NP-6       NP-3       (none)     (none)
// Row 12  NP-0       NP-period  NP-Plus
//
// Row 8   DIP switches 0-7
//
// Physical Resource mapping:
// LED1: UPPER CASE
// LED2: LOCAL
// LED3: SHIFT LOCK
// OUT1: RESET
// OUT2: BREAK
// OUT3: LOCAL

// Notes:
//
// 1) The DIP switch row (ASDF_ARCH_DIPSWITCH_ROW) is the same in every
//    keymap's YAML matrices. Keeping the MAPSEL 0-3 keys in positions 0-3
//    ensures consistent map selection among all keymaps.


#define SOL_PRINT_DELAY 40 // msec

#define SOL_NUM_ROWS 13
#define SOL_NUM_COLS 8

#define SOL_KBD_VRESET VOUT1
#define SOL_KBD_VBREAK VOUT2
#define SOL_KBD_VLOCAL VOUT3
#define SOL_KBD_LED_ON 1
#define SOL_KBD_LED_OFF 0

// The SOL manual (sec. 7.7.8) indicates shiftlock locks SHIFT on, and SHIFT
// returns to unshifted. For Toggle behavior, change SHIFTLOCK_ON to
// SHIFTLOCK_TOGGLE in asdf_keymap_sol_maps.yaml.

#define SOL_ASCII_LOAD 0x8C
#define SOL_ASCII_MODE_SELECT 0x80
#define SOL_ASCII_UP_ARROW 0x97
#define SOL_ASCII_LT_ARROW 0x81
#define SOL_ASCII_RT_ARROW 0x93
#define SOL_ASCII_DN_ARROW 0x9a
#define SOL_ASCII_HOME 0x8e
#define SOL_ASCII_CLEAR 0x8b

#define SOL_KBD_TTL_HIGH 1
#define SOL_KBD_TTL_LOW 0

#define SOL_KBD_LED_UPPERCASE PHYSICAL_LED1
#define SOL_KBD_LED_LOCAL PHYSICAL_LED2
#define SOL_KBD_LED_SHIFTLOCK PHYSICAL_LED3
#define SOL_KBD_TTLOUT_RESET PHYSICAL_OUT3_OPEN_HI // Emulate open collector output.
#define SOL_KBD_TTLOUT_BREAK PHYSICAL_OUT2
#define SOL_KBD_TTLOUT_LOCAL PHYSICAL_OUT1



#endif /* !defined (ASDF_KEYMAP_SOL_H) */

//-------|---------|---------+---------+---------+---------+---------+---------+
// Above line is 80 columns, and should display completely in the editor.
