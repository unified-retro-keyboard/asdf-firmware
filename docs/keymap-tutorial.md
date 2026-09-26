# Writing a keymap

This tutorial adds a keymap for a new keyboard, from the key matrix to a
firmware you can select with the DIP switches. The running example is a small
keyboard called `demo`. The Sol-20 keymap (`src/Keymaps/asdf_keymap_sol*`) is a
complete real one to compare against.

A keymap is four files and three registrations:

| File | Holds |
|---|---|
| `src/Keymaps/asdf_keymap_demo_maps.yaml` | The key matrices, one per modifier state. |
| `src/Keymaps/asdf_keymap_demo.h` | The matrix size, output assignments, and special codes. |
| `src/Keymaps/asdf_keymap_demo.c` | The keymap descriptor: which matrix is which, flags, outputs, ID message. |
| `keymap_list.cmake`, `cmake/asdf_keymap_sources.cmake` | Registration: the keymap's number, and its YAML and C files. |

The exact YAML rules are in [the keymap generator spec](../specs/keymap-generator.md).

## 1. Map the keyboard

Trace the keyboard's switch matrix and write it down as a table of rows and
columns. The firmware scans up to 16 rows and 8 columns. Row 8 is reserved for
the controller's DIP switches, so a keyboard with more than 8 rows skips it.

For `demo`:

```
Col->   0        1        2        3        4        5        6        7
Row 0   Ctrl     Shift    A        B        C        D        E        F
Row 1   1        2        3        4        Space    Return   Del      Caps
Row 2   Esc      Break
Row 8   DIP switches 0-7
```

Keep this table in a comment in the header, as the Sol-20 keymap does. It is
the reference when the matrices are wrong.

## 2. Write the header

The header names the matrix size and anything the YAML refers to by name.

```c
#if !defined(ASDF_KEYMAP_DEMO_H)
#define ASDF_KEYMAP_DEMO_H

#include "asdf_arch.h"
#include "asdf_keymaps.h"
#include "asdf.h"

#define DEMO_NUM_ROWS 9 // DIP switches are row 8
#define DEMO_NUM_COLS 8

#define DEMO_PRINT_DELAY 40 // msec between characters of the ID message

// Outputs
#define DEMO_VBREAK      VOUT1
#define DEMO_TTL_BREAK   PHYSICAL_OUT1
#define DEMO_TTL_HIGH    1
#define DEMO_LED_CAPS    PHYSICAL_LED1
#define DEMO_LED_OFF     0

/** The demo keymap's descriptor, in flash. */
extern const asdf_keymap_t FLASH demo_keymap;

#endif /* !defined(ASDF_KEYMAP_DEMO_H) */
```

The header must be named `asdf_keymap_<name>.h` and declare `<name>_keymap`:
the build finds the keymap by that name.

## 3. Write the key matrices

A matrix pairs each row with a sequence of key bindings for that row's columns,
column 0 first. A keymap has four matrices, one for each modifier state:

- **plain:** no modifier.
- **shift:** SHIFT held, or shift lock on.
- **caps:** caps lock on.
- **ctrl:** CTRL held. CTRL wins over SHIFT and caps lock, and SHIFT wins over
  caps lock.

`src/Keymaps/asdf_keymap_demo_maps.yaml`:

