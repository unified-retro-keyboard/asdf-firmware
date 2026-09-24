// -*- mode: C; tab-width: 2 ; indent-tabs-mode: nil -*-
/**
 * @file asdf_keyboard.h
 *
 * The keyboard object and its entry points. An asdf_t holds all of the
 * changeable state of one keyboard, so any number of keyboards can be run
 * independently. Every function operates on the keyboard passed to it.
 * There is no default keyboard: the application owns each asdf_t.
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

#if !defined(ASDF_KEYBOARD_H)
#define ASDF_KEYBOARD_H

#include <stdint.h>
#include "asdf.h"
#include "asdf_config.h"
#include "asdf_keymaps.h"
#include "asdf_modifiers.h"
#include "asdf_platform.h"
#include "asdf_repeat.h"
#include "asdf_ring.h"
#include "asdf_virtual.h"

/**
 * One keyboard: all of its changeable state.
 *
 * The application owns each keyboard and passes it to every function;
 * nothing is shared between keyboards. Initialize it with asdf_init() before
 * any other call.
 *
 * Invariants, between public calls after asdf_init():
 * - every debounce counter is in 1..ASDF_DEBOUNCE_TIME_MS
 * - last_key_row and last_key_col are both 0xff (no repeating key) or both
 *   name a key; in the latter case that key's bit in stable_rows is set
 * - repeat_armed is 0, provided asdf_arm_repeat() is called only from press
 *   actions
 * - keymap.current is a valid keymap index
 * - platform is never NULL
 *
 * @code
 * #include "asdf_arch.h"
 * #include "asdf_keyboard.h"
 *
 * static asdf_arch_t arch;
 * static asdf_t kb;
 *
 * asdf_arch_init(&arch);                   // hardware and tick interrupt
 * asdf_init(&kb, &arch.platform);        // keyboard logic, keymap 0
 * while (1) {
 *     // scan, debounce, repeat, and send codes for the ticks since last time
 *     asdf_process(&kb, asdf_arch_tick(&arch));
 * }
 * @endcode
 */
struct asdf_keyboard {
  // Key matrix scanner
  asdf_cols_t stable_rows[ASDF_MAX_ROWS];                    ///< debounced key state
  uint8_t debounce[ASDF_MAX_ROWS][ASDF_MAX_COLS];            ///< ticks left to debounce
  asdf_key_t repeat_key;                                     ///< the repeating key
  uint8_t last_key_row;                                      ///< ... and its position
  uint8_t last_key_col;
  uint8_t repeat_armed; ///< set by a press action to make its key repeat

  // Output queues. System messages have priority over typed keycodes.
  asdf_ring_t keycodes;
  asdf_ring_t messages;
  asdf_keycode_t keycode_storage[ASDF_KEYCODE_BUFFER_SIZE];
  asdf_keycode_t message_storage[ASDF_MESSAGE_BUFFER_SIZE];
  uint8_t print_delay_ms; ///< delay after each system message character
  uint8_t output_wait_ms; ///< ticks before the next code may be output

  asdf_keymap_state_t keymap;
  asdf_modifier_state_t modifiers;
  asdf_repeat_state_t repeat;
  asdf_virtual_state_t outputs;

  const asdf_platform_t *base_platform; ///< platform given to asdf_init()
  const asdf_platform_t *platform;      ///< platform in use (a keymap may override)
};

/**
 * Initialize a keyboard, or reset one already in use.
 *
 * Empties the output queues and ends any message pause, forgets all key state
 * (no keys pressed, no repeating key, debounce counters reloaded), and selects
 * keymap 0, which in turn resets modifiers, repeat, the each-scan action, and
 * virtual outputs, and applies keymap 0's descriptor, driving the outputs
 * through @p platform. Key state is cleared before the keymap is selected, so
 * keys held before a reset are not re-applied as held configuration.
 *
 * @param kb        Keyboard to initialize.
 * @param platform  Hardware the keyboard is scanned and sent through; must
 *                  not be NULL and must remain valid for the life of @p kb.
 */
