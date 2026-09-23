// -*- mode: C; tab-width: 2 ; indent-tabs-mode: nil -*-
//
// Tests that the core reads the key matrix and sends codes only through the
// installed platform, using independent fake hardware.

#include <stdint.h>
#include "unity.h"
#include "asdf.h"
#include "asdf_config.h"
#include "asdf_platform.h"
#include "fake_platform.h"

// Key 'a' in the test keymap selected by asdf_init()
#define KEY_A_ROW 1
#define KEY_A_COL 6

static fake_platform_t fake_a, fake_b;

static void scan(int ticks)
{
  while (ticks-- > 0) {
    asdf_keyscan();
  }
}

static void send_pending_codes(void)
{
  asdf_keycode_t code;
  while (ASDF_INVALID_CODE != (code = asdf_next_code())) {
    asdf_send_code(code);
  }
}

void setUp(void)
{
  asdf_init();
  fake_platform_init(&fake_a);
  fake_platform_init(&fake_b);
}

void tearDown(void)
{
  asdf_install_platform(NULL);
}

void installing_null_selects_arch_platform(void)
{
  asdf_install_platform(&fake_a.platform);
  asdf_install_platform(NULL);
  TEST_ASSERT_EQUAL_PTR(&asdf_arch_platform, asdf_current_platform());
}

// Only the installed fake is scanned and receives output.
void scan_and_output_use_only_installed_platform(void)
{
  fake_platform_press(&fake_a, KEY_A_ROW, KEY_A_COL);
  fake_platform_press(&fake_b, KEY_A_ROW, KEY_A_COL);

  asdf_install_platform(&fake_a.platform);
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

  asdf_install_platform(&fake_a.platform);
  scan(ASDF_DEBOUNCE_TIME_MS);
  send_pending_codes();
  TEST_ASSERT_EQUAL_INT(0, fake_a.num_sent);

  asdf_install_platform(&fake_b.platform);
  scan(ASDF_DEBOUNCE_TIME_MS);
  send_pending_codes();
  TEST_ASSERT_EQUAL_INT(0, fake_a.num_sent);
  TEST_ASSERT_EQUAL_INT(1, fake_b.num_sent);
  TEST_ASSERT_EQUAL_INT('a', fake_b.sent[0]);
}

int main(void)
{
  UNITY_BEGIN();
  RUN_TEST(installing_null_selects_arch_platform);
  RUN_TEST(scan_and_output_use_only_installed_platform);
  RUN_TEST(switching_platform_switches_hardware);
  return UNITY_END();
}
