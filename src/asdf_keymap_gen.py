#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.8"
# dependencies = ["pyyaml"]
# ///
#
# Unified Keyboard Project
# ASDF keyboard firmware
#
# asdf_keymap_gen.py
#
# Generates C key matrices from a YAML file of named matrices. The output is a
# .c file defining each matrix as a FLASH array of asdf_key_t, and a .h file
# declaring them, for inclusion by the master keymap files.
#
# usage: asdf_keymap_gen.py INPUT.yaml OUTPUT_DIR
#
# The outputs are OUTPUT_DIR/<stem>.c and OUTPUT_DIR/<stem>.h, where <stem> is
# the input file name without its extension.
#
# Input format:
#
#   include: [asdf_keymap_classic.h]  # headers naming the symbols used below
#   rows: CLASSIC_NUM_ROWS            # number or C expression
#   cols: CLASSIC_NUM_COLS
#   maps:
#     classic_plain_matrix:           # a list of rows, from row 0 ...
#       - [~, KEY_SHIFT, KEY_SHIFT, ~, $ASCII_ESC, $ASCII_TAB, KEY_CTRL, '\']
#       - ...
#     classic_dip_example:            # ... or a mapping of row to keys, where a
#       ASDF_ARCH_DIPSWITCH_ROW: [KEY_MAPSEL(0), KEY_MAPSEL(1)]  # row is a number or C symbol
#
# Rows shorter than the column count, and rows not given, are filled with
# KEY_NOTHING(0). Each key is one of:
#
#   NAME(PARAM)            a key macro, passed to C as written. Every key macro
#   NAME                   takes one parameter, which is 0 if omitted. PARAM is
#                          a C macro, a quoted character ('a'), or a number
#                          0-255 (decimal or 0x..). Key macros are named KEY_
#                          by convention: KEY_SHIFT, KEY_MAPSEL(2).
#   $SYMBOL                sends the code the C macro SYMBOL names:
#                          KEY_SEND(SYMBOL)
#   a hex number (0x8C)    sends that code: KEY_SEND(0x8C)
#   a single digit (7)     sends that digit character: KEY_SEND('7')
#   a one-character string sends that character: KEY_SEND('a')
#   ~ (null)               does nothing: KEY_NOTHING(0)
#
# So a bare symbol is always a key, and a code symbol is marked with $. A
# symbol used as the wrong kind fails to compile: ASCII_ESC(0) is not a macro
# call, and neither is KEY_SEND(KEY_SHIFT) a code.
#
# KEY_SEND keys autorepeat while held; KEY_SEND_ONCE(code) sends once. The key
# macros are defined in asdf_actions.h and in the headers named by include.
#
# A YAML list [PRESS_FN, PRESS_PARAM, RELEASE_FN, RELEASE_PARAM] gives the four
# fields of the key directly.
#
# Write a key containing a comma, bracket, brace, colon, or '#' in quotes, for
# example "KEY_SEND_ONCE(',')".
#
# The C compiler checks every symbol; this script checks the layout: row and
# column counts where they are numbers, key syntax, and value ranges.

import re
import sys
from pathlib import Path

import yaml

IDENT = re.compile(r"^[A-Za-z_][A-Za-z0-9_]*$")
KEY_MACRO = re.compile(r"^([A-Za-z_][A-Za-z0-9_]*)(?:\((.*)\))?$")
CODE_SYMBOL = re.compile(r"^\$([A-Za-z_][A-Za-z0-9_]*)$")


class KeymapError(Exception):
    pass


class Number(str):
    """A number in the YAML source, kept as written. As a key, a single digit
    is that digit character and a hex number is a code; other bare numbers are
    rejected, since YAML would otherwise silently take them as codes."""


class KeymapLoader(yaml.SafeLoader):
    """A YAML loader that keeps numbers as written (see Number)."""


def construct_number(loader, node):
    return Number(node.value)


KeymapLoader.add_constructor("tag:yaml.org,2002:int", construct_number)


def c_char(ch):
    """Return a C character literal for one character."""
    escapes = {"\\": "\\\\", "'": "\\'", "\n": "\\n", "\t": "\\t", "\r": "\\r"}
    if ch in escapes:
        return "'" + escapes[ch] + "'"
    if " " <= ch <= "~":
        return "'" + ch + "'"
    raise KeymapError("character %r is not printable; give its code" % ch)


