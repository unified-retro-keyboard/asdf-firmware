// -*- mode: C; tab-width: 2 ; indent-tabs-mode: nil -*-
//
// Characterization tests
//
// These tests record current firmware behavior, including behavior that looks
// accidental, so that the reentrant refactor can tell an intentional change
// from a regression. A test named "currently_..." pins behavior that is
// expected to change; when a refactor phase changes it on purpose, update the
// test in the same commit.

#include <stdint.h>
#include <stdio.h>
#include "unity.h"
#include "asdf.h"
#include "asdf_arch.h"
#include "asdf_config.h"
#include "asdf_keymaps.h"
#include "asdf_modifiers.h"
#include "asdf_physical.h"
#include "asdf_repeat.h"
#include "asdf_virtual.h"
#include "test_asdf_keymap_defs.h"
#include "test_asdf_lib.h"
#include "asdf_keyboard.h"

// The keyboard under test.
static asdf_t kb;

// Key positions in the test keymap (ASDF_TEST_PLAIN_MAP) selected by
// asdf_init(&kb).
#define KEY_A_ROW 1
#define KEY_A_COL 6
#define KEY_CAPS_ROW 0
#define KEY_CAPS_COL 4
#define KEY_NOTHING_ROW 0
#define KEY_NOTHING_COL 3
#define KEY_REPEAT_ROW 5
#define KEY_REPEAT_COL 0
#define KEY_HERE_IS_ROW 5
#define KEY_HERE_IS_COL 1
#define KEY_NOREPEAT_ROW 7 // NOREPEAT 0x7F
#define KEY_NOREPEAT_COL 0
#define KEY_CTRL_ROW 0
#define KEY_CTRL_COL 6
#define KEY_ESC1_ROW 0 // ASCII_ESC in the test CTRL map
#define KEY_ESC1_COL 5
#define KEY_ESC2_ROW 6 // ASCII_ESC in the test CTRL map
#define KEY_ESC2_COL 2
#define DIP_ROW (TEST_NUM_ROWS - 1)
#define DIP_MAPSEL_1_COL 1
#define DIP_STROBE_COL 6

static uint32_t key_matrix[TEST_NUM_ROWS];

asdf_cols_t asdf_arch_read_row(uint8_t row)
{
  return key_matrix[row];
}

static void scan(int32_t ticks)
{
  while (ticks-- > 0) {
    asdf_keyscan(&kb);
  }
}

static void hold(uint8_t row, uint8_t col)
{
  key_matrix[row] |= (1u << col);
}

static void lift(uint8_t row, uint8_t col)
{
  key_matrix[row] &= ~(1u << col);
}

// Number of scans after the key is first held until it produces a code.
static int32_t scans_until_code(void)
{
  for (int32_t ticks = 1; ticks <= 1000; ticks++) {
    asdf_keyscan(&kb);
    if (ASDF_INVALID_CODE != test_next_code(&kb)) {
      return ticks;
    }
  }
  return -1;
}

void setUp(void)
{
  asdf_init(&kb, &asdf_arch_platform);
  asdf_modifiers_init(&kb.modifiers);
  asdf_sync_lock_leds(&kb);
  for (uint32_t i = 0; i < TEST_NUM_ROWS; i++) {
    key_matrix[i] = 0;
  }
  test_here_is_clear();
}

void tearDown(void)
{
  for (uint32_t i = 0; i < TEST_NUM_ROWS; i++) {
    key_matrix[i] = 0;
  }
  scan(ASDF_DEBOUNCE_TIME_MS);
}

//
// Debounce
//

// A key registers after exactly ASDF_DEBOUNCE_TIME_MS scans.
void press_registers_after_debounce_time(void)
{
  hold(KEY_A_ROW, KEY_A_COL);
  TEST_ASSERT_EQUAL_INT32(ASDF_DEBOUNCE_TIME_MS, scans_until_code());
}

