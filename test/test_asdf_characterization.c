// -*- mode: C; tab-width: 2 ; indent-tabs-mode: nil -*-
//
// Characterization tests
//
// These tests record current firmware behavior, including behavior that looks
// accidental, so that the reentrant refactor can tell an intentional change
// from a regression. A test named "currently_..." pins behavior that is
// expected to change; when a refactor phase changes it on purpose, update the
// test in the same commit.
//
// Tests for out-of-bounds indices that are not validated today are registered
// as ignored: running them would be undefined behavior. Enable each one when
// its function gains validation.

#include <stdint.h>
#include <stdio.h>
#include "unity.h"
#include "asdf.h"
#include "asdf_arch.h"
#include "asdf_config.h"
#include "asdf_hook.h"
#include "asdf_keymaps.h"
#include "asdf_modifiers.h"
#include "asdf_repeat.h"
#include "test_asdf_keymap_defs.h"

// Key positions in the test keymap (ASDF_TEST_PLAIN_MAP) selected by
// asdf_init().
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
#define DIP_ROW (TEST_NUM_ROWS - 1)
#define DIP_MAPSEL_1_COL 1
#define DIP_STROBE_COL 6

static uint32_t key_matrix[TEST_NUM_ROWS];
static int user1_hook_calls;

asdf_cols_t asdf_arch_read_row(uint8_t row)
{
  return key_matrix[row];
}

