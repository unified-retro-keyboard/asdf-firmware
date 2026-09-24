#include <stdint.h>
#include "unity.h"
#include "asdf.h"
#include "asdf_keymaps.h"
#include "test_asdf_lib.h"
#include "asdf_modifiers.h"
#include "asdf_arch.h"
#include "asdf_keyboard.h"

// The keyboard under test.
static asdf_t kb;

// Tests in this file focus on validating that test_get_code(&kb)
// returns the correct values for the active modifier map, regardless of the
// dimensions of other modifier maps or previously selected keyboards.

// Provide a trivial row scanner so the tests link cleanly.
asdf_cols_t asdf_arch_read_row(uint8_t row)
{
    (void) row;
    return 0;
}

// Keymap 1: Small dimensions (2x2)
static const asdf_key_t small_plain_matrix[2][2] = {
    { KEY_SEND(0x01), KEY_SEND('a') },
    { KEY_SEND(0x02), KEY_SEND('b') }
};

static const asdf_key_t small_shift_matrix[2][2] = {
    { KEY_SEND(0x01), KEY_SEND('A') },
    { KEY_SEND(0x02), KEY_SEND('B') }
};

// Keymap 2: Large dimensions (4x4)
static const asdf_key_t large_plain_matrix[4][4] = {
    { KEY_SEND(0x01), KEY_SEND('x'), KEY_SEND('y'), KEY_SEND('z') },
    { KEY_SEND(0x02), KEY_SEND('1'), KEY_SEND('2'), KEY_SEND('3') },
    { KEY_SEND(0x03), KEY_SEND('4'), KEY_SEND('5'), KEY_SEND('6') },
    { KEY_SEND(0x04), KEY_SEND('7'), KEY_SEND('8'), KEY_SEND('9') }
};

static const asdf_key_t large_shift_matrix[4][4] = {
    { KEY_SEND(0x01), KEY_SEND('X'), KEY_SEND('Y'), KEY_SEND('Z') },
    { KEY_SEND(0x02), KEY_SEND('!'), KEY_SEND('@'), KEY_SEND('#') },
    { KEY_SEND(0x03), KEY_SEND('$'), KEY_SEND('%'), KEY_SEND('^') },
    { KEY_SEND(0x04), KEY_SEND('&'), KEY_SEND('*'), KEY_SEND('(') }
};

static void load_keymap(const asdf_key_t *plain_matrix,
                        uint8_t plain_rows,
                        uint8_t plain_cols,
                        const asdf_key_t *shift_matrix,
                        uint8_t shift_rows,
                        uint8_t shift_cols)
{
    asdf_init_r(&kb, &asdf_arch_platform);

    // Clear modifier entries so default keymaps do not leak into the tests.
    asdf_keymaps_add_map_r(&kb.keymap, NULL, MOD_PLAIN_MAP, 0, 0);
    asdf_keymaps_add_map_r(&kb.keymap, NULL, MOD_SHIFT_MAP, 0, 0);

    if (plain_matrix) {
        asdf_keymaps_add_map_r(&kb.keymap, plain_matrix, MOD_PLAIN_MAP, plain_rows, plain_cols);
    }

    if (shift_matrix) {
        asdf_keymaps_add_map_r(&kb.keymap, shift_matrix, MOD_SHIFT_MAP, shift_rows, shift_cols);
    }
}

void setUp(void)
{
    asdf_arch_test_reset();
}

void tearDown(void) {}

static void expect_shift_lookup(uint8_t row, uint8_t col, uint16_t expected)
{
    TEST_ASSERT_EQUAL_INT(expected, test_get_code(&kb, row, col, MOD_SHIFT_MAP));
}

static void expect_plain_lookup(uint8_t row, uint8_t col, uint16_t expected)
{
    TEST_ASSERT_EQUAL_INT(expected, test_get_code(&kb, row, col, MOD_PLAIN_MAP));
}

void test_plain_and_shift_maps_can_have_independent_sizes(void)
{
    load_keymap((const asdf_key_t *)small_plain_matrix, 2, 2,
                (const asdf_key_t *)large_shift_matrix, 4, 4);

    expect_shift_lookup(0, 1, 'X');
    expect_shift_lookup(3, 3, '(');

    expect_plain_lookup(0, 1, 'a');
    expect_plain_lookup(1, 1, 'b');

    // Out-of-bounds on the plain map returns a key that does nothing without affecting
    // shift
    expect_plain_lookup(3, 3, TEST_ACTION(ACTION_NOTHING));
}

void test_switching_between_keyboard_layouts_updates_dimensions(void)
{
    load_keymap((const asdf_key_t *)large_plain_matrix, 4, 4,
                (const asdf_key_t *)large_shift_matrix, 4, 4);
    expect_plain_lookup(2, 2, '5');
    expect_shift_lookup(1, 2, '@');

    load_keymap((const asdf_key_t *)small_plain_matrix, 2, 2,
                (const asdf_key_t *)small_shift_matrix, 2, 2);
    expect_plain_lookup(1, 1, 'b');
    expect_shift_lookup(1, 1, 'B');
}

void test_unconfigured_modifier_returns_nothing(void)
{
    load_keymap((const asdf_key_t *)small_plain_matrix, 2, 2,
                NULL, 0, 0);

    expect_plain_lookup(0, 0, 0x01);
    expect_plain_lookup(1, 1, 'b');

    // SHIFT map was never configured, so lookups return ACTION_NOTHING
    expect_shift_lookup(0, 0, TEST_ACTION(ACTION_NOTHING));
    expect_shift_lookup(1, 1, TEST_ACTION(ACTION_NOTHING));
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_plain_and_shift_maps_can_have_independent_sizes);
    RUN_TEST(test_switching_between_keyboard_layouts_updates_dimensions);
    RUN_TEST(test_unconfigured_modifier_returns_nothing);

    return UNITY_END();
}
