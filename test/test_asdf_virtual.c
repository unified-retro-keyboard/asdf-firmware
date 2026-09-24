#include <stdint.h>
#include <stdarg.h>
#include "asdf_arch_test.h"
#include "unity.h"
#include "asdf.h"
#include "asdf_ascii.h"
#include "asdf_modifiers.h"
#include "asdf_keymaps.h"
#include "asdf_config.h"
#include "test_asdf_lib.h"
#include "test_asdf_keymap_defs.h"
#include "fake_platform.h"
#include "asdf_keyboard.h"

// The keyboard under test.
static asdf_t kb;

static uint32_t key_matrix[TEST_NUM_ROWS];


void setUp(void)
{
  asdf_init(&kb, &asdf_arch_platform);

  asdf_keymaps_select(&kb, SINGLE_TESTS_KEYMAP);

  for (uint32_t i = 0; i < TEST_NUM_ROWS; i++) {
    key_matrix[i] = 0;
  }
}

void tearDown(void) {}

// needed for keymap / scanner integration.
asdf_cols_t asdf_arch_read_row(uint8_t row)
{
  return key_matrix[row];
}

void test_single_virtual_output_is_initialized(void)
{
  // initially on keymap 0.  Test to see that OUT1 has been initialized to 0.
  TEST_ASSERT_EQUAL_INT32(asdf_arch_check_output(PHYSICAL_OUT1), 0);
  // and verify that this is not just the default value
  TEST_ASSERT_NOT_EQUAL(ASDF_VIRTUAL_OUT_DEFAULT_VALUE, asdf_arch_check_output(PHYSICAL_OUT1));
}

void test_uninitialized_virtual_out_is_default(void)
{
  TEST_ASSERT_EQUAL_INT32(ASDF_VIRTUAL_OUT_DEFAULT_VALUE, asdf_arch_check_output(PHYSICAL_LED2));
}

void test_set_virtual_output(void)
{
  asdf_virtual_action(&kb.outputs, VOUT1, V_SET_LO);
  TEST_ASSERT_EQUAL_INT32(0, asdf_arch_check_output(PHYSICAL_OUT1));

  asdf_virtual_action(&kb.outputs, VOUT1, V_SET_HI);
  TEST_ASSERT_EQUAL_INT32(1, asdf_arch_check_output(PHYSICAL_OUT1));
}

void test_toggle_virtual_output(void)
{
  // start by setting vout1 to 0
  asdf_virtual_action(&kb.outputs, VOUT1, V_SET_LO);
  TEST_ASSERT_EQUAL_INT32(0, asdf_arch_check_output(PHYSICAL_OUT1));

  // toggle high
  asdf_virtual_action(&kb.outputs, VOUT1, V_TOGGLE);
  TEST_ASSERT_EQUAL_INT32(1, asdf_arch_check_output(PHYSICAL_OUT1));

  // toggle back low.
  asdf_virtual_action(&kb.outputs, VOUT1, V_TOGGLE);
  TEST_ASSERT_EQUAL_INT32(0, asdf_arch_check_output(PHYSICAL_OUT1));

}

void test_pulse_high_virtual_output(void)
{
  asdf_virtual_action(&kb.outputs, VOUT1, V_SET_LO);
  TEST_ASSERT_EQUAL_INT32(PD_ST_STABLE_LOW, asdf_arch_check_pulse(PHYSICAL_OUT1));
  TEST_ASSERT_EQUAL_INT32(0, asdf_arch_check_output(PHYSICAL_OUT1));

  asdf_virtual_action(&kb.outputs, VOUT1, V_PULSE_SHORT);

  // output should be low
  TEST_ASSERT_EQUAL_INT32(0, asdf_arch_check_output(PHYSICAL_OUT1));
  // high pulse should be detected.
  TEST_ASSERT_EQUAL_INT32(PD_ST_PULSE_HIGH_DETECTED, asdf_arch_check_pulse(PHYSICAL_OUT1));
}

