// -*- mode: C; tab-width: 2 ; indent-tabs-mode: nil -*-
//
// Fake hardware for host tests; see fake_platform.h.

#include <string.h>
#include "fake_platform.h"

static asdf_cols_t fake_read_row(void *user, uint8_t row)
{
  fake_platform_t *fake = user;

  fake->rows_read++;
  return (row < FAKE_PLATFORM_ROWS) ? fake->matrix[row] : 0;
}

static void fake_send_code(void *user, asdf_keycode_t code)
{
  fake_platform_t *fake = user;

  if (fake->num_sent < FAKE_PLATFORM_MAX_SENT) {
    fake->sent[fake->num_sent++] = code;
  }
}

void fake_platform_init(fake_platform_t *fake)
{
  memset(fake, 0, sizeof(*fake));
  fake->platform.user = fake;
  fake->platform.read_row = fake_read_row;
  fake->platform.send_code = fake_send_code;
}

void fake_platform_press(fake_platform_t *fake, uint8_t row, uint8_t col)
{
  fake->matrix[row] |= (asdf_cols_t) (1u << col);
}

void fake_platform_release(fake_platform_t *fake, uint8_t row, uint8_t col)
{
  fake->matrix[row] &= (asdf_cols_t) ~(1u << col);
}
