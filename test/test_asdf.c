#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "unity.h"
#include "asdf.h"
#include "asdf_arch.h"
#include "asdf_ascii.h"
#include "asdf_config.h"
#include "asdf_modifiers.h"
#include "asdf_keymaps.h"
#include "test_asdf_keymap_defs.h"
#include "asdf_repeat.h"
#include "test_keymaps.h"
#include "test_asdf_lib.h"
#include "asdf_keyboard.h"

// The keyboard under test.
static asdf_t kb;

#define A 'a'
#define B 'b'

#define TEST_STRING "abcdefghijklmnop"

#define NUM_REPEATS (ASDF_KEYCODE_BUFFER_SIZE - 2)

const char test_string[] = TEST_STRING;

const asdf_keycode_t key_a = A;
const asdf_keycode_t key_b = B;

typedef struct {
  int32_t row;
  int32_t col;
} coord_t;

// The test key matrices (test_keymaps.h) of the keymap (index 0) that
// asdf_init(&kb) selects are used to find key positions and expected codes. Keys
// are identified by test_key_value(): a code, or TEST_ACTION(fn).

static uint32_t key_matrix[TEST_NUM_ROWS];

void keyscan_delay(int32_t ticks);


void setUp(void)
{
  asdf_init(&kb, &asdf_arch_platform);

  // asdf_init(&kb) does not reset modifier state, and CAPS is a toggle that
  // tearDown() cannot release, so reset modifiers explicitly.
  asdf_modifiers_init(&kb.modifiers);
  asdf_sync_lock_leds(&kb);

  // initialize simulated key matrix
  for (uint32_t i = 0; i < TEST_NUM_ROWS; i++) {
    key_matrix[i] = 0;
  }
}

// Release every key and let the releases debounce, so no key is left held for
// the next test.
void tearDown(void)
{
  for (uint32_t i = 0; i < TEST_NUM_ROWS; i++) {
    key_matrix[i] = 0;
  }
  keyscan_delay(ASDF_DEBOUNCE_TIME_MS);
}

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


uint16_t shifted(uint16_t code)
{
  coord_t *location = find_code(code);
  return test_key_value(test_SHIFT_matrix[location->row][location->col]);
}

uint16_t caps(uint16_t code)
{
  coord_t *xy = find_code(code);
  return test_key_value(test_CAPS_matrix[xy->row][xy->col]);
}

uint16_t ctrl(uint16_t code)
{
  coord_t *xy = find_code(code);
  return test_key_value(test_CTRL_matrix[xy->row][xy->col]);
}


void keyscan_delay(int32_t ticks)
{
  for (; ticks; ticks--) {
    asdf_keyscan(&kb);
  }
}


void press_no_debounce(uint16_t code)
{
  coord_t *location = find_code(code);
  key_matrix[location->row] |= (1 << location->col);
}

void release_no_debounce(uint16_t code)
{
  coord_t *location = find_code(code);
  key_matrix[location->row] &= ~(1 << location->col);
}

void press(uint16_t code)
{
  press_no_debounce(code);
  keyscan_delay(ASDF_DEBOUNCE_TIME_MS);
}

void release(uint16_t code)
{
  release_no_debounce(code);
  keyscan_delay(ASDF_DEBOUNCE_TIME_MS);
}


asdf_cols_t asdf_arch_read_row(uint8_t row)
{
  return key_matrix[row];
}


// No repsonse to a keypress before it is debounced.
void pressing_a_gives_nothing_before_debounce(void)
{
  press_no_debounce(key_a);

  // no keypress after only ASDF_DEBOUNCE_TIME_MS -1  ticks (not yet debounced):
  keyscan_delay(ASDF_DEBOUNCE_TIME_MS - 1);
  TEST_ASSERT_EQUAL_INT32(ASDF_INVALID_CODE, (int32_t) test_next_code(&kb));
}

