// -*- mode: C; tab-width: 2 ; indent-tabs-mode: nil -*-
//
// Tests for the physical output state (asdf_physical_state_t).

#include <stdint.h>
#include "unity.h"
#include "asdf.h"
#include "asdf_arch.h"
#include "asdf_config.h"
#include "asdf_physical.h"

asdf_cols_t asdf_arch_read_row(uint8_t row)
{
  (void) row;
  return 0;
}

void setUp(void)
{
  asdf_arch_init();
}

void tearDown(void) {}

void init_puts_every_device_on_available_list(void)
{
  asdf_physical_state_t phys;
  asdf_physical_init_r(&phys);

  for (int dev = PHYSICAL_OUT1; dev < ASDF_PHYSICAL_NUM_RESOURCES; dev++) {
    TEST_ASSERT_TRUE(asdf_physical_allocate_r(&phys, (asdf_physical_dev_t) dev, PHYSICAL_NO_OUT, 0));
  }
}

void device_cannot_be_allocated_twice(void)
{
  asdf_physical_state_t phys;
  asdf_physical_init_r(&phys);

  TEST_ASSERT_TRUE(asdf_physical_allocate_r(&phys, PHYSICAL_LED1, PHYSICAL_NO_OUT, 0));
  TEST_ASSERT_FALSE(asdf_physical_allocate_r(&phys, PHYSICAL_LED1, PHYSICAL_NO_OUT, 0));
}

void no_out_cannot_be_allocated(void)
{
  asdf_physical_state_t phys;
  asdf_physical_init_r(&phys);

  TEST_ASSERT_FALSE(asdf_physical_allocate_r(&phys, PHYSICAL_NO_OUT, PHYSICAL_NO_OUT, 0));
}

void allocate_links_tail(void)
{
  asdf_physical_state_t phys;
  asdf_physical_init_r(&phys);

  asdf_physical_allocate_r(&phys, PHYSICAL_LED2, PHYSICAL_NO_OUT, 0);
  asdf_physical_allocate_r(&phys, PHYSICAL_LED1, PHYSICAL_LED2, 0);
  TEST_ASSERT_EQUAL_INT(PHYSICAL_LED2, asdf_physical_next_device_r(&phys, PHYSICAL_LED1));
  TEST_ASSERT_EQUAL_INT(PHYSICAL_NO_OUT, asdf_physical_next_device_r(&phys, PHYSICAL_LED2));
}

void set_toggle_and_assert_drive_output(void)
{
  asdf_physical_state_t phys;
  asdf_physical_init_r(&phys);

  asdf_physical_set_r(&phys, PHYSICAL_LED1, 1);
  TEST_ASSERT_EQUAL_INT(1, asdf_arch_check_output(PHYSICAL_LED1));
  asdf_physical_toggle_r(&phys, PHYSICAL_LED1);
  TEST_ASSERT_EQUAL_INT(0, asdf_arch_check_output(PHYSICAL_LED1));

  asdf_arch_led1_set(1); // drive the output behind the shadow's back
  asdf_physical_assert_r(&phys, PHYSICAL_LED1);
  TEST_ASSERT_EQUAL_INT(0, asdf_arch_check_output(PHYSICAL_LED1));
}

void invalid_devices_are_ignored(void)
{
  asdf_physical_state_t phys;
  asdf_physical_init_r(&phys);

  asdf_physical_set_r(&phys, ASDF_PHYSICAL_NUM_RESOURCES, 1);
  asdf_physical_on_r(&phys, ASDF_PHYSICAL_NUM_RESOURCES);
  asdf_physical_off_r(&phys, ASDF_PHYSICAL_NUM_RESOURCES);
  asdf_physical_toggle_r(&phys, ASDF_PHYSICAL_NUM_RESOURCES);
  asdf_physical_assert_r(&phys, ASDF_PHYSICAL_NUM_RESOURCES);
  TEST_ASSERT_EQUAL_INT(PHYSICAL_NO_OUT,
                        asdf_physical_next_device_r(&phys, ASDF_PHYSICAL_NUM_RESOURCES));
  TEST_ASSERT_FALSE(
    asdf_physical_allocate_r(&phys, ASDF_PHYSICAL_NUM_RESOURCES, PHYSICAL_NO_OUT, 0));
}

// Two physical states track allocation and shadow values independently. (The
// fake hardware outputs behind them are shared.)
void independent_physical_states(void)
{
  asdf_physical_state_t a, b;
  asdf_physical_init_r(&a);
  asdf_physical_init_r(&b);

  TEST_ASSERT_TRUE(asdf_physical_allocate_r(&a, PHYSICAL_LED1, PHYSICAL_NO_OUT, 1));
  TEST_ASSERT_TRUE(asdf_physical_allocate_r(&b, PHYSICAL_LED1, PHYSICAL_LED2, 0));
  TEST_ASSERT_TRUE(asdf_physical_allocate_r(&a, PHYSICAL_LED2, PHYSICAL_NO_OUT, 0));
  TEST_ASSERT_EQUAL_INT(PHYSICAL_NO_OUT, asdf_physical_next_device_r(&a, PHYSICAL_LED1));
  TEST_ASSERT_EQUAL_INT(PHYSICAL_LED2, asdf_physical_next_device_r(&b, PHYSICAL_LED1));

  asdf_physical_assert_r(&a, PHYSICAL_LED1);
  TEST_ASSERT_EQUAL_INT(1, asdf_arch_check_output(PHYSICAL_LED1));
  asdf_physical_assert_r(&b, PHYSICAL_LED1);
  TEST_ASSERT_EQUAL_INT(0, asdf_arch_check_output(PHYSICAL_LED1));
}

int main(void)
{
  UNITY_BEGIN();
  RUN_TEST(init_puts_every_device_on_available_list);
  RUN_TEST(device_cannot_be_allocated_twice);
  RUN_TEST(no_out_cannot_be_allocated);
  RUN_TEST(allocate_links_tail);
  RUN_TEST(set_toggle_and_assert_drive_output);
  RUN_TEST(invalid_devices_are_ignored);
  RUN_TEST(independent_physical_states);
  return UNITY_END();
}