def param_literal(text, where):
    """Return the C expression for a key parameter: a C macro, a quoted
    character, or a number 0-255."""
    text = text.strip()
    if IDENT.match(text):
        return text
    quoted = re.match(r"""^(['"])(.+)\1$""", text)
    if quoted:
        ch = quoted.group(2)
        if len(ch) == 2 and ch[0] == "\\":
            ch = {"n": "\n", "t": "\t", "r": "\r"}.get(ch[1], ch[1])
        if len(ch) != 1:
            raise KeymapError("%s: parameter %s is not one character" % (where, text))
        return c_char(ch)
    if re.match(r"^(0[xX][0-9A-Fa-f]+|[0-9]+)$", text):
        value = int(text, 0) if not re.match(r"^0[0-9]+$", text) else int(text, 10)
        if value > 0xFF:
            raise KeymapError("%s: parameter %s is out of range 0-255" % (where, text))
        return text
    raise KeymapError("%s: parameter %r is not a macro, character, or number" % (where, text))


def key_initializer(key, where):
    """Return the C initializer for one key of a YAML matrix."""
    if key is None:
        return "KEY_NOTHING(0)"
    if isinstance(key, bool) or isinstance(key, float):
        raise KeymapError("%s: %r is not a key; quote it" % (where, key))
    if isinstance(key, Number):
        if re.match(r"^[0-9]$", key):
            return "KEY_SEND(%s)" % c_char(key)
        if not re.match(r"^0[xX][0-9A-Fa-f]+$", key):
            raise KeymapError("%s: write codes in hex (0x..), not %s" % (where, key))
        code = int(key, 16)
        if code > 0xFF:
            raise KeymapError("%s: code %s is out of range 0x00-0xFF" % (where, key))
        return "KEY_SEND(0x%02X)" % code
    if isinstance(key, list):
        if len(key) != 4:
            raise KeymapError("%s: a raw key needs 4 fields, not %d" % (where, len(key)))
        return "{ %s }" % ", ".join(str(field) for field in key)
    if not isinstance(key, str):
        raise KeymapError("%s: %r is not a key" % (where, key))
    if len(key) == 1:
        return "KEY_SEND(%s)" % c_char(key)

    code = CODE_SYMBOL.match(key.strip())
    if code:
        return "KEY_SEND(%s)" % code.group(1)
    macro = KEY_MACRO.match(key.strip())
    if macro:
        name, param = macro.groups()
        if param is None or not param.strip():
            return "%s(0)" % name
        return "%s(%s)" % (name, param_literal(param, where))
    raise KeymapError("%s: %r is not a key" % (where, key))


def dimension(value, name):
    """Return (C text, number or None) for a rows or cols value."""
    if isinstance(value, Number):
        try:
            return value, int(value, 0)
        except ValueError:
            raise KeymapError("%s: %s is not a number" % (name, value))
    if isinstance(value, str) and IDENT.match(value):
        return value, None
    raise KeymapError("%s must be a number or a C identifier" % name)


def matrix_rows(name, spec, num_rows, num_cols):
    """Return {row: [initializer, ...]} for one named matrix."""
    if isinstance(spec, list):
        rows = dict(enumerate(spec))
    elif isinstance(spec, dict):
        rows = spec
    else:
        raise KeymapError("%s: a matrix is a list or mapping of rows" % name)

    result = {}
    for row, keys in rows.items():
        if isinstance(row, Number):
            row = int(row, 0)
        if isinstance(row, str) and IDENT.match(row):
            pass  # a C symbol; the compiler checks it
        elif not isinstance(row, int) or row < 0:
            raise KeymapError("%s: row %r is not a row number or C symbol" % (name, row))
        elif num_rows is not None and row >= num_rows:
            raise KeymapError("%s: row %d is beyond %d rows" % (name, row, num_rows))
        if not keys:
            continue  # an empty row does nothing, as rows left out do
        if not isinstance(keys, list):
            raise KeymapError("%s: row %s is not a list of keys" % (name, row))
        if num_cols is not None and len(keys) > num_cols:
            raise KeymapError(
                "%s: row %s has %d keys, more than %d columns" % (name, row, len(keys), num_cols)
            )
        # Each key is designated by its column, so a short row is a sparse
        # initialization rather than a partial one (MISRA C Rule 9.3).
        result[row] = [
            "[%d] = %s" % (col, key_initializer(key, "%s[%s][%d]" % (name, row, col)))
            for col, key in enumerate(keys)
        ]
    return result


