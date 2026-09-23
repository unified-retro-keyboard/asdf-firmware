// -*- mode: C; tab-width: 4 ; indent-tabs-mode: nil -*-
//
// Unfified Keyboard Project
// ASDF keyboard firmware
//
// asdf_hook.h
//
// Definitions and prototypes for configurable function hooks
//
// Copyright 2019 David Fenyes
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

#if !defined(ASDF_HOOKS_H)
#define ASDF_HOOKS_H

// Hook identifiers. Each hook holds a function with no arguments, called for an
// event: EACH_SCAN at the start of every matrix scan, USER_n by the ACTION_FN_n
// key actions. Hardware access (row scanning and code output) is not a hook;
// see asdf_platform.h.
typedef enum {
  ASDF_HOOK_NULL,
  ASDF_HOOK_EACH_SCAN,
  ASDF_HOOK_USER_1,
  ASDF_HOOK_USER_2,
  ASDF_HOOK_USER_3,
  ASDF_HOOK_USER_4,
  ASDF_HOOK_USER_5,
  ASDF_HOOK_USER_6,
  ASDF_HOOK_USER_7,
  ASDF_HOOK_USER_8,
  ASDF_HOOK_USER_9,
  ASDF_HOOK_USER_10,
  ASDF_HOOK_USER_11,
  ASDF_NUM_HOOKS,
} asdf_hook_id_t;

typedef void (*asdf_hook_function_t)(void);

// The functions bound to each hook, for one keyboard.
typedef struct {
  asdf_hook_function_t map[ASDF_NUM_HOOKS];
} asdf_hook_state_t;

// Instance API: each function operates only on the hook state passed to it.
void asdf_hook_init_r(asdf_hook_state_t *hooks);
void asdf_hook_assign_r(asdf_hook_state_t *hooks, asdf_hook_id_t hook_id,
                        asdf_hook_function_t func);
void asdf_hook_execute_r(const asdf_hook_state_t *hooks, asdf_hook_id_t hook_id);

// Single-keyboard API, operating on the default keyboard (asdf_compat.c).


// PROCEDURE: asdf_hook_execute
// INPUTS: (asdf_hook_id_t) hook_id: The hook for which to execute attache functions.
// OUTPUTS: none
// DESCRIPTION: if the hook ID is valid, then execute the function attached to the hook.
// SIDE EFFECTS: variable depending on the hooks.
void asdf_hook_execute(asdf_hook_id_t hook_id);


// PROCEDURE: asdf_hook_init
// INPUTS: none
// OUTPUTS: none
// DESCRIPTION: Initializes function hooks for the selected keymap. If a
// function is assigned to the "KEYMAP_SETUP" hook, then execute the function.
// There is no actual slot where KEYMAP_SETUP functions are stored.
void asdf_hook_init(void);

// PROCEDURE: asdf_hook_assign
// INPUTS: (asdf_hook_id_t) hook_id: The hook for which to execute attache functions.
//         (asdf_hook_function_t) func: function to be attached to the hook.
// OUTPUTS: none
// DESCRIPTION: If the hook ID is valid, map the function to the hook ID.
// Ignore if not valid.
void asdf_hook_assign(asdf_hook_id_t hook_id, asdf_hook_function_t func);


#endif /* !defined (ASDF_HOOKS_H) */

//-------|---------|---------+---------+---------+---------+---------+---------+
// Above line is 80 columns, and should display completely in the editor.