void test_pulse_low_virtual_output(void)
{
  asdf_virtual_action(&kb.outputs, VOUT1, V_SET_HI);
  asdf_virtual_action(&kb.outputs, VOUT1, V_SET_HI);
  TEST_ASSERT_EQUAL_INT32(PD_ST_STABLE_HIGH, asdf_arch_check_pulse(PHYSICAL_OUT1));
  TEST_ASSERT_EQUAL_INT32(1, asdf_arch_check_output(PHYSICAL_OUT1));

  asdf_virtual_action(&kb.outputs, VOUT1, V_PULSE_SHORT);

  // output should be high
  TEST_ASSERT_EQUAL_INT32(1, asdf_arch_check_output(PHYSICAL_OUT1));
  // low pulse should be detected.
  TEST_ASSERT_EQUAL_INT32(PD_ST_PULSE_LOW_DETECTED, asdf_arch_check_pulse(PHYSICAL_OUT1));
}

// This test ties three real outputs to a virtual output and toggles the virtual
// output.
void test_toggle_triple_output(void)
{
  asdf_keymaps_select(&kb, TRIPLE_TESTS_KEYMAP);

  // check that initial values have been set:
  TEST_ASSERT_EQUAL_INT32(0, asdf_arch_check_output(PHYSICAL_OUT1));
  TEST_ASSERT_EQUAL_INT32(1, asdf_arch_check_output(PHYSICAL_OUT2));
  TEST_ASSERT_EQUAL_INT32(0, asdf_arch_check_output(PHYSICAL_OUT3));

  asdf_virtual_activate(&kb.outputs, VOUT1); // funtion is set to toggle
  TEST_ASSERT_EQUAL_INT32(1, asdf_arch_check_output(PHYSICAL_OUT1));
  TEST_ASSERT_EQUAL_INT32(0, asdf_arch_check_output(PHYSICAL_OUT2));
  TEST_ASSERT_EQUAL_INT32(1, asdf_arch_check_output(PHYSICAL_OUT3));

  asdf_virtual_action(&kb.outputs, VOUT1, V_TOGGLE);
  TEST_ASSERT_EQUAL_INT32(0, asdf_arch_check_output(PHYSICAL_OUT1));
  TEST_ASSERT_EQUAL_INT32(1, asdf_arch_check_output(PHYSICAL_OUT2));
  TEST_ASSERT_EQUAL_INT32(0, asdf_arch_check_output(PHYSICAL_OUT3));
}

// This test ties three real outputs to a virtual output and sets the virtual
// output high and low
void test_set_triple_output(void)
{
  asdf_keymaps_select(&kb, TRIPLE_TESTS_KEYMAP);

  // check that initial values have been set:
  TEST_ASSERT_EQUAL_INT32(0, asdf_arch_check_output(PHYSICAL_OUT1));
  TEST_ASSERT_EQUAL_INT32(1, asdf_arch_check_output(PHYSICAL_OUT2));
  TEST_ASSERT_EQUAL_INT32(0, asdf_arch_check_output(PHYSICAL_OUT3));

  asdf_virtual_action(&kb.outputs, VOUT1, V_SET_HI);
  TEST_ASSERT_EQUAL_INT32(1, asdf_arch_check_output(PHYSICAL_OUT1));
  TEST_ASSERT_EQUAL_INT32(1, asdf_arch_check_output(PHYSICAL_OUT2));
  TEST_ASSERT_EQUAL_INT32(1, asdf_arch_check_output(PHYSICAL_OUT3));

  asdf_virtual_action(&kb.outputs, VOUT1, V_SET_LO);
  TEST_ASSERT_EQUAL_INT32(0, asdf_arch_check_output(PHYSICAL_OUT1));
  TEST_ASSERT_EQUAL_INT32(0, asdf_arch_check_output(PHYSICAL_OUT2));
  TEST_ASSERT_EQUAL_INT32(0, asdf_arch_check_output(PHYSICAL_OUT3));
}