void asdf_init(asdf_t *kb, const asdf_platform_t *platform);

/**
 * Scan the key matrix once, as if one tick had elapsed.
 *
 * Debounces and acts on key changes, repeats the held repeating key, and
 * applies any keymap change requested during the scan. Reads the matrix
 * through the platform; the key actions run may queue codes, change modifiers,
 * drive outputs, and switch keymaps. Does not advance the timers or send
 * codes; asdf_process() is the usual entry point.
 *
 * @param kb  Keyboard to scan.
 */
void asdf_keyscan(asdf_t *kb);

/**
 * Run the keyboard for the ticks elapsed since the last call.
 *
 * In order: advances the timers by @p elapsed_ms, sends up to one queued code
 * per elapsed tick through the platform (subject to message pacing, see
 * asdf_next_code()), then scans the key matrix once with debounce and
 * repeat advanced by @p elapsed_ms. Timing therefore follows real time even
 * if a call takes longer than a tick. Codes queued by this scan are sent on a
 * later call. A key first seen changed after a gap of several ticks is taken
 * to have been in its new state for the whole gap. Key actions run by the scan
 * may queue codes, change modifiers, drive outputs, and switch keymaps.
 *
 * Never blocks. Does nothing if @p elapsed_ms is 0.
 *
 * @param kb          Keyboard to run.
 * @param elapsed_ms  1 ms ticks since the last call, as returned by the
 *                    platform adapter's asdf_arch_tick(); values above 255
 *                    are treated as 255.
 */
void asdf_process(asdf_t *kb, uint16_t elapsed_ms);

/**
 * Run the keyboard like asdf_process(), but send nothing.
 *
 * Advances the timers and scans the key matrix once, leaving codes queued for
 * the caller to take with asdf_next_code(). For applications that deliver
 * codes themselves. Key actions run by the scan may queue codes, change
 * modifiers, drive outputs, and switch keymaps.
 *
 * Never blocks. Does nothing if @p elapsed_ms is 0.
 *
 * @param kb          Keyboard to run.
 * @param elapsed_ms  1 ms ticks since the last call; values above 255 are
 *                    treated as 255.
 */
void asdf_update(asdf_t *kb, uint16_t elapsed_ms);

/**
 * Advance the keyboard's timers without scanning or sending.
 *
 * Counts down the pause after a system message character and any long output
 * pulses; an expiring pulse drives its output through the platform.
 *
 * @param kb          Keyboard whose timers to advance.
 * @param elapsed_ms  1 ms ticks elapsed.
 */
void asdf_tick(asdf_t *kb, uint8_t elapsed_ms);

/**
 * Take the next code to send to the host.
 *
 * System message characters take priority over typed keycodes. After each
 * message character, output pauses for the current keymap's print delay, to
 * reduce the risk of dropped characters on unbuffered polling hosts; the pause
 * is counted down by asdf_tick(), so it does not block scanning. Typed
 * keycodes are not paced, as they arrive at human speed. Removes the code
 * taken from its queue, and starts the pause if it is a message character.
 *
 * @param kb    Keyboard to take the code from.
 * @param code  Receives the code; not written when none is taken.
 * @return 1 if a code was taken into @p code; 0 if none is queued or output is
 *         paused after a message character.
 */
uint8_t asdf_next_code(asdf_t *kb, asdf_keycode_t *code);

/**
 * Send a code to the host through the keyboard's platform, immediately.
 *
 * Drives the output through the platform; the queues are not involved.
 *
 * @param kb    Keyboard to send through.
 * @param code  Code to send.
 */
void asdf_send_code(asdf_t *kb, asdf_keycode_t code);

/**
 * Queue a typed keycode for output.
 *
 * Queues the code on the keyboard's keycode queue.
 *
 * @param kb    Keyboard to queue on.
 * @param code  Code to queue.
 * @return 1 if queued; 0 if the keycode queue was full, in which case the
 *         code is dropped and counted (see asdf_dropped_codes()).
 */
