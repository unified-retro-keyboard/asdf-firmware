// -*- mode: C; tab-width: 2 ; indent-tabs-mode: nil -*-
/**
 * @file asdf_actions.h
 *
 * Keys and key actions. Each key in a keymap matrix is an asdf_key_t: an
 * action performed when the key is pressed, and one performed when it is
 * released. An action is a function number, indexing the action table, and a
 * parameter passed to the function. Sending a code is an action whose
 * parameter is the code, so a key can send any code 0x00-0xFF. A press action
 * can make its key autorepeat by calling asdf_arm_repeat(); of the built-in
 * actions, only ACTION_SEND_REPEATABLE_CODE does.
 *
 * Part of the Unified Keyboard Project ASDF keyboard firmware.
 *
 * @copyright Copyright 2019 David Fenyes. MIT License; see LICENSE.
 */
// SPDX-License-Identifier: MIT

#if !defined(ASDF_ACTIONS_H)
#define ASDF_ACTIONS_H

#include <stdbool.h>
#include <stdint.h>
#include "asdf.h"
#include "asdf_arch.h" // FLASH

/**
 * One key: the action on press and the action on release.
 *
 * An all-zero key does nothing, so keys left out of a matrix initializer do
 * nothing.
 */
typedef struct {
  uint8_t press_fn;      ///< action table index
  uint8_t press_param;   ///< passed to the press action
  uint8_t release_fn;    ///< action table index
  uint8_t release_param; ///< passed to the release action
} asdf_key_t;

/** An action function: acts on the keyboard, with the key's parameter. */
typedef void (*asdf_action_fn_t)(asdf_t *kb, uint8_t param);

/**
 * The built-in actions, numbered from 0.
 *
 * Numbers from ASDF_KEYMAP_ACTIONS on are free for keymap-provided actions (see
 * asdf_action_table).
 */
typedef enum {
  ACTION_NOTHING = 0,          ///< must be 0: a zero-filled key does nothing
  ACTION_SEND_CODE,            ///< queue the code param
  ACTION_SEND_REPEATABLE_CODE, ///< queue the code param; the key autorepeats
  ACTION_SHIFT,            ///< SHIFT pressed
  ACTION_SHIFT_RELEASE,    ///< SHIFT released
  ACTION_SHIFTLOCK_ON,     ///< turn SHIFT lock on
  ACTION_SHIFTLOCK_TOGGLE, ///< toggle SHIFT lock
  ACTION_CAPS,             ///< toggle CAPS lock
  ACTION_CTRL,             ///< CTRL pressed
  ACTION_CTRL_RELEASE,     ///< CTRL released
  ACTION_REPEAT,           ///< REPEAT pressed
  ACTION_REPEAT_RELEASE,   ///< REPEAT released
  ACTION_MAPSEL_SET,       ///< set keymap select bit param (configuration)
  ACTION_MAPSEL_CLEAR,     ///< clear keymap select bit param
  ACTION_STROBE_POSITIVE,  ///< positive output strobe (configuration)
  ACTION_STROBE_NEGATIVE,  ///< negative output strobe
  ACTION_AUTOREPEAT_ON,    ///< enable autorepeat (configuration)
  ACTION_AUTOREPEAT_OFF,   ///< disable autorepeat
  ACTION_VIRTUAL,          ///< activate virtual output param
  ACTION_KEYMAP_ID,        ///< print the keymap's ID message
  ASDF_NUM_BUILTIN_ACTIONS
} asdf_action_t;

/** First action number for keymap-provided actions. */
#define ASDF_KEYMAP_ACTIONS 0x80

/** Number of action table entries: every uint8_t action number has an entry. */
#define ASDF_NUM_ACTION_SLOTS 256