// This test ties three real outputs to a virtual output and pulses the virtual
// output high and low
void test_pulse_triple_output(void)
{
  asdf_keymaps_select(&kb, TRIPLE_TESTS_KEYMAP);
  // check that initial values have been set:
  TEST_ASSERT_EQUAL_INT32(0, asdf_arch_check_output(PHYSICAL_OUT1));
  TEST_ASSERT_EQUAL_INT32(1, asdf_arch_check_output(PHYSICAL_OUT2));
  TEST_ASSERT_EQUAL_INT32(0, asdf_arch_check_output(PHYSICAL_OUT3));

  // create stable (non-pulse) hi state by asserting high twice.
  asdf_virtual_action(&kb.outputs, VOUT1, V_SET_HI);
  asdf_virtual_action(&kb.outputs, VOUT1, V_SET_HI);
  TEST_ASSERT_EQUAL_INT32(1, asdf_arch_check_output(PHYSICAL_OUT1));
  TEST_ASSERT_EQUAL_INT32(1, asdf_arch_check_output(PHYSICAL_OUT2));
  TEST_ASSERT_EQUAL_INT32(1, asdf_arch_check_output(PHYSICAL_OUT3));

  TEST_ASSERT_EQUAL_INT32(PD_ST_STABLE_HIGH, asdf_arch_check_pulse(PHYSICAL_OUT1));
  TEST_ASSERT_EQUAL_INT32(PD_ST_STABLE_HIGH, asdf_arch_check_pulse(PHYSICAL_OUT2));
  TEST_ASSERT_EQUAL_INT32(PD_ST_STABLE_HIGH, asdf_arch_check_pulse(PHYSICAL_OUT3));

  asdf_virtual_action(&kb.outputs, VOUT1, V_SET_LO);
  TEST_ASSERT_EQUAL_INT32(0, asdf_arch_check_output(PHYSICAL_OUT1));
  TEST_ASSERT_EQUAL_INT32(0, asdf_arch_check_output(PHYSICAL_OUT2));
  TEST_ASSERT_EQUAL_INT32(0, asdf_arch_check_output(PHYSICAL_OUT3));

  TEST_ASSERT_EQUAL_INT32(PD_ST_TRANSITION_LOW, asdf_arch_check_pulse(PHYSICAL_OUT1));
  TEST_ASSERT_EQUAL_INT32(PD_ST_TRANSITION_LOW, asdf_arch_check_pulse(PHYSICAL_OUT2));
  TEST_ASSERT_EQUAL_INT32(PD_ST_TRANSITION_LOW, asdf_arch_check_pulse(PHYSICAL_OUT3));
}

// This test ties three real outputs to a virtual output and pulses the virtual
// output high and low
void test_activate_triple_output(void)
{
  asdf_keymaps_select(&kb, TRIPLE_TESTS_KEYMAP);
  // check that initial values have been set:
  TEST_ASSERT_EQUAL_INT32(0, asdf_arch_check_output(PHYSICAL_OUT1));
  TEST_ASSERT_EQUAL_INT32(1, asdf_arch_check_output(PHYSICAL_OUT2));
  TEST_ASSERT_EQUAL_INT32(0, asdf_arch_check_output(PHYSICAL_OUT3));

  asdf_virtual_activate(&kb.outputs, VOUT1);
  TEST_ASSERT_EQUAL_INT32(1, asdf_arch_check_output(PHYSICAL_OUT1));
  TEST_ASSERT_EQUAL_INT32(0, asdf_arch_check_output(PHYSICAL_OUT2));
  TEST_ASSERT_EQUAL_INT32(1, asdf_arch_check_output(PHYSICAL_OUT3));

  asdf_virtual_activate(&kb.outputs, VOUT1);
  TEST_ASSERT_EQUAL_INT32(0, asdf_arch_check_output(PHYSICAL_OUT1));
  TEST_ASSERT_EQUAL_INT32(1, asdf_arch_check_output(PHYSICAL_OUT2));
  TEST_ASSERT_EQUAL_INT32(0, asdf_arch_check_output(PHYSICAL_OUT3));
}

uint8_t *output_array(void)
{
  static uint8_t outputs[ASDF_PHYSICAL_NUM_RESOURCES] = { 0 };
  for (uint8_t i = 0; i < ASDF_PHYSICAL_NUM_RESOURCES; i++) {
    outputs[i] = asdf_arch_check_output(i);
    printf("output %d: %d\n", i, outputs[i]);
  }
  return outputs;
}

uint8_t *all_set_array(void)
{
  static uint8_t outputs[ASDF_PHYSICAL_NUM_RESOURCES] = { 0 };
  for (uint8_t i = 0; i < ASDF_PHYSICAL_NUM_RESOURCES; i++) {
    outputs[i] = 1;
  }
  return outputs;
}

uint8_t *all_zero_array(void)
{
  static uint8_t outputs[ASDF_PHYSICAL_NUM_RESOURCES] = { 0 };
  for (uint8_t i = 0; i < ASDF_PHYSICAL_NUM_RESOURCES; i++) {
    outputs[i] = 0;
  }
  return outputs;
}

