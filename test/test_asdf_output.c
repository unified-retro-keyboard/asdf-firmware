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

asdf_cols_t asdf_arch_read_row(uint8_t row)
{
  (void) row;
  return 0;
}

void setUp(void)
{
  asdf_init(&asdf_arch_platform);
}

void tearDown(void) {}

// Verify that system messages (via asdf_putc) are delivered before queued
// keypresses, matching the documented behavior of asdf_next_code.
void test_message_buffer_has_priority_over_keycodes(void)
{
  // Queue a keycode first, then a message byte.
  asdf_put_code('k');
  asdf_putc('m', NULL);

  TEST_ASSERT_EQUAL_INT('m', asdf_next_code());
  TEST_ASSERT_EQUAL_INT('k', asdf_next_code());
  TEST_ASSERT_EQUAL_INT(ASDF_INVALID_CODE, asdf_next_code());
}

// Ensure the firmware's newline handling converts LF to CRLF when buffering
// system messages.
void test_putc_translates_newline_to_crlf(void)
{
  asdf_putc('\n', NULL);

  TEST_ASSERT_EQUAL_INT('\r', asdf_next_code());
  TEST_ASSERT_EQUAL_INT('\n', asdf_next_code());
  TEST_ASSERT_EQUAL_INT(ASDF_INVALID_CODE, asdf_next_code());
}

// After a message byte, output pauses for the print delay, counted in ticks;
// typed keycodes do not start a pause.
void test_print_delay_paces_output_after_messages(void)
{
  const uint8_t delay = 77;

  asdf_set_print_delay(delay);
  asdf_putc('x', NULL);
  asdf_put_code('y');
  asdf_put_code('z');

  TEST_ASSERT_EQUAL_INT('x', asdf_next_code());
  TEST_ASSERT_EQUAL_INT(ASDF_INVALID_CODE, asdf_next_code());

  asdf_tick(delay - 1);
  TEST_ASSERT_EQUAL_INT(ASDF_INVALID_CODE, asdf_next_code());

  asdf_tick(1);
  TEST_ASSERT_EQUAL_INT('y', asdf_next_code());
  TEST_ASSERT_EQUAL_INT('z', asdf_next_code());
  TEST_ASSERT_EQUAL_INT(ASDF_INVALID_CODE, asdf_next_code());
}

// Ticks elapsed beyond the pause are not carried over.
void test_print_delay_does_not_underflow(void)
{
  asdf_set_print_delay(5);
  asdf_putc('x', NULL);
  asdf_putc('y', NULL);

  TEST_ASSERT_EQUAL_INT('x', asdf_next_code());
  asdf_tick(200);
  TEST_ASSERT_EQUAL_INT('y', asdf_next_code());
  TEST_ASSERT_EQUAL_INT(ASDF_INVALID_CODE, asdf_next_code());
}

int main(void)
{
  UNITY_BEGIN();
  RUN_TEST(test_message_buffer_has_priority_over_keycodes);
  RUN_TEST(test_putc_translates_newline_to_crlf);
  RUN_TEST(test_print_delay_paces_output_after_messages);
  RUN_TEST(test_print_delay_does_not_underflow);
  return UNITY_END();
}