/**
 * The action table, indexed by action number, in flash.
 *
 * The build supplies it with its keymaps (the keymap action table in Keymaps/,
 * or the test keymaps), filling every entry: unused entries are
 * asdf_action_nothing, the built-in entries are ASDF_BUILTIN_ACTIONS, and the
 * rest are keymap-provided actions.
 *
 * A keymap-provided action is an asdf_action_fn_t with an action number from
 * ASDF_KEYMAP_ACTIONS on, a KEY_ macro that binds it to a key, and an entry in
 * the table:
 *
 * @code
 * #include "asdf_arch.h"
 * #include "asdf_actions.h"
 *
 * #define ACTION_SEND_TWICE (ASDF_KEYMAP_ACTIONS + 0)
 *
 * // A key that sends code twice on press; for example KEY_SEND_TWICE('x').
 * #define KEY_SEND_TWICE(code) \
 *   ASDF_KEY(ACTION_SEND_TWICE, (code), ACTION_NOTHING, 0)
 *
 * void send_twice(asdf_t *kb, uint8_t code)
 * {
 *   asdf_action_send_code(kb, code);
 *   asdf_action_send_code(kb, code);
 * }
 *
 * // The range designator is a GCC extension; see Keymaps/asdf_keymap_actions.c
 * // for the warnings it needs disabled.
 * const asdf_action_fn_t FLASH asdf_action_table[ASDF_NUM_ACTION_SLOTS] = {
 *   [0 ... ASDF_NUM_ACTION_SLOTS - 1] = asdf_action_nothing,
 *   ASDF_BUILTIN_ACTIONS,
 *   [ACTION_SEND_TWICE] = send_twice,
 * };
 * @endcode
 */
extern const asdf_action_fn_t FLASH asdf_action_table[ASDF_NUM_ACTION_SLOTS];

// The built-in action functions, one for each asdf_action_t value below
// ASDF_KEYMAP_ACTIONS. Each is called through the action table with the key's
// parameter; actions that need no value ignore it.

/**
 * Does nothing.
 *
 * No side effects.
 *
 * @param kb     Keyboard to act on.
 * @param param  Ignored.
 */
void asdf_action_nothing(asdf_t *kb, uint8_t param);

/**
 * Queue a code, sending it once however long the key is held.
 *
 * Queues @p code on the keyboard's keycode queue; if the queue is full, the
 * code is dropped and counted.
 *
 * @param kb     Keyboard to act on.
 * @param code   Code to queue.
 */
void asdf_action_send_code(asdf_t *kb, uint8_t code);

/**
 * Queue a code, and make the pressed key autorepeat.
 *
 * Queues @p code on the keyboard's keycode queue (a full queue drops and
 * counts it), and makes the key being pressed the repeating key
 * (asdf_arm_repeat()).
 *
 * @param kb     Keyboard to act on.
 * @param code   Code to queue.
 */
void asdf_action_send_repeatable_code(asdf_t *kb, uint8_t code);

/**
 * SHIFT pressed.
 *
 * Turns SHIFT on, which also ends SHIFT lock, and updates the lock
 * indicators.
 *
 * @param kb     Keyboard to act on.
 * @param param  Ignored.
 */
void asdf_action_shift(asdf_t *kb, uint8_t param);

/**
 * SHIFT released.
 *
 * Turns SHIFT off and updates the lock indicators.
 *
 * @param kb     Keyboard to act on.
 * @param param  Ignored.
 */
void asdf_action_shift_release(asdf_t *kb, uint8_t param);

/**
 * Turn SHIFT lock on.
 *
 * Sets SHIFT lock and updates the lock indicators.
 *
 * @param kb     Keyboard to act on.
 * @param param  Ignored.
 */
void asdf_action_shiftlock_on(asdf_t *kb, uint8_t param);

/**
 * Toggle SHIFT lock.
 *
 * Toggles SHIFT lock and updates the lock indicators.
 *
 * @param kb     Keyboard to act on.
 * @param param  Ignored.
 */
void asdf_action_shiftlock_toggle(asdf_t *kb, uint8_t param);

/**
 * Toggle CAPS lock.
 *
 * Toggles CAPS lock and updates the lock indicators.
 *
 * @param kb     Keyboard to act on.
 * @param param  Ignored.
 */
void asdf_action_caps(asdf_t *kb, uint8_t param);

/**
 * CTRL pressed.
 *
 * Turns CTRL on.
 *
 * @param kb     Keyboard to act on.
 * @param param  Ignored.
 */
void asdf_action_ctrl(asdf_t *kb, uint8_t param);

/**
 * CTRL released.
 *
 * Turns CTRL off.
 *
 * @param kb     Keyboard to act on.
 * @param param  Ignored.
 */
void asdf_action_ctrl_release(asdf_t *kb, uint8_t param);

/**
 * REPEAT pressed.
 *
 * Activates REPEAT in the keyboard's repeat state (see
 * asdf_repeat_activate()).
 *
 * @param kb     Keyboard to act on.
 * @param param  Ignored.
 */
void asdf_action_repeat(asdf_t *kb, uint8_t param);

