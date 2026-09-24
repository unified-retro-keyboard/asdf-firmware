// -*- mode: C; tab-width: 2 ; indent-tabs-mode: nil -*-
//
// Randomized key event sequences. Several keyboards, each on its own fake
// hardware, run pseudo-random scripts: keys (including modifiers, REPEAT, and
// the DIP switches, so keymaps change) are pressed and released, time passes
// in steps from 0 to beyond the 255-tick limit, and codes are taken at random
// moments. After every step, each keyboard's state is checked against
// invariants that must hold whatever the input. Each keyboard is also run alone
// with the same script, and must produce the same codes, in the same order, as
// when interleaved with the others.
//
// Run under ASDF_SANITIZE, this also checks the scripts for out-of-bounds
// accesses and undefined behavior.

#include <stdint.h>
#include <string.h>
#include "unity.h"
#include "asdf.h"
#include "asdf_config.h"
#include "asdf_keyboard.h"
#include "asdf_keymap_setup.h"
#include "asdf_keymaps.h"
#include "fake_platform.h"
#include "test_asdf_keymap_defs.h"

#define NUM_KEYBOARDS 3
#define SCRIPT_STEPS 40000

// Position of no key (asdf.c)
#define NO_KEY_POSITION 0xff

typedef struct {
  asdf_t kb;
  fake_platform_t hw;
  uint32_t rng;
  uint32_t codes;     // codes taken
  uint32_t code_hash; // FNV-1a hash of every code taken, in order
  // what the script reached, to check that it is not degenerate
  uint16_t keymap_changes;
  uint16_t repeating_steps;
} fuzz_runner_t;

static fuzz_runner_t solo[NUM_KEYBOARDS];
static fuzz_runner_t paired[NUM_KEYBOARDS];

static uint32_t next_random(uint32_t *state)
{
  // xorshift32: deterministic and seedable
  uint32_t x = *state;
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  return *state = x;
}

static void runner_init(fuzz_runner_t *r, uint32_t seed)
{
  fake_platform_init(&r->hw);
  asdf_init(&r->kb, &r->hw.platform);
  r->rng = seed;
  r->codes = 0;
  r->code_hash = 2166136261u;
  r->keymap_changes = 0;
  r->repeating_steps = 0;
}

static void take_code(fuzz_runner_t *r)
{
  asdf_keycode_t code;
  if (asdf_next_code(&r->kb, &code)) {
    r->codes++;
    r->code_hash = (r->code_hash ^ code) * 16777619u;
  }
}

// Invariants that must hold after any sequence of operations.
static void check_invariants(const asdf_t *kb)
{
  const asdf_ring_t *queues[] = { &kb->keycodes, &kb->messages };

  for (unsigned q = 0; q < 2; q++) {
    TEST_ASSERT_TRUE(queues[q]->capacity > 0);
    TEST_ASSERT_TRUE(queues[q]->count <= queues[q]->capacity);
    TEST_ASSERT_TRUE(queues[q]->head < queues[q]->capacity);
  }

  for (uint8_t row = 0; row < ASDF_MAX_ROWS; row++) {
    for (uint8_t col = 0; col < ASDF_MAX_COLS; col++) {
      TEST_ASSERT_TRUE(kb->debounce[row][col] >= 1);
      TEST_ASSERT_TRUE(kb->debounce[row][col] <= ASDF_DEBOUNCE_TIME_MS);
    }
  }

  // The repeating key, if any, is a valid position that is held down.
  if (kb->last_key_row != NO_KEY_POSITION) {
    TEST_ASSERT_TRUE(kb->last_key_row < ASDF_MAX_ROWS);
    TEST_ASSERT_TRUE(kb->last_key_col < ASDF_MAX_COLS);
    TEST_ASSERT_TRUE(kb->stable_rows[kb->last_key_row] & (1u << kb->last_key_col));
  }
  else {
    TEST_ASSERT_EQUAL_INT(NO_KEY_POSITION, kb->last_key_col);
  }
  TEST_ASSERT_FALSE(kb->repeat_armed); // only set during a press action

  TEST_ASSERT_TRUE(asdf_keymap_valid(kb->keymap.current));
  TEST_ASSERT_TRUE(asdf_modifier_index(&kb->modifiers) < ASDF_MOD_NUM_MODIFIERS);
  // output_wait_ms is not bounded by print_delay_ms: a keymap switch during a
  // pause leaves that pause at the old keymap's delay.

  for (uint8_t v = 0; v < ASDF_VIRTUAL_NUM_RESOURCES; v++) {
    TEST_ASSERT_TRUE(kb->outputs.pulse_ticks[v] <= ASDF_PULSE_DELAY_LONG_MS);
  }
  for (uint8_t p = 0; p < ASDF_PHYSICAL_NUM_RESOURCES; p++) {
    TEST_ASSERT_TRUE(kb->outputs.physical.shadow[p] <= 1);
    TEST_ASSERT_TRUE(kb->outputs.physical.next[p] < ASDF_PHYSICAL_NUM_RESOURCES);
  }
}

