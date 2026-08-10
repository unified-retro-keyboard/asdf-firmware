#ifndef ASDF_SIMAVR_TEST_SOL_H
#define ASDF_SIMAVR_TEST_SOL_H

#include "test_types.h"

/* Coordinates derived from src/Keymaps/asdf_keymap_sol.c (DIP value 4).
 *
 * SOL has a different matrix layout from the classic/apple2 keymaps.
 * setup_sol_keymap() calls asdf_modifier_capslock_activate(), so CAPS is
 * always active on boot.  The CAPS map is therefore the default:
 * sol_caps_map[0][2] = 'A'     (row 0, col 2 — always-on CAPS)
 * sol_shift_map[0][2] = 'A'     (row 0, col 2 with SHIFT held)
 * sol_plain_map[2][1] = ACTION_SHIFT
 * sol_plain_map[0][0] = ACTION_CTRL
 * sol_plain_map[7][5] = ASCII_CR = '\r'
 *
 * Note: SOL calls asdf_arch_set_neg_strobe so strobe polarity is inverted,
 * but the harness captures on both edges and handles this transparently.
 *
 * The bit-paired punctuation keys are also covered here.  On the Sol-20 (as on
 * any bit-paired ASCII keyboard) SHIFT toggles one code bit, so each of these
 * keys has a distinct upper legend:
 * sol_plain_map[1][3] = ';'  sol_shift_map[1][3] = '+'
 * sol_plain_map[1][4] = ':'  sol_shift_map[1][4] = '*'
 * sol_plain_map[5][5] = '['  sol_shift_map[5][5] = '{'
 * sol_plain_map[5][6] = '\\' sol_shift_map[5][6] = '|'
 * sol_plain_map[5][7] = ']'  sol_shift_map[5][7] = '}'
 */
static const sim_event_t sol_events[] = {
    { .row = 0, .col = 2, .hold_cycles = 400000, .expected = 'A',  .with_modifier = SIM_MOD_NONE  },
    { .row = 0, .col = 2, .hold_cycles = 400000, .expected = 'A',  .with_modifier = SIM_MOD_SHIFT },
    { .row = 7, .col = 5, .hold_cycles = 400000, .expected = '\r', .with_modifier = SIM_MOD_NONE  },
    /* bit-paired punctuation: unshifted lower legend, then shifted upper legend */
    { .row = 1, .col = 3, .hold_cycles = 400000, .expected = ';',  .with_modifier = SIM_MOD_NONE  },
    { .row = 1, .col = 3, .hold_cycles = 400000, .expected = '+',  .with_modifier = SIM_MOD_SHIFT },
    { .row = 1, .col = 4, .hold_cycles = 400000, .expected = ':',  .with_modifier = SIM_MOD_NONE  },
    { .row = 1, .col = 4, .hold_cycles = 400000, .expected = '*',  .with_modifier = SIM_MOD_SHIFT },
    { .row = 5, .col = 5, .hold_cycles = 400000, .expected = '[',  .with_modifier = SIM_MOD_NONE  },
    { .row = 5, .col = 5, .hold_cycles = 400000, .expected = '{',  .with_modifier = SIM_MOD_SHIFT },
    { .row = 5, .col = 6, .hold_cycles = 400000, .expected = '\\', .with_modifier = SIM_MOD_NONE  },
    { .row = 5, .col = 6, .hold_cycles = 400000, .expected = '|',  .with_modifier = SIM_MOD_SHIFT },
    { .row = 5, .col = 7, .hold_cycles = 400000, .expected = ']',  .with_modifier = SIM_MOD_NONE  },
    { .row = 5, .col = 7, .hold_cycles = 400000, .expected = '}',  .with_modifier = SIM_MOD_SHIFT },
};

static const sim_keymap_test_t sol_test = {
    .name            = "sol",
    .dip_value       = 4,
    .boot_scan_ticks = 1000,
    .modifier_shift  = { .row = 2, .col = 1 },
    .modifier_ctrl   = { .row = 0, .col = 0 },
    .events          = sol_events,
    .num_events      = sizeof(sol_events) / sizeof(sol_events[0]),
};

/* SOL identity test.
 *
 * sol_id_message() prints "[Keybd: Sol-20]" (no trailing newline).
 * ACTION_FN_10 is at sol_ctrl_map[5][2] (the 0 key), so CTRL+0 triggers the hook.
 * SOL activates capslock on boot; setup_sol_keymap() also calls
 * asdf_arch_set_neg_strobe() but the harness captures on both edges
 * transparently.
 * sol_plain_map[2][0] = ACTION_CAPS (caps toggle).
 * Modifier coords from sol_test above.
 */
