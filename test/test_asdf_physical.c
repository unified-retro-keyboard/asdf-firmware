// -*- mode: C; tab-width: 2 ; indent-tabs-mode: nil -*-
//
// Tests for the physical output state (asdf_physical_state_t).

#include <stdint.h>
#include "unity.h"
#include "asdf.h"
#include "asdf_arch.h"
#include "asdf_config.h"
#include "asdf_physical.h"
#include "fake_platform.h"

asdf_cols_t asdf_arch_read_row(uint8_t row)
{
  (void) row;
  return 0;
}

void setUp(void)
{
  asdf_arch_test_reset();
}

void tearDown(void) {}

void init_puts_every_device_on_available_list(void)
{
  asdf_physical_state_t phys;
  asdf_physical_init(&phys, &asdf_arch_platform);

  for (int dev = PHYSICAL_OUT1; dev < ASDF_PHYSICAL_NUM_RESOURCES; dev++) {
    TEST_ASSERT_TRUE(asdf_physical_allocate(&phys, (asdf_physical_dev_t) dev, PHYSICAL_NO_OUT, 0));
  }
}

void device_cannot_be_allocated_twice(void)
{
  asdf_physical_state_t phys;
  asdf_physical_init(&phys, &asdf_arch_platform);

  TEST_ASSERT_TRUE(asdf_physical_allocate(&phys, PHYSICAL_LED1, PHYSICAL_NO_OUT, 0));
  TEST_ASSERT_FALSE(asdf_physical_allocate(&phys, PHYSICAL_LED1, PHYSICAL_NO_OUT, 0));
}

void no_out_cannot_be_allocated(void)
{
  asdf_physical_state_t phys;
  asdf_physical_init(&phys, &asdf_arch_platform);

  TEST_ASSERT_FALSE(asdf_physical_allocate(&phys, PHYSICAL_NO_OUT, PHYSICAL_NO_OUT, 0));
}

void allocate_links_tail(void)
{
  asdf_physical_state_t phys;
  asdf_physical_init(&phys, &asdf_arch_platform);

  asdf_physical_allocate(&phys, PHYSICAL_LED2, PHYSICAL_NO_OUT, 0);
  asdf_physical_allocate(&phys, PHYSICAL_LED1, PHYSICAL_LED2, 0);
  TEST_ASSERT_EQUAL_INT(PHYSICAL_LED2, asdf_physical_next_device(&phys, PHYSICAL_LED1));
  TEST_ASSERT_EQUAL_INT(PHYSICAL_NO_OUT, asdf_physical_next_device(&phys, PHYSICAL_LED2));
}

void set_toggle_and_assert_drive_output(void)
{
  asdf_physical_state_t phys;
  asdf_physical_init(&phys, &asdf_arch_platform);

  asdf_physical_set(&phys, PHYSICAL_LED1, 1);
  TEST_ASSERT_EQUAL_INT(1, asdf_arch_check_output(PHYSICAL_LED1));
  asdf_physical_toggle(&phys, PHYSICAL_LED1);
  TEST_ASSERT_EQUAL_INT(0, asdf_arch_check_output(PHYSICAL_LED1));

  asdf_arch_led1_set(1); // drive the output behind the shadow's back
  asdf_physical_assert(&phys, PHYSICAL_LED1);
  TEST_ASSERT_EQUAL_INT(0, asdf_arch_check_output(PHYSICAL_LED1));
}

void invalid_devices_are_ignored(void)
{
  asdf_physical_state_t phys;
  asdf_physical_init(&phys, &asdf_arch_platform);

  asdf_physical_set(&phys, ASDF_PHYSICAL_NUM_RESOURCES, 1);
  asdf_physical_on(&phys, ASDF_PHYSICAL_NUM_RESOURCES);
  asdf_physical_off(&phys, ASDF_PHYSICAL_NUM_RESOURCES);
  asdf_physical_toggle(&phys, ASDF_PHYSICAL_NUM_RESOURCES);
  asdf_physical_assert(&phys, ASDF_PHYSICAL_NUM_RESOURCES);
  TEST_ASSERT_EQUAL_INT(PHYSICAL_NO_OUT,
                        asdf_physical_next_device(&phys, ASDF_PHYSICAL_NUM_RESOURCES));
  TEST_ASSERT_FALSE(
    asdf_physical_allocate(&phys, ASDF_PHYSICAL_NUM_RESOURCES, PHYSICAL_NO_OUT, 0));
}

