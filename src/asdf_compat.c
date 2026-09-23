// -*- mode: C; tab-width: 2 ; indent-tabs-mode: nil -*-
//
// Unified Keyboard Project
// ASDF keyboard firmware
//
// asdf_compat.c
//
// Single-keyboard API. The keyboard core keeps no state of its own: every
// operation takes the keyboard (asdf_t) or module state it acts on (the "_r"
// functions). This file owns one default keyboard, and implements the original
// argument-less functions on it, so that existing callers, including main(),
// keymap hooks, and tests, keep working unchanged.
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

#include <stdint.h>
#include <stdio.h>
#include "asdf.h"
#include "asdf_hook.h"
#include "asdf_keyboard.h"
#include "asdf_keymaps.h"
#include "asdf_modifiers.h"
#include "asdf_physical.h"
#include "asdf_platform.h"
#include "asdf_print.h"
#include "asdf_repeat.h"
#include "asdf_virtual.h"

static asdf_t default_keyboard;
#define kb (&default_keyboard)

//
// Keyboard
//

void asdf_init(void) { asdf_init_r(kb, &asdf_arch_platform); }
void asdf_keyscan(void) { asdf_keyscan_r(kb); }
void asdf_apply_configuration(void) { asdf_apply_configuration_r(kb); }
void asdf_send_code(asdf_keycode_t code) { asdf_send_code_r(kb, code); }
uint8_t asdf_put_code(asdf_keycode_t code) { return asdf_put_code_r(kb, code); }
int asdf_putc(char c, FILE *stream)
{
  (void) stream;
  return asdf_putc_r(kb, c);
}
asdf_keycode_t asdf_next_code(void) { return asdf_next_code_r(kb); }
void asdf_set_print_delay(uint8_t delay_ms) { kb->print_delay_ms = delay_ms; }
void asdf_install_platform(const asdf_platform_t *platform) { asdf_install_platform_r(kb, platform); }
const asdf_platform_t *asdf_current_platform(void) { return kb->platform; }
void asdf_print_flash(const char *str) { asdf_print_flash_r(kb, str); }

//
// Keymaps
//

void asdf_keymaps_add_map(const asdf_keycode_t *matrix, modifier_index_t modifier_index,
                          uint8_t num_rows, uint8_t num_cols)
{
  asdf_keymaps_add_map_r(&kb->keymap, matrix, modifier_index, num_rows, num_cols);
}
uint8_t asdf_keymaps_num_rows(void)
{
  return asdf_keymaps_num_rows_r(&kb->keymap, asdf_modifier_index_r(&kb->modifiers));
}
uint8_t asdf_keymaps_num_cols(void)
{
  return asdf_keymaps_num_cols_r(&kb->keymap, asdf_modifier_index_r(&kb->modifiers));
}
asdf_keycode_t asdf_keymaps_get_code(uint8_t row, uint8_t col, uint8_t modifier_index)
{
  return asdf_keymaps_get_code_r(&kb->keymap, row, col, modifier_index);
}
void asdf_keymaps_init(void) { asdf_keymaps_init_r(kb); }
void asdf_keymaps_select(uint8_t index) { asdf_keymaps_select_r(kb, index); }
void asdf_keymaps_apply_request(void) { asdf_keymaps_apply_request_r(kb); }
void asdf_keymaps_map_select_0_clear(void) { asdf_keymaps_request_bit_r(&kb->keymap, ASDF_KEYMAP_BIT_0, 0); }
void asdf_keymaps_map_select_0_set(void) { asdf_keymaps_request_bit_r(&kb->keymap, ASDF_KEYMAP_BIT_0, 1); }
void asdf_keymaps_map_select_1_clear(void) { asdf_keymaps_request_bit_r(&kb->keymap, ASDF_KEYMAP_BIT_1, 0); }
void asdf_keymaps_map_select_1_set(void) { asdf_keymaps_request_bit_r(&kb->keymap, ASDF_KEYMAP_BIT_1, 1); }
void asdf_keymaps_map_select_2_clear(void) { asdf_keymaps_request_bit_r(&kb->keymap, ASDF_KEYMAP_BIT_2, 0); }
void asdf_keymaps_map_select_2_set(void) { asdf_keymaps_request_bit_r(&kb->keymap, ASDF_KEYMAP_BIT_2, 1); }
void asdf_keymaps_map_select_3_clear(void) { asdf_keymaps_request_bit_r(&kb->keymap, ASDF_KEYMAP_BIT_3, 0); }
void asdf_keymaps_map_select_3_set(void) { asdf_keymaps_request_bit_r(&kb->keymap, ASDF_KEYMAP_BIT_3, 1); }

//
// Hooks
//

void asdf_hook_init(void) { asdf_hook_init_r(&kb->hooks); }
void asdf_hook_assign(asdf_hook_id_t hook_id, asdf_hook_function_t func)
{
  asdf_hook_assign_r(&kb->hooks, hook_id, func);
}
void asdf_hook_execute(asdf_hook_id_t hook_id) { asdf_hook_execute_r(&kb->hooks, hook_id); }