static const sim_identity_test_t sol_identity_test = {
    .dip_value             = 4,
    .boot_scan_ticks       = 200,
    .trigger_key           = { .row = 5, .col = 2 },
    .trigger_modifier      = SIM_MOD_CTRL,
    .modifier_shift        = { .row = 2, .col = 1 },
    .modifier_caps_toggle  = { .row = 2, .col = 0 },
    .modifier_ctrl         = { .row = 0, .col = 0 },
    .capture_ticks         = 1500,
    .expected              = "[Keybd: Sol-20]",
    .expected_len          = sizeof("[Keybd: Sol-20]") - 1,
};


/* Typed-string test for sol (DIP 4).
 *
 * Script: <shift>T</shift>HIS IS A TEST OF THE SOL-20 KEYMAP.<ctrl>m</ctrl>
 * Expected output: THIS IS A TEST OF THE SOL-20 KEYMAP.\r
 *
 * setup_sol_keymap() calls asdf_modifier_capslock_activate(), so caps is ON at
 * boot and the SOL-20 produces uppercase throughout.  The caps-toggle key at
 * (2,0) is deliberately omitted from this test because simavr 1.6 exhibits an
 * interaction between asdf_arch_set_neg_strobe() and the atmega1280 timer model
 * that makes the caps debounce unreliable on that target.  All other targets
 * would also support a mixed-case variant, but consistency across targets is
 * more important here.  The shift key exercises the CAPS+SHIFT modifier path
 * (MOD_SHIFT_MAP) for the first letter.
 *
 * SOL key coordinates (from sol_caps_map / sol_shift_map):
 *   space (9,3)   T (6,6)   H (0,7)   I (7,1)   S (0,3)
 *   A     (0,2)   E (6,4)   O (7,2)   F (0,5)
 *   L     (1,2)   K (1,1)   Y (6,7)   M (3,0)   P (7,3)
 *   .     (3,2)   - (5,3)   2 (4,2)   0 (5,2)
 *
 * CTRL+m (row 3, col 0 with CTRL) maps to ASCII_CTRL_M = '\r' via
 * sol_ctrl_map[3][0].
 */
static const sim_string_step_t sol_string_steps[] = {
    /* <shift>T: CAPS+SHIFT => sol_shift_map => 'T' */
    { .type = SIM_STEP_MOD_DOWN, .modifier = SIM_MOD_SHIFT },
    { .type = SIM_STEP_KEY,  .row = 6, .col = 6, .expected = 'T' },
    { .type = SIM_STEP_MOD_UP,   .modifier = SIM_MOD_SHIFT },
    /* HIS IS A  (caps ON -> sol_caps_map -> uppercase) */
    { .type = SIM_STEP_KEY,  .row = 0, .col = 7, .expected = 'H' },
    { .type = SIM_STEP_KEY,  .row = 7, .col = 1, .expected = 'I' },
    { .type = SIM_STEP_KEY,  .row = 0, .col = 3, .expected = 'S' },
    { .type = SIM_STEP_KEY,  .row = 9, .col = 3, .expected = ' ' },
    /* IS  */
    { .type = SIM_STEP_KEY,  .row = 7, .col = 1, .expected = 'I' },
    { .type = SIM_STEP_KEY,  .row = 0, .col = 3, .expected = 'S' },
    { .type = SIM_STEP_KEY,  .row = 9, .col = 3, .expected = ' ' },
    /* A  */
    { .type = SIM_STEP_KEY,  .row = 0, .col = 2, .expected = 'A' },
    { .type = SIM_STEP_KEY,  .row = 9, .col = 3, .expected = ' ' },
    /* TEST  */
    { .type = SIM_STEP_KEY,  .row = 6, .col = 6, .expected = 'T' },
    { .type = SIM_STEP_KEY,  .row = 6, .col = 4, .expected = 'E' },
    { .type = SIM_STEP_KEY,  .row = 0, .col = 3, .expected = 'S' },
    { .type = SIM_STEP_KEY,  .row = 6, .col = 6, .expected = 'T' },
    { .type = SIM_STEP_KEY,  .row = 9, .col = 3, .expected = ' ' },
    /* OF  */
    { .type = SIM_STEP_KEY,  .row = 7, .col = 2, .expected = 'O' },
    { .type = SIM_STEP_KEY,  .row = 0, .col = 5, .expected = 'F' },
    { .type = SIM_STEP_KEY,  .row = 9, .col = 3, .expected = ' ' },
    /* THE  */
    { .type = SIM_STEP_KEY,  .row = 6, .col = 6, .expected = 'T' },
    { .type = SIM_STEP_KEY,  .row = 0, .col = 7, .expected = 'H' },
    { .type = SIM_STEP_KEY,  .row = 6, .col = 4, .expected = 'E' },
    { .type = SIM_STEP_KEY,  .row = 9, .col = 3, .expected = ' ' },
    /* SOL-20  */
    { .type = SIM_STEP_KEY,  .row = 0, .col = 3, .expected = 'S' },
    { .type = SIM_STEP_KEY,  .row = 7, .col = 2, .expected = 'O' },
    { .type = SIM_STEP_KEY,  .row = 1, .col = 2, .expected = 'L' },
    { .type = SIM_STEP_KEY,  .row = 5, .col = 3, .expected = '-' },
    { .type = SIM_STEP_KEY,  .row = 4, .col = 2, .expected = '2' },
    { .type = SIM_STEP_KEY,  .row = 5, .col = 2, .expected = '0' },
    { .type = SIM_STEP_KEY,  .row = 9, .col = 3, .expected = ' ' },
    /* KEYMAP. */
    { .type = SIM_STEP_KEY,  .row = 1, .col = 1, .expected = 'K' },
    { .type = SIM_STEP_KEY,  .row = 6, .col = 4, .expected = 'E' },
    { .type = SIM_STEP_KEY,  .row = 6, .col = 7, .expected = 'Y' },
    { .type = SIM_STEP_KEY,  .row = 3, .col = 0, .expected = 'M' },
    { .type = SIM_STEP_KEY,  .row = 0, .col = 2, .expected = 'A' },
    { .type = SIM_STEP_KEY,  .row = 7, .col = 3, .expected = 'P' },
    { .type = SIM_STEP_KEY,  .row = 3, .col = 2, .expected = '.' },
    /* <ctrl>m</ctrl> => '\r' */
    { .type = SIM_STEP_MOD_DOWN, .modifier = SIM_MOD_CTRL },
    { .type = SIM_STEP_KEY,  .row = 3, .col = 0, .expected = '\r' },
    { .type = SIM_STEP_MOD_UP,   .modifier = SIM_MOD_CTRL },
};