// A key that returns to its stable state before debouncing restarts the
// debounce count, so a 3-scan glitch does not shorten the next press.
void debounce_glitch_does_not_shorten_next_press(void)
{
  hold(KEY_A_ROW, KEY_A_COL);
  scan(3);
  lift(KEY_A_ROW, KEY_A_COL);
  scan(ASDF_DEBOUNCE_TIME_MS * 2);
  TEST_ASSERT_EQUAL_INT32(ASDF_INVALID_CODE, test_next_code(&kb));

  hold(KEY_A_ROW, KEY_A_COL);
  TEST_ASSERT_EQUAL_INT32(ASDF_DEBOUNCE_TIME_MS, scans_until_code());
}

//
// Repeat timing
//

// Autorepeat starts ASDF_AUTOREPEAT_TIME_MS scans after the key registers, and
// then repeats every ASDF_REPEAT_TIME_MS scans.
void autorepeat_timing_after_registration(void)
{
  hold(KEY_A_ROW, KEY_A_COL);
  TEST_ASSERT_EQUAL_INT32(ASDF_DEBOUNCE_TIME_MS, scans_until_code());
  TEST_ASSERT_EQUAL_INT32(ASDF_AUTOREPEAT_TIME_MS, scans_until_code());
  TEST_ASSERT_EQUAL_INT32(ASDF_REPEAT_TIME_MS, scans_until_code());
  TEST_ASSERT_EQUAL_INT32(ASDF_REPEAT_TIME_MS, scans_until_code());
}

// A NOREPEAT key sends its code once, however long it is held.
void norepeat_key_sends_once(void)
{
  hold(KEY_NOREPEAT_ROW, KEY_NOREPEAT_COL);
  TEST_ASSERT_EQUAL_INT32(ASDF_DEBOUNCE_TIME_MS, scans_until_code());
  TEST_ASSERT_EQUAL_INT32(-1, scans_until_code());
}

// A NOREPEAT key does not take over repeat: a key already repeating keeps its
// timing.
void norepeat_key_leaves_repeating_key_alone(void)
{
  hold(KEY_A_ROW, KEY_A_COL);
  TEST_ASSERT_EQUAL_INT32(ASDF_DEBOUNCE_TIME_MS, scans_until_code());
  hold(KEY_NOREPEAT_ROW, KEY_NOREPEAT_COL);
  TEST_ASSERT_EQUAL_INT32(ASDF_DEBOUNCE_TIME_MS, scans_until_code());
  TEST_ASSERT_EQUAL_INT32(ASDF_AUTOREPEAT_TIME_MS - ASDF_DEBOUNCE_TIME_MS, scans_until_code());
}

// The repeating key is tracked by position. ESC is on two keys of the test
// CTRL map; with both held, releasing the first does not stop the second
// from repeating.
void repeat_follows_key_position_not_code(void)
{
  hold(KEY_CTRL_ROW, KEY_CTRL_COL);
  scan(ASDF_DEBOUNCE_TIME_MS);

  hold(KEY_ESC1_ROW, KEY_ESC1_COL);
  TEST_ASSERT_EQUAL_INT32(ASDF_DEBOUNCE_TIME_MS, scans_until_code());
  hold(KEY_ESC2_ROW, KEY_ESC2_COL);
  TEST_ASSERT_EQUAL_INT32(ASDF_DEBOUNCE_TIME_MS, scans_until_code());

  lift(KEY_ESC1_ROW, KEY_ESC1_COL);
  // the autorepeat timer restarted when the second key registered
  TEST_ASSERT_EQUAL_INT32(ASDF_AUTOREPEAT_TIME_MS, scans_until_code());
}

//
// Initialization
//

