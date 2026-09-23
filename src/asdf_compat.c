// -*- mode: C; tab-width: 2 ; indent-tabs-mode: nil -*-
//
// Unified Keyboard Project
// ASDF keyboard firmware
//
// asdf_compat.c
//
// Single-keyboard API. The keyboard modules keep no state of their own: every
// operation takes the state object it acts on (the "_r" functions). This file
// owns the one default state object of each module and implements the
// original argument-less functions on top of them, so that existing callers,
// including every keymap, keep working unchanged.
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
#include "asdf_modifiers.h"
#include "asdf_physical.h"
#include "asdf_repeat.h"
#include "asdf_virtual.h"

//
// Repeat
//

static asdf_repeat_state_t default_repeat;

void asdf_repeat_init(void) { asdf_repeat_init_r(&default_repeat); }
void asdf_repeat_reset_count(void) { asdf_repeat_reset_count_r(&default_repeat); }
void asdf_repeat_auto_off(void) { asdf_repeat_auto_off_r(&default_repeat); }
void asdf_repeat_auto_on(void) { asdf_repeat_auto_on_r(&default_repeat); }
uint8_t asdf_repeat_is_autorepeat_enabled(void)
{
  return asdf_repeat_is_autorepeat_enabled_r(&default_repeat);
}
void asdf_repeat_activate(void) { asdf_repeat_activate_r(&default_repeat); }
void asdf_repeat_deactivate(void) { asdf_repeat_deactivate_r(&default_repeat); }
uint8_t asdf_repeat(void) { return asdf_repeat_r(&default_repeat); }

//
// Modifiers
//

static asdf_modifier_state_t default_modifiers;

static void sync_shiftlock_led(void)
{
  asdf_virtual_action(VSHIFT_LED,
                      asdf_modifier_shift_locked_r(&default_modifiers) ? V_SET_HI : V_SET_LO);
}

static void sync_capslock_led(void)
{
  asdf_virtual_action(VCAPS_LED,
                      asdf_modifier_caps_locked_r(&default_modifiers) ? V_SET_HI : V_SET_LO);
}

void asdf_modifiers_init(void)
{
  asdf_modifiers_init_r(&default_modifiers);
  sync_shiftlock_led();
  sync_capslock_led();
}

void asdf_modifier_shift_activate(void)
{
  asdf_modifier_shift_activate_r(&default_modifiers);
  sync_shiftlock_led();
}

void asdf_modifier_shiftlock_on_activate(void)
{
  asdf_modifier_shiftlock_on_activate_r(&default_modifiers);
  sync_shiftlock_led();
}

void asdf_modifier_shiftlock_toggle_activate(void)
{
  asdf_modifier_shiftlock_toggle_activate_r(&default_modifiers);
  sync_shiftlock_led();
}

void asdf_modifier_shift_deactivate(void)
{
  asdf_modifier_shift_deactivate_r(&default_modifiers);
  sync_shiftlock_led();
}

void asdf_modifier_capslock_activate(void)
{
  asdf_modifier_capslock_activate_r(&default_modifiers);
  sync_capslock_led();
}

void asdf_modifier_ctrl_activate(void) { asdf_modifier_ctrl_activate_r(&default_modifiers); }
void asdf_modifier_ctrl_deactivate(void) { asdf_modifier_ctrl_deactivate_r(&default_modifiers); }
modifier_index_t asdf_modifier_index(void) { return asdf_modifier_index_r(&default_modifiers); }

//
// Virtual and physical outputs
//

// The default physical output state is the one embedded in the default
// virtual output state.
static asdf_virtual_state_t default_virtual;
#define default_physical (default_virtual.physical)

void asdf_virtual_init(void) { asdf_virtual_init_r(&default_virtual); }
void asdf_virtual_action(asdf_virtual_dev_t virtual_out, asdf_virtual_function_t function)
{
  asdf_virtual_action_r(&default_virtual, virtual_out, function);
}
void asdf_virtual_activate(asdf_virtual_dev_t virtual_out)
{
  asdf_virtual_activate_r(&default_virtual, virtual_out);
}
void asdf_virtual_assign(asdf_virtual_dev_t virtual_out, asdf_physical_dev_t physical_out,
                         asdf_virtual_function_t function, uint8_t initial_value)
{
  asdf_virtual_assign_r(&default_virtual, virtual_out, physical_out, function, initial_value);
}
void asdf_virtual_sync(void) { asdf_virtual_sync_r(&default_virtual); }

void asdf_physical_init(void) { asdf_physical_init_r(&default_physical); }
void asdf_physical_set(asdf_physical_dev_t physical_out, uint8_t value)
{
  asdf_physical_set_r(&default_physical, physical_out, value);
}
void asdf_physical_on(asdf_physical_dev_t physical_out)
{
  asdf_physical_on_r(&default_physical, physical_out);
}
void asdf_physical_off(asdf_physical_dev_t physical_out)
{
  asdf_physical_off_r(&default_physical, physical_out);
}
void asdf_physical_assert(asdf_physical_dev_t physical_out)
{
  asdf_physical_assert_r(&default_physical, physical_out);
}
void asdf_physical_toggle(asdf_physical_dev_t physical_out)
{
  asdf_physical_toggle_r(&default_physical, physical_out);
}
asdf_physical_dev_t asdf_physical_next_device(asdf_physical_dev_t device)
{
  return asdf_physical_next_device_r(&default_physical, device);
}
uint8_t asdf_physical_allocate(asdf_physical_dev_t physical_out, asdf_physical_dev_t tail,
                               uint8_t initial_value)
{
  return asdf_physical_allocate_r(&default_physical, physical_out, tail, initial_value);
}

//-------|---------|---------+---------+---------+---------+---------+---------+
// Above line is 80 columns, and should display completely in the editor.