// Two physical states track allocation and shadow values independently, and
// drive their own hardware.
void independent_physical_states(void)
{
  fake_platform_t hw_a, hw_b;
  asdf_physical_state_t a, b;
  fake_platform_init(&hw_a);
  fake_platform_init(&hw_b);
  asdf_physical_init(&a, &hw_a.platform);
  asdf_physical_init(&b, &hw_b.platform);

  TEST_ASSERT_TRUE(asdf_physical_allocate(&a, PHYSICAL_LED1, PHYSICAL_NO_OUT, 1));
  TEST_ASSERT_TRUE(asdf_physical_allocate(&b, PHYSICAL_LED1, PHYSICAL_LED2, 0));
  TEST_ASSERT_TRUE(asdf_physical_allocate(&a, PHYSICAL_LED2, PHYSICAL_NO_OUT, 0));
  TEST_ASSERT_EQUAL_INT(PHYSICAL_NO_OUT, asdf_physical_next_device(&a, PHYSICAL_LED1));
  TEST_ASSERT_EQUAL_INT(PHYSICAL_LED2, asdf_physical_next_device(&b, PHYSICAL_LED1));

  hw_b.outputs[PHYSICAL_LED1] = 1;
  asdf_physical_assert(&a, PHYSICAL_LED1);
  asdf_physical_assert(&b, PHYSICAL_LED1);
  TEST_ASSERT_EQUAL_INT(1, hw_a.outputs[PHYSICAL_LED1]);
  TEST_ASSERT_EQUAL_INT(0, hw_b.outputs[PHYSICAL_LED1]);
}

// With no platform, a physical state tracks shadow values without driving
// hardware.
void no_platform_tracks_shadow_only(void)
{
  asdf_physical_state_t phys;
  asdf_physical_init(&phys, NULL);

  asdf_physical_set(&phys, PHYSICAL_LED1, 1);
  asdf_physical_pulse_delay_short(&phys);
  TEST_ASSERT_EQUAL_INT(0, asdf_arch_check_output(PHYSICAL_LED1));
  TEST_ASSERT_EQUAL_INT(1, phys.shadow[PHYSICAL_LED1]);
}

// Each emulated output setter records its own device, so tests can tell the
// outputs apart.
void emulated_output_setters_are_distinct(void)
{
  static const struct {
    void (*set)(uint8_t value);
    asdf_physical_dev_t dev;
  } setters[] = {
    { asdf_arch_out1_set, PHYSICAL_OUT1 },
    { asdf_arch_out1_open_hi_set, PHYSICAL_OUT1_OPEN_HI },
    { asdf_arch_out1_open_lo_set, PHYSICAL_OUT1_OPEN_LO },
    { asdf_arch_out2_set, PHYSICAL_OUT2 },
    { asdf_arch_out2_open_hi_set, PHYSICAL_OUT2_OPEN_HI },
    { asdf_arch_out2_open_lo_set, PHYSICAL_OUT2_OPEN_LO },
    { asdf_arch_out3_set, PHYSICAL_OUT3 },
    { asdf_arch_out3_open_hi_set, PHYSICAL_OUT3_OPEN_HI },
    { asdf_arch_out3_open_lo_set, PHYSICAL_OUT3_OPEN_LO },
    { asdf_arch_led1_set, PHYSICAL_LED1 },
    { asdf_arch_led2_set, PHYSICAL_LED2 },
    { asdf_arch_led3_set, PHYSICAL_LED3 },
  };

  for (unsigned i = 0; i < sizeof(setters) / sizeof(setters[0]); i++) {
    asdf_arch_test_reset();
    setters[i].set(1);
    for (int dev = PHYSICAL_OUT1; dev < ASDF_PHYSICAL_NUM_RESOURCES; dev++) {
      TEST_ASSERT_EQUAL_INT(dev == (int) setters[i].dev,
                            asdf_arch_check_output((asdf_physical_dev_t) dev));
    }
  }
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
  RUN_TEST(no_platform_tracks_shadow_only);
  RUN_TEST(emulated_output_setters_are_distinct);
  return UNITY_END();
}
