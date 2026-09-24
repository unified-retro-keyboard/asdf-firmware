// -*- mode: C; tab-width: 4 ; indent-tabs-mode: nil -*-
/**
 * @file asdf_print.h
 *
 * Writes strings stored in flash to the system message output.
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

#if !defined (ASDF_PRINT_H)
#define ASDF_PRINT_H

#include "asdf.h"
#include "asdf_arch.h"

/**
 * Queue a string stored in flash on the keyboard's system message output.
 *
 * Adds the characters to the keyboard's message queue, each newline as CR LF.
 * Characters that do not fit in the message queue are dropped and counted (see
 * asdf_putc()).
 *
 * @param kb   Keyboard to print to.
 * @param str  NUL-terminated string in flash, as made by FLASH_STRING().
 */
void asdf_print_flash(asdf_t *kb, const char *str);

/**
 * Queue a string literal on the keyboard's system message output.
 *
 * The literal is stored in flash by FLASH_STRING(), so @p literal must be a
 * string literal, not a pointer.
 *
 * @param kb       Keyboard to print to.
 * @param literal  String literal to print.
 */
#define asdf_print(kb, literal) asdf_print_flash((kb), FLASH_STRING(literal))

#endif /* !defined (ASDF_PRINT_H) */

//-------|---------|---------+---------+---------+---------+---------+---------+
// Above line is 80 columns, and should display completely in the editor.