def generate(source, stem):
    """Return (c_text, h_text) for a parsed YAML keymap file."""
    if not isinstance(source, dict) or "maps" not in source:
        raise KeymapError("the file needs a 'maps' mapping")
    rows_c, num_rows = dimension(source.get("rows"), "rows")
    cols_c, num_cols = dimension(source.get("cols"), "cols")
    includes = source.get("include", [])
    if isinstance(includes, str):
        includes = [includes]

    guard = re.sub(r"[^A-Za-z0-9]", "_", stem).upper() + "_H"
    def banner(ext, what):
        return "\n".join([
            "/**",
            " * @file %s.%s" % (stem, ext),
            " *",
            " * %s, generated by asdf_keymap_gen.py from %s.yaml." % (what, stem),
            " * Do not edit: edit the YAML file instead.",
            " */",
        ])
    header_includes = ['#include "asdf_arch.h"', '#include "asdf_actions.h"'] + [
        '#include "%s"' % name for name in includes
    ]

    h = [banner("h", "Key matrix declarations"), "", "#if !defined(%s)" % guard,
         "#define %s" % guard, ""]
    h += header_includes + [""]
    c = [banner("c", "Key matrices"), "", '#include "%s.h"' % stem, ""]
    # Keys left out of a row are zero, and a zero key is KEY_NOTHING(0)
    # (ACTION_NOTHING is 0). The _fit typedefs exist only to fail compilation.
    c.append("//lint -e785 keys not designated are zero: KEY_NOTHING(0)")
    c.append("//lint -esym(751, *_fit) compile-time size checks, never referenced")
    c.append("")

    # The matrices must fit the scanner: a negative array size fails to compile.
    check = re.sub(r"[^A-Za-z0-9]", "_", stem)
    c.append("typedef char %s_rows_fit[((uint16_t) (%s) <= ASDF_MAX_ROWS) ? 1 : -1];" % (check, rows_c))
    c.append("typedef char %s_cols_fit[((uint16_t) (%s) <= ASDF_MAX_COLS) ? 1 : -1];" % (check, cols_c))
    c.append("")

    for name, spec in source["maps"].items():
        if not IDENT.match(str(name)):
            raise KeymapError("%r is not a C identifier" % name)
        decl = "const FLASH asdf_key_t %s[%s][%s]" % (name, rows_c, cols_c)
        h.append("extern %s;" % decl)
        c.append("%s = {" % decl)
        rows = matrix_rows(name, spec, num_rows, num_cols)
        # numbered rows in order, then rows named by C symbols
        for row in sorted(rows, key=lambda r: (isinstance(r, str), r if isinstance(r, int) else 0, str(r))):
            c.append("  [%s] = { %s }," % (row, ", ".join(rows[row])))
        c.append("};")
        c.append("")

    h += ["", "#endif /* !defined(%s) */" % guard, ""]
    return "\n".join(c), "\n".join(h)


def main(argv):
    if len(argv) != 3:
        print("usage: %s INPUT.yaml OUTPUT_DIR" % argv[0], file=sys.stderr)
        return 2
    source_path = Path(argv[1])
    out_dir = Path(argv[2])
    stem = source_path.stem
    try:
        with source_path.open() as f:
            source = yaml.load(f, Loader=KeymapLoader)
        c_text, h_text = generate(source, stem)
    except (KeymapError, yaml.YAMLError) as err:
        print("%s: %s" % (source_path, err), file=sys.stderr)
        return 1
    out_dir.mkdir(parents=True, exist_ok=True)
    (out_dir / (stem + ".c")).write_text(c_text)
    (out_dir / (stem + ".h")).write_text(h_text)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