// pressing 'A' gives 'a'
void pressing_a_gives_a(void)
{
  press_no_debounce(key_a);

  // no keypress after only ASDF_DEBOUNCE_TIME_MS -1  ticks (not yet debounced):
  keyscan_delay(ASDF_DEBOUNCE_TIME_MS - 1);
  TEST_ASSERT_EQUAL_INT32(ASDF_INVALID_CODE, (int32_t) test_next_code(&kb));

  // allow the key to finish debounce
  keyscan_delay(1);
  TEST_ASSERT_EQUAL_INT32((int32_t) key_a, (int32_t) test_next_code(&kb));

  // no more codes in the buffer.
  TEST_ASSERT_EQUAL_INT32(ASDF_INVALID_CODE, (int32_t) test_next_code(&kb));
}

// pressing SHIFT+A gives 'A'
void pressing_shift_a_gives_shifted_a(void)
{
  press(TEST_ACTION(ACTION_SHIFT));
  press(key_a);
  TEST_ASSERT_EQUAL_INT32((int32_t) shifted(key_a), (int32_t) test_next_code(&kb));
}

// pressing CAPS+A gives 'A'
void pressing_caps_a_gives_caps_a(void)
{
  press(TEST_ACTION(ACTION_CAPS));
  release(TEST_ACTION(ACTION_CAPS));
  press(key_a);

  TEST_ASSERT_EQUAL_INT32((int32_t) caps(key_a), (int32_t) test_next_code(&kb));
}

// pressing CTRL+A gives 0x01 (Ctrl-A)
void pressing_ctrl_a_gives_ctrl_a(void)
{
  press(TEST_ACTION(ACTION_CTRL));
  press(key_a);
  TEST_ASSERT_EQUAL_INT32((int32_t) ctrl(key_a), (int32_t) test_next_code(&kb));
}

// pressing REPT+A repeats 'a'
void pressing_rept_a_repeats_a(void)
{
  press(TEST_ACTION(ACTION_REPEAT));
  press(key_a);

  TEST_ASSERT_EQUAL_INT32((int32_t) key_a, (uint32_t) test_next_code(&kb));

  // hold "a" for NUM_REPEATS repeat cycles:
  keyscan_delay(NUM_REPEATS * ASDF_REPEAT_TIME_MS);

  for (int i = 0; i < NUM_REPEATS; i++) {
    TEST_ASSERT_EQUAL_INT32((int32_t) key_a, (uint32_t) test_next_code(&kb));
  }

  // and verify there are no more codes in buffer:
  TEST_ASSERT_EQUAL_INT32((int32_t) ASDF_INVALID_CODE, (int32_t) test_next_code(&kb));
}


// pressing REPT+SHIFT+A repeats 'A'
void pressing_shift_rept_a_repeats_shifted_a(void)
{
  press(TEST_ACTION(ACTION_REPEAT));
  press(TEST_ACTION(ACTION_SHIFT));
  press(key_a);

  TEST_ASSERT_EQUAL_INT32((int32_t) shifted(key_a), (uint32_t) test_next_code(&kb));

  // hold "a" for NUM_REPEATS repeat cycles:
  keyscan_delay(NUM_REPEATS * ASDF_REPEAT_TIME_MS);

  for (int i = 0; i < NUM_REPEATS; i++) {
    TEST_ASSERT_EQUAL_INT32((int32_t) shifted(key_a), (uint32_t) test_next_code(&kb));
  }

  // and verify there are no more codes in buffer:
  TEST_ASSERT_EQUAL_INT32((int32_t) ASDF_INVALID_CODE, (int32_t) test_next_code(&kb));
}


// pressing REPT+CAPS+A repeats 'A'
void pressing_caps_rept_a_repeats_caps_a(void)
{
  press(TEST_ACTION(ACTION_REPEAT));
  press(TEST_ACTION(ACTION_CAPS));

  press(key_a);

  TEST_ASSERT_EQUAL_INT32((int32_t) caps(key_a), (uint32_t) test_next_code(&kb));


  // hold "a" for NUM_REPEATS repeat cycles:
  keyscan_delay(NUM_REPEATS * ASDF_REPEAT_TIME_MS);

  for (int i = 0; i < NUM_REPEATS; i++) {
    TEST_ASSERT_EQUAL_INT32((int32_t) caps(key_a), (uint32_t) test_next_code(&kb));
  }

  // and verify there are no more codes in buffer:
  TEST_ASSERT_EQUAL_INT32((int32_t) ASDF_INVALID_CODE, (int32_t) test_next_code(&kb));
}

