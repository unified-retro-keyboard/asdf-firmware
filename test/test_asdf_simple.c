// -*- mode: C; tab-width: 2 ; indent-tabs-mode: nil -*-
//
// Tests for the simple wrapper (asdf_simple.h), on the emulated hardware.

#include <stdint.h>
#include "unity.h"
#include "asdf.h"
#include "asdf_arch.h"
#include "asdf_config.h"
#include "asdf_simple.h"
#include "test_asdf_keymap_defs.h"

// Key 'a' in the test keymap selected by asdf_begin()
#define KEY_A_ROW 1
#define KEY_A_COL 6

static asdf_cols_t key_matrix[TEST_NUM_ROWS];

asdf_cols_t asdf_arch_read_row(uint8_t row)
{
  return (row < TEST_NUM_ROWS) ? key_matrix[row] : 0;
}

// Let ticks pass, polling after each.
static void run_ticks(int ticks)
{
  while (ticks-- > 0) {
    asdf_arch_test_tick_isr();
    asdf_poll();
  }
}

void setUp(void)
{
  for (int row = 0; row < TEST_NUM_ROWS; row++) {
    key_matrix[row] = 0;
  }
  asdf_begin();
}

void tearDown(void) {}

void nothing_available_after_begin(void)
{
  run_ticks(ASDF_DEBOUNCE_TIME_MS);
  TEST_ASSERT_FALSE(asdf_available());
  TEST_ASSERT_EQUAL_INT(0, asdf_read());
}

// A pressed key's code is available once debounced, and is read once.
void pressed_key_is_read_once(void)
{
  key_matrix[KEY_A_ROW] |= (1u << KEY_A_COL);
  run_ticks(ASDF_DEBOUNCE_TIME_MS - 1);
  TEST_ASSERT_FALSE(asdf_available());

  run_ticks(1);
  TEST_ASSERT_TRUE(asdf_available());
  TEST_ASSERT_TRUE(asdf_available()); // asking again does not consume it
  TEST_ASSERT_EQUAL_INT('a', asdf_read());
  TEST_ASSERT_FALSE(asdf_available());
}

// Ticks that pass between polls are caught up by the next poll.
void one_poll_catches_up_elapsed_ticks(void)
{
  key_matrix[KEY_A_ROW] |= (1u << KEY_A_COL);
  asdf_poll(); // first sees the key
  for (int t = 0; t < ASDF_DEBOUNCE_TIME_MS; t++) {
    asdf_arch_test_tick_isr();
  }
  asdf_poll();
  TEST_ASSERT_EQUAL_INT('a', asdf_read());
}

// The wrapper only queues codes: nothing is sent through the platform.
void codes_are_not_sent_through_the_platform(void)
{
  key_matrix[KEY_A_ROW] |= (1u << KEY_A_COL);
  run_ticks(ASDF_DEBOUNCE_TIME_MS);
  TEST_ASSERT_FALSE(asdf_arch_was_code_sent());
  TEST_ASSERT_EQUAL_INT('a', asdf_read());
}

int main(void)
{
  UNITY_BEGIN();
  RUN_TEST(nothing_available_after_begin);
  RUN_TEST(pressed_key_is_read_once);
  RUN_TEST(one_poll_catches_up_elapsed_ticks);
  RUN_TEST(codes_are_not_sent_through_the_platform);
  return UNITY_END();
}
