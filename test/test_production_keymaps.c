// -*- mode: C; tab-width: 2 ; indent-tabs-mode: nil -*-
//
// Checks every production keymap (keymap_list.cmake) on the host: each one
// applies with no rejected descriptor entries, every modifier state has a
// matrix of the descriptor's dimensions, and each has an ID message.

#include <stdint.h>
#include "unity.h"
#include "asdf.h"
#include "asdf_arch.h"
#include "asdf_keyboard.h"
#include "asdf_keymap_setup.h"
#include "asdf_keymaps.h"
#include "fake_platform.h"

#define MAX_KEYMAPS 16

static asdf_t kb;
static fake_platform_t hw;

void setUp(void)
{
  fake_platform_init(&hw);
  asdf_init_r(&kb, &hw.platform);
}

void tearDown(void) {}

void production_keymaps_apply_cleanly(void)
{
  uint8_t num_keymaps = 0;

  for (uint8_t index = 0; index < MAX_KEYMAPS; index++) {
    const asdf_keymap_t *keymap = asdf_keymap_descriptor(index);
    if (!keymap) {
      continue;
    }
    num_keymaps++;
    asdf_keymaps_select_r(&kb, index);
    if (0 == index) {
      asdf_keymaps_switch_r(&kb, index); // already current after init
    }

    TEST_ASSERT_EQUAL_INT_MESSAGE(index, kb.keymap.current, "keymap not selected");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, asdf_keymap_errors_r(&kb), "keymap has errors");
    for (uint8_t m = 0; m < ASDF_MOD_NUM_MODIFIERS; m++) {
      TEST_ASSERT_NOT_NULL(kb.keymap.maps[m].matrix);
      TEST_ASSERT_EQUAL_INT(keymap->rows, kb.keymap.maps[m].rows);
      TEST_ASSERT_EQUAL_INT(keymap->cols, kb.keymap.maps[m].cols);
    }
    TEST_ASSERT_NOT_NULL(keymap->id_message);
  }
  TEST_ASSERT_TRUE(num_keymaps > 0);
}

int main(void)
{
  UNITY_BEGIN();
  RUN_TEST(production_keymaps_apply_cleanly);
  return UNITY_END();
}
