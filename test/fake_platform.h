// -*- mode: C; tab-width: 2 ; indent-tabs-mode: nil -*-
//
// Fake hardware for host tests. Each fake_platform_t is independent: it holds
// its own key matrix, outputs, and strobe polarity, and records the codes sent
// through it, and its asdf_platform_t passes the fake as the platform context.

#if !defined(FAKE_PLATFORM_H)
#define FAKE_PLATFORM_H

#include <stdint.h>
#include "asdf.h"
#include "asdf_platform.h"

#define FAKE_PLATFORM_ROWS     ASDF_MAX_ROWS
#define FAKE_PLATFORM_MAX_SENT 64

typedef struct {
  asdf_platform_t platform; // install this; its user pointer is the fake
  asdf_cols_t matrix[FAKE_PLATFORM_ROWS];
  asdf_keycode_t sent[FAKE_PLATFORM_MAX_SENT];
  uint8_t num_sent;
  uint32_t rows_read;
  uint8_t outputs[ASDF_PHYSICAL_NUM_RESOURCES]; // value last driven on each output
  uint8_t strobe_positive;                      // strobe polarity
  uint8_t short_pulses;                         // short pulse delays waited
  uint8_t resets;                               // platform resets
} fake_platform_t;

void fake_platform_init(fake_platform_t *fake);
void fake_platform_press(fake_platform_t *fake, uint8_t row, uint8_t col);
void fake_platform_release(fake_platform_t *fake, uint8_t row, uint8_t col);

#endif
