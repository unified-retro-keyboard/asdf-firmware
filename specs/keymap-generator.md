# Keymap generator

`asdf_keymap_gen.py` translates a YAML keymap file into a C source file and a C
header. The keymap modules include the generated header.

The generator checks the keymap for proper formatting, syntax, and valid values.
The generated code is checked by the C compiler.

## Invocation

The command line is

```
asdf_keymap_gen.py <basename>.yaml OUTPUT_DIR
```

where ``basename` is the input file name without its extension.

If successful, the generator
1. Creates `OUTPUT_DIR` along with any missing intermediate directories in the
   path
2. Writes `OUTPUT_DIR/<stem>.c` and `OUTPUT_DIR/<stem>.h`
3. exits with status 0

On failure, the generator prints a message to standard error and writes no
output files:

- **Wrong argument count:** prints `usage: <argv0> INPUT.yaml OUTPUT_DIR`;
  exits with status 2.
- **Input or file error:** prints `<path>: <message>` on one line, naming the
  file at fault; exits with status 1. This includes an input that cannot be
  read and an output that cannot be written.

## Input file

The input is a YAML mapping with these keys; others are ignored:

- **`include`** (optional): a header name, or a sequence of them, declaring the
  symbols the matrices use.
- **`rows`**, **`cols`** (required): the dimensions shared by every matrix.
  Each is a number (decimal or `0x` hex) or a C identifier. Range checks apply
  only to numbers.
- **`maps`** (required): matrix names, each a C identifier, mapped to their
  matrices. Matrices are generated in this order.

Numbers are kept as written, so `0x8C` and `140` stay distinct.

Errors:

- An input that is not a mapping.
- A missing `rows`, `cols` or `maps`.
- A `rows` or `cols` that is not a number or C identifier.
- An `include` that is not a string or a sequence of strings.
- A `maps` that is not a mapping, or has no matrices.
- A matrix name that is not a C identifier.

## Matrices

A matrix is a mapping that pairs each row (identified by row number or C
identifier) with a sequence of key bindings for that row's columns, column 0
first. A row number is a non-negative integer. Rows and keys left out, and null
or empty rows, are `KEY_NOTHING(0)`.

Errors:

- A matrix that is not a mapping.
- A row that is not a non-negative integer or C identifier.
- A row number at or beyond a numeric `rows`.
- A row's keys that are not a sequence.
- A row with more keys than a numeric `cols`.

## Keys

Each key is one of these forms. Blanks around a key macro or code symbol are
ignored.

| YAML key | C initializer | Notes |
|---|---|---|
| `~` | `KEY_NOTHING(0)` | |
| `140`, `0x8c` | `KEY_SEND(140)`, `KEY_SEND(0x8c)` | A code, decimal or hex, at most 255, as written. |
| `'a'` | `KEY_SEND('a')` | Printable, or tab, newline, carriage return. |
| `$SYMBOL` | `KEY_SEND(SYMBOL)` | `SYMBOL` names a code. |
| `NAME`, `NAME()` | `NAME(0)` | |
| `NAME(PARAM)` | `NAME(<param>)` | See [Key parameters](#key-parameters). |
| `[a, b, c, d]` | `{ a, b, c, d }` | Press action and parameter, release action and parameter, as written. |

Characters are written as C character literals, with backslash, quote, tab,
newline and carriage return escaped.

Errors:

- A boolean or floating-point value; the message asks for quotes.
- A number above 255, or a decimal number with a leading zero.
- A one-character string that is neither printable nor tab, newline or carriage
  return.
- A sequence without exactly four fields, or with a null field.
- Anything else.

### Key parameters

`PARAM`, with surrounding blanks ignored, is one of:

- **C identifier:** passed through.
- **Quoted character:** one character in single or double quotes, written as a
  C character literal. `\n`, `\t` and `\r` are control characters; a backslash
  before any other character is that character.
- **Number:** decimal or `0x` hex, at most 255, passed through. A decimal
  number has no leading zero, so `0` is valid and `010` is not.

Anything else is an error.

## Generated header

`<basename>.h` contains, in order:

1. A Doxygen `@file` block saying the file is generated from
   `<basename>.yaml` and should not be edited.
2. An include guard, `<BASENAME>_H`: the basename in upper case, with anything
   other than letters and digits replaced by `_`.
3. `#include "asdf_arch.h"`, `#include "asdf_actions.h"`, then each `include`
   entry.
4. An `extern const FLASH asdf_key_t <name>[<rows>][<cols>];` for each matrix,
   with `rows` and `cols` as written in the input.
5. The closing `#endif`.

## Generated source

`<basename>.c` contains, in order:

1. A Doxygen `@file` block, as in the header.
2. `#include "<basename>.h"`.
3. PC-lint Plus comments disabling `785` (keys left zero) and `751` for
   `*_fit` (the size checks).
4. Size checks `<id>_rows_fit` and `<id>_cols_fit`, where `<id>` is the
   basename with anything other than letters and digits replaced by `_`. Each
   is a `char` array typedef that fails to compile if `rows` exceeds
   `ASDF_MAX_ROWS` or `cols` exceeds `ASDF_MAX_COLS`.
5. A `const FLASH asdf_key_t <name>[<rows>][<cols>]` definition for each
   matrix.

Each definition has one line per row, in the form
`[<row>] = { [<col>] = <key>, ... },`. Numbered rows come first in ascending
order, then symbolic rows by name.
