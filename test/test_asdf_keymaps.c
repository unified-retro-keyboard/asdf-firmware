#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include "asdf_arch.h"
#include "unity.h"
#include "asdf.h"
#include "asdf_ascii.h"
#include "asdf_modifiers.h"
#include "asdf_keymap_setup.h"
#include "test_asdf_keymap_defs.h"
#include "asdf_keymaps.h"
#include "test_asdf_lib.h"
#include "test_keymaps.h"
#include "asdf_keyboard.h"

// The keyboard under test.
static asdf_t kb;

#define TESTALPHA 'a'
#define TESTNUM '2'
#define TESTKEYMAP_TAG PLAIN_MATRIX_1
#define NUM_DIPSWITCHES 4

// row: row number
// col: column number
// keymap_name: id of the file defining the matrix to be used.
// defnum: numerical order of the file defining the matrix to be used.
// mapindex: matrix of keymaps to be used (contains all modifier maps for the matrix).
// modifier_name: name of the modifier to be accessed within the map.
#define TESTMAP(row, col, keymap_name, defnum, mapindex, modifier_name)                            \
  do {                                                                                             \
    asdf_keymaps_select(&kb, ASDF_##mapindex##_MAP_INDEX);                                       \
    uint16_t expected = test_key_value(keymap_name##_##modifier_name##_matrix[(row)][(col)]);    \
    uint16_t result = test_get_code(&kb, (row), (col), MOD_##modifier_name##_MAP);        \
    uint16_t map_id = test_get_code(&kb, 0, 0, MOD_##modifier_name##_MAP);                \
    TEST_ASSERT_EQUAL_INT32((uint32_t) expected, (uint32_t) result);                               \
    TEST_ASSERT_EQUAL_INT32((uint32_t) modifier_name##_MATRIX_##defnum, (uint32_t) map_id);        \
  } while (0)

#define TEST0MAP(row, col, modifier) TESTMAP(row, col, test, 1, TEST_PLAIN, modifier)
#define TEST1MAP(row, col, modifier) TESTMAP(row, col, test, 1, TEST_CAPS, modifier)
#define TEST2MAP(row, col, modifier) TESTMAP(row, col, test2, 2, TEST2_PLAIN, modifier)
#define TEST3MAP(row, col, modifier) TESTMAP(row, col, test2, 2, TEST2_CAPS, modifier)


#define TEST_VALID_CODE(position)                                                                  \
  do {                                                                                             \
    coord_t pos = position;                                                                        \
    TEST_ASSERT_FALSE(pos.row == -1);                                                              \
    TEST_ASSERT_FALSE(pos.col == -1);                                                              \
  } while (0)

// check against the "test" keymaps
#define TEST0PLAIN(row, col) TEST0MAP((row), (col), PLAIN)
#define TEST0SHIFT(row, col) TEST0MAP((row), (col), SHIFT)
#define TEST0CAPS(row, col) TEST0MAP((row), (col), CAPS)
#define TEST0CTRL(row, col) TEST0MAP((row), (col), CTRL)

#define TEST1PLAIN(row, col) TEST1MAP((row), (col), PLAIN)
#define TEST1SHIFT(row, col) TEST1MAP((row), (col), SHIFT)
#define TEST1CAPS(row, col) TEST1MAP((row), (col), CAPS)
#define TEST1CTRL(row, col) TEST1MAP((row), (col), CTRL)

// check against the "test2" keymaps

#define TEST2PLAIN(row, col) TEST2MAP((row), (col), PLAIN)
#define TEST2SHIFT(row, col) TEST2MAP((row), (col), SHIFT)
#define TEST2CAPS(row, col) TEST2MAP((row), (col), CAPS)
#define TEST2CTRL(row, col) TEST2MAP((row), (col), CTRL)

#define TEST3PLAIN(row, col) TEST3MAP((row), (col), PLAIN)
#define TEST3SHIFT(row, col) TEST3MAP((row), (col), SHIFT)
#define TEST3CAPS(row, col) TEST3MAP((row), (col), CAPS)
#define TEST3CTRL(row, col) TEST3MAP((row), (col), CTRL)

typedef struct {
  int32_t row;
  int32_t col;
} coord_t;

// The dip switch positions do not need to reflect real hardware. These
// positions reflect the organization of the test keymaps, to ensure that tools
// used to place the codes are functioning and are being properly used.


// keymap coordinates for special functions
static coord_t alpha_sample;
static coord_t num_sample;
static coord_t keymap_tag;

coord_t *find_code(uint16_t code)
{
  uint32_t done = 0;
  static coord_t location = { .row = -1, .col = -1 };

  for (uint32_t row = 0; !done && (row < TEST_NUM_ROWS); row++) {
    for (uint32_t col = 0; !done && (col < TEST_NUM_COLS); col++) {
      if (test_key_value(test_PLAIN_matrix[row][col]) == code) {
        done = 1;
        location.row = row;
        location.col = col;
      }
    }
  }
  return &location;
}


void setUp(void)
{
  coord_t *temp;

  asdf_init(&kb, &asdf_arch_platform);
  asdf_keymaps_select(&kb, ASDF_TEST_PLAIN_MAP_INDEX);

  temp = find_code(TESTALPHA);
  alpha_sample = *temp;

  temp = find_code(TESTNUM);
  num_sample = *temp;

  temp = find_code(TESTKEYMAP_TAG);
  keymap_tag = *temp;

}

void tearDown(void) {}


// set a keymap using the keymap select (DIP switch) actions
void complicated_set_keymap(uint8_t mapnum)
{
  for (uint8_t i = 0; i < NUM_DIPSWITCHES; i++) {
    if (mapnum & 1) {
      asdf_action_mapsel_set(&kb, i);
    }
    else {
      asdf_action_mapsel_clear(&kb, i);
    }
    mapnum >>= 1;
  }

  // The select actions only request a keymap; the end of a scan applies it.
  asdf_keymaps_apply_request(&kb);
}

// dummy row reader: no keys pressed.
asdf_cols_t asdf_arch_read_row(uint8_t row)
{
  (void) row;
  return 0;
}

void test_chars_are_in_map(void)
{
  TEST_VALID_CODE(alpha_sample);
  TEST_VALID_CODE(num_sample);
}

void keymap0_plain_gives_plain_values(void)
{

  asdf_keymaps_select(&kb, ASDF_TEST_PLAIN_MAP_INDEX);

  TEST0PLAIN(alpha_sample.row, alpha_sample.col);
  TEST0PLAIN(num_sample.row, num_sample.col);
}

void keymap0_shift_gives_shift_values(void)
{
  TEST0SHIFT(alpha_sample.row, alpha_sample.col);
  TEST0SHIFT(num_sample.row, num_sample.col);
}
void keymap0_caps_gives_caps_values(void)
{
  TEST0CAPS(alpha_sample.row, alpha_sample.col);
  TEST0CAPS(num_sample.row, num_sample.col);
}
void keymap0_ctrl_gives_ctrl_values(void)
{
  TEST0CTRL(alpha_sample.row, alpha_sample.col);
  TEST0CTRL(num_sample.row, num_sample.col);
}

void keymap2_plain_gives_plain_values(void)
{
  TEST2PLAIN(alpha_sample.row, alpha_sample.col);
  TEST2PLAIN(num_sample.row, num_sample.col);
}

void keymap2_shift_gives_shift_values(void)
{
  TEST2SHIFT(alpha_sample.row, alpha_sample.col);
  TEST2SHIFT(num_sample.row, num_sample.col);
}
void keymap2_caps_gives_caps_values(void)
{
  TEST2CAPS(alpha_sample.row, alpha_sample.col);
  TEST2CAPS(num_sample.row, num_sample.col);
}
void keymap2_ctrl_gives_ctrl_values(void)
{
  TEST2CTRL(alpha_sample.row, alpha_sample.col);
  TEST2CTRL(num_sample.row, num_sample.col);
}


void keymap1_capsmap_plain_maps_to_caps(void)
{
  // set bit 0 to select keymap 1
  asdf_keymaps_request_bit(&kb.keymap, ASDF_KEYMAP_BIT_0, 1);
  TEST1CAPS(alpha_sample.row, alpha_sample.col);
  TEST1CAPS(num_sample.row, num_sample.col);
}

void dip_switch_codes_are_in_last_row_test1_map(void)
{
  coord_t dip_switches[NUM_DIPSWITCHES] = { { .row = (TEST_NUM_ROWS - 1), .col = 0 },
                                            { .row = (TEST_NUM_ROWS - 1), .col = 1 },
                                            { .row = (TEST_NUM_ROWS - 1), .col = 2 },
                                            { .row = (TEST_NUM_ROWS - 1), .col = 3 } };
  for (uint8_t i = 0; i < NUM_DIPSWITCHES; i++) {
    asdf_key_t key =
      asdf_keymaps_get_key(&kb.keymap, dip_switches[i].row, dip_switches[i].col,
                             ASDF_TEST_PLAIN_MAP_INDEX);
    TEST_ASSERT_EQUAL_INT(ACTION_MAPSEL_SET, key.press_fn);
    TEST_ASSERT_EQUAL_INT(i, key.press_param);
    TEST_ASSERT_EQUAL_INT(ACTION_MAPSEL_CLEAR, key.release_fn);
    TEST_ASSERT_EQUAL_INT(i, key.release_param);
  }
}

void dip_switch_codes_are_in_last_row_test2_map(void)
{
  coord_t dip_switches[NUM_DIPSWITCHES] = { { .row = (TEST_NUM_ROWS - 1), .col = 0 },
                                            { .row = (TEST_NUM_ROWS - 1), .col = 1 },
                                            { .row = (TEST_NUM_ROWS - 1), .col = 2 },
                                            { .row = (TEST_NUM_ROWS - 1), .col = 3 } };
  for (uint8_t i = 0; i < NUM_DIPSWITCHES; i++) {
    asdf_key_t key =
      asdf_keymaps_get_key(&kb.keymap, dip_switches[i].row, dip_switches[i].col,
                             ASDF_TEST2_PLAIN_MAP_INDEX);
    TEST_ASSERT_EQUAL_INT(ACTION_MAPSEL_SET, key.press_fn);
    TEST_ASSERT_EQUAL_INT(i, key.press_param);
    TEST_ASSERT_EQUAL_INT(ACTION_MAPSEL_CLEAR, key.release_fn);
    TEST_ASSERT_EQUAL_INT(i, key.release_param);
  }
}

void dip_switch_properly_sets_bits(void)
{
  for (uint8_t i = 0; i < ASDF_NUM_KEYMAPS; i++) {
    uint16_t expected;
    uint16_t result;
    asdf_keymaps_select(&kb, i);
    expected = test_get_code(&kb, keymap_tag.row, keymap_tag.col, MOD_PLAIN_MAP);

    // set all keymap bits to '0'
    asdf_keymaps_select(&kb, 0);
    complicated_set_keymap(i);
    result = test_get_code(&kb, keymap_tag.row, keymap_tag.col, MOD_PLAIN_MAP);

    TEST_ASSERT_EQUAL_INT32(expected, result);
  }
}


void dip_switch_properly_clears_bits(void)
{
  uint8_t mask = 0;
  uint8_t next = 1;

  // calculate word with most 1's less than (or equal to) ASDF_NUM_KEYMAPS
  while (next < ASDF_NUM_KEYMAPS) {
    mask = next;
    next = (next << 1) | 1;
  }
  for (uint8_t i = 0; i < ASDF_NUM_KEYMAPS; i++) {
    uint16_t expected;
    uint16_t result;
    asdf_keymaps_select(&kb, i);
    expected = test_get_code(&kb, keymap_tag.row, keymap_tag.col, MOD_PLAIN_MAP);

    // set as many keymap bits to '1' as possible.
    asdf_keymaps_select(&kb, mask);
    complicated_set_keymap(i);
    result = test_get_code(&kb, keymap_tag.row, keymap_tag.col, MOD_PLAIN_MAP);
    TEST_ASSERT_EQUAL_INT32(expected, result);
  }
}


void dip_switch_invalid_keymap_has_no_effect(void)
{
  uint16_t map_id;

  // First, assert that changing to matrix 2 works:
  asdf_keymaps_select(&kb, ASDF_TEST2_PLAIN_MAP_INDEX);
  map_id = test_get_code(&kb, keymap_tag.row, keymap_tag.col, MOD_PLAIN_MAP);
  TEST_ASSERT_EQUAL_INT32(PLAIN_MATRIX_2, map_id);

  // assert that resetting keymap to 0 works:
  asdf_keymaps_select(&kb, 0);
  map_id = test_get_code(&kb, keymap_tag.row, keymap_tag.col, MOD_PLAIN_MAP);
  TEST_ASSERT_EQUAL_INT32(PLAIN_MATRIX_1, map_id);

  // selecting one above the highest keymap should have no effect
  asdf_keymaps_select(&kb, ASDF_NUM_KEYMAPS);
  map_id = test_get_code(&kb, keymap_tag.row, keymap_tag.col, MOD_PLAIN_MAP);
  TEST_ASSERT_EQUAL_INT32(PLAIN_MATRIX_1, map_id);

  // selecting the highest possible keymap should have no effect
  asdf_keymaps_select(&kb, UINT8_MAX);
  map_id = test_get_code(&kb, keymap_tag.row, keymap_tag.col, MOD_PLAIN_MAP);
  TEST_ASSERT_EQUAL_INT32(PLAIN_MATRIX_1, map_id);
}

// A correct keymap applies without errors. A keymap that assigns a physical
// output twice, or assigns an invalid one, reports one error for it.
void keymap_errors_count_rejected_descriptor_entries(void)
{
  asdf_keymaps_select(&kb, ASDF_TEST_PLAIN_MAP_INDEX);
  TEST_ASSERT_EQUAL_INT(0, asdf_keymap_errors(&kb));

  asdf_keymaps_select(&kb, DOUBLE_ASSIGN_TEST_KEYMAP);
  TEST_ASSERT_EQUAL_INT(1, asdf_keymap_errors(&kb));

  asdf_keymaps_select(&kb, VCAPS_TEST_KEYMAP);
  TEST_ASSERT_EQUAL_INT(1, asdf_keymap_errors(&kb));

  asdf_keymaps_select(&kb, ASDF_TEST_PLAIN_MAP_INDEX);
  TEST_ASSERT_EQUAL_INT(0, asdf_keymap_errors(&kb));
}

// Keymap initialization sets up the keymap state itself, rather than relying
// on it being zeroed, and selects the first keymap (keymap 0 in the test
// registry).
void keymaps_init_selects_first_keymap_from_any_state(void)
{
  memset(&kb.keymap, 0xa5, sizeof(kb.keymap));
  asdf_keymaps_init(&kb);
  TEST_ASSERT_EQUAL_INT(0, kb.keymap.current);
  TEST_ASSERT_EQUAL_INT(0, kb.keymap.requested);
  TEST_ASSERT_EQUAL_INT(0, asdf_keymap_errors(&kb));
  TEST_ASSERT_EQUAL_INT(PLAIN_MATRIX_1, test_get_code(&kb, 0, 0, MOD_PLAIN_MAP));
}

int main(void)
{
  UNITY_BEGIN();
  RUN_TEST(test_chars_are_in_map);
  RUN_TEST(keymap0_plain_gives_plain_values);
  RUN_TEST(keymap0_shift_gives_shift_values);
  RUN_TEST(keymap0_caps_gives_caps_values);
  RUN_TEST(keymap0_ctrl_gives_ctrl_values);
  RUN_TEST(keymap2_plain_gives_plain_values);
  RUN_TEST(keymap2_shift_gives_shift_values);
  RUN_TEST(keymap2_caps_gives_caps_values);
  RUN_TEST(keymap2_ctrl_gives_ctrl_values);
  RUN_TEST(keymap1_capsmap_plain_maps_to_caps);
  RUN_TEST(dip_switch_codes_are_in_last_row_test1_map);
  RUN_TEST(dip_switch_codes_are_in_last_row_test2_map);
  RUN_TEST(dip_switch_properly_clears_bits);
  RUN_TEST(dip_switch_properly_sets_bits);
  RUN_TEST(dip_switch_invalid_keymap_has_no_effect);
  RUN_TEST(keymap_errors_count_rejected_descriptor_entries);
  RUN_TEST(keymaps_init_selects_first_keymap_from_any_state);
  return UNITY_END();
}
