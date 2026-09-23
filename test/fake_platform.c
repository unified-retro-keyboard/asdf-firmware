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

static void fake_set_output(void *user, asdf_physical_dev_t output, uint8_t value)
{
  fake_platform_t *fake = user;

  if (output < ASDF_PHYSICAL_NUM_RESOURCES) {
    fake->outputs[output] = value;
  }
}

static void fake_set_strobe_polarity(void *user, uint8_t positive)
{
  fake_platform_t *fake = user;

  fake->strobe_positive = positive ? 1 : 0;
}

static void fake_pulse_delay_short(void *user)
{
  fake_platform_t *fake = user;

  fake->short_pulses++;
}

static void fake_reset(void *user)
{
  fake_platform_t *fake = user;

  fake->strobe_positive = 0;
  fake->resets++;
}

void fake_platform_init(fake_platform_t *fake)
{
  memset(fake, 0, sizeof(*fake));
  fake->platform.user = fake;
  fake->platform.read_row = fake_read_row;
  fake->platform.send_code = fake_send_code;
  fake->platform.set_output = fake_set_output;
  fake->platform.set_strobe_polarity = fake_set_strobe_polarity;
  fake->platform.pulse_delay_short = fake_pulse_delay_short;
  fake->platform.reset = fake_reset;
}

void fake_platform_press(fake_platform_t *fake, uint8_t row, uint8_t col)
{
  fake->matrix[row] |= (asdf_cols_t) (1u << col);
}

void fake_platform_release(fake_platform_t *fake, uint8_t row, uint8_t col)
{
  fake->matrix[row] &= (asdf_cols_t) ~(1u << col);
}
