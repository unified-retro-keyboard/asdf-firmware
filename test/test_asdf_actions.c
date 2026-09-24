// -*- mode: C; tab-width: 2 ; indent-tabs-mode: nil -*-
//
// DIP/action regression tests
//
// Exercises the action codes bound to the DIP switch row to ensure that
// architecture hooks (strobe polarity) and repeat-mode toggles are invoked on
// key press and reverted on release.

#include <stdint.h>
#include "unity.h"
#include "asdf.h"
#include "asdf_arch_test.h"
#include "asdf_keymaps.h"
#include "test_asdf_keymap_defs.h"
#include "asdf_repeat.h"
#include "asdf_keyboard.h"
#include "test_asdf_lib.h"

// The keyboard under test.
static asdf_t kb;

#define DIP_ROW_INDEX (TEST_NUM_ROWS - 1)
#define STROBE_COL 6
#define AUTOREPEAT_COL 7

static uint32_t key_matrix[TEST_NUM_ROWS];

static void keyscan_delay(int32_t ticks)
{
  while (ticks--) {
    asdf_keyscan(&kb);
  }
}

asdf_cols_t asdf_arch_read_row(uint8_t row)
{
  return key_matrix[row];
}

static void drive_dip(uint8_t col, uint8_t pressed)
{
  if (pressed) {
    key_matrix[DIP_ROW_INDEX] |= (1u << col);
  } else {
    key_matrix[DIP_ROW_INDEX] &= ~(1u << col);
  }
  keyscan_delay(ASDF_DEBOUNCE_TIME_MS);
}

static void press_dip(uint8_t col)
{
  drive_dip(col, 1);
}

static void release_dip(uint8_t col)
{
  drive_dip(col, 0);
}

void setUp(void)
{
  asdf_init(&kb, &asdf_arch_platform);
  for (uint32_t i = 0; i < TEST_NUM_ROWS; i++) {
    key_matrix[i] = 0;
  }
}

void tearDown(void) {}

void test_dip_strobe_action_toggles_polarity(void)
{
  TEST_ASSERT_FALSE(asdf_arch_is_strobe_positive());

  press_dip(STROBE_COL);
  TEST_ASSERT_TRUE(asdf_arch_is_strobe_positive());

  release_dip(STROBE_COL);
  TEST_ASSERT_FALSE(asdf_arch_is_strobe_positive());
}

void test_dip_autorepeat_action_toggles_mode(void)
{
  asdf_repeat_auto_off(&kb.repeat);
  TEST_ASSERT_FALSE(asdf_repeat_is_autorepeat_enabled(&kb.repeat));

  press_dip(AUTOREPEAT_COL);
  TEST_ASSERT_TRUE(asdf_repeat_is_autorepeat_enabled(&kb.repeat));

  release_dip(AUTOREPEAT_COL);
  TEST_ASSERT_FALSE(asdf_repeat_is_autorepeat_enabled(&kb.repeat));
}

void test_keymap_switch_reapplies_dip_actions(void)
{
  asdf_repeat_auto_off(&kb.repeat);
  TEST_ASSERT_FALSE(asdf_repeat_is_autorepeat_enabled(&kb.repeat));
  TEST_ASSERT_FALSE(asdf_arch_is_strobe_positive());

  press_dip(STROBE_COL);
  press_dip(AUTOREPEAT_COL);

  TEST_ASSERT_TRUE(asdf_arch_is_strobe_positive());
  TEST_ASSERT_TRUE(asdf_repeat_is_autorepeat_enabled(&kb.repeat));

  // Switching keymaps should reapply any dip-action state
  asdf_keymaps_select(&kb, ASDF_TEST_CAPS_MAP_INDEX);

  TEST_ASSERT_TRUE(asdf_arch_is_strobe_positive());
  TEST_ASSERT_TRUE(asdf_repeat_is_autorepeat_enabled(&kb.repeat));

  release_dip(STROBE_COL);
  release_dip(AUTOREPEAT_COL);
}

// SHIFTLOCK_TOGGLE locks SHIFT on the first press and unlocks it on the next.
void test_shiftlock_toggle_action_toggles_lock(void)
{
  TEST_ASSERT_FALSE(asdf_modifier_shift_locked(&kb.modifiers));
  asdf_action(&kb, ACTION_SHIFTLOCK_TOGGLE, 0);
  TEST_ASSERT_TRUE(asdf_modifier_shift_locked(&kb.modifiers));
  asdf_action(&kb, ACTION_SHIFTLOCK_TOGGLE, 0);
  TEST_ASSERT_FALSE(asdf_modifier_shift_locked(&kb.modifiers));
}

int main(void)
{
  UNITY_BEGIN();
  RUN_TEST(test_dip_strobe_action_toggles_polarity);
  RUN_TEST(test_dip_autorepeat_action_toggles_mode);
  RUN_TEST(test_keymap_switch_reapplies_dip_actions);
  RUN_TEST(test_shiftlock_toggle_action_toggles_lock);
  return UNITY_END();
}
