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

// asdf_init() selects keymap 0 before clearing the stable key state, so the
// keymap switch replays the actions of keys held before the re-init. Here a
// held REPEAT key leaves repeat mode on even after it is lifted, because the
// cleared stable state never sees its release.
void currently_reinit_replays_keys_held_before_init(void)
{
  hold(KEY_REPEAT_ROW, KEY_REPEAT_COL);
  scan(ASDF_DEBOUNCE_TIME_MS);

  asdf_init();
  lift(KEY_REPEAT_ROW, KEY_REPEAT_COL);
  scan(ASDF_DEBOUNCE_TIME_MS);

  hold(KEY_A_ROW, KEY_A_COL);
  TEST_ASSERT_EQUAL_INT32(ASDF_DEBOUNCE_TIME_MS, scans_until_code());
  // repeat mode: next code after ASDF_REPEAT_TIME_MS, not the autorepeat delay
  TEST_ASSERT_EQUAL_INT32(ASDF_REPEAT_TIME_MS, scans_until_code());
}

//
// Keymap switching
//

// Replay (asdf_apply_all_actions(), run on every keymap switch) re-executes
// the actions of all held keys, including toggles. CAPS is held and its toggle
// undone; replay then toggles CAPS back on.
void currently_replay_reexecutes_held_toggle_actions(void)
{
  hold(KEY_CAPS_ROW, KEY_CAPS_COL);
  scan(ASDF_DEBOUNCE_TIME_MS);
  asdf_modifiers_init(); // CAPS off, key still held

  asdf_apply_all_actions();

  lift(KEY_CAPS_ROW, KEY_CAPS_COL);
  scan(ASDF_DEBOUNCE_TIME_MS);
  hold(KEY_A_ROW, KEY_A_COL);
  scan(ASDF_DEBOUNCE_TIME_MS);
  TEST_ASSERT_EQUAL_INT32('A', asdf_next_code());
}

// Replay looks up each held key in the map for the current modifier state,
// not the map in which the key was pressed. With CAPS on, the held CAPS key is
// looked up in the test caps map, where that position is ACTION_NOTHING, which
// runs the USER_1 hook instead of toggling CAPS.
void currently_replay_uses_current_modifier_map(void)
{
  asdf_hook_assign(ASDF_HOOK_USER_1, count_user1_hook);
  hold(KEY_CAPS_ROW, KEY_CAPS_COL);
  scan(ASDF_DEBOUNCE_TIME_MS); // CAPS on

  asdf_apply_all_actions();
  TEST_ASSERT_EQUAL_INT(1, user1_hook_calls);
}

// Modifier state set before a keymap switch survives it: the switch does not
// reset modifiers.
void currently_keymap_switch_keeps_caps_state(void)
{
  hold(KEY_CAPS_ROW, KEY_CAPS_COL);
  scan(ASDF_DEBOUNCE_TIME_MS);
  lift(KEY_CAPS_ROW, KEY_CAPS_COL);
  scan(ASDF_DEBOUNCE_TIME_MS);

  hold(DIP_ROW, DIP_MAPSEL_1_COL); // selects keymap 2 (test2 plain map)
  scan(ASDF_DEBOUNCE_TIME_MS);

  hold(KEY_A_ROW, KEY_A_COL);
  scan(ASDF_DEBOUNCE_TIME_MS);
  TEST_ASSERT_EQUAL_INT32('A', asdf_next_code());
}

// asdf_apply_all_actions() (run on every keymap switch) sends every held key
// through the action dispatcher. ACTION_NOTHING falls through to the
// ACTION_FN_1 case there, so a held key mapped to ACTION_NOTHING runs the
// USER_1 hook.
void currently_replay_of_held_nothing_key_runs_user1_hook(void)
{
  asdf_hook_assign(ASDF_HOOK_USER_1, count_user1_hook);
  hold(KEY_NOTHING_ROW, KEY_NOTHING_COL);
  scan(ASDF_DEBOUNCE_TIME_MS);
  TEST_ASSERT_EQUAL_INT(0, user1_hook_calls);

  asdf_apply_all_actions();
  TEST_ASSERT_EQUAL_INT(1, user1_hook_calls);
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

// With one slot left, the CR is queued and the LF is silently dropped.
void currently_newline_with_one_slot_queues_cr_only(void)
{
  char tail[1] = { 0 };
  fill_message_buffer_leaving(1);
  asdf_putc('\n', NULL);
  TEST_ASSERT_EQUAL_INT(ASDF_MESSAGE_BUFFER_SIZE, drain_message_buffer_tail(tail, 1));
  TEST_ASSERT_EQUAL_INT('\r', tail[0]);
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
  RUN_TEST(currently_reinit_replays_keys_held_before_init);
  RUN_TEST(currently_replay_reexecutes_held_toggle_actions);
  RUN_TEST(currently_replay_uses_current_modifier_map);
  RUN_TEST(currently_keymap_switch_keeps_caps_state);
  RUN_TEST(currently_replay_of_held_nothing_key_runs_user1_hook);
  RUN_TEST(currently_here_is_runs_user1_hook);
  RUN_TEST(currently_nothing_key_queues_invalid_code);
  RUN_TEST(newline_with_two_slots_queues_crlf);
  RUN_TEST(currently_newline_with_one_slot_queues_cr_only);
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
