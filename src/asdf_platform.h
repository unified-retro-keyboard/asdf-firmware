// -*- mode: C; tab-width: 2 ; indent-tabs-mode: nil -*-
/**
 * @file asdf_platform.h
 *
 * Typed interface between the portable keyboard core and the hardware. A
 * platform object bundles the operations the core needs from the hardware
 * with a context pointer owned by the platform adapter, so each operation has
 * its real signature and no function pointer is ever cast. Each architecture
 * adapter (Arch/asdf_arch_*.c) embeds a platform in its own state object, so
 * every set of hardware has its own platform.
 *
 * Part of the Unified Keyboard Project ASDF keyboard firmware.
 *
 * @copyright Copyright 2019 David Fenyes. GNU General Public License
 * version 3 or later; see the license notice below.
 */
//
// This program is free software: you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free Software
// Foundation, either version 3 of the License, or (at your option) any later
// version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
// details.
//
// You should have received a copy of the GNU General Public License along with
// this program. If not, see <https://www.gnu.org/licenses/>.
//

#if !defined(ASDF_PLATFORM_H)
#define ASDF_PLATFORM_H

#include <stdint.h>
#include "asdf.h"
#include "asdf_physical.h"

/**
 * Read one row of the key matrix.
 *
 * @param user  The platform's user pointer.
 * @param row   Row to read.
 * @return One bit per column; a bit is set if the key in that column is
 *         pressed.
 */
typedef asdf_cols_t (*asdf_platform_read_row_t)(void *user, uint8_t row);

/**
 * Send one code to the host.
 *
 * @param user  The platform's user pointer.
 * @param code  Code to send.
 */
typedef void (*asdf_platform_send_code_t)(void *user, asdf_keycode_t code);

/**
 * Drive one physical output (LED or OUTn line) to a value.
 *
 * Outputs the hardware does not have, including PHYSICAL_NO_OUT, are ignored.
 *
 * @param user    The platform's user pointer.
 * @param output  Output to drive.
 * @param value   Level to drive: nonzero for high, 0 for low.
 */
typedef void (*asdf_platform_set_output_t)(void *user, asdf_physical_dev_t output,
                                           uint8_t value);

/**
 * Set the idle level of the output strobe.
 *
 * @param user      The platform's user pointer.
 * @param positive  Nonzero for a positive strobe (idle low); 0 for a negative
 *                  strobe (idle high).
 */
typedef void (*asdf_platform_set_strobe_polarity_t)(void *user, uint8_t positive);

/**
 * Wait for the width of a short output pulse.
 *
 * A bounded busy-wait of ASDF_PULSE_DELAY_SHORT_US microseconds.
 *
 * @param user  The platform's user pointer.
 */
typedef void (*asdf_platform_pulse_delay_short_t)(void *user);

/**
 * Return the output configuration to its power-on defaults.
 *
 * Restores the default data polarity and strobe polarity. Called when a keymap
 * is selected, before the keymap configures the hardware.
 *
 * @param user  The platform's user pointer.
 */
typedef void (*asdf_platform_reset_t)(void *user);

/**
 * The hardware operations the keyboard core needs, with the adapter's context.
 *
 * The core calls every operation with @p user as its first argument, so one
 * adapter can serve several sets of hardware. Every operation must be set: the
 * core calls them without checking for NULL. The core calls the operations
 * only from the thread that owns the keyboard, so an operation needs no
 * locking against the core.
 *
 * @code
 * #include "asdf_keyboard.h"
 * #include "asdf_platform.h"
 *
 * typedef struct {
 *   uint8_t strobe_positive;
 * } my_adapter_t;
 *
 * static my_adapter_t my_adapter;
 *
 * static asdf_cols_t my_read_row(void *user, uint8_t row);
 * static void my_send_code(void *user, asdf_keycode_t code);
 * static void my_set_output(void *user, asdf_physical_dev_t output,
 *                           uint8_t value);
 * static void my_pulse_delay_short(void *user);
 * static void my_reset(void *user);
 *
 * static void my_set_strobe_polarity(void *user, uint8_t positive)
 * {
 *   my_adapter_t *adapter = user; // &my_adapter
 *
 *   adapter->strobe_positive = positive ? 1 : 0;
 *   // ... drive the strobe line to its new idle level
 * }
 *
 * static const asdf_platform_t my_platform = {
 *   .user = &my_adapter,
 *   .read_row = my_read_row,
 *   .send_code = my_send_code,
 *   .set_output = my_set_output,
 *   .set_strobe_polarity = my_set_strobe_polarity,
 *   .pulse_delay_short = my_pulse_delay_short,
 *   .reset = my_reset,
 * };
 *
 * asdf_t kb;
 *
 * asdf_init(&kb, &my_platform);
 * @endcode
 */
typedef struct asdf_platform {
  void *user; ///< passed to every operation; owned by the platform adapter
  asdf_platform_read_row_t read_row;
  asdf_platform_send_code_t send_code;
  asdf_platform_set_output_t set_output;
  asdf_platform_set_strobe_polarity_t set_strobe_polarity;
  asdf_platform_pulse_delay_short_t pulse_delay_short;
  asdf_platform_reset_t reset;
} asdf_platform_t;

#endif /* !defined (ASDF_PLATFORM_H) */

//-------|---------|---------+---------+---------+---------+---------+---------+
// Above line is 80 columns, and should display completely in the editor.
