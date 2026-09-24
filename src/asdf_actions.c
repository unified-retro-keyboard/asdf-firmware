// -*- mode: C; tab-width: 2 ; indent-tabs-mode: nil -*-
//
// Unified Keyboard Project
// ASDF keyboard firmware
//
// asdf_actions.c
//
// The built-in key actions, and action dispatch. See asdf_actions.h.
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

#include <stddef.h>
#include <stdint.h>
#include "asdf.h"
#include "asdf_actions.h"
#include "asdf_arch.h"
#include "asdf_keyboard.h"
#include "asdf_keymap_setup.h"
#include "asdf_keymaps.h"
#include "asdf_modifiers.h"
#include "asdf_print.h"
#include "asdf_repeat.h"
#include "asdf_virtual.h"

// PROCEDURE: asdf_action_r
// INPUTS: (asdf_t *) kb - keyboard
//         (uint8_t) fn - action number
//         (uint8_t) param - parameter for the action
// OUTPUTS: none
//
// DESCRIPTION: Calls the action's function from the action table. Every
// action number has a table entry, so no check is needed.
//
// SCOPE: public
//
// COMPLEXITY: 1
//
void asdf_action_r(asdf_t *kb, uint8_t fn, uint8_t param)
{
  asdf_action_fn_t action = (asdf_action_fn_t) FLASH_READ_PTR(&asdf_action_table[fn]);

  action(kb, param);
}

// PROCEDURE: asdf_is_configuration_action
// INPUTS: (uint8_t) fn - action number
// OUTPUTS: returns TRUE (nonzero) if fn is a configuration action
//
// SCOPE: public
//
// COMPLEXITY: 2
//
uint8_t asdf_is_configuration_action(uint8_t fn)
{
  switch (fn) {
    case ACTION_MAPSEL_SET:
    case ACTION_STROBE_POSITIVE:
    case ACTION_AUTOREPEAT_ON: return 1;
    default: return 0;
  }
}

// The built-in actions. Actions that take no parameter ignore it. SHIFT and
// CAPS changes also drive the lock indicators.

void asdf_action_nothing(asdf_t *kb, uint8_t param)
{
  (void) kb;
  (void) param;
}

void asdf_action_send_code(asdf_t *kb, uint8_t code) { asdf_put_code_r(kb, code); }

// Queues the code, and makes the key that ran this action the repeating key.
void asdf_action_send_repeatable_code(asdf_t *kb, uint8_t code)
{
  asdf_put_code_r(kb, code);
  asdf_arm_repeat_r(kb);
}

void asdf_action_shift(asdf_t *kb, uint8_t param)
{
  (void) param;
  asdf_modifier_shift_activate_r(&kb->modifiers);
  asdf_sync_lock_leds_r(kb);
}

void asdf_action_shift_release(asdf_t *kb, uint8_t param)
{
  (void) param;
  asdf_modifier_shift_deactivate_r(&kb->modifiers);
  asdf_sync_lock_leds_r(kb);
}

void asdf_action_shiftlock_on(asdf_t *kb, uint8_t param)
{
  (void) param;
  asdf_modifier_shiftlock_on_activate_r(&kb->modifiers);
  asdf_sync_lock_leds_r(kb);
}

void asdf_action_shiftlock_toggle(asdf_t *kb, uint8_t param)
{
  (void) param;
  asdf_modifier_shiftlock_toggle_activate_r(&kb->modifiers);
  asdf_sync_lock_leds_r(kb);
}

void asdf_action_caps(asdf_t *kb, uint8_t param)
{
  (void) param;
  asdf_modifier_capslock_activate_r(&kb->modifiers);
  asdf_sync_lock_leds_r(kb);
}

void asdf_action_ctrl(asdf_t *kb, uint8_t param)
{
  (void) param;
  asdf_modifier_ctrl_activate_r(&kb->modifiers);
}

void asdf_action_ctrl_release(asdf_t *kb, uint8_t param)
{
  (void) param;
  asdf_modifier_ctrl_deactivate_r(&kb->modifiers);
}

void asdf_action_repeat(asdf_t *kb, uint8_t param)
{
  (void) param;
  asdf_repeat_activate_r(&kb->repeat);
}

void asdf_action_repeat_release(asdf_t *kb, uint8_t param)
{
  (void) param;
  asdf_repeat_deactivate_r(&kb->repeat);
}

// Keymap select bits: param is the bit number (0-7). The request is applied at
// the end of the scan.
void asdf_action_mapsel_set(asdf_t *kb, uint8_t bit)
{
  asdf_keymaps_request_bit_r(&kb->keymap, (uint8_t) (1u << (bit & 7)), 1);
}

void asdf_action_mapsel_clear(asdf_t *kb, uint8_t bit)
{
  asdf_keymaps_request_bit_r(&kb->keymap, (uint8_t) (1u << (bit & 7)), 0);
}

void asdf_action_strobe_positive(asdf_t *kb, uint8_t param)
{
  (void) param;
  asdf_set_strobe_polarity_r(kb, 1);
}

void asdf_action_strobe_negative(asdf_t *kb, uint8_t param)
{
  (void) param;
  asdf_set_strobe_polarity_r(kb, 0);
}

void asdf_action_autorepeat_on(asdf_t *kb, uint8_t param)
{
  (void) param;
  asdf_repeat_auto_on_r(&kb->repeat);
}

void asdf_action_autorepeat_off(asdf_t *kb, uint8_t param)
{
  (void) param;
  asdf_repeat_auto_off_r(&kb->repeat);
}

// Invalid virtual outputs are ignored.
void asdf_action_virtual(asdf_t *kb, uint8_t virtual_out)
{
  asdf_virtual_activate_r(&kb->outputs, (asdf_virtual_dev_t) virtual_out);
}

// Prints the current keymap's ID message, if it has one.
void asdf_action_keymap_id(asdf_t *kb, uint8_t param)
{
  (void) param;
  const asdf_keymap_t *keymap = asdf_keymap_descriptor(kb->keymap.current);

  if (keymap) {
    const char *message = (const char *) FLASH_READ_PTR(&keymap->id_message);
    if (message) {
      asdf_print_flash_r(kb, message);
    }
  }
}

//-------|---------|---------+---------+---------+---------+---------+---------+
// Above line is 80 columns, and should display completely in the editor.