uint8_t *single_zero_array(asdf_physical_dev_t set_element)
{
  static uint8_t outputs[ASDF_PHYSICAL_NUM_RESOURCES] = { 0 };
  for (uint8_t i = 0; i < ASDF_PHYSICAL_NUM_RESOURCES; i++) {
    outputs[i] = 1;
  }
  outputs[set_element] = 0;
  return outputs;
}


void test_virtual_capslock_indicator(void)
{

  asdf_keymaps_select(&kb, VCAPS_TEST_KEYMAP);

  // CAPS LED output should be initialized to zero:
  TEST_ASSERT_EQUAL_INT32(0, asdf_arch_check_output(PHYSICAL_LED1));

  // emulate capslock press and release.  Should set LED1
  asdf_modifier_capslock_activate(&kb.modifiers);
  asdf_sync_lock_leds(&kb);

  TEST_ASSERT_EQUAL_INT32(1, asdf_arch_check_output(PHYSICAL_LED1));


  // emulate capslock press and release.  clear LED1
  asdf_modifier_capslock_activate(&kb.modifiers);
  asdf_sync_lock_leds(&kb);

  TEST_ASSERT_EQUAL_INT32(0, asdf_arch_check_output(PHYSICAL_LED1));
}

void test_virtual_shiftlock_indicator(void)
{

  asdf_keymaps_select(&kb, VSHIFT_TEST_KEYMAP);

  // CAPS LED output should be initialized to zero:
  TEST_ASSERT_EQUAL_INT32(0, asdf_arch_check_output(PHYSICAL_LED2));

  // emulate shiftlock press and release.  Should set LED2
  asdf_modifier_shiftlock_on_activate(&kb.modifiers);
  asdf_sync_lock_leds(&kb);

  TEST_ASSERT_EQUAL_INT32(1, asdf_arch_check_output(PHYSICAL_LED2));


  // emulate shift press and release.  clear LED2
  asdf_modifier_shift_activate(&kb.modifiers);
  asdf_sync_lock_leds(&kb);
  asdf_modifier_shift_deactivate(&kb.modifiers);
  asdf_sync_lock_leds(&kb);

  TEST_ASSERT_EQUAL_INT32(0, asdf_arch_check_output(PHYSICAL_LED2));
}


void test_cant_assign_real_output_twice(void)
{
  asdf_keymaps_select(&kb, DOUBLE_ASSIGN_TEST_KEYMAP);

  // initial value should be set to 0:
  TEST_ASSERT_EQUAL_INT32(0, asdf_arch_check_output(PHYSICAL_LED1));

  // set LED1 high from valid VOUT4
  asdf_virtual_action(&kb.outputs, VOUT4, V_SET_HI);
  TEST_ASSERT_EQUAL_INT32(1, asdf_arch_check_output(PHYSICAL_LED1));

  // set LED1 low from valid VOUT4
  asdf_virtual_action(&kb.outputs, VOUT4, V_SET_LO);
  TEST_ASSERT_EQUAL_INT32(0, asdf_arch_check_output(PHYSICAL_LED1));

  // set LED1 high from invalid VOUT5
  asdf_virtual_action(&kb.outputs, VOUT5, V_SET_HI);
  // Should not have changed.
  TEST_ASSERT_EQUAL_INT32(0, asdf_arch_check_output(PHYSICAL_LED1));
}

// Two virtual output states map and drive outputs independently, each on its
// own hardware.
void test_independent_virtual_states(void)
{
  fake_platform_t hw_a, hw_b;
  asdf_virtual_state_t a, b;

  fake_platform_init(&hw_a);
  fake_platform_init(&hw_b);
  asdf_virtual_init(&a, &hw_a.platform);
  asdf_virtual_init(&b, &hw_b.platform);
  asdf_virtual_assign(&a, VOUT1, PHYSICAL_LED1, V_TOGGLE, 0);
  asdf_virtual_assign(&b, VOUT1, PHYSICAL_LED2, V_TOGGLE, 1);
  asdf_virtual_assign(&b, VOUT2, PHYSICAL_LED1, V_PULSE_SHORT, 0);
  asdf_virtual_sync(&a);
  asdf_virtual_sync(&b);
  TEST_ASSERT_EQUAL_INT(0, hw_a.outputs[PHYSICAL_LED1]);
  TEST_ASSERT_EQUAL_INT(1, hw_b.outputs[PHYSICAL_LED2]);

  asdf_virtual_activate(&a, VOUT1);
  TEST_ASSERT_EQUAL_INT(1, hw_a.outputs[PHYSICAL_LED1]);
  TEST_ASSERT_EQUAL_INT(0, hw_b.outputs[PHYSICAL_LED1]);

  asdf_virtual_activate(&b, VOUT1);
  TEST_ASSERT_EQUAL_INT(0, hw_b.outputs[PHYSICAL_LED2]);
  TEST_ASSERT_EQUAL_INT(ASDF_VIRTUAL_OUT_DEFAULT_VALUE, hw_a.outputs[PHYSICAL_LED2]);

  // a short pulse waits on its own hardware only
  asdf_virtual_activate(&b, VOUT2);
  TEST_ASSERT_EQUAL_INT(1, hw_b.short_pulses);
  TEST_ASSERT_EQUAL_INT(0, hw_a.short_pulses);
  TEST_ASSERT_EQUAL_INT(1, hw_a.outputs[PHYSICAL_LED1]);
}