/**
 * REPEAT released.
 *
 * Deactivates REPEAT in the keyboard's repeat state (see
 * asdf_repeat_deactivate()).
 *
 * @param kb     Keyboard to act on.
 * @param param  Ignored.
 */
void asdf_action_repeat_release(asdf_t *kb, uint8_t param);

/**
 * Set a keymap select bit (configuration action).
 *
 * Sets bit @p bit of the requested keymap number. The request takes effect
 * at the end of the scan.
 *
 * @param kb     Keyboard to act on.
 * @param bit    Bit number, 0-7; only the low three bits are used.
 */
void asdf_action_mapsel_set(asdf_t *kb, uint8_t bit);

/**
 * Clear a keymap select bit.
 *
 * Clears bit @p bit of the requested keymap number. The request takes
 * effect at the end of the scan.
 *
 * @param kb     Keyboard to act on.
 * @param bit    Bit number, 0-7; only the low three bits are used.
 */
void asdf_action_mapsel_clear(asdf_t *kb, uint8_t bit);

/**
 * Positive output strobe (configuration action).
 *
 * Sets the output strobe polarity to positive (idle low) through the
 * keyboard's platform.
 *
 * @param kb     Keyboard to act on.
 * @param param  Ignored.
 */
void asdf_action_strobe_positive(asdf_t *kb, uint8_t param);

/**
 * Negative output strobe.
 *
 * Sets the output strobe polarity to negative (idle high) through the
 * keyboard's platform.
 *
 * @param kb     Keyboard to act on.
 * @param param  Ignored.
 */
void asdf_action_strobe_negative(asdf_t *kb, uint8_t param);

/**
 * Enable autorepeat (configuration action).
 *
 * Enables autorepeat in the keyboard's repeat state.
 *
 * @param kb     Keyboard to act on.
 * @param param  Ignored.
 */
void asdf_action_autorepeat_on(asdf_t *kb, uint8_t param);

/**
 * Disable autorepeat.
 *
 * Disables autorepeat in the keyboard's repeat state.
 *
 * @param kb     Keyboard to act on.
 * @param param  Ignored.
 */
void asdf_action_autorepeat_off(asdf_t *kb, uint8_t param);

/**
 * Activate a virtual output.
 *
 * Applies the virtual output's function to its physical outputs, driving
 * them through the platform. Invalid virtual outputs are ignored.
 *
 * @param kb           Keyboard to act on.
 * @param virtual_out  Virtual output to activate.
 */
void asdf_action_virtual(asdf_t *kb, uint8_t virtual_out);

/**
 * Print the current keymap's ID message.
 *
 * Queues the ID message of the current keymap's descriptor on the
 * keyboard's message queue, if it has one.
 *
 * @param kb     Keyboard to act on.
 * @param param  Ignored.
 */
void asdf_action_keymap_id(asdf_t *kb, uint8_t param);

/** Designated initializers for the built-in entries of the action table. */
#define ASDF_BUILTIN_ACTIONS                                                                       \
  [ACTION_NOTHING] = &asdf_action_nothing, [ACTION_SEND_CODE] = &asdf_action_send_code,            \
  [ACTION_SEND_REPEATABLE_CODE] = &asdf_action_send_repeatable_code,                               \
  [ACTION_SHIFT] = &asdf_action_shift, [ACTION_SHIFT_RELEASE] = &asdf_action_shift_release,        \
  [ACTION_SHIFTLOCK_ON] = &asdf_action_shiftlock_on,                                               \
  [ACTION_SHIFTLOCK_TOGGLE] = &asdf_action_shiftlock_toggle, [ACTION_CAPS] = &asdf_action_caps,    \
  [ACTION_CTRL] = &asdf_action_ctrl, [ACTION_CTRL_RELEASE] = &asdf_action_ctrl_release,            \
  [ACTION_REPEAT] = &asdf_action_repeat, [ACTION_REPEAT_RELEASE] = &asdf_action_repeat_release,    \
  [ACTION_MAPSEL_SET] = &asdf_action_mapsel_set,                                                   \
  [ACTION_MAPSEL_CLEAR] = &asdf_action_mapsel_clear,                                               \
  [ACTION_STROBE_POSITIVE] = &asdf_action_strobe_positive,                                         \
  [ACTION_STROBE_NEGATIVE] = &asdf_action_strobe_negative,                                         \
  [ACTION_AUTOREPEAT_ON] = &asdf_action_autorepeat_on,                                             \
  [ACTION_AUTOREPEAT_OFF] = &asdf_action_autorepeat_off, [ACTION_VIRTUAL] = &asdf_action_virtual,  \
  [ACTION_KEYMAP_ID] = &asdf_action_keymap_id

