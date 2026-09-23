#include <stdint.h>
#include <string.h>

#include "unity.h"
#include "asdf.h"
#include "asdf_ring.h"

// Each test owns its rings and storage; there is no global state to reset.

#define TEST_CAPACITY 16

static const char test_string[] = "abcdefghijklmnopqrstuvwxyz1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZ";

static const char *const more_strings[] = { "abcdefghijkl", "12345", "ABCDEFG", "The quick brown",
                                            "We the people" };

void setUp(void) {}
void tearDown(void) {}

// function to generate pseudo-random sequence to fill buffer.
static asdf_keycode_t random_code(asdf_keycode_t seed)
{
  uint8_t msb = seed & 0x80;
  seed <<= 1;
  if (msb) {
    seed ^= 0x88;
  }
  return seed;
}

void init_rejects_zero_capacity(void)
{
  asdf_keycode_t storage[1];
  asdf_ring_t ring;

  TEST_ASSERT_FALSE(asdf_ring_init(&ring, storage, 0));
  TEST_ASSERT_FALSE(asdf_ring_put(&ring, 'a'));
}

void init_rejects_null_storage(void)
{
  asdf_ring_t ring;
  asdf_keycode_t code;

  TEST_ASSERT_FALSE(asdf_ring_init(&ring, NULL, TEST_CAPACITY));
  TEST_ASSERT_FALSE(asdf_ring_put(&ring, 'a'));
  TEST_ASSERT_FALSE(asdf_ring_get(&ring, &code));
}

void get_from_empty_ring_fails_without_writing(void)
{
  asdf_keycode_t storage[TEST_CAPACITY];
  asdf_ring_t ring;
  asdf_keycode_t code = 'z';

  TEST_ASSERT_TRUE(asdf_ring_init(&ring, storage, TEST_CAPACITY));
  TEST_ASSERT_FALSE(asdf_ring_get(&ring, &code));
  TEST_ASSERT_EQUAL_INT('z', code);
}

// A queued code equal to ASDF_INVALID_CODE is still returned as a code.
void invalid_code_value_is_stored_like_any_code(void)
{
  asdf_keycode_t storage[TEST_CAPACITY];
  asdf_ring_t ring;
  asdf_keycode_t code;

  asdf_ring_init(&ring, storage, TEST_CAPACITY);
  asdf_ring_put(&ring, ASDF_INVALID_CODE);
  TEST_ASSERT_TRUE(asdf_ring_get(&ring, &code));
  TEST_ASSERT_EQUAL_INT(ASDF_INVALID_CODE, code);
}

void put_then_get_returns_same_code(void)
{
  asdf_keycode_t storage[TEST_CAPACITY];
  asdf_ring_t ring;
  asdf_keycode_t code;

  asdf_ring_init(&ring, storage, TEST_CAPACITY);
  TEST_ASSERT_TRUE(asdf_ring_put(&ring, 'a'));
  TEST_ASSERT_EQUAL_INT(1, asdf_ring_count(&ring));
  TEST_ASSERT_TRUE(asdf_ring_get(&ring, &code));
  TEST_ASSERT_EQUAL_INT('a', code);
  TEST_ASSERT_EQUAL_INT(0, asdf_ring_count(&ring));
}

// Codes come out in order across many wraps of the ring indices.
void order_is_preserved_across_wraparound(void)
{
  asdf_keycode_t storage[TEST_CAPACITY];
  asdf_ring_t ring;
  asdf_keycode_t code;
  asdf_keycode_t put_seed = 0x3b, get_seed = 0x3b;

  asdf_ring_init(&ring, storage, TEST_CAPACITY);
  for (int round = 0; round < 50; round++) {
    // alternate between filling part-way and draining part-way, so the head
    // and tail wrap at different points
    int puts = 1 + (round % (TEST_CAPACITY - 1));
    for (int i = 0; i < puts && asdf_ring_count(&ring) < TEST_CAPACITY; i++) {
      put_seed = random_code(put_seed);
      TEST_ASSERT_TRUE(asdf_ring_put(&ring, put_seed));
    }
    int gets = 1 + ((round * 7) % TEST_CAPACITY);
    for (int i = 0; i < gets && asdf_ring_count(&ring); i++) {
      get_seed = random_code(get_seed);
      TEST_ASSERT_TRUE(asdf_ring_get(&ring, &code));
      TEST_ASSERT_EQUAL_INT(get_seed, code);
    }
  }
  TEST_ASSERT_EQUAL_INT(0, asdf_ring_dropped(&ring));
}