// One step of a keyboard's script.
static void runner_step(fuzz_runner_t *r)
{
  uint32_t roll = next_random(&r->rng);
  uint8_t keymap = r->kb.keymap.current;

  switch (roll & 0x7) {
    case 0:
    case 1: {
      // toggle a key anywhere in the test matrix, including the DIP switches
      uint8_t row = (uint8_t) ((roll >> 8) % TEST_NUM_ROWS);
      uint8_t col = (uint8_t) ((roll >> 16) % TEST_NUM_COLS);
      if (r->hw.matrix[row] & (1u << col)) {
        fake_platform_release(&r->hw, row, col);
      }
      else {
        fake_platform_press(&r->hw, row, col);
      }
      break;
    }
    case 2: {
      // a long gap, sometimes beyond the 255-tick limit
      asdf_update(&r->kb, (uint16_t) ((roll >> 8) % 400));
      break;
    }
    case 3:
      take_code(r);
      break;
    default:
      // ordinary time passing: one to three ticks
      asdf_update(&r->kb, (uint16_t) (1 + ((roll >> 8) % 3)));
      break;
  }
  check_invariants(&r->kb);

  if (r->kb.keymap.current != keymap) {
    r->keymap_changes++;
  }
  if (r->kb.last_key_row != NO_KEY_POSITION) {
    r->repeating_steps++;
  }
}

static void run_seeds(const uint32_t seeds[NUM_KEYBOARDS])
{
  for (int k = 0; k < NUM_KEYBOARDS; k++) {
    runner_init(&solo[k], seeds[k]);
    for (int step = 0; step < SCRIPT_STEPS; step++) {
      runner_step(&solo[k]);
    }
  }

  for (int k = 0; k < NUM_KEYBOARDS; k++) {
    runner_init(&paired[k], seeds[k]);
  }
  for (int step = 0; step < SCRIPT_STEPS; step++) {
    for (int k = 0; k < NUM_KEYBOARDS; k++) {
      runner_step(&paired[k]);
    }
  }

  for (int k = 0; k < NUM_KEYBOARDS; k++) {
    // the script reached keymap switches, held repeating keys, and codes
    TEST_ASSERT_TRUE(solo[k].keymap_changes > 0);
    TEST_ASSERT_TRUE(solo[k].repeating_steps > 0);
    TEST_ASSERT_TRUE(solo[k].codes > 0);
    TEST_ASSERT_EQUAL_UINT32(solo[k].codes, paired[k].codes);
    TEST_ASSERT_EQUAL_HEX32(solo[k].code_hash, paired[k].code_hash);
    TEST_ASSERT_EQUAL_UINT8(solo[k].kb.keymap.current, paired[k].kb.keymap.current);
  }
}

void setUp(void) {}
void tearDown(void) {}

void random_scripts_keep_invariants(void)
{
  const uint32_t seeds[NUM_KEYBOARDS] = { 0x1234567u, 0x89abcdefu, 0x2468ace0u };
  run_seeds(seeds);
}

void random_scripts_keep_invariants_other_seeds(void)
{
  const uint32_t seeds[NUM_KEYBOARDS] = { 0xdeadbeefu, 0x0badf00du, 0x13579bdfu };
  run_seeds(seeds);
}

int main(void)
{
  UNITY_BEGIN();
  RUN_TEST(random_scripts_keep_invariants);
  RUN_TEST(random_scripts_keep_invariants_other_seeds);
  return UNITY_END();
}