// asdf_init(&kb) clears the stable key state before selecting keymap 0, so keys
// held before a re-init are forgotten: a held REPEAT key does not leave repeat
// mode on.
void reinit_forgets_keys_held_before_init(void)
{
  hold(KEY_REPEAT_ROW, KEY_REPEAT_COL);
  scan(ASDF_DEBOUNCE_TIME_MS);

  asdf_init(&kb, &asdf_arch_platform);
  lift(KEY_REPEAT_ROW, KEY_REPEAT_COL);
  scan(ASDF_DEBOUNCE_TIME_MS);

  hold(KEY_A_ROW, KEY_A_COL);
  TEST_ASSERT_EQUAL_INT32(ASDF_DEBOUNCE_TIME_MS, scans_until_code());
  TEST_ASSERT_EQUAL_INT32(ASDF_AUTOREPEAT_TIME_MS, scans_until_code());
}

//
// Keymap switching
//
// A keymap switch resets runtime state (modifiers, repeat, last key) and then
// re-applies only the configuration actions of held switches.

static void switch_keymap_by_dip(void)
{
  hold(DIP_ROW, DIP_MAPSEL_1_COL); // selects keymap 2 (test2 plain map)
  scan(ASDF_DEBOUNCE_TIME_MS);
}

// CAPS set before a switch is cleared by it.
void keymap_switch_resets_caps(void)
{
  hold(KEY_CAPS_ROW, KEY_CAPS_COL);
  scan(ASDF_DEBOUNCE_TIME_MS);
  lift(KEY_CAPS_ROW, KEY_CAPS_COL);
  scan(ASDF_DEBOUNCE_TIME_MS);

  switch_keymap_by_dip();

  hold(KEY_A_ROW, KEY_A_COL);
  scan(ASDF_DEBOUNCE_TIME_MS);
  TEST_ASSERT_EQUAL_INT32('a', test_next_code(&kb));
}

// A key held across a switch is not re-activated, and its later release is
// not treated as a new event: holding CAPS through a switch leaves CAPS off.
void keymap_switch_does_not_reactivate_held_keys(void)
{
  hold(KEY_CAPS_ROW, KEY_CAPS_COL);
  scan(ASDF_DEBOUNCE_TIME_MS);

  switch_keymap_by_dip();

  lift(KEY_CAPS_ROW, KEY_CAPS_COL);
  scan(ASDF_DEBOUNCE_TIME_MS);
  hold(KEY_A_ROW, KEY_A_COL);
  scan(ASDF_DEBOUNCE_TIME_MS);
  TEST_ASSERT_EQUAL_INT32('a', test_next_code(&kb));
}

// REPEAT held across a switch is reset: the next key autorepeats normally.
void keymap_switch_resets_held_repeat(void)
{
  hold(KEY_REPEAT_ROW, KEY_REPEAT_COL);
  scan(ASDF_DEBOUNCE_TIME_MS);

  switch_keymap_by_dip();

  hold(KEY_A_ROW, KEY_A_COL);
  TEST_ASSERT_EQUAL_INT32(ASDF_DEBOUNCE_TIME_MS, scans_until_code());
  TEST_ASSERT_EQUAL_INT32(ASDF_AUTOREPEAT_TIME_MS, scans_until_code());
}

// A held DIP configuration switch survives a keymap switch, even though the
// switch re-initializes the architecture (which resets strobe polarity).
void keymap_switch_keeps_dip_configuration(void)
{
  hold(DIP_ROW, DIP_STROBE_COL);
  scan(ASDF_DEBOUNCE_TIME_MS);
  TEST_ASSERT_TRUE(asdf_arch_is_strobe_positive());

  switch_keymap_by_dip();
  TEST_ASSERT_TRUE(asdf_arch_is_strobe_positive());
}

// Re-applying configuration runs only configuration actions: the press action
// of a held non-configuration key is not run again.
void apply_configuration_ignores_non_configuration_keys(void)
{
  hold(KEY_HERE_IS_ROW, KEY_HERE_IS_COL);
  scan(ASDF_DEBOUNCE_TIME_MS);
  TEST_ASSERT_EQUAL_INT(1, test_here_is_count());

  asdf_apply_configuration(&kb);
  TEST_ASSERT_EQUAL_INT(1, test_here_is_count());
}

//
// Action dispatch
//

