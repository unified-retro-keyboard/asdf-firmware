// -*- mode: C; tab-width: 2 ; indent-tabs-mode: nil -*-
//
// Unified Keyboard Project
// ASDF keyboard firmware
//
// asdf_ring.c
//
// A ring buffer of keycodes in caller-provided storage. Buffering provides an
// interface between the generation of keycodes and the hardware-level keycode
// transmission, which may occur at different rates.
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

#include <stddef.h>
#include <stdint.h>
#include "asdf_ring.h"

// Implementation Notes:
//
// 1) The ring holds no state outside the asdf_ring_t object, so independent
// rings can be used concurrently by different owners.
//
// 2) Indices wrap by comparison rather than by modulo, which avoids a division
// on 8-bit targets.

// PROCEDURE: asdf_ring_init
// INPUTS: (asdf_ring_t *) ring - the ring to initialize
//         (asdf_keycode_t *) storage - array of at least capacity codes
//         (uint8_t) capacity - number of codes the ring can hold
// OUTPUTS: returns TRUE (nonzero) on success, FALSE (0) if storage is NULL or
//          capacity is 0.
//
// DESCRIPTION: Initializes an empty ring using the given storage. On failure,
// the ring is set to zero capacity, so every put fails and every get finds it
// empty.
//
// SIDE EFFECTS: overwrites *ring
//
// SCOPE: public
//
// COMPLEXITY: 2
//
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

// PROCEDURE: asdf_ring_drop
// INPUTS: (asdf_ring_t *) ring, (uint8_t) n - number of codes dropped
// OUTPUTS: none
//
// DESCRIPTION: Adds n to the ring's dropped count, saturating at 255.
//
// SIDE EFFECTS: modifies ring->dropped
//
// SCOPE: private
//
// COMPLEXITY: 2
//
static void asdf_ring_drop(asdf_ring_t *ring, uint8_t n)
{
  ring->dropped = (ring->dropped > (uint8_t) (UINT8_MAX - n)) ? (uint8_t) UINT8_MAX
                                                              : (uint8_t) (ring->dropped + n);
}

// PROCEDURE: asdf_ring_store
// INPUTS: (asdf_ring_t *) ring, (asdf_keycode_t) code
// OUTPUTS: none
//
// DESCRIPTION: Appends a code to a ring known to have room for it.
//
// SIDE EFFECTS: modifies ring storage and count
//
// SCOPE: private
//
// COMPLEXITY: 2
//
static void asdf_ring_store(asdf_ring_t *ring, asdf_keycode_t code)
{
  uint8_t room_to_end = ring->capacity - ring->head;
  uint8_t tail = (ring->count < room_to_end) ? ring->head + ring->count
                                             : ring->count - room_to_end;

  ring->storage[tail] = code;
  ring->count++;
}

// PROCEDURE: asdf_ring_put
// INPUTS: (asdf_ring_t *) ring, (asdf_keycode_t) code
// OUTPUTS: returns TRUE (nonzero) if the code was queued, FALSE (0) if the ring
//          was full.
//
// DESCRIPTION: Appends a code. A code that does not fit is dropped and counted.
//
// SIDE EFFECTS: modifies ring state
//
// SCOPE: public
//
// COMPLEXITY: 2
//
uint8_t asdf_ring_put(asdf_ring_t *ring, asdf_keycode_t code)
{
  if (ring->count >= ring->capacity) {
    asdf_ring_drop(ring, 1);
    return 0;
  }
  asdf_ring_store(ring, code);
  return 1;
}

// PROCEDURE: asdf_ring_put_pair
// INPUTS: (asdf_ring_t *) ring, (asdf_keycode_t) first, (asdf_keycode_t) second
// OUTPUTS: returns TRUE (nonzero) if both codes were queued, FALSE (0) if the
//          ring did not have room for both.
//
// DESCRIPTION: Appends two codes as a unit: either both are queued or neither
// is. A pair that does not fit counts as two dropped codes.
//
// SIDE EFFECTS: modifies ring state
//
// SCOPE: public
//
// COMPLEXITY: 2
//
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

// PROCEDURE: asdf_ring_get
// INPUTS: (asdf_ring_t *) ring, (asdf_keycode_t *) code - receives the code
// OUTPUTS: returns TRUE (nonzero) if a code was removed, FALSE (0) if the ring
//          was empty (code is not written).
//
// DESCRIPTION: Removes the oldest code.
//
// SIDE EFFECTS: modifies ring state
//
// SCOPE: public
//
// COMPLEXITY: 3
//
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

// PROCEDURE: asdf_ring_count
// INPUTS: (const asdf_ring_t *) ring
// OUTPUTS: returns the number of codes queued.
//
// SCOPE: public
//
// COMPLEXITY: 1
//
uint8_t asdf_ring_count(const asdf_ring_t *ring)
{
  return ring->count;
}

// PROCEDURE: asdf_ring_dropped
// INPUTS: (const asdf_ring_t *) ring
// OUTPUTS: returns the number of codes dropped because the ring was full,
//          saturating at 255.
//
// SCOPE: public
//
// COMPLEXITY: 1
//
uint8_t asdf_ring_dropped(const asdf_ring_t *ring)
{
  return ring->dropped;
}

//-------|---------|---------+---------+---------+---------+---------+---------+
// Above line is 80 columns, and should display completely in the editor.
