// -*- mode: C; tab-width: 2 ; indent-tabs-mode: nil -*- 
//
// Unified Keyboard Project
// ASDF keyboard firmware
//
// asdf_print.c
//
// Writes strings stored in flash to the system message output.
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


//
// Headers
//
#include "asdf.h"
#include "asdf_arch.h"
#include "asdf_keyboard.h"
#include "asdf_print.h"

//
// Regular functions
//

// PROCEDURE: asdf_print_flash_r
// INPUTS: (asdf_t *) kb - keyboard
//         (const char *) str - NUL-terminated string stored in flash (see
//         FLASH_STRING)
// OUTPUTS: none
//
// DESCRIPTION: Queues the string on the system message output, sending each
// newline as CR LF.
//
// SIDE EFFECTS: see DESCRIPTION
//
// NOTES: Characters that do not fit in the message queue are dropped (see
// asdf_putc).
//
// SCOPE: public
//
// COMPLEXITY: 2
//
void asdf_print_flash_r(asdf_t *kb, const char *str)
{
  char c;

  while ((c = (char) FLASH_READ(str++))) {
    asdf_putc_r(kb, c);
  }
}

//-------|---------|---------+---------+---------+---------+---------+---------+
// Above line is 80 columns, and should display completely in the editor.