// A key's press action runs once when the key is pressed.
void function_key_runs_its_action(void)
{
  hold(KEY_HERE_IS_ROW, KEY_HERE_IS_COL);
  scan(ASDF_DEBOUNCE_TIME_MS);
  scan(ASDF_DEBOUNCE_TIME_MS);
  TEST_ASSERT_EQUAL_INT(1, test_here_is_count());
}

// A key that does nothing queues nothing. (Before keys had press and release
// actions, a "nothing" key queued an invalid code.)
void nothing_key_queues_nothing(void)
{
  hold(KEY_NOTHING_ROW, KEY_NOTHING_COL);
  scan(ASDF_DEBOUNCE_TIME_MS);
  hold(KEY_A_ROW, KEY_A_COL);
  scan(ASDF_DEBOUNCE_TIME_MS);

  TEST_ASSERT_EQUAL_INT32('a', test_next_code(&kb));
  TEST_ASSERT_EQUAL_INT32(ASDF_INVALID_CODE, test_next_code(&kb));
}

//
// LF-to-CRLF conversion in the message buffer
//

static void fill_message_buffer_leaving(int slots)
{
  for (int i = 0; i < ASDF_MESSAGE_BUFFER_SIZE - slots; i++) {
    asdf_putc(&kb, 'x');
  }
}

static int drain_message_buffer_tail(char *tail, int tail_len)
{
  int count = 0;
  uint16_t code;
  while (ASDF_INVALID_CODE != (code = test_next_code(&kb))) {
    tail[count % tail_len] = (char) code;
    count++;
  }
  return count;
}

void newline_with_two_slots_queues_crlf(void)
{
  char tail[2] = { 0 };
  fill_message_buffer_leaving(2);
  asdf_putc(&kb, '\n');
  TEST_ASSERT_EQUAL_INT(ASDF_MESSAGE_BUFFER_SIZE, drain_message_buffer_tail(tail, 2));
  TEST_ASSERT_EQUAL_INT('\r', tail[(ASDF_MESSAGE_BUFFER_SIZE - 2) % 2]);
  TEST_ASSERT_EQUAL_INT('\n', tail[(ASDF_MESSAGE_BUFFER_SIZE - 1) % 2]);
}

// With one slot left, CR LF does not fit as a unit, so neither is queued.
void newline_with_one_slot_queues_nothing(void)
{
  char tail[1] = { 0 };
  fill_message_buffer_leaving(1);
  TEST_ASSERT_EQUAL_INT(EOF, asdf_putc(&kb, '\n'));
  TEST_ASSERT_EQUAL_INT(ASDF_MESSAGE_BUFFER_SIZE - 1, drain_message_buffer_tail(tail, 1));
  TEST_ASSERT_EQUAL_INT('x', tail[0]);
}

void newline_with_no_slots_queues_nothing(void)
{
  char tail[1] = { 0 };
  fill_message_buffer_leaving(0);
  asdf_putc(&kb, '\n');
  TEST_ASSERT_EQUAL_INT(ASDF_MESSAGE_BUFFER_SIZE, drain_message_buffer_tail(tail, 1));
  TEST_ASSERT_EQUAL_INT('x', tail[0]);
}

//
// Invalid indices at public boundaries
//

void invalid_keymap_select_is_ignored(void)
{
  asdf_keymaps_select(&kb, 200);
  hold(KEY_A_ROW, KEY_A_COL);
  scan(ASDF_DEBOUNCE_TIME_MS);
  TEST_ASSERT_EQUAL_INT32('a', test_next_code(&kb));
}

void invalid_virtual_assign_is_ignored(void)
{
  asdf_virtual_assign(&kb.outputs, (asdf_virtual_dev_t) 200, PHYSICAL_LED1, V_NOFUNC, 0);
  asdf_virtual_assign(&kb.outputs, VLED1, (asdf_physical_dev_t) 200, V_NOFUNC, 0);
  TEST_PASS();
}