```yaml
include: [asdf_keymap_demo.h, asdf_ascii.h, asdf_virtual.h]
rows: DEMO_NUM_ROWS
cols: DEMO_NUM_COLS
maps:
  demo_plain_map:
    0: [KEY_CTRL, KEY_SHIFT, 'a', 'b', 'c', 'd', 'e', 'f']
    1: ['1', '2', '3', '4', $ASCII_SPACE, $ASCII_CR, $ASCII_DEL, KEY_CAPS]
    2: [$ASCII_ESC, KEY_VIRTUAL(DEMO_VBREAK)]
    ASDF_ARCH_DIPSWITCH_ROW: &dip [KEY_MAPSEL(0), KEY_MAPSEL(1), KEY_MAPSEL(2), KEY_MAPSEL(3), ~, ~, KEY_STROBE_POLARITY, KEY_AUTOREPEAT]
  demo_shift_map:
    0: [KEY_CTRL, KEY_SHIFT, 'A', 'B', 'C', 'D', 'E', 'F']
    1: ['!', $ASCII_DOUBLE_QUOTE, '#', '$', $ASCII_SPACE, $ASCII_CR, $ASCII_DEL, KEY_CAPS]
    2: [$ASCII_ESC, KEY_VIRTUAL(DEMO_VBREAK)]
    ASDF_ARCH_DIPSWITCH_ROW: *dip
  demo_caps_map:
    0: [KEY_CTRL, KEY_SHIFT, 'A', 'B', 'C', 'D', 'E', 'F']
    1: ['1', '2', '3', '4', $ASCII_SPACE, $ASCII_CR, $ASCII_DEL, KEY_CAPS]
    2: [$ASCII_ESC, KEY_VIRTUAL(DEMO_VBREAK)]
    ASDF_ARCH_DIPSWITCH_ROW: *dip
  demo_ctrl_map:
    0: [KEY_CTRL, KEY_SHIFT, $ASCII_CTRL_A, $ASCII_CTRL_B, $ASCII_CTRL_C, $ASCII_CTRL_D, $ASCII_CTRL_E, $ASCII_CTRL_F]
    1: ['1', '2', '3', KEY_KEYMAP_ID, $ASCII_SPACE, $ASCII_CR, $ASCII_DEL, KEY_CAPS]
    2: [$ASCII_ESC, KEY_VIRTUAL(DEMO_VBREAK)]
    ASDF_ARCH_DIPSWITCH_ROW: *dip
```

Things to notice:

- **Characters are quoted.** `'a'` sends the character `a`, and `'1'` sends the
  digit character. An unquoted number is a code, in decimal (`140`) or hex
  (`0x8C`), so an unquoted `7` sends code 7 (BELL), not the digit.
- **`$` names a code.** `$ASCII_CR` sends the code `ASCII_CR` from
  `asdf_ascii.h`. Use it for anything YAML would misread, such as a space,
  comma, quote or colon, and for control codes.
- **A bare name is a key action.** `KEY_SHIFT`, `KEY_CAPS` and `KEY_KEYMAP_ID`
  are key macros from `src/asdf_actions.h`. A macro with a parameter is written
  with it: `KEY_VIRTUAL(DEMO_VBREAK)`, `KEY_MAPSEL(2)`.
- **`~` is no key.** Rows and keys left out do nothing too, so row 2 stops
  after column 1, and rows 3 to 7 are not written at all.
- **Modifier keys appear in every matrix.** Without `KEY_SHIFT` in the shift
  matrix, releasing SHIFT would do nothing.
- **The DIP switch row is the same in every keymap.** Write it once with a YAML
  anchor (`&dip`) and reuse it (`*dip`). Keeping `KEY_MAPSEL(0)` to
  `KEY_MAPSEL(3)` in columns 0 to 3 keeps map selection the same across
  keymaps.
- **`KEY_KEYMAP_ID` prints the keymap's name.** Putting it on a control key
  lets you check which keymap is running.

The key macros:

| Key | Action |
|---|---|
| `KEY_SEND(code)` | Sends `code`, and autorepeats while held. A quoted character or `$` code is this key. |
| `KEY_SEND_ONCE(code)` | Sends `code` once, however long it is held. |
| `KEY_SHIFT`, `KEY_CTRL` | Modifiers, active while held. |
| `KEY_CAPS` | Toggles caps lock. |
| `KEY_SHIFTLOCK_ON`, `KEY_SHIFTLOCK_TOGGLE` | Shift lock: SHIFT releases it, or the key toggles it. |
| `KEY_REPEAT` | Repeats the held key while pressed. |
| `KEY_VIRTUAL(vout)` | Activates a virtual output; see step 4. |
| `KEY_KEYMAP_ID` | Prints the keymap's ID message. |
| `KEY_MAPSEL(bit)`, `KEY_STROBE_POLARITY`, `KEY_AUTOREPEAT` | DIP switches: keymap select bit, strobe polarity, autorepeat. |

## 4. Write the descriptor

The descriptor ties the matrices together and sets the keymap's outputs.

