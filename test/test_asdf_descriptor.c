// -*- mode: C; tab-width: 2 ; indent-tabs-mode: nil -*-
//
// Tests for the keymap descriptor's optional platform and each-scan action.

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "unity.h"
#include "asdf.h"
#include "asdf_arch_test.h"
#include "asdf_ascii.h"
#include "asdf_modifiers.h"
#include "asdf_keymaps.h"
#include "asdf_repeat.h"
#include "asdf_platform.h"
#include "test_asdf_lib.h"
#include "test_asdf_keymap_defs.h"

// emulates the arch row reader, to see that the platform's read_row is called.
asdf_cols_t asdf_arch_read_row(uint8_t row)
{
  return (asdf_keycode_t) (row + 1);
}


void setUp(void)
{
  test_hook_clear();
  asdf_init(&asdf_arch_platform);

  asdf_keymaps_select(ASDF_TEST_DEFAULT_SCANNER_MAP);
}

void tearDown(void) {}


void test_default_platform_is_arch_platform(void)
{
  const asdf_platform_t *p = asdf_current_platform();

  TEST_ASSERT_EQUAL_PTR(&asdf_arch_platform, p);
  TEST_ASSERT_EQUAL_INT((int) asdf_arch_read_row(100), (int) p->read_row(p->user, 100));
}

void test_keymap_can_install_platform(void)
{
  asdf_keymaps_select(ASDF_TEST_ALTERNATE_SCANNER_MAP);
  const asdf_platform_t *p = asdf_current_platform();

  TEST_ASSERT_EQUAL_PTR(&test_alt_platform, p);
  TEST_ASSERT_EQUAL_INT((int) test_hook_read_row(100), (int) p->read_row(p->user, 100));

  asdf_send_code(0x42);
  TEST_ASSERT_EQUAL_INT(0x42, (int) test_hook_readback());
}

void test_keymap_switch_restores_arch_platform(void)
{
  asdf_keymaps_select(ASDF_TEST_ALTERNATE_SCANNER_MAP);
  asdf_keymaps_select(ASDF_TEST_DEFAULT_SCANNER_MAP);
  TEST_ASSERT_EQUAL_PTR(&asdf_arch_platform, asdf_current_platform());
}

#define NUM_SCAN_TEST_REPS 101

void test_each_scan_action_runs_each_scan(void)
{

  test_hook_clear();
  asdf_keymaps_select(ASDF_TEST_EACH_SCAN_MAP);
  TEST_ASSERT_EQUAL_INT(0, test_hook_readback());
  for (int i = 0; i < NUM_SCAN_TEST_REPS; i++) {
    asdf_keyscan();
  }
  TEST_ASSERT_EQUAL_INT(NUM_SCAN_TEST_REPS, test_hook_readback());
}



int main(void)
{
  UNITY_BEGIN();
  RUN_TEST(test_default_platform_is_arch_platform);
  RUN_TEST(test_keymap_can_install_platform);
  RUN_TEST(test_keymap_switch_restores_arch_platform);
  RUN_TEST(test_each_scan_action_runs_each_scan);
  return UNITY_END();
}
