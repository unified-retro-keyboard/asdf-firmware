// -*- mode: C; tab-width: 2 ; indent-tabs-mode: nil -*-
/**
 * @file asdf_actions.c
 *
 * The built-in key actions, and action dispatch. See asdf_actions.h.
 *
 * Part of the Unified Keyboard Project ASDF keyboard firmware.
 *
 * @copyright Copyright 2019 David F. MIT License; see LICENSE.
 */
// SPDX-License-Identifier: MIT

#include <stddef.h>
#include <stdbool.h>
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

/**
 * Perform an action on the keyboard.
 *
 * Calls the action table entry for @p fn with @p param; has the side effects
 * of that action.
 *
 * @param kb     Keyboard to act on.
 * @param fn     Action number, indexing asdf_action_table.
 * @param param  Parameter passed to the action.
 *
 * Every action number has a table entry, so no bounds check is needed.
 * The entry is copied out of flash, rather than cast from a data pointer.
 */
void asdf_action(asdf_t *kb, uint8_t fn, uint8_t param)
{
  asdf_action_fn_t action;

  FLASH_MEMCPY(&action, &asdf_action_table[fn], sizeof(action));
  action(kb, param);
}

/**
 * Report whether an action sets persistent keyboard configuration.
 *
 * No side effects.
 *
 * @param fn  Action number.
 * @return true if @p fn is ACTION_MAPSEL_SET, ACTION_STROBE_POSITIVE, or
 *         ACTION_AUTOREPEAT_ON; false otherwise.
 *
 * Complexity: 3
 */
bool asdf_is_configuration_action(uint8_t fn)
{
  return (fn == (uint8_t) ACTION_MAPSEL_SET) || (fn == (uint8_t) ACTION_STROBE_POSITIVE) ||
         (fn == (uint8_t) ACTION_AUTOREPEAT_ON);
}

// The built-in actions. Actions that take no parameter ignore it. SHIFT, SHIFT
// lock, and CAPS changes also drive the lock indicators.

/**
 * Do nothing.
 *
 * No side effects.
 *
 * @param kb     Keyboard to act on.
 * @param param  Ignored.
 */
void asdf_action_nothing(asdf_t *kb, uint8_t param) //lint !e818 D16
{
  (void) kb;
  (void) param;
}

/**
 * Send a code.
 *
 * Queues @p code on the keyboard's keycode queue.
 *
 * @param kb    Keyboard to act on.
 * @param code  Code to queue.
 */
void asdf_action_send_code(asdf_t *kb, uint8_t code) { (void)asdf_put_code(kb, code); }

/**
 * Send a code from a key that autorepeats.
 *
 * Queues @p code on the keyboard's keycode queue, and makes the key that ran
 * this action the repeating key.
 *
 * @param kb    Keyboard to act on.
 * @param code  Code to queue.
 */
void asdf_action_send_repeatable_code(asdf_t *kb, uint8_t code)
{
  (void)asdf_put_code(kb, code);
  asdf_arm_repeat(kb);
}

/**
 * Activate SHIFT.
 *
 * Marks SHIFT held in the modifier state and updates the lock indicators.
 *
 * @param kb     Keyboard to act on.
 * @param param  Ignored.
 */
void asdf_action_shift(asdf_t *kb, uint8_t param)
{
  (void) param;
  asdf_modifier_shift_activate(&kb->modifiers);
  asdf_sync_lock_leds(kb);
}

/**
 * Release SHIFT.
 *
 * Marks SHIFT released in the modifier state and updates the lock
 * indicators.
 *
 * @param kb     Keyboard to act on.
 * @param param  Ignored.
 */
void asdf_action_shift_release(asdf_t *kb, uint8_t param)
{
  (void) param;
  asdf_modifier_shift_deactivate(&kb->modifiers);
  asdf_sync_lock_leds(kb);
}

/**
 * Turn SHIFT lock on.
 *
 * Changes the modifier state and updates the lock indicators.
 *
 * @param kb     Keyboard to act on.
 * @param param  Ignored.
 */
void asdf_action_shiftlock_on(asdf_t *kb, uint8_t param)
{
  (void) param;
  asdf_modifier_shiftlock_on_activate(&kb->modifiers);
  asdf_sync_lock_leds(kb);
}

/**
 * Toggle SHIFT lock.
 *
 * Changes the modifier state and updates the lock indicators.
 *
 * @param kb     Keyboard to act on.
 * @param param  Ignored.
 */
void asdf_action_shiftlock_toggle(asdf_t *kb, uint8_t param)
{
  (void) param;
  asdf_modifier_shiftlock_toggle_activate(&kb->modifiers);
  asdf_sync_lock_leds(kb);
}

/**
 * Toggle CAPS lock.
 *
 * Changes the modifier state and updates the lock indicators.
 *
 * @param kb     Keyboard to act on.
 * @param param  Ignored.
 */
void asdf_action_caps(asdf_t *kb, uint8_t param)
{
  (void) param;
  asdf_modifier_capslock_activate(&kb->modifiers);
  asdf_sync_lock_leds(kb);
}