// Overfilling keeps the first capacity codes, rejects the rest, and counts
// each rejected code.
void overfill_keeps_first_codes_and_counts_drops(void)
{
  asdf_keycode_t storage[TEST_CAPACITY];
  asdf_ring_t ring;
  asdf_keycode_t code;

  asdf_ring_init(&ring, storage, TEST_CAPACITY);
  for (size_t i = 0; i < strlen(test_string); i++) {
    uint8_t queued = asdf_ring_put(&ring, (asdf_keycode_t) test_string[i]);
    TEST_ASSERT_EQUAL_INT(i < TEST_CAPACITY, queued);
  }
  TEST_ASSERT_EQUAL_INT(strlen(test_string) - TEST_CAPACITY, asdf_ring_dropped(&ring));

  for (int i = 0; i < TEST_CAPACITY; i++) {
    TEST_ASSERT_TRUE(asdf_ring_get(&ring, &code));
    TEST_ASSERT_EQUAL_INT(test_string[i], code);
  }
  TEST_ASSERT_FALSE(asdf_ring_get(&ring, &code));
}

void dropped_count_saturates(void)
{
  asdf_keycode_t storage[1];
  asdf_ring_t ring;

  asdf_ring_init(&ring, storage, 1);
  asdf_ring_put(&ring, 'a');
  for (int i = 0; i < 300; i++) {
    asdf_ring_put(&ring, 'b');
  }
  asdf_ring_put_pair(&ring, 'c', 'd');
  TEST_ASSERT_EQUAL_INT(UINT8_MAX, asdf_ring_dropped(&ring));
}

void pair_fits_in_two_slots(void)
{
  asdf_keycode_t storage[TEST_CAPACITY];
  asdf_ring_t ring;
  asdf_keycode_t code;

  asdf_ring_init(&ring, storage, TEST_CAPACITY);
  for (int i = 0; i < TEST_CAPACITY - 2; i++) {
    asdf_ring_put(&ring, 'x');
  }
  TEST_ASSERT_TRUE(asdf_ring_put_pair(&ring, '\r', '\n'));
  for (int i = 0; i < TEST_CAPACITY - 2; i++) {
    asdf_ring_get(&ring, &code);
  }
  TEST_ASSERT_TRUE(asdf_ring_get(&ring, &code));
  TEST_ASSERT_EQUAL_INT('\r', code);
  TEST_ASSERT_TRUE(asdf_ring_get(&ring, &code));
  TEST_ASSERT_EQUAL_INT('\n', code);
}

// With one slot left, neither code of a pair is queued.
void pair_with_one_slot_queues_neither(void)
{
  asdf_keycode_t storage[TEST_CAPACITY];
  asdf_ring_t ring;

  asdf_ring_init(&ring, storage, TEST_CAPACITY);
  for (int i = 0; i < TEST_CAPACITY - 1; i++) {
    asdf_ring_put(&ring, 'x');
  }
  TEST_ASSERT_FALSE(asdf_ring_put_pair(&ring, '\r', '\n'));
  TEST_ASSERT_EQUAL_INT(TEST_CAPACITY - 1, asdf_ring_count(&ring));
  TEST_ASSERT_EQUAL_INT(2, asdf_ring_dropped(&ring));
}