void test_invalid_virtual_output_is_ignored(void)
{
  asdf_virtual_state_t v;

  asdf_virtual_init(&v, &asdf_arch_platform);
  asdf_virtual_action(&v, ASDF_VIRTUAL_NUM_RESOURCES, V_SET_HI);
  asdf_virtual_activate(&v, ASDF_VIRTUAL_NUM_RESOURCES);
  asdf_virtual_assign(&v, ASDF_VIRTUAL_NUM_RESOURCES, PHYSICAL_LED1, V_SET_HI, 0);
  // LED1 is still available, so the invalid assign did not allocate it
  asdf_virtual_assign(&v, VOUT1, PHYSICAL_LED1, V_SET_HI, 0);
  asdf_virtual_activate(&v, VOUT1);
  TEST_ASSERT_EQUAL_INT(1, asdf_arch_check_output(PHYSICAL_LED1));
}

// A long pulse toggles its outputs, and toggles them back after
// ASDF_PULSE_DELAY_LONG_MS ticks without blocking. Activating it again while
// it is in progress does not restart it.
void test_long_pulse_is_scheduled(void)
{
  asdf_virtual_state_t v;

  asdf_virtual_init(&v, &asdf_arch_platform);
  asdf_virtual_assign(&v, VOUT1, PHYSICAL_OUT1, V_PULSE_LONG, 1);
  asdf_virtual_sync(&v);
  TEST_ASSERT_EQUAL_INT(1, asdf_arch_check_output(PHYSICAL_OUT1));

  asdf_virtual_activate(&v, VOUT1);
  TEST_ASSERT_EQUAL_INT(0, asdf_arch_check_output(PHYSICAL_OUT1));

  asdf_virtual_tick(&v, ASDF_PULSE_DELAY_LONG_MS - 10);
  asdf_virtual_activate(&v, VOUT1); // ignored: pulse in progress
  TEST_ASSERT_EQUAL_INT(0, asdf_arch_check_output(PHYSICAL_OUT1));

  asdf_virtual_tick(&v, 9);
  TEST_ASSERT_EQUAL_INT(0, asdf_arch_check_output(PHYSICAL_OUT1));

  asdf_virtual_tick(&v, 1);
  TEST_ASSERT_EQUAL_INT(1, asdf_arch_check_output(PHYSICAL_OUT1));

  // a later tick has no further effect
  asdf_virtual_tick(&v, 100);
  TEST_ASSERT_EQUAL_INT(1, asdf_arch_check_output(PHYSICAL_OUT1));
}

int main(void)
{
  UNITY_BEGIN();
  RUN_TEST(test_single_virtual_output_is_initialized);
  RUN_TEST(test_uninitialized_virtual_out_is_default);
  RUN_TEST(test_set_virtual_output);
  RUN_TEST(test_toggle_virtual_output);
  RUN_TEST(test_pulse_high_virtual_output);
  RUN_TEST(test_pulse_low_virtual_output);
  RUN_TEST(test_toggle_triple_output);
  RUN_TEST(test_set_triple_output);
  RUN_TEST(test_activate_triple_output);
  RUN_TEST(test_pulse_triple_output);
  RUN_TEST(test_virtual_capslock_indicator);
  RUN_TEST(test_virtual_shiftlock_indicator);
  RUN_TEST(test_cant_assign_real_output_twice);
  RUN_TEST(test_independent_virtual_states);
  RUN_TEST(test_invalid_virtual_output_is_ignored);
  RUN_TEST(test_long_pulse_is_scheduled);
  return UNITY_END();
}