static void scan(int32_t ticks)
{
  while (ticks-- > 0) {
    asdf_keyscan();
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

static void count_user1_hook(void)
{
  user1_hook_calls++;
}

// Number of scans after the key is first held until it produces a code.
static int32_t scans_until_code(void)
{
  for (int32_t ticks = 1; ticks <= 1000; ticks++) {
    asdf_keyscan();
    if (ASDF_INVALID_CODE != asdf_next_code()) {
      return ticks;
    }
  }
  return -1;
}

void setUp(void)
{
  asdf_init();
  asdf_modifiers_init();
  for (uint32_t i = 0; i < TEST_NUM_ROWS; i++) {
    key_matrix[i] = 0;
  }
  user1_hook_calls = 0;
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

// The debounce counter is not reset when a key returns to its stable state
// before debouncing. A 3-scan glitch leaves the counter part-way down, so the
// next real press registers after only ASDF_DEBOUNCE_TIME_MS - 3 scans.
void currently_debounce_glitch_shortens_next_press(void)
{
  hold(KEY_A_ROW, KEY_A_COL);
  scan(3);
  lift(KEY_A_ROW, KEY_A_COL);
  scan(ASDF_DEBOUNCE_TIME_MS * 2);
  TEST_ASSERT_EQUAL_INT32(ASDF_INVALID_CODE, asdf_next_code());

  hold(KEY_A_ROW, KEY_A_COL);
  TEST_ASSERT_EQUAL_INT32(ASDF_DEBOUNCE_TIME_MS - 3, scans_until_code());
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

//
// Initialization
//

// asdf_init() clears the stable key state before selecting keymap 0, so keys
// held before a re-init are forgotten: a held REPEAT key does not leave repeat
// mode on.
void reinit_forgets_keys_held_before_init(void)
{
  hold(KEY_REPEAT_ROW, KEY_REPEAT_COL);
  scan(ASDF_DEBOUNCE_TIME_MS);

  asdf_init();
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
  TEST_ASSERT_EQUAL_INT32('a', asdf_next_code());
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
  TEST_ASSERT_EQUAL_INT32('a', asdf_next_code());
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

// Re-applying configuration runs only configuration actions: a held key mapped
// to ACTION_NOTHING (which the dispatcher routes to the USER_1 hook) is not
// dispatched.
void apply_configuration_ignores_non_configuration_keys(void)
{
  asdf_hook_assign(ASDF_HOOK_USER_1, count_user1_hook);
  hold(KEY_NOTHING_ROW, KEY_NOTHING_COL);
  scan(ASDF_DEBOUNCE_TIME_MS);

  asdf_apply_configuration();
  TEST_ASSERT_EQUAL_INT(0, user1_hook_calls);
}

//
// Action dispatch
//

// ACTION_HERE_IS shares the ACTION_FN_1 case, so it runs the USER_1 hook.
void currently_here_is_runs_user1_hook(void)
{
  asdf_hook_assign(ASDF_HOOK_USER_1, count_user1_hook);
  hold(KEY_HERE_IS_ROW, KEY_HERE_IS_COL);
  scan(ASDF_DEBOUNCE_TIME_MS);
  TEST_ASSERT_EQUAL_INT(1, user1_hook_calls);
}

// A key mapped to ACTION_NOTHING is not treated as an action: its code (equal
// to ASDF_INVALID_CODE) is queued as a keycode. It occupies a buffer slot and
// reads back as "empty" ahead of the next real code.
void currently_nothing_key_queues_invalid_code(void)
{
  hold(KEY_NOTHING_ROW, KEY_NOTHING_COL);
  scan(ASDF_DEBOUNCE_TIME_MS);
  hold(KEY_A_ROW, KEY_A_COL);
  scan(ASDF_DEBOUNCE_TIME_MS);

  TEST_ASSERT_EQUAL_INT32(ASDF_INVALID_CODE, asdf_next_code());
  TEST_ASSERT_EQUAL_INT32('a', asdf_next_code());
}

//
// LF-to-CRLF conversion in the message buffer
//

static void fill_message_buffer_leaving(int slots)
{
  for (int i = 0; i < ASDF_MESSAGE_BUFFER_SIZE - slots; i++) {
    asdf_putc('x', NULL);
  }
}

static int drain_message_buffer_tail(char *tail, int tail_len)
{
  int count = 0;
  asdf_keycode_t code;
  while (ASDF_INVALID_CODE != (code = asdf_next_code())) {
    tail[count % tail_len] = (char) code;
    count++;
  }
  return count;
}

void newline_with_two_slots_queues_crlf(void)
{
  char tail[2] = { 0 };
  fill_message_buffer_leaving(2);
  asdf_putc('\n', NULL);
  TEST_ASSERT_EQUAL_INT(ASDF_MESSAGE_BUFFER_SIZE, drain_message_buffer_tail(tail, 2));
  TEST_ASSERT_EQUAL_INT('\r', tail[(ASDF_MESSAGE_BUFFER_SIZE - 2) % 2]);
  TEST_ASSERT_EQUAL_INT('\n', tail[(ASDF_MESSAGE_BUFFER_SIZE - 1) % 2]);
}

// With one slot left, CR LF does not fit as a unit, so neither is queued.
void newline_with_one_slot_queues_nothing(void)
{
  char tail[1] = { 0 };
  fill_message_buffer_leaving(1);
  TEST_ASSERT_EQUAL_INT(EOF, asdf_putc('\n', NULL));
  TEST_ASSERT_EQUAL_INT(ASDF_MESSAGE_BUFFER_SIZE - 1, drain_message_buffer_tail(tail, 1));
  TEST_ASSERT_EQUAL_INT('x', tail[0]);
}

void newline_with_no_slots_queues_nothing(void)
{
  char tail[1] = { 0 };
  fill_message_buffer_leaving(0);
  asdf_putc('\n', NULL);
  TEST_ASSERT_EQUAL_INT(ASDF_MESSAGE_BUFFER_SIZE, drain_message_buffer_tail(tail, 1));
  TEST_ASSERT_EQUAL_INT('x', tail[0]);
}

//
// Invalid indices at public boundaries
//

static void null_hook(void) {}

void invalid_keymap_select_is_ignored(void)
{
  asdf_keymaps_select(200);
  hold(KEY_A_ROW, KEY_A_COL);
  scan(ASDF_DEBOUNCE_TIME_MS);
  TEST_ASSERT_EQUAL_INT32('a', asdf_next_code());
}

void invalid_hook_ids_are_ignored(void)
{
  asdf_hook_assign((asdf_hook_id_t) 200, null_hook);
  asdf_hook_execute((asdf_hook_id_t) 200);
  TEST_ASSERT_NOT_NULL(asdf_hook_get((asdf_hook_id_t) 200));
}

void invalid_virtual_assign_is_ignored(void)
{
  asdf_virtual_assign((asdf_virtual_dev_t) 200, PHYSICAL_LED1, V_NOFUNC, 0);
  asdf_virtual_assign(VLED1, (asdf_physical_dev_t) 200, V_NOFUNC, 0);
  TEST_PASS();
}

void unchecked_keymaps_get_code_row_and_col(void)
{
  TEST_IGNORE_MESSAGE("asdf_keymaps_get_code: row and col are not validated (out-of-bounds read)");
}

void unchecked_keymaps_get_code_modifier(void)
{
  TEST_IGNORE_MESSAGE("asdf_keymaps_get_code: modifier index is not validated (out-of-bounds read)");
}

void unchecked_virtual_action_and_activate(void)
{
  TEST_IGNORE_MESSAGE("asdf_virtual_action/asdf_virtual_activate: virtual device is not validated");
}

void unchecked_physical_set_toggle_next(void)
{
  TEST_IGNORE_MESSAGE("asdf_physical_set/toggle/next_device: physical device is not validated");
}

int main(void)
{
  UNITY_BEGIN();
  RUN_TEST(press_registers_after_debounce_time);
  RUN_TEST(currently_debounce_glitch_shortens_next_press);
  RUN_TEST(autorepeat_timing_after_registration);
  RUN_TEST(reinit_forgets_keys_held_before_init);
  RUN_TEST(keymap_switch_resets_caps);
  RUN_TEST(keymap_switch_does_not_reactivate_held_keys);
  RUN_TEST(keymap_switch_resets_held_repeat);
  RUN_TEST(keymap_switch_keeps_dip_configuration);
  RUN_TEST(apply_configuration_ignores_non_configuration_keys);
  RUN_TEST(currently_here_is_runs_user1_hook);
  RUN_TEST(currently_nothing_key_queues_invalid_code);
  RUN_TEST(newline_with_two_slots_queues_crlf);
  RUN_TEST(newline_with_one_slot_queues_nothing);
  RUN_TEST(newline_with_no_slots_queues_nothing);
  RUN_TEST(invalid_keymap_select_is_ignored);
  RUN_TEST(invalid_hook_ids_are_ignored);
  RUN_TEST(invalid_virtual_assign_is_ignored);
  RUN_TEST(unchecked_keymaps_get_code_row_and_col);
  RUN_TEST(unchecked_keymaps_get_code_modifier);
  RUN_TEST(unchecked_virtual_action_and_activate);
  RUN_TEST(unchecked_physical_set_toggle_next);
  return UNITY_END();
}
