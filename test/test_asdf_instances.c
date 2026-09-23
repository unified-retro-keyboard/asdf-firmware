// -*- mode: C; tab-width: 2 ; indent-tabs-mode: nil -*-
//
// Integration test for independent keyboard instances. Two asdf_t keyboards,
// each on its own fake hardware, run pseudo-random key event scripts. Each
// keyboard is first run alone and its output recorded; then both are run with
// their scans interleaved. Each must produce exactly the same output, and end
// in the same modifier and keymap state, as when it ran alone.

#include <stdint.h>
#include <string.h>
#include "unity.h"
#include "asdf.h"
#include "asdf_config.h"
#include "asdf_keyboard.h"
#include "asdf_keymaps.h"
#include "fake_platform.h"
#include "test_asdf_keymap_defs.h"

#define SCRIPT_TICKS 20000
#define MAX_LOG 2048

// Keys the scripts may toggle, in the test keymaps: letters, SHIFT, CAPS, CTRL,
// REPEAT, SHIFTLOCK, and the DIP switch for keymap select bit 1 (keymap 0 <->
// keymap 2).
static const uint8_t script_keys[][2] = {
  { 1, 6 }, { 1, 5 }, { 1, 7 }, { 2, 4 }, { 3, 3 }, { 4, 4 }, { 4, 6 }, // a z q b h t e
  { 0, 1 },                                                             // SHIFT
  { 0, 4 },                                                             // CAPS
  { 0, 6 },                                                             // CTRL
  { 5, 0 },                                                             // REPEAT
  { 5, 2 },                                                             // SHIFTLOCK
  { TEST_NUM_ROWS - 1, 1 },                                             // DIP: keymap bit 1
};
#define NUM_SCRIPT_KEYS (sizeof(script_keys) / sizeof(script_keys[0]))

typedef struct {
  asdf_t kb;
  fake_platform_t hw;
  uint32_t rng;
  asdf_keycode_t log[MAX_LOG];
  uint16_t log_len;
} runner_t;

static uint32_t next_random(uint32_t *state)
{
  // xorshift32: deterministic and seedable
  uint32_t x = *state;
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  return *state = x;
}

static void runner_init(runner_t *r, uint32_t seed)
{
  fake_platform_init(&r->hw);
  asdf_init_r(&r->kb, &r->hw.platform);
  r->rng = seed;
  r->log_len = 0;
}

// Advance one tick: maybe toggle a key (holding it for a while), scan, and log
// any output.
static void runner_tick(runner_t *r)
{
  uint32_t roll = next_random(&r->rng);

  if ((roll & 0x1f) == 0) { // on average every 32 ticks
    const uint8_t *key = script_keys[(roll >> 8) % NUM_SCRIPT_KEYS];
    if (r->hw.matrix[key[0]] & (1u << key[1])) {
      fake_platform_release(&r->hw, key[0], key[1]);
    } else {
      fake_platform_press(&r->hw, key[0], key[1]);
    }
  }

  asdf_keyscan_r(&r->kb);

  asdf_keycode_t code;
  while (ASDF_INVALID_CODE != (code = asdf_next_code_r(&r->kb))) {
    if (r->log_len < MAX_LOG) {
      r->log[r->log_len++] = code;
    }
  }
}

static runner_t solo_a, solo_b, pair_a, pair_b;

void setUp(void) {}
void tearDown(void) {}

static void check_same(const runner_t *solo, const runner_t *paired)
{
  TEST_ASSERT_TRUE(solo->log_len > 0);
  TEST_ASSERT_EQUAL_UINT16(solo->log_len, paired->log_len);
  TEST_ASSERT_EQUAL_UINT8_ARRAY(solo->log, paired->log, solo->log_len);
  TEST_ASSERT_EQUAL_MEMORY(&solo->kb.modifiers, &paired->kb.modifiers,
                           sizeof(solo->kb.modifiers));
  TEST_ASSERT_EQUAL_UINT8(solo->kb.keymap.current, paired->kb.keymap.current);
}

