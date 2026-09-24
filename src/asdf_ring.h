// -*- mode: C; tab-width: 4 ; indent-tabs-mode: nil -*-
/**
 * @file asdf_ring.h
 *
 * A first-in first-out queue of keycodes in caller-provided storage.
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

#if !defined(ASDF_RING_H)
#define ASDF_RING_H

#include <stdint.h>
#include "asdf.h"

/**
 * A first-in first-out queue of keycodes held in caller-provided storage.
 *
 * Each ring is independent: operations on one ring never affect another.
 *
 * Invariants, after asdf_ring_init() and any sequence of operations:
 * - count <= capacity
 * - head < capacity, when capacity > 0
 * - the queued codes are storage[head] onward, wrapping at capacity
 * - dropped only increases, and saturates at 255
 *
 * @code
 * #include "asdf_ring.h"
 *
 * asdf_keycode_t storage[16];
 * asdf_ring_t ring;
 * asdf_keycode_t code;
 *
 * asdf_ring_init(&ring, storage, 16);
 * asdf_ring_put(&ring, 'a');
 * asdf_ring_put_pair(&ring, '\r', '\n');      // CR LF queued as a unit
 * while (asdf_ring_get(&ring, &code)) {
 *     // 'a', '\r', '\n', in order
 * }
 * @endcode
 */
typedef struct {
    asdf_keycode_t *storage;
    uint8_t capacity;
    uint8_t head;    ///< index of the next code to read
    uint8_t count;   ///< number of codes queued
    uint8_t dropped; ///< codes rejected because the ring was full (saturates)
} asdf_ring_t;

/**
 * Initialize an empty ring over caller-provided storage.
 *
 * Overwrites all of @p ring. On failure the ring has zero capacity, so every
 * put fails (and is counted as dropped) and every get finds it empty.
 *
 * @param ring      Ring to initialize.
 * @param storage   Array of at least @p capacity codes, owned by the caller
 *                  and valid for the life of the ring.
 * @param capacity  Number of codes the ring can hold.
 * @return 1 on success; 0 if @p storage is NULL or @p capacity is 0.
 */
uint8_t asdf_ring_init(asdf_ring_t *ring, asdf_keycode_t *storage, uint8_t capacity);

/**
 * Append a code.
 *
 * Modifies the ring's contents and count, or its drop count.
 *
 * @param ring  Ring to append to.
 * @param code  Code to queue.
 * @return 1 if queued; 0 if the ring was full, in which case the code is
 *         dropped and counted (see asdf_ring_dropped()).
 */
uint8_t asdf_ring_put(asdf_ring_t *ring, asdf_keycode_t code);

/**
 * Append two codes as a unit: both are queued, or neither is.
 *
 * Used for CR LF, so a full queue never leaves half a line ending. Modifies
 * the ring's contents and count, or its drop count.
 *
 * @param ring    Ring to append to.
 * @param first   First code.
 * @param second  Second code.
 * @return 1 if both were queued; 0 if the ring did not have room for both, in
 *         which case neither is queued and two drops are counted.
 */
uint8_t asdf_ring_put_pair(asdf_ring_t *ring, asdf_keycode_t first, asdf_keycode_t second);

/**
 * Remove the oldest code.
 *
 * Modifies the ring's head and count.
 *
 * @param ring  Ring to read from.
 * @param code  Receives the code; not written if the ring is empty.
 * @return 1 if a code was removed; 0 if the ring was empty.
 */
uint8_t asdf_ring_get(asdf_ring_t *ring, asdf_keycode_t *code);

/**
 * Number of codes queued.
 *
 * No side effects.
 *
 * @param ring  Ring to query.
 * @return Codes waiting to be read, 0 to capacity.
 */
uint8_t asdf_ring_count(const asdf_ring_t *ring);

/**
 * Number of codes dropped because the ring was full.
 *
 * No side effects.
 *
 * @param ring  Ring to query.
 * @return Codes rejected by asdf_ring_put() and asdf_ring_put_pair() since
 *         initialization, saturating at 255.
 */
uint8_t asdf_ring_dropped(const asdf_ring_t *ring);

#endif /* !defined (ASDF_RING_H) */

//-------|---------|---------+---------+---------+---------+---------+---------+
// Above line is 80 columns, and should display completely in the editor.
