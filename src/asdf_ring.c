// -*- mode: C; tab-width: 2 ; indent-tabs-mode: nil -*-
/**
 * @file asdf_ring.c
 *
 * A ring buffer of keycodes in caller-provided storage. Buffering provides an
 * interface between the generation of keycodes and the hardware-level keycode
 * transmission, which may occur at different rates.
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

#include <stddef.h>
#include <stdint.h>
#include "asdf_ring.h"

// The ring holds no state outside its asdf_ring_t, so independent rings can be
// used by different owners. Indices wrap by comparison rather than by modulo,
// which avoids a division on 8-bit targets. The public contracts are in
// asdf_ring.h.

/**
 * Initialize an empty ring over caller-provided storage.
 *
 * Overwrites @p ring. On failure the ring is set to zero capacity, so every
 * put fails and every get finds it empty.
 *
 * @param ring      Ring to initialize.
 * @param storage   Array of at least @p capacity codes.
 * @param capacity  Number of codes the ring can hold.
 * @return 1 on success; 0 if @p storage is NULL or @p capacity is 0.
 *
 * Complexity: 4
 */
uint8_t asdf_ring_init(asdf_ring_t *ring, asdf_keycode_t *storage, uint8_t capacity)
{
  uint8_t valid = (NULL != storage) && (capacity > 0);

  ring->storage = valid ? storage : NULL;
  ring->capacity = valid ? capacity : 0;
  ring->head = 0;
  ring->count = 0;
  ring->dropped = 0;
  return valid;
}

/**
 * Count @p n dropped codes.
 *
 * Adds @p n to the ring's dropped count, saturating at 255.
 *
 * @param ring  Ring whose drop count to increase.
 * @param n     Codes dropped.
 *
 * Complexity: 2
 */
static void asdf_ring_drop(asdf_ring_t *ring, uint8_t n)
{
  ring->dropped = (ring->dropped > (uint8_t) (UINT8_MAX - n)) ? (uint8_t) UINT8_MAX
                                                              : (uint8_t) (ring->dropped + n);
}

/**
 * Append a code to a ring known to have room for it.
 *
 * Writes the code at the tail and increments the count.
 *
 * @param ring  Ring with count < capacity.
 * @param code  Code to store.
 *
 * The tail is head + count, wrapped once: count < capacity, so it wraps at
 * most one lap.
 *
 * Complexity: 2
 */
static void asdf_ring_store(asdf_ring_t *ring, asdf_keycode_t code)
{
  uint8_t room_to_end = ring->capacity - ring->head;
  uint8_t tail = (ring->count < room_to_end) ? ring->head + ring->count
                                             : ring->count - room_to_end;

  ring->storage[tail] = code;
  ring->count++;
}

/**
 * Append a code.
 *
 * A code that does not fit is dropped and counted.
 *
 * @param ring  Ring to append to.
 * @param code  Code to queue.
 * @return 1 if the code was queued; 0 if the ring was full.
 *
 * Complexity: 2
 */
uint8_t asdf_ring_put(asdf_ring_t *ring, asdf_keycode_t code)
{
  if (ring->count >= ring->capacity) {
    asdf_ring_drop(ring, 1);
    return 0;
  }
  asdf_ring_store(ring, code);
  return 1;
}

/**
 * Append two codes as a unit: either both are queued or neither is.
 *
 * A pair that does not fit counts as two dropped codes.
 *
 * @param ring    Ring to append to.
 * @param first   First code.
 * @param second  Second code.
 * @return 1 if both codes were queued; 0 if the ring did not have room for
 *         both.
 *
 * Complexity: 2
 */
uint8_t asdf_ring_put_pair(asdf_ring_t *ring, asdf_keycode_t first, asdf_keycode_t second)
{
  if (ring->capacity - ring->count < 2) {
    asdf_ring_drop(ring, 2);
    return 0;
  }
  asdf_ring_store(ring, first);
  asdf_ring_store(ring, second);
  return 1;
}

/**
 * Remove the oldest code.
 *
 * Advances the head, wrapping at capacity, and decrements the count.
 *
 * @param ring  Ring to read from.
 * @param code  Receives the code; not written if the ring is empty.
 * @return 1 if a code was removed; 0 if the ring was empty.
 *
 * Complexity: 3
 */
uint8_t asdf_ring_get(asdf_ring_t *ring, asdf_keycode_t *code)
{
  if (!ring->count) {
    return 0;
  }
  *code = ring->storage[ring->head];
  ring->head++;
  if (ring->head >= ring->capacity) {
    ring->head = 0;
  }
  ring->count--;
  return 1;
}

/**
 * Number of codes queued.
 *
 * No side effects.
 *
 * @param ring  Ring to query.
 * @return Codes waiting to be read, 0 to capacity.
 */
uint8_t asdf_ring_count(const asdf_ring_t *ring)
{
  return ring->count;
}

/**
 * Number of codes dropped because the ring was full.
 *
 * No side effects.
 *
 * @param ring  Ring to query.
 * @return Codes dropped since initialization, saturating at 255.
 */
uint8_t asdf_ring_dropped(const asdf_ring_t *ring)
{
  return ring->dropped;
}

//-------|---------|---------+---------+---------+---------+---------+---------+
// Above line is 80 columns, and should display completely in the editor.