uint8_t asdf_put_code(asdf_t *kb, asdf_keycode_t code);

/**
 * Queue a system message character for output.
 *
 * A newline is queued as CR LF, as a unit: if both do not fit, neither is
 * queued, so a message never ends with half a line ending. Queues the
 * character on the keyboard's message queue.
 *
 * @param kb  Keyboard to queue on.
 * @param c   Character to queue.
 * @return @p c if queued; EOF if the message queue had no room, in which case
 *         the character (or both CR and LF) is dropped and counted (see
 *         asdf_dropped_messages()).
 */
int asdf_putc(asdf_t *kb, char c);

/**
 * Number of typed keycodes dropped because the keycode queue was full.
 *
 * No side effects.
 *
 * @param kb  Keyboard to query.
 * @return Codes dropped since asdf_init(), saturating at 255.
 */
uint8_t asdf_dropped_codes(const asdf_t *kb);

/**
 * Number of system message characters dropped because the message queue was
 * full.
 *
 * No side effects.
 *
 * @param kb  Keyboard to query.
 * @return Characters dropped since asdf_init(), saturating at 255.
 */
uint8_t asdf_dropped_messages(const asdf_t *kb);

/**
 * Number of entries of the current keymap's descriptor that could not be
 * applied.
 *
 * An entry fails if it is a missing or oversize modifier map, or a virtual
 * output assignment that is invalid or conflicts with another. No side
 * effects.
 *
 * @param kb  Keyboard to query.
 * @return Failed entries, saturating at 255; 0 for a correct keymap.
 */
uint8_t asdf_keymap_errors(const asdf_t *kb);

/**
 * Select the platform through which the key matrix is read, codes are sent,
 * and outputs are driven.
 *
 * Keymaps with special hardware needs install their own. Changes the platform
 * used by the keyboard and its virtual outputs; drives nothing.
 *
 * @param kb        Keyboard to configure.
 * @param platform  Platform to use, or NULL to restore the platform given to
 *                  asdf_init().
 */
void asdf_install_platform(asdf_t *kb, const asdf_platform_t *platform);

/**
 * Make the key being pressed the repeating key.
 *
 * Called by a press action. While the key is held, and is still the same key
 * under the current modifiers, its press action runs again at the repeat
 * rate. Pressing a different key restarts the repeat timer. Sets a flag in
 * @p kb that is consumed when the press action returns.
 *
 * @param kb  Keyboard whose key is being pressed.
 */
void asdf_arm_repeat(asdf_t *kb);

/**
 * Set the output strobe polarity through the keyboard's platform.
 *
 * Drives the strobe output through the platform.
 *
 * @param kb        Keyboard to configure.
 * @param positive  Nonzero for a positive (idle low) strobe; 0 for negative.
 */
void asdf_set_strobe_polarity(asdf_t *kb, uint8_t positive);

/**
 * Re-apply held configuration switches after a keymap switch.
 *
 * Called after a keymap switch has reset the keyboard state. Forgets the
 * repeating key, then runs the press actions of held keys that are
 * configuration actions (see asdf_is_configuration_action()), such as DIP
 * switches, so their settings persist across keymap changes. Other held keys,
 * such as a held SHIFT, are not re-activated; they take effect again only when
 * released and pressed. Uses the last debounced key state rather than
 * rescanning. The press actions run have their usual effects.
 *
 * @param kb  Keyboard to configure.
 */
void asdf_apply_configuration(asdf_t *kb);

/**
 * Drive the SHIFTLOCK (VSHIFT_LED) and CAPSLOCK (VCAPS_LED) indicators from
 * the keyboard's modifier state.
 *
 * Drives the outputs through the platform.
 *
 * @param kb  Keyboard whose indicators to update.
 */
void asdf_sync_lock_leds(asdf_t *kb);

#endif /* !defined (ASDF_KEYBOARD_H) */

//-------|---------|---------+---------+---------+---------+---------+---------+
// Above line is 80 columns, and should display completely in the editor.
