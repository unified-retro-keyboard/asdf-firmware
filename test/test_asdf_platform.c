// -*- mode: C; tab-width: 2 ; indent-tabs-mode: nil -*-
//
// Tests that the core reads the key matrix and sends codes only through the
// installed platform, using independent fake hardware.

#include <stdint.h>
#include "unity.h"
#include "asdf.h"
#include "asdf_config.h"
#include "asdf_arch.h"
#include "asdf_platform.h"
#include "fake_platform.h"
#include "asdf_keyboard.h"
#include "test_asdf_lib.h"

// The keyboard under test.
static asdf_t kb;

// Key 'a' in the test keymap selected by asdf_init_r(&kb)
#define KEY_A_ROW 1
#define KEY_A_COL 6

static fake_platform_t fake_a, fake_b;

static void scan(int ticks)
{
  while (ticks-- > 0) {
    asdf_keyscan_r(&kb);
  }
}

static void send_pending_codes(void)
{
  uint16_t code;
  while (ASDF_INVALID_CODE != (code = test_next_code(&kb))) {
    asdf_send_code_r(&kb, (asdf_keycode_t) code);
  }
}

void setUp(void)
{
  asdf_init_r(&kb, &asdf_arch_platform);
  fake_platform_init(&fake_a);
  fake_platform_init(&fake_b);
}

void tearDown(void)
{
  asdf_install_platform_r(&kb, NULL);
}

void installing_null_selects_arch_platform(void)
{
  asdf_install_platform_r(&kb, &fake_a.platform);
  asdf_install_platform_r(&kb, NULL);
  TEST_ASSERT_EQUAL_PTR(&asdf_arch_platform, kb.platform);
}

// Only the installed fake is scanned and receives output.
void scan_and_output_use_only_installed_platform(void)
{
  fake_platform_press(&fake_a, KEY_A_ROW, KEY_A_COL);
  fake_platform_press(&fake_b, KEY_A_ROW, KEY_A_COL);

  asdf_install_platform_r(&kb, &fake_a.platform);
  scan(ASDF_DEBOUNCE_TIME_MS);
  send_pending_codes();

  TEST_ASSERT_TRUE(fake_a.rows_read > 0);
  TEST_ASSERT_EQUAL_UINT32(0, fake_b.rows_read);
  TEST_ASSERT_EQUAL_INT(1, fake_a.num_sent);
  TEST_ASSERT_EQUAL_INT('a', fake_a.sent[0]);
  TEST_ASSERT_EQUAL_INT(0, fake_b.num_sent);
}

// Switching platforms switches which fake hardware is read and written.
void switching_platform_switches_hardware(void)
{
  fake_platform_press(&fake_b, KEY_A_ROW, KEY_A_COL);

  asdf_install_platform_r(&kb, &fake_a.platform);
  scan(ASDF_DEBOUNCE_TIME_MS);
  send_pending_codes();
  TEST_ASSERT_EQUAL_INT(0, fake_a.num_sent);

  asdf_install_platform_r(&kb, &fake_b.platform);
  scan(ASDF_DEBOUNCE_TIME_MS);
  send_pending_codes();
  TEST_ASSERT_EQUAL_INT(0, fake_a.num_sent);
  TEST_ASSERT_EQUAL_INT(1, fake_b.num_sent);
  TEST_ASSERT_EQUAL_INT('a', fake_b.sent[0]);
}

// asdf_process_r(&kb) catches up elapsed ticks in one call: a key held for the
// debounce time registers within a single call, and is sent through the
// platform.
void process_catches_up_elapsed_ticks(void)
{
  asdf_install_platform_r(&kb, &fake_a.platform);
  fake_platform_press(&fake_a, KEY_A_ROW, KEY_A_COL);

  asdf_process_r(&kb, ASDF_DEBOUNCE_TIME_MS);
  asdf_process_r(&kb, 1); // send the code queued by the last scan
  TEST_ASSERT_EQUAL_INT(1, fake_a.num_sent);
  TEST_ASSERT_EQUAL_INT('a', fake_a.sent[0]);
}

// Message pacing does not block scanning: a key pressed while output is
// paused registers on time, and is sent when the pause ends.
void scanning_continues_during_message_pacing(void)
{
  const uint8_t delay = 40;

  asdf_install_platform_r(&kb, &fake_a.platform);
  kb.print_delay_ms = delay;
  asdf_putc_r(&kb, 'm');
  fake_platform_press(&fake_a, KEY_A_ROW, KEY_A_COL);

  asdf_process_r(&kb, ASDF_DEBOUNCE_TIME_MS + 1);
  TEST_ASSERT_EQUAL_INT(1, fake_a.num_sent); // only 'm'; output paused
  TEST_ASSERT_EQUAL_INT('m', fake_a.sent[0]);

  asdf_process_r(&kb, delay);
  TEST_ASSERT_EQUAL_INT(2, fake_a.num_sent);
  TEST_ASSERT_EQUAL_INT('a', fake_a.sent[1]);
}

int main(void)
{
  UNITY_BEGIN();
  RUN_TEST(installing_null_selects_arch_platform);
  RUN_TEST(scan_and_output_use_only_installed_platform);
  RUN_TEST(switching_platform_switches_hardware);
  RUN_TEST(process_catches_up_elapsed_ticks);
  RUN_TEST(scanning_continues_during_message_pacing);
  return UNITY_END();
}
