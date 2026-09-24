// -*- mode: C; tab-width: 2 ; indent-tabs-mode: nil -*-
//
// Output-path regression tests
//
// These tests focus on the public buffering APIs that ferry bytes from the
// firmware into host-visible streams. They exercise:
//   1. Message buffer priority vs. keycodes (`asdf_next_code`).
//   2. CRLF translation performed by `asdf_putc`.
//   3. Print-delay throttling via `asdf_set_print_delay`.
//
#include <stdint.h>
#include "unity.h"
#include "asdf.h"
#include "asdf_arch_test.h"
#include "asdf_keyboard.h"
#include "test_asdf_lib.h"

// The keyboard under test.
static asdf_t kb;

asdf_cols_t asdf_arch_read_row(uint8_t row)
{
  (void) row;
  return 0;
}

void setUp(void)
{
  asdf_init_r(&kb, &asdf_arch_platform);
}

void tearDown(void) {}

// Verify that system messages (via asdf_putc) are delivered before queued
// keypresses, matching the documented behavior of asdf_next_code.
void test_message_buffer_has_priority_over_keycodes(void)
{
  // Queue a keycode first, then a message byte.
  asdf_put_code_r(&kb, 'k');
  asdf_putc_r(&kb, 'm');

  TEST_ASSERT_EQUAL_INT('m', test_next_code(&kb));
  TEST_ASSERT_EQUAL_INT('k', test_next_code(&kb));
  TEST_ASSERT_EQUAL_INT(ASDF_INVALID_CODE, test_next_code(&kb));
}

// Ensure the firmware's newline handling converts LF to CRLF when buffering
// system messages.
void test_putc_translates_newline_to_crlf(void)
{
  asdf_putc_r(&kb, '\n');

  TEST_ASSERT_EQUAL_INT('\r', test_next_code(&kb));
  TEST_ASSERT_EQUAL_INT('\n', test_next_code(&kb));
  TEST_ASSERT_EQUAL_INT(ASDF_INVALID_CODE, test_next_code(&kb));
}

// After a message byte, output pauses for the print delay, counted in ticks;
// typed keycodes do not start a pause.
void test_print_delay_paces_output_after_messages(void)
{
  const uint8_t delay = 77;

  kb.print_delay_ms = delay;
  asdf_putc_r(&kb, 'x');
  asdf_put_code_r(&kb, 'y');
  asdf_put_code_r(&kb, 'z');

  TEST_ASSERT_EQUAL_INT('x', test_next_code(&kb));
  TEST_ASSERT_EQUAL_INT(ASDF_INVALID_CODE, test_next_code(&kb));

  asdf_tick_r(&kb, delay - 1);
  TEST_ASSERT_EQUAL_INT(ASDF_INVALID_CODE, test_next_code(&kb));

  asdf_tick_r(&kb, 1);
  TEST_ASSERT_EQUAL_INT('y', test_next_code(&kb));
  TEST_ASSERT_EQUAL_INT('z', test_next_code(&kb));
  TEST_ASSERT_EQUAL_INT(ASDF_INVALID_CODE, test_next_code(&kb));
}

// Ticks elapsed beyond the pause are not carried over.
void test_print_delay_does_not_underflow(void)
{
  kb.print_delay_ms = 5;
  asdf_putc_r(&kb, 'x');
  asdf_putc_r(&kb, 'y');

  TEST_ASSERT_EQUAL_INT('x', test_next_code(&kb));
  asdf_tick_r(&kb, 200);
  TEST_ASSERT_EQUAL_INT('y', test_next_code(&kb));
  TEST_ASSERT_EQUAL_INT(ASDF_INVALID_CODE, test_next_code(&kb));
}

// Codes that do not fit in a full queue are dropped and counted.
void test_full_queues_count_dropped_codes(void)
{
  TEST_ASSERT_EQUAL_INT(0, asdf_dropped_codes_r(&kb));
  for (int i = 0; i < ASDF_KEYCODE_BUFFER_SIZE + 3; i++) {
    asdf_put_code_r(&kb, 'x');
  }
  TEST_ASSERT_EQUAL_INT(3, asdf_dropped_codes_r(&kb));

  TEST_ASSERT_EQUAL_INT(0, asdf_dropped_messages_r(&kb));
  for (int i = 0; i < ASDF_MESSAGE_BUFFER_SIZE + 2; i++) {
    asdf_putc_r(&kb, 'y');
  }
  TEST_ASSERT_EQUAL_INT(2, asdf_dropped_messages_r(&kb));
}

int main(void)
{
  UNITY_BEGIN();
  RUN_TEST(test_message_buffer_has_priority_over_keycodes);
  RUN_TEST(test_putc_translates_newline_to_crlf);
  RUN_TEST(test_print_delay_paces_output_after_messages);
  RUN_TEST(test_print_delay_does_not_underflow);
  RUN_TEST(test_full_queues_count_dropped_codes);
  return UNITY_END();
}
