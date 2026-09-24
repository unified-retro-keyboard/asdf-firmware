#if !defined(TEST_ASDF_LIB_H)
#define TEST_ASDF_LIB_H

#include <stdint.h>
#include "asdf.h"
#include "asdf_actions.h"

// A key's value for comparisons in tests: the code it sends (repeatable or
// not), or TEST_ACTION(fn) for a key whose press action is fn.
#define TEST_ACTION(fn) (0x100u + (fn))
uint16_t test_key_value(asdf_key_t key);

// The value (see test_key_value) of the key at a position in a keyboard's
// keymap.
#define test_get_code(kb, row, col, modifier)                                                     \
  test_key_value(asdf_keymaps_get_key(&(kb)->keymap, (row), (col), (modifier)))

// The next code ready to send from kb, or ASDF_INVALID_CODE if none is ready.
uint16_t test_next_code(asdf_t *kb);

uint32_t max(uint8_t first, uint8_t second);
void test_hook_clear(void);
uint32_t test_hook_readback(void);
uint8_t test_hook_read_row(uint8_t val);
void test_hook_output(uint8_t val);

// Test actions (see test_asdf_keymap_defs.h). Each scan adds 1 to the
// test_hook_readback() value; the HERE_IS action counts its calls.
void test_action_each_scan(asdf_t *kb, uint8_t param);
void test_action_here_is(asdf_t *kb, uint8_t param);
uint32_t test_here_is_count(void);
void test_here_is_clear(void);


#endif // if !defined(TEST_ASDF_LIB_H)
