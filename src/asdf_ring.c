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
 * @copyright Copyright 2019 David F. MIT License; see LICENSE.
 */
// SPDX-License-Identifier: MIT

#include <stddef.h>
#include <stdbool.h>
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
 * @return true on success; false if @p storage is NULL or @p capacity is 0.
 *
 * Complexity: 4
 */
bool asdf_ring_init(asdf_ring_t *ring, asdf_keycode_t *storage, uint8_t capacity)
{
  bool valid = (NULL != storage) && (capacity > 0u);

  ring->storage = valid ? storage : NULL;
  ring->capacity = valid ? capacity : 0u;
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
  ring->dropped = (ring->dropped > (0xFFu - n)) ? 0xFFu : (uint8_t) (ring->dropped + n);
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
  uint8_t room_to_end = (uint8_t)(ring->capacity - ring->head);
  uint8_t tail = (ring->count < room_to_end) ? (uint8_t)(ring->head + ring->count)
                                             : (uint8_t)(ring->count - room_to_end);

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
 * @return true if the code was queued; false if the ring was full.
 *
 * Complexity: 2
 */
bool asdf_ring_put(asdf_ring_t *ring, asdf_keycode_t code)
{
  if (ring->count >= ring->capacity) {
    asdf_ring_drop(ring, 1);
    return false;
  }
  asdf_ring_store(ring, code);
  return true;
}

/**
 * Append two codes as a unit: either both are queued or neither is.
 *
 * A pair that does not fit counts as two dropped codes.
 *
 * @param ring    Ring to append to.
 * @param first   First code.
 * @param second  Second code.
 * @return true if both codes were queued; false if the ring did not have room
 *         for both.
 *
 * Complexity: 2
 */
bool asdf_ring_put_pair(asdf_ring_t *ring, asdf_keycode_t first, asdf_keycode_t second)
{
  uint8_t room = (uint8_t) (ring->capacity - ring->count);

  if (room < 2u) {
    asdf_ring_drop(ring, 2);
    return false;
  }
  asdf_ring_store(ring, first);
  asdf_ring_store(ring, second);
  return true;
}

/**
 * Remove the oldest code.
 *
 * Advances the head, wrapping at capacity, and decrements the count.
 *
 * @param ring  Ring to read from.
 * @param code  Receives the code; not written if the ring is empty.
 * @return true if a code was removed; false if the ring was empty.
 *
 * Complexity: 3
 */
bool asdf_ring_get(asdf_ring_t *ring, asdf_keycode_t *code)
{
  if (ring->count == 0u) {
    return false;
  }
  *code = ring->storage[ring->head];
  ring->head++;
  if (ring->head >= ring->capacity) {
    ring->head = 0;
  }
  ring->count--;
  return true;
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
