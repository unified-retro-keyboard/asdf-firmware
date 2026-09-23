// -*- mode: C; tab-width: 4 ; indent-tabs-mode: nil -*-
//
// Unified Keyboard Project
// ASDF keyboard firmware
//
// asdf_ring.h
//
// Contains definitions and prototypes for the keycode ring buffer.
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

#if !defined(ASDF_RING_H)
#define ASDF_RING_H

#include <stdint.h>
#include "asdf.h"

// A first-in first-out queue of keycodes held in caller-provided storage. Each
// ring is independent: operations on one ring never affect another.
typedef struct {
    asdf_keycode_t *storage;
    uint8_t capacity;
    uint8_t head;    // index of the next code to read
    uint8_t count;   // number of codes queued
    uint8_t dropped; // codes rejected because the ring was full (saturates)
} asdf_ring_t;

// PROCEDURE: asdf_ring_init
// INPUTS: (asdf_ring_t *) ring - the ring to initialize
//         (asdf_keycode_t *) storage - array of at least capacity codes
//         (uint8_t) capacity - number of codes the ring can hold
// OUTPUTS: returns TRUE (nonzero) on success, FALSE (0) if storage is NULL or
//          capacity is 0.
// DESCRIPTION: Initializes an empty ring using the given storage. On failure,
// the ring is set to zero capacity, so every put fails and every get finds it
// empty.
uint8_t asdf_ring_init(asdf_ring_t *ring, asdf_keycode_t *storage, uint8_t capacity);

// PROCEDURE: asdf_ring_put
// INPUTS: (asdf_ring_t *) ring, (asdf_keycode_t) code
// OUTPUTS: returns TRUE (nonzero) if the code was queued, FALSE (0) if the ring
//          was full.
// DESCRIPTION: Appends a code. A code that does not fit is dropped and counted.
uint8_t asdf_ring_put(asdf_ring_t *ring, asdf_keycode_t code);

// PROCEDURE: asdf_ring_put_pair
// INPUTS: (asdf_ring_t *) ring, (asdf_keycode_t) first, (asdf_keycode_t) second
// OUTPUTS: returns TRUE (nonzero) if both codes were queued, FALSE (0) if the
//          ring did not have room for both.
// DESCRIPTION: Appends two codes as a unit: either both are queued or neither
// is. A pair that does not fit counts as two dropped codes.
uint8_t asdf_ring_put_pair(asdf_ring_t *ring, asdf_keycode_t first, asdf_keycode_t second);

// PROCEDURE: asdf_ring_get
// INPUTS: (asdf_ring_t *) ring, (asdf_keycode_t *) code - receives the code
// OUTPUTS: returns TRUE (nonzero) if a code was removed, FALSE (0) if the ring
//          was empty (code is not written).
// DESCRIPTION: Removes the oldest code.
uint8_t asdf_ring_get(asdf_ring_t *ring, asdf_keycode_t *code);

// PROCEDURE: asdf_ring_count
// INPUTS: (const asdf_ring_t *) ring
// OUTPUTS: returns the number of codes queued.
uint8_t asdf_ring_count(const asdf_ring_t *ring);

// PROCEDURE: asdf_ring_dropped
// INPUTS: (const asdf_ring_t *) ring
// OUTPUTS: returns the number of codes dropped because the ring was full,
//          saturating at 255.
uint8_t asdf_ring_dropped(const asdf_ring_t *ring);

#endif /* !defined (ASDF_RING_H) */

//-------|---------|---------+---------+---------+---------+---------+---------+
// Above line is 80 columns, and should display completely in the editor.
