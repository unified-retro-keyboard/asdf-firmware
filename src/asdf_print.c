// -*- mode: C; tab-width: 2 ; indent-tabs-mode: nil -*-
/**
 * @file asdf_print.c
 *
 * Writes strings stored in flash to the system message output.
 *
 * Part of the Unified Keyboard Project ASDF keyboard firmware.
 *
 * @copyright Copyright 2019 David F. MIT License; see LICENSE.
 */
// SPDX-License-Identifier: MIT

#include "asdf.h"
#include "asdf_arch.h"
#include "asdf_keyboard.h"
#include "asdf_print.h"

/**
 * Queue a string stored in flash on the system message output.
 *
 * Queues each character with asdf_putc(), which sends each newline as
 * CR LF. Characters that do not fit in the message queue are dropped and
 * counted by asdf_putc().
 *
 * @param kb   Keyboard to print to.
 * @param str  NUL-terminated string in flash (see FLASH_STRING()).
 *
 * Complexity: 2
 */
void asdf_print_flash(asdf_t *kb, const char *str)
{
  const char *p = str;
  char c = (char) FLASH_READ(p);

  while (c != '\0') {
    (void) asdf_putc(kb, c);
    p++;
    c = (char) FLASH_READ(p);
  }
}

//-------|---------|---------+---------+---------+---------+---------+---------+
// Above line is 80 columns, and should display completely in the editor.
