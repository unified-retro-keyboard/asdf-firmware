// -*- mode: C; tab-width: 2 ; indent-tabs-mode: nil -*-
/**
 * @file <FIXME-module>.h
 *
 * <FIXME: one or two sentences on what the module provides.>
 *
 * Part of the Unified Keyboard Project ASDF keyboard firmware.
 *
 * @copyright Copyright <FIXME-year> <FIXME-author>. GNU General Public License
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

#if !defined(FIXME_MODULE_H)
#define FIXME_MODULE_H

#include <stdint.h>

/**
 * <FIXME: what the state holds.>
 *
 * Invariants, after <FIXME>_init() and any sequence of operations:
 * - <FIXME>
 *
 * @code
 * #include "<FIXME-module>.h"
 *
 * <FIXME: one example of typical use, including use of the result>
 * @endcode
 */
typedef struct {
  uint8_t fixme; // <FIXME: meaning>
} fixme_state_t;

/**
 * <FIXME: summary sentence: what it does.>
 *
 * <FIXME: behavior, preconditions, and guarantees a caller needs. Side
 * effects, or "No side effects.">
 *
 * @param state  <FIXME>
 * @return <FIXME: every distinct return case.>
 */
uint8_t fixme_function(fixme_state_t *state);

#endif /* !defined(FIXME_MODULE_H) */

//-------|---------|---------+---------+---------+---------+---------+---------+
// Above line is 80 columns, and should display completely in the editor.
