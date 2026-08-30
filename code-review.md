- **Resolved – Modifier keymap bounds:** `asdf_keymaps_get_code` now pulls row/column
  metadata from `keymaps[modifier_index]`, so higher-numbered layouts no longer read
  past the four-entry modifier array. The undefined behaviour called out previously
  is closed by keeping `current_keyboard_index` strictly for tracking the active map.

- **Resolved – Configuration replay:** `asdf_keymaps_switch()` now invokes
  `asdf_apply_all_actions()` (and the dispatcher filters for handlers flagged with
  `ASDF_HANDLER_APPLY_ON_KEYBOARD_INIT`), so DIP-switch driven actions and other
  configuration bindings are replayed whenever a layout changes.

- **Resolved – Initial virtual outputs:** `asdf_keymaps_switch()` finishes by calling
  `asdf_virtual_sync()`, ensuring shadowed LED/reset states are asserted on the new
  hardware configuration immediately after a keymap changes.

- **Outstanding – Hook type-safety:** The hook subsystem still stores every function
  as `void (*)(void)` (`asdf_hook.h`) and callers continue to cast those pointers to
  unrelated signatures (e.g., the row scanner in `asdf_keyscan`). This remains
  undefined behaviour on most embedded compilers. We still need typed APIs (one per
  hook kind or a tagged union) so the casts disappear.



  1. Missing Bounds Checking in asdf_keymaps_get_code() (Design Decision)

  - Status: Not really a bug, but a deliberate design tradeoff
  - Impact: Could read out of bounds if scanner generates invalid row/col
  - Risk: Low - scanner should only generate valid indices
  - Recommendation: Leave as-is for performance, or add DEBUG-only checks

  2. Potential Debounce Counter Underflow (asdf.c:492)

  - Status: Minor theoretical issue
  - Impact: If debounce counter is already 0, pre-decrement causes underflow to 255
  - Risk: Very low - counters initialized to ASDF_DEBOUNCE_TIME_MS (10)
  - Fix: Could add a check, but probably unnecessary given initialization

  3. Modifier State Style Issue (asdf_modifiers.c:146)

  - Status: Style/clarity issue, not a bug
  - Code: asdf_modifier_set_caps_state(caps_state ^= CAPS_LOCKED_ST);
  - Impact: Works correctly but confusing to read
  - Recommendation: Separate the XOR from the function call for clarity

  4. Shift Deactivate Clears Shift Lock (asdf_modifiers.c:176-178)

  - Status: Intentional per comments, but may not match user expectations
  - Impact: SHIFT_LOCK is cleared when releasing SHIFT key
  - Note: Comment says "this also clears any SHIFT_LOCK condition"

  5. Fall-through in switch statement (asdf_virtual.c:119)

  - Status: Style issue, functionally correct
  - Impact: V_SET_LO falls through to V_NOFUNC (which does nothing)
  - Recommendation: Add explicit break; for clarity

  6. Inconsistent Line Ending Handling (asdf.c:111-119)

  - Status: Minor issue
  - Impact: Recursive call for LF→CR+LF, silent drop if buffer full
  - Risk: Low - system messages are infrequent