```c
#include "asdf_arch.h"
#include "asdf_keymap_demo.h"
#include "asdf_keymap_demo_maps.h"
#include "asdf.h"
#include "asdf_keymaps.h"

// Printed by the KEYMAP_ID key.
static const char FLASH demo_id_message[] = "[Keybd: Demo]";

static const asdf_virtual_initializer_t FLASH demo_outputs[] = {
  // The caps lock LED, off at start.
  { VCAPS_LED,   DEMO_LED_CAPS,  V_NOFUNC,     DEMO_LED_OFF  },
  // BREAK: a long pulse on OUT1, normally high.
  { DEMO_VBREAK, DEMO_TTL_BREAK, V_PULSE_LONG, DEMO_TTL_HIGH },
};

const asdf_keymap_t FLASH demo_keymap = {
  .maps = { [MOD_PLAIN_MAP] = &demo_plain_map[0][0],
            [MOD_SHIFT_MAP] = &demo_shift_map[0][0],
            [MOD_CAPS_MAP] = &demo_caps_map[0][0],
            [MOD_CTRL_MAP] = &demo_ctrl_map[0][0] },
  .rows = DEMO_NUM_ROWS,
  .cols = DEMO_NUM_COLS,
  .print_delay_ms = DEMO_PRINT_DELAY,
  .id_message = demo_id_message,
  .num_outputs = ASDF_NUM_ELEMENTS(demo_outputs),
  .outputs = demo_outputs,
};
```

`asdf_keymap_demo_maps.h` is generated from the YAML file at build time; it
declares the four matrices.

Each output line binds a *virtual output* to a *physical output* on the
controller, with a function and a starting value. Keys activate virtual
outputs (`KEY_VIRTUAL(DEMO_VBREAK)`), and the firmware drives the LEDs for
caps lock and shift lock (`VCAPS_LED`, `VSHIFT_LED`) itself. The functions are
`V_NOFUNC` (driven by the firmware), `V_SET_HI`, `V_SET_LO`, `V_TOGGLE`,
`V_PULSE_SHORT` and `V_PULSE_LONG`. Bind two physical outputs to one virtual
output to drive both from one key, as the Sol-20 keymap does for LOCAL.

Flags are optional: `.flags = ASDF_KEYMAP_CAPS_ON` starts with caps lock on,
and `ASDF_KEYMAP_NEGATIVE_STROBE` starts with a negative strobe.

## 5. Register the keymap

Give the keymap a number from 0 to 15, the DIP switch setting that selects it.
In `keymap_list.cmake`:

```cmake
list(APPEND keymap_list
  ...
  "<ace1000:5>"
  "<demo:6>"
  )
```

In `cmake/asdf_keymap_sources.cmake`, add the YAML file to `ASDF_KEYMAP_YAML`
and the C file to `ASDF_KEYMAP_SOURCES`:

```cmake
set(ASDF_KEYMAP_YAML
  ...
  Keymaps/asdf_keymap_demo_maps.yaml
  )

set(ASDF_KEYMAP_SOURCES
  ...
  Keymaps/asdf_keymap_demo.c
  ...
  )
```

## 6. Build and check

Run the host tests first:

```
cmake --workflow --preset test
```

Mistakes show up at three stages:

- **Generating the matrices:** a malformed key or matrix fails with a message
  naming the YAML file, matrix, row and column, for example
  `demo_plain_map[0][4]: code 0x100 is out of range 0x00-0xFF`.
- **Compiling:** a name the C code does not define, such as a misspelled key
  macro or `$` code, fails to compile in the generated `asdf_keymap_demo_maps.c`.
- **Testing:** `test_production_keymaps` applies every registered keymap and
  fails if any part of the descriptor is rejected: a missing matrix, a matrix
  too large for the scanner, or a bad output assignment.

Then build the firmware for your controller:

```
cmake --workflow --preset atmega328p
```

The build's test, `size_budget`, checks the firmware against the flash and RAM
budgets in `test/size-ceilings.csv`. Each budget sits about 5% above the size
when it was set, so a new keymap can push past it. If it does, raise that
target's budget in the same commit as the keymap.

Flash the firmware, set DIP switches 0 to 3 to the keymap's number in binary
(6 is switches 1 and 2 on), and press the key bound to `KEY_KEYMAP_ID`. The
keyboard prints `[Keybd: Demo]`.
