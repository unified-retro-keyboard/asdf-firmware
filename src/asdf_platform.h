// -*- mode: C; tab-width: 2 ; indent-tabs-mode: nil -*-
//
// Unified Keyboard Project
// ASDF keyboard firmware
//
// asdf_platform.h
//
// Typed interface between the portable keyboard core and the hardware. A
// platform object bundles the operations the core needs from the hardware
// with a context pointer owned by the platform adapter, so each operation has
// its real signature and no function pointer is ever cast. Each architecture
// adapter (Arch/asdf_arch_*.c) embeds a platform in its own state object, so
// every set of hardware has its own platform.
//
// Copyright 2019 David Fenyes
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

// Reads one row of the key matrix; returns a bit per pressed column.
typedef asdf_cols_t (*asdf_platform_read_row_t)(void *user, uint8_t row);

// Sends one code to the host.
typedef void (*asdf_platform_send_code_t)(void *user, asdf_keycode_t code);

// Drives one physical output (LED or OUTn line) to value. Outputs the hardware
// does not have are ignored.
typedef void (*asdf_platform_set_output_t)(void *user, asdf_physical_dev_t output,
                                           uint8_t value);

// Sets the idle level of the output strobe: positive (idle low) if positive is
// nonzero, negative (idle high) otherwise.
typedef void (*asdf_platform_set_strobe_polarity_t)(void *user, uint8_t positive);

// Waits for the width of a short output pulse (ASDF_PULSE_DELAY_SHORT_US). A
// bounded busy-wait of a few microseconds.
typedef void (*asdf_platform_pulse_delay_short_t)(void *user);

// Returns the output configuration (data and strobe polarity) to its power-on
// defaults. Called when a keymap is selected, before the keymap configures the
// hardware.
typedef void (*asdf_platform_reset_t)(void *user);

typedef struct asdf_platform {
  void *user; // passed to every operation; owned by the platform adapter
  asdf_platform_read_row_t read_row;
  asdf_platform_send_code_t send_code;
  asdf_platform_set_output_t set_output;
  asdf_platform_set_strobe_polarity_t set_strobe_polarity;
  asdf_platform_pulse_delay_short_t pulse_delay_short;
  asdf_platform_reset_t reset;
} asdf_platform_t;

// PROCEDURE: asdf_install_platform
// INPUTS: (const asdf_platform_t *) new_platform - platform to use, or NULL for
//         the platform given to asdf_init()
// OUTPUTS: none
// DESCRIPTION: Selects the platform through which the key matrix is read,
// codes are sent, and outputs are driven. A keymap switch restores the
// platform given to asdf_init().
void asdf_install_platform(const asdf_platform_t *new_platform);

// PROCEDURE: asdf_current_platform
// OUTPUTS: returns the platform currently in use
const asdf_platform_t *asdf_current_platform(void);

#endif /* !defined (ASDF_PLATFORM_H) */

//-------|---------|---------+---------+---------+---------+---------+---------+
// Above line is 80 columns, and should display completely in the editor.
