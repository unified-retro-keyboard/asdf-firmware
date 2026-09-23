// -*- mode: C; tab-width: 2 ; indent-tabs-mode: nil -*-
//
//  Unified Keyboard Project
//  ASDF keyboard firmware
//
//  asdf_arch_pic32cm_pl10_dip28.h
//
// Architecture-specific definitions for the PIC32CM6408PL10028 (SPDIP-28),
// "328p-class" variant: a 4-bit encoded row value driving an external 74LS138
// decoder, and a serial shift-register column read (COLCLK/COLMODE). The public
// API and scan/read logic mirror asdf_arch_atmega328p; only the pin-I/O
// primitives, clock/tick, and toolchain differ. Shared ARM mechanics live in
// asdf_arch_pic32cm_common.{c,h}. The external support chips (74LS138, column
// shift register) are retained from the 328P board.

#if !defined(ASDF_ARCH_H)
#define ASDF_ARCH_H

#include <stdint.h>
#include "asdf_arch_pic32cm_common.h"
#include "asdf.h"

#define ASDF_STROBE_LENGTH_US 10 // strobe length in microseconds

// Default key matrix row scanner
// Default keyboard output
// DIP switch is on row 8
#define ASDF_ARCH_DIP_SWITCH_ROW 8
#define ASDF_ARCH_DIPSWITCH_ROW 8

// --- pin map (PIC32CM6408PL10028, SPDIP-28) ---
// Provisional: any in-package PA pin compiles; the real SPDIP-28 pinout (which
// PA pins are bonded / 5V) is settled at hardware bring-up. All on PORT A.
// Rows: 4-bit encoded value -> external 74LS138 decoder, PA00..PA03.
#define ROW_GROUP 0u
#define ROW_SHIFT 0u
#define ROW_MASK 0x0000000Fu
// Columns: serial shift-register read. PA04 data in, PA05 clock, PA06 mode.
#define COL_GROUP 0u
#define COL_PIN 4u
#define COLCLK_GROUP 0u
#define COLCLK_PIN 5u
#define COLMODE_GROUP 0u
#define COLMODE_PIN 6u
// Strobe: PA07.
#define STROBE_GROUP 0u
#define STROBE_PIN 7u
// ASCII: 8-bit parallel output on PA08..PA15.
#define ASCII_GROUP 0u
#define ASCII_SHIFT 8u
#define ASCII_MASK (0xFFu << ASCII_SHIFT)
// LEDs (active low) on PA16/PA17/PA18.
#define LED1_GROUP 0u
#define LED1_PIN 16u
#define LED2_GROUP 0u
#define LED2_PIN 17u
#define LED3_GROUP 0u
#define LED3_PIN 18u
// OUT1-3 on PA19/PA20/PA21.
#define OUT1_GROUP 0u
#define OUT1_PIN 19u
#define OUT2_GROUP 0u
#define OUT2_PIN 20u
#define OUT3_GROUP 0u
#define OUT3_PIN 21u

// --- public API (mirrors asdf_arch_atmega328p.h) ---
void asdf_arch_set_pos_strobe(void);
void asdf_arch_set_neg_strobe(void);
void asdf_arch_null_output(uint8_t value);
void asdf_arch_led1_set(uint8_t value);
void asdf_arch_led2_set(uint8_t value);
void asdf_arch_led3_set(uint8_t value);
void asdf_arch_out1_set(uint8_t value);
void asdf_arch_out1_open_hi_set(uint8_t value);
void asdf_arch_out1_open_lo_set(uint8_t value);
void asdf_arch_out2_set(uint8_t value);
void asdf_arch_out2_open_hi_set(uint8_t value);
void asdf_arch_out2_open_lo_set(uint8_t value);
void asdf_arch_out3_set(uint8_t value);
void asdf_arch_out3_open_hi_set(uint8_t value);
void asdf_arch_out3_open_lo_set(uint8_t value);
asdf_cols_t asdf_arch_read_row(uint8_t row);
void asdf_arch_pulse_delay_short(void);
void asdf_arch_pulse_delay_long(void);
void asdf_arch_delay_ms(uint16_t delay_ms);
uint8_t asdf_arch_tick(void);
void asdf_arch_send_code(asdf_keycode_t code);
void asdf_arch_init(void);

#endif /* !defined (ASDF_ARCH_H) */
