#include<stdint.h>
#include "asdf.h"
#include "asdf_virtual.h"
#include "asdf_physical.h"
#include "asdf_keymaps.h"
#include "asdf_arch.h"
#include "test_asdf_lib.h"

uint32_t hook_register;

uint16_t test_key_value(asdf_key_t key)
{
  return (ACTION_SEND_CODE == key.press_fn || ACTION_SEND_REPEATABLE_CODE == key.press_fn)
           ? key.press_param
           : (uint16_t) TEST_ACTION(key.press_fn);
}

uint32_t max(uint8_t first, uint8_t second)
{
  uint32_t max = first;
  if (second > max) {
    max = second;
  }
  return max;
}

void test_hook_clear(void)
{
  hook_register = 0;
}

uint32_t test_hook_readback(void)
{
  return hook_register;
}

uint8_t test_hook_read_row(uint8_t val)
{
  return (uint8_t) val+2;
}

void test_hook_output(uint8_t val)
{
  hook_register = val;
}

void test_action_each_scan(asdf_t *kb, uint8_t param)
{
  (void) kb;
  (void) param;
  hook_register++;
}

static uint32_t here_is_calls;

void test_action_here_is(asdf_t *kb, uint8_t param)
{
  (void) kb;
  (void) param;
  here_is_calls++;
}

uint32_t test_here_is_count(void)
{
  return here_is_calls;
}

void test_here_is_clear(void)
{
  here_is_calls = 0;
}
