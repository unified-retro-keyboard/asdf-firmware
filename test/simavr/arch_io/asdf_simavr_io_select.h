#ifndef ASDF_SIMAVR_IO_SELECT_H
#define ASDF_SIMAVR_IO_SELECT_H

#include <stdint.h>

#define ASDF_IO_MAX_LEDS 4

typedef struct {
  char port;
  int bit;
  int active_low;
} asdf_io_led_t;

typedef struct {
  const char *family_name;
  uint32_t cpu_frequency_hz;

  char row_port;
  uint8_t row_mask;
  int row_shift;
  char row_port_hi; /* '\0' if unused (family A) */
  int row_active_low;

  char col_port;
  int col_parallel;   /* 1 = parallel read, 0 = serial via shift reg */
  uint8_t col_mask;   /* only meaningful if col_parallel */
  int col_active_low; /* only meaningful if col_parallel */
  int col_bit_serial; /* only meaningful if !col_parallel */
  char col_load_clock_port;
  int col_load_clock_bit;
  char col_mode_port;
  int col_mode_bit;
  uint16_t col_pin_addr; /* data-memory address of the column PIN register;
                          * used by the harness to inject column states directly.
                          * For AVR standard IO: PIN_io_base + 0x20.
                          * For family A (serial), set to 0. */

  char data_port;
  uint8_t data_mask;

  char strobe_port;
  int strobe_bit;
  int strobe_active_high;

  int led_count;
  asdf_io_led_t leds[ASDF_IO_MAX_LEDS];

  /* OUT2 and LED2, named explicitly for the out2 regression mode.  LED2
   * also appears in leds[] above; it is repeated here because the mode
   * asserts on it by name, and indexing into leds[] would silently follow
   * any future reordering of that array.
   * out2_port is '\0' if OUT2 is not mapped for this family. */
  char out2_port;
  int out2_bit;
  char led2_port;
  int led2_bit;

  int dip_row;
  int dip_col_count;

  int scan_tick_hz;
} asdf_io_map_t;

const asdf_io_map_t *asdf_io_pick(const char *target);

#endif
