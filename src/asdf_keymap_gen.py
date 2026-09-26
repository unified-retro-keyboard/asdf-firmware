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
# The input format, the key forms, and the generated files are specified in
# specs/keymap-generator.md, and docs/keymap-tutorial.md shows them in use. In
# short:
#
#   include: [asdf_keymap_classic.h]  # headers naming the symbols used below
#   rows: CLASSIC_NUM_ROWS            # number or C identifier
#   cols: CLASSIC_NUM_COLS
#   maps:
#     classic_plain_matrix:           # row (number or C identifier) -> keys
#       0: [~, KEY_SHIFT, 'a', $ASCII_ESC, 0x8C, KEY_MAPSEL(1)]
#       ASDF_ARCH_DIPSWITCH_ROW: [KEY_MAPSEL(0), KEY_MAPSEL(1)]
#
# ~ is no key, a quoted character or a number (decimal or hex) is sent, $SYMBOL
# sends the code SYMBOL names, and NAME or NAME(PARAM) is a key macro. Rows and
# keys left out are KEY_NOTHING(0).
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
# Decimal without a leading zero, which C would read as octal, or 0x hex.
NUMBER = re.compile(r"^(0|[1-9][0-9]*|0[xX][0-9A-Fa-f]+)$")


class KeymapError(Exception):
    pass


class Number(str):
    """A number in the YAML source, kept as written, so it is passed to C in
    the form the author chose."""


class KeymapLoader(yaml.SafeLoader):
    """A YAML loader that keeps numbers as written (see Number)."""


def construct_number(loader, node):
    return Number(node.value)


KeymapLoader.add_constructor("tag:yaml.org,2002:int", construct_number)


def number_value(text, where):
    """Return the value of a number written as decimal or 0x hex."""
    if not NUMBER.match(text):
        raise KeymapError("%s: %s is not a decimal (no leading zero) or 0x hex number"
                          % (where, text))
    return int(text, 0)


def byte_literal(text, where):
    """Return a number 0-255 as written, for C."""
    if number_value(text, where) > 0xFF:
        raise KeymapError("%s: %s is out of range 0-255" % (where, text))
    return text


def c_char(ch, where):
    """Return a C character literal for one character."""
    escapes = {"\\": "\\\\", "'": "\\'", "\n": "\\n", "\t": "\\t", "\r": "\\r"}
    if ch in escapes:
        return "'" + escapes[ch] + "'"
    if " " <= ch <= "~":
        return "'" + ch + "'"
    raise KeymapError("%s: character %r is not printable; give its code" % (where, ch))


def param_literal(text, where):
    """Return the C expression for a key parameter: a C identifier, a quoted
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
        return c_char(ch, where)
    if NUMBER.match(text):
        return byte_literal(text, where)
    raise KeymapError("%s: parameter %r is not a C identifier, character, or number"
                      % (where, text))


def raw_field(field, where):
    """Return one field of a four-field key, as written."""
    if isinstance(field, str) and field.strip():
        return field.strip()
    raise KeymapError("%s: %r is not a key field" % (where, field))


def key_initializer(key, where):
    """Return the C initializer for one key of a YAML matrix."""
    if key is None:
        return "KEY_NOTHING(0)"
    if isinstance(key, bool) or isinstance(key, float):
        raise KeymapError("%s: %r is not a key; quote it" % (where, key))
    if isinstance(key, Number):
        return "KEY_SEND(%s)" % byte_literal(key, where)
    if isinstance(key, list):
        if len(key) != 4:
            raise KeymapError("%s: a raw key needs 4 fields, not %d" % (where, len(key)))
        return "{ %s }" % ", ".join(raw_field(field, where) for field in key)
    if not isinstance(key, str):
        raise KeymapError("%s: %r is not a key" % (where, key))
    if len(key) == 1:
        return "KEY_SEND(%s)" % c_char(key, where)

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


def dimension(source, name):
    """Return (C text, number or None) for the rows or cols value."""
    if name not in source:
        raise KeymapError("the file needs '%s'" % name)
    value = source[name]
    if isinstance(value, Number):
        return value, number_value(value, name)
    if isinstance(value, str) and IDENT.match(value):
        return value, None
    raise KeymapError("%s must be a number or a C identifier" % name)


def header_names(source):
    """Return the header names given by include."""
    if "include" not in source:
        return []
    includes = source["include"]
    if isinstance(includes, str):
        includes = [includes]
    if not isinstance(includes, list) or not all(
        isinstance(name, str) and not isinstance(name, Number) for name in includes
    ):
        raise KeymapError("include must be a header name or a sequence of them")
    return includes


def row_designator(row, name, num_rows):
    """Return the C designator of a row: its number or C identifier."""
    if isinstance(row, Number):
        value = number_value(row, "%s: row" % name)
        if num_rows is not None and value >= num_rows:
            raise KeymapError("%s: row %s is beyond %d rows" % (name, row, num_rows))
        return row
    if isinstance(row, str) and IDENT.match(row):
        return row  # a C symbol; the compiler checks it
    raise KeymapError("%s: row %r is not a row number or C identifier" % (name, row))


def matrix_rows(name, spec, num_rows, num_cols):
    """Return {row: [initializer, ...]} for one named matrix."""
    if not isinstance(spec, dict):
        raise KeymapError("%s: a matrix is a mapping of row to keys" % name)

    result = {}
    for row, keys in spec.items():
        row = row_designator(row, name, num_rows)
        if not keys:
            continue  # an empty row does nothing, as rows left out do
        if not isinstance(keys, list):
            raise KeymapError("%s: row %s is not a sequence of keys" % (name, row))
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


def row_order(row):
    """Sort key: numbered rows ascending, then rows named by C identifiers."""
    if isinstance(row, Number):
        return (0, int(row, 0), "")
    return (1, 0, row)


def generate(source, stem):
    """Return (c_text, h_text) for a parsed YAML keymap file."""
    if not isinstance(source, dict):
        raise KeymapError("the file must be a YAML mapping")
    includes = header_names(source)
    rows_c, num_rows = dimension(source, "rows")
    cols_c, num_cols = dimension(source, "cols")
    if not isinstance(source.get("maps"), dict) or not source["maps"]:
        raise KeymapError("the file needs a 'maps' mapping of one or more matrices")

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
        if not isinstance(name, str) or not IDENT.match(name):
            raise KeymapError("%r is not a C identifier" % name)
        decl = "const FLASH asdf_key_t %s[%s][%s]" % (name, rows_c, cols_c)
        h.append("extern %s;" % decl)
        c.append("%s = {" % decl)
        rows = matrix_rows(name, spec, num_rows, num_cols)
        for row in sorted(rows, key=row_order):
            c.append("  [%s] = { %s }," % (row, ", ".join(rows[row])))
        c.append("};")
        c.append("")

    h += ["", "#endif /* !defined(%s) */" % guard, ""]
    return "\n".join(c), "\n".join(h)


def write_outputs(out_dir, stem, c_text, h_text):
    """Write <stem>.c and <stem>.h, or neither."""
    out_dir.mkdir(parents=True, exist_ok=True)
    c_path = out_dir / (stem + ".c")
    c_path.write_text(c_text)
    try:
        (out_dir / (stem + ".h")).write_text(h_text)
    except OSError:
        c_path.unlink()
        raise


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
        write_outputs(out_dir, stem, c_text, h_text)
    except (KeymapError, yaml.YAMLError) as err:
        print("%s: %s" % (source_path, str(err).replace("\n", " ")), file=sys.stderr)
        return 1
    except OSError as err:
        print("%s: %s" % (err.filename or source_path, err.strerror), file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