// pressing REPT+CTRL+A repeats CTRL-A
void pressing_ctrl_rept_a_repeats_ctrl_a(void)
{
  press(TEST_ACTION(ACTION_REPEAT));
  press(TEST_ACTION(ACTION_CTRL));

  press(key_a);
  TEST_ASSERT_EQUAL_INT32((int32_t) ctrl(key_a), (uint32_t) test_next_code(&kb));


  // hold "a" for NUM_REPEATS repeat cycles:
  keyscan_delay(NUM_REPEATS * ASDF_REPEAT_TIME_MS);

  for (int i = 0; i < NUM_REPEATS; i++) {
    TEST_ASSERT_EQUAL_INT32((int32_t) ctrl(key_a), (uint32_t) test_next_code(&kb));
  }

  // and verify there are no more codes in buffer:
  TEST_ASSERT_EQUAL_INT32((int32_t) ASDF_INVALID_CODE, (int32_t) test_next_code(&kb));
}

// pressing and holding 'A' autorepeats 'A'
void holding_a_autorepeats_a(void)
{
  press(key_a);

  TEST_ASSERT_EQUAL_INT32((int32_t) key_a, (uint32_t) test_next_code(&kb));

  // hold "a" for NUM_REPEATS repeat cycles:
  keyscan_delay(ASDF_AUTOREPEAT_TIME_MS);
  TEST_ASSERT_EQUAL_INT32((int32_t) key_a, (uint32_t) test_next_code(&kb));

  // and verify there are no more codes in buffer:
  TEST_ASSERT_EQUAL_INT32((int32_t) ASDF_INVALID_CODE, (int32_t) test_next_code(&kb));
}


// pressing and holding 'A' autorepeats 'A'
void holding_a_autorepeats_slow_then_fast(void)
{
  press(key_a);

  TEST_ASSERT_EQUAL_INT32((int32_t) key_a, (uint32_t) test_next_code(&kb));

  // hold "a" for NUM_REPEATS repeat cycles:
  keyscan_delay(ASDF_AUTOREPEAT_TIME_MS);
  TEST_ASSERT_EQUAL_INT32((int32_t) key_a, (uint32_t) test_next_code(&kb));

  // hold "a" for NUM_REPEATS repeat cycles:
  keyscan_delay(NUM_REPEATS * ASDF_REPEAT_TIME_MS);

  for (int i = 0; i < NUM_REPEATS; i++) {
    TEST_ASSERT_EQUAL_INT32((int32_t) key_a, (uint32_t) test_next_code(&kb));
  }

  // and verify there are no more codes in buffer:
  TEST_ASSERT_EQUAL_INT32((int32_t) ASDF_INVALID_CODE, (int32_t) test_next_code(&kb));
}


// pressing and holding 'A' and 'B' within less than debounce interval
// eventually returns 'A' then 'B'
void pressing_a_then_b_before_debounce_gives_a_then_b(void)
{
  press_no_debounce(key_a);

  // press B very quickly after a
  keyscan_delay(1);

  press(key_b);

  // first get back A
  TEST_ASSERT_EQUAL_INT32((int32_t) key_a, (uint32_t) test_next_code(&kb));

  // next get back B
  TEST_ASSERT_EQUAL_INT32((int32_t) key_b, (uint32_t) test_next_code(&kb));

  // and then verify there are no more codes in buffer:
  TEST_ASSERT_EQUAL_INT32((int32_t) ASDF_INVALID_CODE, (int32_t) test_next_code(&kb));
}