// Interleaved keyboards produce the same output as each keyboard alone.
static void run_interleaved(uint32_t seed_a, uint32_t seed_b)
{
  runner_init(&solo_a, seed_a);
  for (int t = 0; t < SCRIPT_TICKS; t++) {
    runner_tick(&solo_a);
  }

  runner_init(&solo_b, seed_b);
  for (int t = 0; t < SCRIPT_TICKS; t++) {
    runner_tick(&solo_b);
  }

  runner_init(&pair_a, seed_a);
  runner_init(&pair_b, seed_b);
  for (int t = 0; t < SCRIPT_TICKS; t++) {
    runner_tick(&pair_a);
    runner_tick(&pair_b);
  }

  check_same(&solo_a, &pair_a);
  check_same(&solo_b, &pair_b);
}

void interleaved_instances_match_solo_runs(void)
{
  run_interleaved(0x12345678u, 0x9abcdef1u);
}

void interleaved_instances_match_solo_runs_other_seeds(void)
{
  run_interleaved(0xdeadbeefu, 0x0badf00du);
}

// The two scripts differ, so a keyboard that read or wrote the other's state
// would change its output.
void scripts_produce_different_output(void)
{
  run_interleaved(0x12345678u, 0x9abcdef1u);
  TEST_ASSERT_TRUE(solo_a.log_len != solo_b.log_len
                   || memcmp(solo_a.log, solo_b.log, solo_a.log_len) != 0);
}

// Re-initializing one keyboard does not affect the other.
void reinit_of_one_instance_leaves_other_alone(void)
{
  runner_init(&pair_a, 1);
  runner_init(&pair_b, 2);

  fake_platform_press(&pair_a.hw, 0, 4); // CAPS on A
  for (int t = 0; t < ASDF_DEBOUNCE_TIME_MS; t++) {
    asdf_keyscan_r(&pair_a.kb);
  }
  modifier_index_t a_mods = asdf_modifier_index_r(&pair_a.kb.modifiers);
  TEST_ASSERT_EQUAL_INT(MOD_CAPS_MAP, a_mods);

  asdf_init_r(&pair_b.kb, &pair_b.hw.platform);
  TEST_ASSERT_EQUAL_INT(a_mods, asdf_modifier_index_r(&pair_a.kb.modifiers));
  TEST_ASSERT_EQUAL_INT(MOD_PLAIN_MAP, asdf_modifier_index_r(&pair_b.kb.modifiers));
}

static void scan_r(runner_t *r)
{
  for (int t = 0; t < ASDF_DEBOUNCE_TIME_MS; t++) {
    asdf_keyscan_r(&r->kb);
  }
}

// Each keyboard drives outputs and strobe polarity, and resets its hardware on
// a keymap switch, only through its own platform.
void each_instance_drives_only_its_own_hardware(void)
{
  runner_init(&pair_a, 1);
  runner_init(&pair_b, 2);
  asdf_keymaps_select_r(&pair_a.kb, VCAPS_TEST_KEYMAP);
  TEST_ASSERT_EQUAL_INT(2, pair_a.hw.resets); // at init, and at the switch
  TEST_ASSERT_EQUAL_INT(1, pair_b.hw.resets);
  asdf_keymaps_select_r(&pair_b.kb, VCAPS_TEST_KEYMAP);

  fake_platform_press(&pair_a.hw, 0, 4); // CAPS on A: CAPS LED on LED1
  scan_r(&pair_a);
  scan_r(&pair_b);
  TEST_ASSERT_EQUAL_INT(1, pair_a.hw.outputs[PHYSICAL_LED1]);
  TEST_ASSERT_EQUAL_INT(0, pair_b.hw.outputs[PHYSICAL_LED1]);

  fake_platform_press(&pair_b.hw, TEST_NUM_ROWS - 1, 6); // strobe polarity DIP on B
  scan_r(&pair_a);
  scan_r(&pair_b);
  TEST_ASSERT_TRUE(pair_b.hw.strobe_positive);
  TEST_ASSERT_FALSE(pair_a.hw.strobe_positive);
}

int main(void)
{
  UNITY_BEGIN();
  RUN_TEST(interleaved_instances_match_solo_runs);
  RUN_TEST(interleaved_instances_match_solo_runs_other_seeds);
  RUN_TEST(scripts_produce_different_output);
  RUN_TEST(reinit_of_one_instance_leaves_other_alone);
  RUN_TEST(each_instance_drives_only_its_own_hardware);
  return UNITY_END();
}