void invalid_keymap_row_and_col_return_nothing(void)
{
  TEST_ASSERT_EQUAL_INT(TEST_ACTION(ACTION_NOTHING), 
                        test_get_code(&kb, 200, 0, MOD_PLAIN_MAP));
  TEST_ASSERT_EQUAL_INT(TEST_ACTION(ACTION_NOTHING), 
                        test_get_code(&kb, 0, 200, MOD_PLAIN_MAP));
  TEST_ASSERT_EQUAL_INT(TEST_ACTION(ACTION_NOTHING), 
                        test_get_code(&kb, TEST_NUM_ROWS, 0, MOD_PLAIN_MAP));
  TEST_ASSERT_EQUAL_INT(TEST_ACTION(ACTION_NOTHING), 
                        test_get_code(&kb, 0, TEST_NUM_COLS, MOD_PLAIN_MAP));
}

void invalid_keymap_modifier_returns_nothing(void)
{
  TEST_ASSERT_EQUAL_INT(TEST_ACTION(ACTION_NOTHING), 
                        test_get_code(&kb, 0, 0, ASDF_MOD_NUM_MODIFIERS));
  TEST_ASSERT_EQUAL_INT(TEST_ACTION(ACTION_NOTHING), 
                        test_get_code(&kb, 0, 0, 200));
}

void invalid_virtual_devices_are_ignored(void)
{
  asdf_virtual_action(&kb.outputs, (asdf_virtual_dev_t) 200, V_SET_HI);
  asdf_virtual_activate(&kb.outputs, (asdf_virtual_dev_t) 200);
  TEST_PASS();
}

void invalid_physical_devices_are_ignored(void)
{
  asdf_physical_set(&kb.outputs.physical, (asdf_physical_dev_t) 200, 1);
  asdf_physical_toggle(&kb.outputs.physical, (asdf_physical_dev_t) 200);
  asdf_physical_assert(&kb.outputs.physical, (asdf_physical_dev_t) 200);
  TEST_ASSERT_EQUAL_INT(PHYSICAL_NO_OUT,
                        asdf_physical_next_device(&kb.outputs.physical, (asdf_physical_dev_t) 200));
  TEST_ASSERT_FALSE(
    asdf_physical_allocate(&kb.outputs.physical, (asdf_physical_dev_t) 200, PHYSICAL_NO_OUT, 0));
  TEST_ASSERT_FALSE(
    asdf_physical_allocate(&kb.outputs.physical, PHYSICAL_LED1, (asdf_physical_dev_t) 200, 0));
}

int main(void)
{
  UNITY_BEGIN();
  RUN_TEST(press_registers_after_debounce_time);
  RUN_TEST(debounce_glitch_does_not_shorten_next_press);
  RUN_TEST(autorepeat_timing_after_registration);
  RUN_TEST(repeat_follows_key_position_not_code);
  RUN_TEST(norepeat_key_sends_once);
  RUN_TEST(norepeat_key_leaves_repeating_key_alone);
  RUN_TEST(reinit_forgets_keys_held_before_init);
  RUN_TEST(keymap_switch_resets_caps);
  RUN_TEST(keymap_switch_does_not_reactivate_held_keys);
  RUN_TEST(keymap_switch_resets_held_repeat);
  RUN_TEST(keymap_switch_keeps_dip_configuration);
  RUN_TEST(apply_configuration_ignores_non_configuration_keys);
  RUN_TEST(function_key_runs_its_action);
  RUN_TEST(nothing_key_queues_nothing);
  RUN_TEST(newline_with_two_slots_queues_crlf);
  RUN_TEST(newline_with_one_slot_queues_nothing);
  RUN_TEST(newline_with_no_slots_queues_nothing);
  RUN_TEST(invalid_keymap_select_is_ignored);
  RUN_TEST(invalid_virtual_assign_is_ignored);
  RUN_TEST(invalid_keymap_row_and_col_return_nothing);
  RUN_TEST(invalid_keymap_modifier_returns_nothing);
  RUN_TEST(invalid_virtual_devices_are_ignored);
  RUN_TEST(invalid_physical_devices_are_ignored);
  return UNITY_END();
}