// pressing and holding a series of keys (up to buffer size) in rapid
// succession, allowing each key to debounce, before sending the next, sends all
// the keys in order. (n-key rollover)
void test_key_sequence_nkro(void)
{
  for (int i = 0; i < (int32_t) strlen(test_string); i++) {
    press((asdf_keycode_t) test_string[i]);
  }

  for (int i = 0; i < (int32_t) strlen(test_string); i++) {
    TEST_ASSERT_EQUAL_INT32((int32_t) test_string[i], (int32_t) test_next_code(&kb));
  }

  // and then verify there are no more codes in buffer:
  TEST_ASSERT_EQUAL_INT32((int32_t) ASDF_INVALID_CODE, (int32_t) test_next_code(&kb));
}


// pressing and holding a series of keys (up to buffer size) in rapid succession
// without waiting for prior keys to debounce, eventually debounces and sends
// all the keys in order. (n-key rollover)
void test_key_sequence_nkro_simultaneous_debounce(void)
{
  for (int i = 0; i < (int32_t) strlen(test_string); i++) {
    press_no_debounce((asdf_keycode_t) test_string[i]);
    keyscan_delay(1);
  }

  // several keys are already debounced, but now make sure they all are:
  keyscan_delay(ASDF_DEBOUNCE_TIME_MS);

  for (int i = 0; i < (int32_t) strlen(test_string); i++) {
    TEST_ASSERT_EQUAL_INT32((int32_t) test_string[i], (int32_t) test_next_code(&kb));
  }

  // and then verify there are no more codes in buffer:
  TEST_ASSERT_EQUAL_INT32((int32_t) ASDF_INVALID_CODE, (int32_t) test_next_code(&kb));
}


// holding 'A' then pressing 'B' before autorepeat interval and holding 'B' gives 'A', then repeats
// 'B'
void holding_a_briefly_then_holding_b_gives_a_and_repeats_b(void)
{
  press(key_a);
  keyscan_delay(ASDF_AUTOREPEAT_TIME_MS / 2);
  press(key_b);

  // hold "a" and "b" for autorepeat delay:
  keyscan_delay(ASDF_AUTOREPEAT_TIME_MS);

  // hold "a" and "b" for NUM_REPEATS repeat cycles:
  keyscan_delay(NUM_REPEATS * ASDF_REPEAT_TIME_MS);

  // should get "a" back, then "b"
  TEST_ASSERT_EQUAL_INT32((int32_t) key_a, (uint32_t) test_next_code(&kb));
  TEST_ASSERT_EQUAL_INT32((int32_t) key_b, (uint32_t) test_next_code(&kb));

  // now get back NUM_REEPEATS repetitions of "b"
  for (int i = 0; i < NUM_REPEATS; i++) {
    TEST_ASSERT_EQUAL_INT32((int32_t) key_b, (uint32_t) test_next_code(&kb));
  }

  // and then verify there are no more codes in buffer:
  TEST_ASSERT_EQUAL_INT32((int32_t) ASDF_INVALID_CODE, (int32_t) test_next_code(&kb));
}