static const sim_string_test_t sol_string_test = {
    .dip_value             = 4,
    .boot_scan_ticks       = 1000,
    .modifier_shift        = { .row = 2, .col = 1 },
    .modifier_caps_toggle  = { .row = 2, .col = 0 },
    .modifier_ctrl         = { .row = 0, .col = 0 },
    .steps                 = sol_string_steps,
    .num_steps             = sizeof(sol_string_steps) / sizeof(sol_string_steps[0]),
};

/* OUT2 regression data (see --mode out2).
 *
 * Sol-20 BREAK at (6,0) is the only place in the tree that reaches
 * PHYSICAL_OUT2: SOL_KBD_BREAK_ACTION -> VOUT2 -> SOL_KBD_TTLOUT_BREAK
 * (= PHYSICAL_OUT2) -> asdf_arch_out2_set().  That makes it the trigger for
 * the historical 328P defect in which asdf_arch_out2_set() wrote
 * ASDF_OUT2_PORT using ASDF_OUT1_BIT, driving LED2 (PB5) instead of OUT2
 * (PB3) — so BREAK lit an LED and never reached the Sol-20.
 *
 * The virtual output is assigned V_PULSE_LONG, so a press produces a pulse
 * rather than a level change; settle_ms must outlast the pulse so both of
 * its edges are observed. */
static const sim_out2_test_t sol_out2_test = {
    .dip_value       = 4,
    .boot_scan_ticks = 1000,
    .trigger_key     = { .row = 6, .col = 0 },   /* BREAK */
    .hold_ms         = 50,
    .settle_ms       = 400,
};

/* Column-independent autorepeat regression (see --mode repeat), for
 * GitHub issue #15.
 *
 * Row 6 is { BREAK, TAB, q, w, e, r, t, y }: columns 1-7 all emit a byte and
 * autorepeat, so their repeat counts are directly comparable.  Column 0 is
 * BREAK, which drives a virtual output and emits nothing, so it is excluded.
 *
 * With the fix in place every column yields an identical count.  With the old
 * early-exit term in asdf_keyscan()'s column loop the count varies by more
 * than 2x across these columns, because the loop stopped as soon as the
 * remaining changed/pressed bits shifted to zero — making per-scan cost, and
 * therefore the servicing rate of a held key, a function of its column. */
static const sim_repeat_test_t sol_repeat_test = {
    .dip_value       = 4,
    .boot_scan_ticks = 1000,
    .row             = 6,
    .cols            = { 1, 2, 3, 4, 5, 6, 7 },
    .num_cols        = 7,
    .hold_ms         = 3000,
    .settle_ms       = 100,
    /* One byte is emitted by the initial press. Requiring at least two proves
     * that autorepeat actually ran rather than failing uniformly. */
    .minimum_count   = 2,
    /* Counts are exactly equal in practice; 1 absorbs any scan-phase
     * alignment jitter without admitting the >2x spread the defect causes. */
    .tolerance       = 1,
};

#endif