/**
 * Key initializers for keymap matrices.
 *
 * Every KEY_ macro takes one parameter: the code, bit, output, or other value
 * its action uses. Keys whose actions need no value ignore it, and are written
 * with 0, for example KEY_SHIFT(0). In the YAML key matrices the parameter may
 * be omitted, and is then 0.
 */
#define ASDF_KEY(press_fn, press_param, release_fn, release_param)                                 \
  { (uint8_t) (press_fn), (uint8_t) (press_param), (uint8_t) (release_fn),                          \
    (uint8_t) (release_param) }

// A key that sends code, and autorepeats while held.
#define KEY_SEND(code) ASDF_KEY(ACTION_SEND_REPEATABLE_CODE, (code), ACTION_NOTHING, 0)
// A key that sends code once, however long it is held.
#define KEY_SEND_ONCE(code) ASDF_KEY(ACTION_SEND_CODE, (code), ACTION_NOTHING, 0)
// A key that does nothing.
#define KEY_NOTHING(unused) ASDF_KEY(ACTION_NOTHING, 0, ACTION_NOTHING, 0)
// Modifiers: SHIFT, CTRL, and REPEAT are active while held; CAPS and SHIFT lock
// change on press.
#define KEY_SHIFT(unused) ASDF_KEY(ACTION_SHIFT, 0, ACTION_SHIFT_RELEASE, 0)
#define KEY_SHIFTLOCK_ON(unused) ASDF_KEY(ACTION_SHIFTLOCK_ON, 0, ACTION_NOTHING, 0)
#define KEY_SHIFTLOCK_TOGGLE(unused) ASDF_KEY(ACTION_SHIFTLOCK_TOGGLE, 0, ACTION_NOTHING, 0)
#define KEY_CAPS(unused) ASDF_KEY(ACTION_CAPS, 0, ACTION_NOTHING, 0)
#define KEY_CTRL(unused) ASDF_KEY(ACTION_CTRL, 0, ACTION_CTRL_RELEASE, 0)
#define KEY_REPEAT(unused) ASDF_KEY(ACTION_REPEAT, 0, ACTION_REPEAT_RELEASE, 0)
// Configuration switches: keymap select bit, strobe polarity, and autorepeat
// are set while the switch is closed.
#define KEY_MAPSEL(bit) ASDF_KEY(ACTION_MAPSEL_SET, (bit), ACTION_MAPSEL_CLEAR, (bit))
#define KEY_STROBE_POLARITY(unused) ASDF_KEY(ACTION_STROBE_POSITIVE, 0, ACTION_STROBE_NEGATIVE, 0)
#define KEY_AUTOREPEAT(unused) ASDF_KEY(ACTION_AUTOREPEAT_ON, 0, ACTION_AUTOREPEAT_OFF, 0)
// Activates a virtual output on press.
#define KEY_VIRTUAL(virtual_out) ASDF_KEY(ACTION_VIRTUAL, (virtual_out), ACTION_NOTHING, 0)
// Prints the keymap's ID message on press.
#define KEY_KEYMAP_ID(unused) ASDF_KEY(ACTION_KEYMAP_ID, 0, ACTION_NOTHING, 0)

/**
 * Perform an action on the keyboard.
 *
 * Every action number has an action table entry, so any @p fn is valid. Has
 * the side effects of the action performed.
 *
 * @param kb     Keyboard to act on.
 * @param fn     Action number, indexing asdf_action_table.
 * @param param  Parameter passed to the action.
 */
void asdf_action(asdf_t *kb, uint8_t fn, uint8_t param);

/**
 * Whether an action sets persistent keyboard configuration.
 *
 * Configuration actions set keyboard configuration from a held switch, such as
 * a DIP switch: keymap selection, strobe polarity, and autorepeat selection.
 * On a keymap switch, the press actions of held keys are re-applied only if
 * they are configuration actions. No side effects.
 *
 * @param fn  Action number.
 * @return true if @p fn is ACTION_MAPSEL_SET, ACTION_STROBE_POSITIVE, or
 *         ACTION_AUTOREPEAT_ON; false otherwise.
 */
bool asdf_is_configuration_action(uint8_t fn);

#endif /* !defined (ASDF_ACTIONS_H) */

//-------|---------|---------+---------+---------+---------+---------+---------+
// Above line is 80 columns, and should display completely in the editor.
