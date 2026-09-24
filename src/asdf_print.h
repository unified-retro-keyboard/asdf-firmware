// -*- mode: C; tab-width: 4 ; indent-tabs-mode: nil -*-
//
// Unfified Keyboard Project
// ASDF keyboard firmware
//
// asdf_print.h
//
// Copyright 2019 David Fenyes
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

//-------|---------|---------+---------+---------+---------+---------+---------+
// Above line is 80 columns, and should display completely in the editor.

#include "asdf.h"
#include "asdf_arch.h"

// PROCEDURE: asdf_print_flash_r
// INPUTS: (asdf_t *) kb - keyboard
//         (const char *) str - NUL-terminated string stored in flash
// OUTPUTS: none
// DESCRIPTION: Queues the string on the keyboard's system message output,
// sending each newline as CR LF.
void asdf_print_flash_r(asdf_t *kb, const char *str);

// PROCEDURE: asdf_print_flash
// INPUTS: (const char *) str - NUL-terminated string stored in flash (see
//         FLASH_STRING)
// OUTPUTS: none
// DESCRIPTION: Queues the string on the system message output, sending each
// newline as CR LF.
// COMPLEXITY: 2
void asdf_print_flash(const char *str);

// Queues a string literal on the system message output. The literal is stored
// in flash, not copied to RAM, so the argument must be a string literal.
#define asdf_print(literal) asdf_print_flash(FLASH_STRING(literal))

// Prints a string literal, stored in flash, to the keyboard kb.
#define asdf_print_r(kb, literal) asdf_print_flash_r((kb), FLASH_STRING(literal))

#endif /* !defined (ASDF_PRINT_H) */