/**
 * Activate CTRL.
 *
 * Marks CTRL held in the modifier state.
 *
 * @param kb     Keyboard to act on.
 * @param param  Ignored.
 */
void asdf_action_ctrl(asdf_t *kb, uint8_t param)
{
  (void) param;
  asdf_modifier_ctrl_activate(&kb->modifiers);
}

/**
 * Release CTRL.
 *
 * Marks CTRL released in the modifier state.
 *
 * @param kb     Keyboard to act on.
 * @param param  Ignored.
 */
void asdf_action_ctrl_release(asdf_t *kb, uint8_t param)
{
  (void) param;
  asdf_modifier_ctrl_deactivate(&kb->modifiers);
}

/**
 * Activate the REPEAT key.
 *
 * Changes the keyboard's repeat state.
 *
 * @param kb     Keyboard to act on.
 * @param param  Ignored.
 */
void asdf_action_repeat(asdf_t *kb, uint8_t param)
{
  (void) param;
  asdf_repeat_activate(&kb->repeat);
}

/**
 * Release the REPEAT key.
 *
 * Changes the keyboard's repeat state.
 *
 * @param kb     Keyboard to act on.
 * @param param  Ignored.
 */
void asdf_action_repeat_release(asdf_t *kb, uint8_t param)
{
  (void) param;
  asdf_repeat_deactivate(&kb->repeat);
}

/**
 * Set one bit of the requested keymap number.
 *
 * Changes only the request; it is applied at the end of the scan.
 *
 * @param kb   Keyboard to act on.
 * @param bit  Bit number (0-7); only the low three bits are used.
 */
void asdf_action_mapsel_set(asdf_t *kb, uint8_t bit)
{
  asdf_keymaps_request_bit(&kb->keymap, (uint8_t) (1u << (bit & 7u)), true);
}

/**
 * Clear one bit of the requested keymap number.
 *
 * Changes only the request; it is applied at the end of the scan.
 *
 * @param kb   Keyboard to act on.
 * @param bit  Bit number (0-7); only the low three bits are used.
 */
void asdf_action_mapsel_clear(asdf_t *kb, uint8_t bit)
{
  asdf_keymaps_request_bit(&kb->keymap, (uint8_t) (1u << (bit & 7u)), false);
}

/**
 * Select positive output strobe polarity.
 *
 * Sets the keyboard's strobe polarity.
 *
 * @param kb     Keyboard to act on.
 * @param param  Ignored.
 */
void asdf_action_strobe_positive(asdf_t *kb, uint8_t param)
{
  (void) param;
  asdf_set_strobe_polarity(kb, true);
}

/**
 * Select negative output strobe polarity.
 *
 * Sets the keyboard's strobe polarity.
 *
 * @param kb     Keyboard to act on.
 * @param param  Ignored.
 */
void asdf_action_strobe_negative(asdf_t *kb, uint8_t param)
{
  (void) param;
  asdf_set_strobe_polarity(kb, false);
}

/**
 * Enable autorepeat.
 *
 * Changes the keyboard's repeat state.
 *
 * @param kb     Keyboard to act on.
 * @param param  Ignored.
 */
void asdf_action_autorepeat_on(asdf_t *kb, uint8_t param)
{
  (void) param;
  asdf_repeat_auto_on(&kb->repeat);
}

/**
 * Disable autorepeat.
 *
 * Changes the keyboard's repeat state.
 *
 * @param kb     Keyboard to act on.
 * @param param  Ignored.
 */
void asdf_action_autorepeat_off(asdf_t *kb, uint8_t param)
{
  (void) param;
  asdf_repeat_auto_off(&kb->repeat);
}

/**
 * Activate a virtual output.
 *
 * Drives the virtual output's physical outputs; invalid virtual outputs are
 * ignored.
 *
 * @param kb           Keyboard to act on.
 * @param virtual_out  Virtual output to activate (an asdf_virtual_dev_t).
 */
void asdf_action_virtual(asdf_t *kb, uint8_t virtual_out)
{
  asdf_virtual_activate(&kb->outputs, (asdf_virtual_dev_t) virtual_out); //lint !e9030 D15
}

/**
 * Print the current keymap's ID message.
 *
 * Queues the message, if the keymap has one, on the keyboard's system
 * message output; does nothing otherwise.
 *
 * @param kb     Keyboard to act on.
 * @param param  Ignored.
 *
 * The message pointer is read out of the flash descriptor with
 * FLASH_READ_PTR.
 *
 * Complexity: 3
 */
void asdf_action_keymap_id(asdf_t *kb, uint8_t param)
{
  (void) param;
  const asdf_keymap_t *keymap = asdf_keymap_descriptor(kb->keymap.current);

  if (keymap != NULL) {
    const char *message = (const char *) FLASH_READ_PTR(&keymap->id_message);
    if (message != NULL) {
      asdf_print_flash(kb, message);
    }
  }
}

//-------|---------|---------+---------+---------+---------+---------+---------+
// Above line is 80 columns, and should display completely in the editor.
