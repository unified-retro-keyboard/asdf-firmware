// -*- mode: C; tab-width: 2 ; indent-tabs-mode: nil -*-
//
//  Unified Keyboard Project
//  ASDF keyboard firmware
//
//  asdf_arch_pic32cm_pl10_q64.h
//
// Architecture-specific definitions for the PIC32CM6408PL10064 (64-pin),
// "2560-class" variant: 16 directly-driven one-hot row lines, 8-bit parallel
// inverted column read. The public API and scan/read logic mirror
// asdf_arch_atmega2560; only the pin-I/O primitives, clock/tick, and toolchain
// differ. Shared ARM mechanics live in asdf_arch_pic32cm_common.{c,h}.

#if !defined(ASDF_ARCH_H)
#define ASDF_ARCH_H

#include <stdint.h>
#include "asdf_arch_pic32cm_common.h"
#include "asdf.h"

#define ASDF_STROBE_LENGTH_US 10              // strobe length in microseconds
#define ASDF_KEYBOARD_ROW_SETTLING_TIME_US 4  // row settling time

// DIP switch is on row 8
#define ASDF_ARCH_DIP_SWITCH_ROW 8
#define ASDF_ARCH_DIPSWITCH_ROW 8

// --- pin map (PIC32CM6408PL10064, 64-pin); adjustable, SWCLK/SWDIO reserved ---
// Rows: 16 one-hot active-low lines on PA00..PA15 (single masked write).
#define ROW_GROUP 0u
#define ROW_MASK 0x0000FFFFu
// Columns: 8 inverted inputs on PB00..PB07 (a pressed key pulls the line low).
#define COL_GROUP 1u
#define COL_MASK 0x000000FFu
// ASCII: 8-bit parallel output on PA16..PA23.
#define ASCII_GROUP 0u
#define ASCII_SHIFT 16u
#define ASCII_MASK (0xFFu << ASCII_SHIFT)
// Strobe: PA24.
#define STROBE_GROUP 0u
#define STROBE_PIN 24u
// LEDs (active low) on PB08/PB09/PB10.
#define LED1_GROUP 1u
#define LED1_PIN 8u
#define LED2_GROUP 1u
#define LED2_PIN 9u
#define LED3_GROUP 1u
#define LED3_PIN 10u
// OUT1-3 on PB11/PB12/PB13.
#define OUT1_GROUP 1u
#define OUT1_PIN 11u
#define OUT2_GROUP 1u
#define OUT2_PIN 12u
#define OUT3_GROUP 1u
#define OUT3_PIN 13u
// OSI control lines overlay the upper row-select lines (2560 parity): PA09/PA10.
#define OSI_KBE_GROUP 0u
#define OSI_KBE_PIN 9u
#define OSI_RW_GROUP 0u
#define OSI_RW_PIN 10u

// --- public API (mirrors asdf_arch_atmega2560.h) ---
/**
 * Reads one row of an OSI keyboard.
 *
 * An alternative row reader for OSI keyboards. Not used by any keymap yet; a
 * keymap uses it by installing a platform whose read_row calls it. Drives the
 * row and OSI keyboard control lines, and leaves the column lines as inputs.
 *
 * @param row  Row number to scan.
 * @return The row's columns, one bit per column, with 1 = pressed.
 */
asdf_cols_t asdf_arch_osi_read_row(uint8_t row);

/**
 * Sets up the keyboard hardware and the platform embedded in arch.
 *
 * Call once, before the keyboard runs. Sets the core clock, the pins (row,
 * column, ASCII, strobe, LED, OUT1-3 and OSI control lines) and the default
 * data and strobe polarity; fills in the platform operations and clears the
 * tick count; and starts the 1 ms SysTick tick interrupt.
 *
 * @param arch  Hardware state to initialize.
 */
void asdf_arch_init(asdf_arch_t *arch);

#endif /* !defined (ASDF_ARCH_H) */