// A pair can straddle the end of the storage array.
void pair_wraps_around_end_of_storage(void)
{
  asdf_keycode_t storage[4];
  asdf_ring_t ring;
  asdf_keycode_t code;

  asdf_ring_init(&ring, storage, 4);
  asdf_ring_put(&ring, 'a');
  asdf_ring_put(&ring, 'b');
  asdf_ring_put(&ring, 'c');
  asdf_ring_get(&ring, &code);
  asdf_ring_get(&ring, &code); // head at 2, one code ('c') queued
  TEST_ASSERT_TRUE(asdf_ring_put_pair(&ring, '1', '2')); // slots 3 and 0

  const char expected[] = "c12";
  for (int i = 0; i < 3; i++) {
    TEST_ASSERT_TRUE(asdf_ring_get(&ring, &code));
    TEST_ASSERT_EQUAL_INT(expected[i], code);
  }
}

void maximum_capacity_ring_fills_and_drains(void)
{
  asdf_keycode_t storage[UINT8_MAX];
  asdf_ring_t ring;
  asdf_keycode_t code;

  asdf_ring_init(&ring, storage, UINT8_MAX);
  for (int i = 0; i < UINT8_MAX; i++) {
    TEST_ASSERT_TRUE(asdf_ring_put(&ring, (asdf_keycode_t) i));
  }
  TEST_ASSERT_FALSE(asdf_ring_put(&ring, 'x'));
  for (int i = 0; i < UINT8_MAX; i++) {
    TEST_ASSERT_TRUE(asdf_ring_get(&ring, &code));
    TEST_ASSERT_EQUAL_INT(i, code);
  }
}

// Two rings used alternately do not interfere.
void interleaved_rings_are_independent(void)
{
  asdf_keycode_t storage_a[TEST_CAPACITY], storage_b[5];
  asdf_ring_t ring_a, ring_b;
  asdf_keycode_t code;

  asdf_ring_init(&ring_a, storage_a, TEST_CAPACITY);
  asdf_ring_init(&ring_b, storage_b, 5);

  for (size_t s = 0; s < sizeof(more_strings) / sizeof(more_strings[0]); s++) {
    const char *str = more_strings[s];
    size_t len = strlen(str);
    for (size_t i = 0; i < len; i++) {
      asdf_ring_put(&ring_a, (asdf_keycode_t) str[i]);
      asdf_ring_put(&ring_b, (asdf_keycode_t) str[len - 1 - i]);
    }
    for (size_t i = 0; i < len && i < TEST_CAPACITY; i++) {
      TEST_ASSERT_TRUE(asdf_ring_get(&ring_a, &code));
      TEST_ASSERT_EQUAL_INT(str[i], code);
    }
    for (size_t i = 0; i < len && i < 5; i++) {
      TEST_ASSERT_TRUE(asdf_ring_get(&ring_b, &code));
      TEST_ASSERT_EQUAL_INT(str[len - 1 - i], code);
    }
    TEST_ASSERT_FALSE(asdf_ring_get(&ring_a, &code));
    TEST_ASSERT_FALSE(asdf_ring_get(&ring_b, &code));
  }
  TEST_ASSERT_EQUAL_INT(0, asdf_ring_dropped(&ring_a));
  TEST_ASSERT_TRUE(asdf_ring_dropped(&ring_b) > 0);
}

int main(void)
{
  UNITY_BEGIN();
  RUN_TEST(init_rejects_zero_capacity);
  RUN_TEST(init_rejects_null_storage);
  RUN_TEST(get_from_empty_ring_fails_without_writing);
  RUN_TEST(invalid_code_value_is_stored_like_any_code);
  RUN_TEST(put_then_get_returns_same_code);
  RUN_TEST(order_is_preserved_across_wraparound);
  RUN_TEST(overfill_keeps_first_codes_and_counts_drops);
  RUN_TEST(dropped_count_saturates);
  RUN_TEST(pair_fits_in_two_slots);
  RUN_TEST(pair_with_one_slot_queues_neither);
  RUN_TEST(pair_wraps_around_end_of_storage);
  RUN_TEST(maximum_capacity_ring_fills_and_drains);
  RUN_TEST(interleaved_rings_are_independent);
  return UNITY_END();
}