//
// Modifiers. Changes to SHIFT and CAPS also drive the lock indicator LEDs.
//

void asdf_modifiers_init(void)
{
  asdf_modifiers_init_r(&kb->modifiers);
  asdf_sync_lock_leds_r(kb);
}
void asdf_modifier_shift_activate(void)
{
  asdf_modifier_shift_activate_r(&kb->modifiers);
  asdf_sync_lock_leds_r(kb);
}
void asdf_modifier_shiftlock_on_activate(void)
{
  asdf_modifier_shiftlock_on_activate_r(&kb->modifiers);
  asdf_sync_lock_leds_r(kb);
}
void asdf_modifier_shiftlock_toggle_activate(void)
{
  asdf_modifier_shiftlock_toggle_activate_r(&kb->modifiers);
  asdf_sync_lock_leds_r(kb);
}
void asdf_modifier_shift_deactivate(void)
{
  asdf_modifier_shift_deactivate_r(&kb->modifiers);
  asdf_sync_lock_leds_r(kb);
}
void asdf_modifier_capslock_activate(void)
{
  asdf_modifier_capslock_activate_r(&kb->modifiers);
  asdf_sync_lock_leds_r(kb);
}
void asdf_modifier_ctrl_activate(void) { asdf_modifier_ctrl_activate_r(&kb->modifiers); }
void asdf_modifier_ctrl_deactivate(void) { asdf_modifier_ctrl_deactivate_r(&kb->modifiers); }
modifier_index_t asdf_modifier_index(void) { return asdf_modifier_index_r(&kb->modifiers); }

//
// Repeat
//

void asdf_repeat_init(void) { asdf_repeat_init_r(&kb->repeat); }
void asdf_repeat_reset_count(void) { asdf_repeat_reset_count_r(&kb->repeat); }
void asdf_repeat_auto_off(void) { asdf_repeat_auto_off_r(&kb->repeat); }
void asdf_repeat_auto_on(void) { asdf_repeat_auto_on_r(&kb->repeat); }
uint8_t asdf_repeat_is_autorepeat_enabled(void)
{
  return asdf_repeat_is_autorepeat_enabled_r(&kb->repeat);
}
void asdf_repeat_activate(void) { asdf_repeat_activate_r(&kb->repeat); }
void asdf_repeat_deactivate(void) { asdf_repeat_deactivate_r(&kb->repeat); }
uint8_t asdf_repeat(void) { return asdf_repeat_r(&kb->repeat); }

//
// Virtual and physical outputs. The physical outputs are those embedded in the
// virtual output state.
//

void asdf_virtual_init(void) { asdf_virtual_init_r(&kb->outputs); }
void asdf_virtual_action(asdf_virtual_dev_t virtual_out, asdf_virtual_function_t function)
{
  asdf_virtual_action_r(&kb->outputs, virtual_out, function);
}
void asdf_virtual_activate(asdf_virtual_dev_t virtual_out)
{
  asdf_virtual_activate_r(&kb->outputs, virtual_out);
}
void asdf_virtual_assign(asdf_virtual_dev_t virtual_out, asdf_physical_dev_t physical_out,
                         asdf_virtual_function_t function, uint8_t initial_value)
{
  asdf_virtual_assign_r(&kb->outputs, virtual_out, physical_out, function, initial_value);
}
void asdf_virtual_sync(void) { asdf_virtual_sync_r(&kb->outputs); }

void asdf_physical_init(void) { asdf_physical_init_r(&kb->outputs.physical); }
void asdf_physical_set(asdf_physical_dev_t physical_out, uint8_t value)
{
  asdf_physical_set_r(&kb->outputs.physical, physical_out, value);
}
void asdf_physical_on(asdf_physical_dev_t physical_out)
{
  asdf_physical_on_r(&kb->outputs.physical, physical_out);
}
void asdf_physical_off(asdf_physical_dev_t physical_out)
{
  asdf_physical_off_r(&kb->outputs.physical, physical_out);
}
void asdf_physical_assert(asdf_physical_dev_t physical_out)
{
  asdf_physical_assert_r(&kb->outputs.physical, physical_out);
}
void asdf_physical_toggle(asdf_physical_dev_t physical_out)
{
  asdf_physical_toggle_r(&kb->outputs.physical, physical_out);
}
asdf_physical_dev_t asdf_physical_next_device(asdf_physical_dev_t device)
{
  return asdf_physical_next_device_r(&kb->outputs.physical, device);
}
uint8_t asdf_physical_allocate(asdf_physical_dev_t physical_out, asdf_physical_dev_t tail,
                               uint8_t initial_value)
{
  return asdf_physical_allocate_r(&kb->outputs.physical, physical_out, tail, initial_value);
}

//-------|---------|---------+---------+---------+---------+---------+---------+
// Above line is 80 columns, and should display completely in the editor.