// holding 'B' while repeating 'A' starts autorepeat delay, then starts repeating 'B'
void holding_a_then_holding_b_autorepeats_a_then_autorepeats_b(void)
{
  press(key_a);

  TEST_ASSERT_EQUAL_INT32((int32_t) key_a, (uint32_t) test_next_code(&kb));

  // hold "a" for AUTOREPEAT delay
  keyscan_delay(ASDF_AUTOREPEAT_TIME_MS);
  TEST_ASSERT_EQUAL_INT32((int32_t) key_a, (uint32_t) test_next_code(&kb));

  // hold "a" for NUM_REPEATS repeat cycles:
  keyscan_delay(NUM_REPEATS * ASDF_REPEAT_TIME_MS);

  // empty the buffer to make room for 'B'
  for (int i = 0; i < NUM_REPEATS; i++) {
    TEST_ASSERT_EQUAL_INT32((int32_t) key_a, (uint32_t) test_next_code(&kb));
  }

  // now press "b" while "a" is autorepeating:

  press(key_b);

  TEST_ASSERT_EQUAL_INT32((int32_t) key_b, (uint32_t) test_next_code(&kb));

  // hold "a" for autorepeat delay
  keyscan_delay(ASDF_AUTOREPEAT_TIME_MS);
  TEST_ASSERT_EQUAL_INT32((int32_t) key_b, (uint32_t) test_next_code(&kb));

  // hold "a" for NUM_REPEATS repeat cycles:
  keyscan_delay(NUM_REPEATS * ASDF_REPEAT_TIME_MS);

  // empty the buffer to make room for 'B'
  for (int i = 0; i < NUM_REPEATS; i++) {
    TEST_ASSERT_EQUAL_INT32((int32_t) key_b, (uint32_t) test_next_code(&kb));
  }

  // and verify there are no more codes in buffer:
  TEST_ASSERT_EQUAL_INT32((int32_t) ASDF_INVALID_CODE, (int32_t) test_next_code(&kb));
}

// Pressing and holding 'A' then holding 'B' with repeat key held repeats 'A' then 'B'
void repeating_with_a_then_adding_b_repeats_a_then_repeats_b(void)
{
  press(TEST_ACTION(ACTION_REPEAT));
  press(key_a);

  TEST_ASSERT_EQUAL_INT32((int32_t) key_a, (uint32_t) test_next_code(&kb));

  // hold "a" for NUM_REPEATS repeat cycles:
  keyscan_delay(NUM_REPEATS * ASDF_REPEAT_TIME_MS);

  // empty the buffer to make room for 'B'
  for (int i = 0; i < NUM_REPEATS; i++) {
    TEST_ASSERT_EQUAL_INT32((int32_t) key_a, (uint32_t) test_next_code(&kb));
  }

  // now press "b" while "a" is autorepeating:

  press(key_b);

  TEST_ASSERT_EQUAL_INT32((int32_t) key_b, (uint32_t) test_next_code(&kb));

  // hold "a" for NUM_REPEATS repeat cycles:
  keyscan_delay(NUM_REPEATS * ASDF_REPEAT_TIME_MS);

  // empty the buffer to make room for 'B'
  for (int i = 0; i < NUM_REPEATS; i++) {
    TEST_ASSERT_EQUAL_INT32((int32_t) key_b, (uint32_t) test_next_code(&kb));
  }

  // and verify there are no more codes in buffer:
  TEST_ASSERT_EQUAL_INT32((int32_t) ASDF_INVALID_CODE, (int32_t) test_next_code(&kb));
}


int main(void)
{
  UNITY_BEGIN();
  RUN_TEST(pressing_a_gives_nothing_before_debounce);
  RUN_TEST(pressing_a_gives_a);
  RUN_TEST(pressing_shift_a_gives_shifted_a);
  RUN_TEST(pressing_caps_a_gives_caps_a);
  RUN_TEST(pressing_ctrl_a_gives_ctrl_a);
  RUN_TEST(pressing_rept_a_repeats_a);
  RUN_TEST(pressing_shift_rept_a_repeats_shifted_a);
  RUN_TEST(pressing_caps_rept_a_repeats_caps_a);
  RUN_TEST(pressing_ctrl_rept_a_repeats_ctrl_a);
  RUN_TEST(holding_a_autorepeats_a);
  RUN_TEST(holding_a_autorepeats_slow_then_fast);
  RUN_TEST(pressing_a_then_b_before_debounce_gives_a_then_b);
  RUN_TEST(test_key_sequence_nkro);
  RUN_TEST(test_key_sequence_nkro_simultaneous_debounce);
  RUN_TEST(holding_a_briefly_then_holding_b_gives_a_and_repeats_b);
  RUN_TEST(holding_a_then_holding_b_autorepeats_a_then_autorepeats_b);
  RUN_TEST(repeating_with_a_then_adding_b_repeats_a_then_repeats_b);


  return UNITY_END();
}
