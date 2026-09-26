"""Black-box tests of src/asdf_keymap_gen.py against specs/keymap-generator.md.

Each test writes a YAML file, runs the generator as a separate process, and
checks only what the spec defines: the exit status, standard error, and the
generated files. The compile tests build the generated C against stand-in
headers (stub_headers below).
"""

import json
import re
import shutil
import subprocess
import sys
from pathlib import Path

import pytest

GENERATOR = Path(__file__).resolve().parents[2] / "src" / "asdf_keymap_gen.py"


def q(text):
    """Return text as a double-quoted YAML scalar."""
    return json.dumps(text)


class Result:
    def __init__(self, proc, out_dir, basename):
        self.status = proc.returncode
        self.stderr = proc.stderr
        self.c_path = out_dir / (basename + ".c")
        self.h_path = out_dir / (basename + ".h")

    @property
    def c(self):
        return self.c_path.read_text()

    @property
    def h(self):
        return self.h_path.read_text()

    def outputs(self):
        return [p for p in (self.c_path, self.h_path) if p.exists()]


def run_args(*args):
    return subprocess.run(
        [sys.executable, str(GENERATOR), *map(str, args)], capture_output=True, text=True
    )


@pytest.fixture
def gen(tmp_path):
    """gen(yaml_text, basename="km") -> Result, run with OUTPUT_DIR tmp/out."""

    def run(yaml_text, basename="km", out_dir=None):
        src = tmp_path / (basename + ".yaml")
        src.write_text(yaml_text)
        out = out_dir if out_dir is not None else tmp_path / "out"
        return Result(run_args(src, out), out, basename)

    return run


def keymap(rows="4", cols="4", maps=None, extra=""):
    """YAML text for a keymap; maps is the text under 'maps:', indented."""
    if maps is None:
        maps = "  m:\n    0: [~]\n"
    return "%srows: %s\ncols: %s\nmaps:\n%s" % (extra, rows, cols, maps)


def one_row(*keys, rows="4", cols="4"):
    """YAML text for one matrix m whose row 0 holds the given raw YAML keys."""
    return keymap(rows, cols, "  m:\n    0: [%s]\n" % ", ".join(keys))


def row_line(c_text, row):
    """The definition line of one row of matrix m."""
    lines = [l for l in c_text.splitlines() if l.strip().startswith("[%s] = {" % row)]
    assert len(lines) == 1, c_text
    return lines[0]


def key0(result):
    """The initializer of key [0][0] of matrix m."""
    assert result.status == 0, result.stderr
    match = re.search(r"\[0\] = \{ \[0\] = (.*?)(, \[1\] =| \},$)", row_line(result.c, 0))
    assert match, result.c
    return match.group(1)


def assert_input_error(result, src_name="km.yaml"):
    assert result.status == 1, result.stderr
    lines = result.stderr.splitlines()
    assert len(lines) == 1, result.stderr
    assert lines[0].split(": ", 1)[0].endswith(src_name), result.stderr
    assert result.outputs() == []


# Invocation


def test_success_writes_both_files_into_new_nested_dir(gen, tmp_path):
    result = gen(keymap(), out_dir=tmp_path / "a" / "b" / "out")
    assert result.status == 0, result.stderr
    assert result.stderr == ""
    assert result.c_path.is_file() and result.h_path.is_file()


def test_output_names_come_from_the_input_basename(gen):
    result = gen(keymap(), basename="asdf_keymap_sol_maps")
    assert result.status == 0, result.stderr
    assert result.c_path.name == "asdf_keymap_sol_maps.c"
    assert result.h_path.name == "asdf_keymap_sol_maps.h"


@pytest.mark.parametrize("args", [[], ["a.yaml"], ["a.yaml", "out", "extra"]])
def test_wrong_argument_count(args):
    proc = run_args(*args)
    assert proc.returncode == 2
    assert proc.stderr.startswith("usage: ")
    assert "INPUT.yaml OUTPUT_DIR" in proc.stderr


def test_input_error_is_one_line_naming_the_input(gen):
    assert_input_error(gen("rows: 4\ncols: 4\n"))


def test_unreadable_input(tmp_path):
    missing = tmp_path / "missing.yaml"
    proc = run_args(missing, tmp_path / "out")
    assert proc.returncode == 1
    lines = proc.stderr.splitlines()
    assert len(lines) == 1 and lines[0].startswith(str(missing) + ": "), proc.stderr
    assert not (tmp_path / "out").exists()


def test_output_dir_that_cannot_be_created(gen, tmp_path):
    blocker = tmp_path / "blocker"
    blocker.write_text("")
    result = gen(keymap(), out_dir=blocker / "out")
    assert result.status == 1
    lines = result.stderr.splitlines()
    assert len(lines) == 1 and lines[0].startswith(str(blocker)), result.stderr


def test_output_that_cannot_be_written_leaves_no_files(gen, tmp_path):
    out = tmp_path / "out"
    (out / "km.h").mkdir(parents=True)  # a directory where the header goes
    result = gen(keymap(), out_dir=out)
    assert result.status == 1
    lines = result.stderr.splitlines()
    assert len(lines) == 1 and str(out / "km.h") in lines[0], result.stderr
    assert not result.c_path.exists()


# Input file


def test_include_string(gen):
    h = gen(keymap(extra="include: a.h\n")).h
    assert '#include "asdf_arch.h"\n#include "asdf_actions.h"\n#include "a.h"' in h


def test_include_sequence_in_order(gen):
    h = gen(keymap(extra="include: [b.h, a.h]\n")).h
    assert '#include "asdf_actions.h"\n#include "b.h"\n#include "a.h"' in h


def test_include_omitted(gen):
    h = gen(keymap()).h
    assert re.findall(r'#include "(.*)"', h) == ["asdf_arch.h", "asdf_actions.h"]


@pytest.mark.parametrize("include", ["{a: b}", "7", "[a.h, 7]", "[a.h, [b.h]]", "~"])
def test_include_not_string_or_sequence_of_strings(gen, include):
    assert_input_error(gen(keymap(extra="include: %s\n" % include)))


def test_other_top_level_keys_ignored(gen):
    assert gen(keymap(extra="comment: hello\nversion: 3\n")).status == 0


@pytest.mark.parametrize("rows,cols", [("4", "8"), ("0x4", "0x08"), ("NROWS", "NCOLS")])
def test_dimensions_written_as_given(gen, rows, cols):
    result = gen(keymap(rows, cols))
    assert result.status == 0, result.stderr
    assert "extern const FLASH asdf_key_t m[%s][%s];" % (rows, cols) in result.h
    assert "const FLASH asdf_key_t m[%s][%s] = {" % (rows, cols) in result.c


@pytest.mark.parametrize(
    "text",
    ["cols: 4\nmaps:\n  m:\n    0: [~]\n",
     "rows: 4\nmaps:\n  m:\n    0: [~]\n",
     "rows: 4\ncols: 4\n"],
    ids=["no rows", "no cols", "no maps"],
)
def test_missing_required_key(gen, text):
    assert_input_error(gen(text))


@pytest.mark.parametrize("value", ["1.5", "two words", "[4]", "~", "true", "'4'"])
@pytest.mark.parametrize("which", ["rows", "cols"])
def test_dimension_not_number_or_identifier(gen, which, value):
    dims = {"rows": "4", "cols": "4"}
    dims[which] = value
    assert_input_error(gen(keymap(dims["rows"], dims["cols"])))


@pytest.mark.parametrize("text", ["", "- a\n- b\n", "just text\n", "42\n"])
def test_input_not_a_mapping(gen, text):
    assert_input_error(gen(text))


@pytest.mark.parametrize("maps", [" ~\n", " {}\n", " [a, b]\n", " text\n"])
def test_maps_not_a_mapping_or_empty(gen, maps):
    assert_input_error(gen("rows: 4\ncols: 4\nmaps:%s" % maps))


@pytest.mark.parametrize("name", ["1abc", "a-b", q("a b"), "7"])
def test_matrix_name_not_identifier(gen, name):
    assert_input_error(gen(keymap(maps="  %s:\n    0: [~]\n" % name)))


def test_matrices_generated_in_input_order(gen):
    result = gen(keymap(maps="  zeta:\n    0: [~]\n  alpha:\n    0: [~]\n  mid:\n    0: [~]\n"))
    assert result.status == 0, result.stderr
    for text in (result.h, result.c):
        names = re.findall(r"asdf_key_t (\w+)\[", text)
        assert names == ["zeta", "alpha", "mid"]


# Matrices


@pytest.mark.parametrize("matrix", [" [[a], [b]]\n", " text\n", " 7\n"])
def test_matrix_not_a_mapping(gen, matrix):
    assert_input_error(gen(keymap(maps="  m:%s" % matrix)))


def test_rows_by_number_and_identifier(gen):
    result = gen(keymap("NROWS", maps="  m:\n    DIP_ROW: [~]\n    3: [~]\n"))
    assert result.status == 0, result.stderr
    row_line(result.c, "DIP_ROW")
    row_line(result.c, 3)


@pytest.mark.parametrize("row", ["-1", "1.5", q("two words"), "true", q("3x")])
def test_row_not_nonnegative_integer_or_identifier(gen, row):
    assert_input_error(gen(keymap(maps="  m:\n    %s: [~]\n" % row)))


def test_row_number_at_numeric_rows(gen):
    assert_input_error(gen(keymap("4", maps="  m:\n    4: [~]\n")))


def test_row_number_below_numeric_rows(gen):
    assert gen(keymap("4", maps="  m:\n    3: [~]\n")).status == 0


def test_row_number_unchecked_when_rows_is_identifier(gen):
    assert gen(keymap("NROWS", maps="  m:\n    40: [~]\n")).status == 0


@pytest.mark.parametrize("keys", ["a", "{a: b}", "7"])
def test_row_keys_not_a_sequence(gen, keys):
    assert_input_error(gen(keymap(maps="  m:\n    0: %s\n" % keys)))


def test_null_and_empty_rows_and_left_out_rows_get_no_line(gen):
    result = gen(keymap("8", maps="  m:\n    0: []\n    1: ~\n    2: ['a']\n"))
    assert result.status == 0, result.stderr
    rows = re.findall(r"^\s*\[(\w+)\] = \{", result.c, re.M)
    assert rows == ["2"]


def test_row_longer_than_numeric_cols(gen):
    assert_input_error(gen(one_row("~", "~", "~", "~", "~", cols="4")))


def test_row_as_long_as_numeric_cols(gen):
    assert gen(one_row("~", "~", "~", "~", cols="4")).status == 0


def test_row_length_unchecked_when_cols_is_identifier(gen):
    assert gen(one_row(*["~"] * 12, cols="NCOLS")).status == 0


def test_row_line_format(gen):
    result = gen(one_row("'a'", "~", "KEY_SHIFT"))
    assert row_line(result.c, 0) == (
        "  [0] = { [0] = KEY_SEND('a'), [1] = KEY_NOTHING(0), [2] = KEY_SHIFT(0) },"
    )


def test_row_order_numbers_ascending_then_symbols_by_name(gen):
    maps = "  m:\n    ZED: [~]\n    10: [~]\n    ALPHA: [~]\n    2: [~]\n    0: [~]\n"
    result = gen(keymap("NROWS", maps=maps))
    assert result.status == 0, result.stderr
    rows = re.findall(r"^\s*\[(\w+)\] = \{", result.c, re.M)
    assert rows == ["0", "2", "10", "ALPHA", "ZED"]


# Keys


@pytest.mark.parametrize(
    "key,initializer",
    [
        ("~", "KEY_NOTHING(0)"),
        ("140", "KEY_SEND(140)"),
        ("7", "KEY_SEND(7)"),
        ("0", "KEY_SEND(0)"),
        ("255", "KEY_SEND(255)"),
        ("0x8c", "KEY_SEND(0x8c)"),
        ("0xFF", "KEY_SEND(0xFF)"),
        ("0x00", "KEY_SEND(0x00)"),
        ("'a'", "KEY_SEND('a')"),
        ("'7'", "KEY_SEND('7')"),
        ("' '", "KEY_SEND(' ')"),
        ("'~'", "KEY_SEND('~')"),
        (q("\t"), r"KEY_SEND('\t')"),
        (q("\n"), r"KEY_SEND('\n')"),
        (q("\r"), r"KEY_SEND('\r')"),
        (q("\\"), r"KEY_SEND('\\')"),
        (q("'"), r"KEY_SEND('\'')"),
        ("$ASCII_ESC", "KEY_SEND(ASCII_ESC)"),
        (q("  $ASCII_ESC  "), "KEY_SEND(ASCII_ESC)"),
        ("KEY_SHIFT", "KEY_SHIFT(0)"),
        (q("KEY_SHIFT()"), "KEY_SHIFT(0)"),
        (q("  KEY_SHIFT  "), "KEY_SHIFT(0)"),
        ("[ACTION_A, 1, ACTION_B, PARAM]", "{ ACTION_A, 1, ACTION_B, PARAM }"),
    ],
)
def test_key_form(gen, key, initializer):
    assert key0(gen(one_row(key))) == initializer


@pytest.mark.parametrize("key", ["true", "false", "yes", "1.5", "-0.5"])
def test_boolean_or_float_key_asks_for_quotes(gen, key):
    result = gen(one_row(key))
    assert_input_error(result)
    assert "quot" in result.stderr.lower()


@pytest.mark.parametrize("key", ["256", "0x100", "1000", "007", "010", "00"])
def test_number_out_of_range_or_leading_zero(gen, key):
    assert_input_error(gen(one_row(key)))


@pytest.mark.parametrize("char", ["\x01", "\x1b", "\x7f", "\u00e9"])
def test_one_character_string_not_printable(gen, char):
    assert_input_error(gen(one_row(q(char))))


@pytest.mark.parametrize(
    "key", ["[A, B, C]", "[A, B, C, D, E]", "[]", "[A, ~, B, 0]", "[~, 0, B, 0]"]
)
def test_raw_key_needs_four_non_null_fields(gen, key):
    assert_input_error(gen(one_row(key)))


@pytest.mark.parametrize(
    "key", ["{a: b}", q("two words"), q("$1BAD"), q("$ "), q("KEY_X(1"), q("1KEY"), q("")]
)
def test_anything_else_is_an_error(gen, key):
    assert_input_error(gen(one_row(key)))


# Key parameters


@pytest.mark.parametrize(
    "key,initializer",
    [
        ("KEY_MAPSEL(BIT_2)", "KEY_MAPSEL(BIT_2)"),
        (q("KEY_X(  SYM  )"), "KEY_X(SYM)"),
        (q("KEY_X('x')"), "KEY_X('x')"),
        (q('KEY_X("x")'), "KEY_X('x')"),
        (q("KEY_X(',')"), "KEY_X(',')"),
        (q("KEY_X(' ')"), "KEY_X(' ')"),
        (q(r"KEY_X('\n')"), r"KEY_X('\n')"),
        (q(r"KEY_X('\t')"), r"KEY_X('\t')"),
        (q(r"KEY_X('\r')"), r"KEY_X('\r')"),
        (q(r"KEY_X('\q')"), "KEY_X('q')"),
        (q(r"KEY_X('\\')"), r"KEY_X('\\')"),
        (q(r"KEY_X('\'')"), r"KEY_X('\'')"),
        ("KEY_X(0)", "KEY_X(0)"),
        ("KEY_X(7)", "KEY_X(7)"),
        ("KEY_X(255)", "KEY_X(255)"),
        ("KEY_X(0xff)", "KEY_X(0xff)"),
        ("KEY_X(0x00)", "KEY_X(0x00)"),
        (q("KEY_X( 3 )"), "KEY_X(3)"),
    ],
)
def test_key_parameter(gen, key, initializer):
    assert key0(gen(one_row(key))) == initializer


@pytest.mark.parametrize(
    "key",
    [
        "KEY_X(256)",
        "KEY_X(0x100)",
        "KEY_X(010)",
        "KEY_X(00)",
        q("KEY_X('ab')"),
        q("KEY_X('')"),
        q("KEY_X(a b)"),
        q("KEY_X(1.5)"),
        q("KEY_X(-1)"),
        q("KEY_X(FOO(1))"),
    ],
)
def test_key_parameter_error(gen, key):
    assert_input_error(gen(one_row(key)))


# Generated header


def test_header_layout(gen):
    result = gen(keymap("R", "C", "  m1:\n    0: [~]\n  m2:\n    0: [~]\n",
                        extra="include: [x.h]\n"), basename="asdf_keymap_sol_maps")
    h = result.h
    doc = h.index("@file asdf_keymap_sol_maps.h")
    assert "asdf_keymap_sol_maps.yaml" in h[doc:h.index("*/")]
    assert re.search(r"do not edit", h[doc:h.index("*/")], re.I)
    guard = re.search(
        r"^#(?:ifndef (\w+)|if !defined\((\w+)\))\n#define (\w+)$", h, re.M)
    assert guard, h
    name = guard.group(1) or guard.group(2)
    assert name == guard.group(3) == "ASDF_KEYMAP_SOL_MAPS_H"
    positions = [
        h.index("*/"),
        guard.start(),
        h.index('#include "asdf_arch.h"'),
        h.index('#include "asdf_actions.h"'),
        h.index('#include "x.h"'),
        h.index("extern const FLASH asdf_key_t m1[R][C];"),
        h.index("extern const FLASH asdf_key_t m2[R][C];"),
        h.rindex("#endif"),
    ]
    assert positions == sorted(positions)
    assert h.rstrip().splitlines()[-1].startswith("#endif")


@pytest.mark.parametrize(
    "basename,guard", [("km", "KM_H"), ("my-map.v2", "MY_MAP_V2_H"), ("a b", "A_B_H")]
)
def test_include_guard_from_basename(gen, basename, guard):
    h = gen(keymap(), basename=basename).h
    assert re.search(r"^#(?:ifndef %s|if !defined\(%s\))$" % (guard, guard), h, re.M), h
    assert "#define %s" % guard in h


# Generated source


def test_source_layout(gen):
    c = gen(keymap("R", "C", "  m1:\n    0: [~]\n  m2:\n    0: [~]\n"),
            basename="my-map.v2").c
    doc = c.index("@file my-map.v2.c")
    assert "my-map.v2.yaml" in c[doc:c.index("*/")]
    lint = [l for l in c.splitlines() if l.startswith("//lint")]
    assert any("785" in l for l in lint), c
    assert any("751" in l and "_fit" in l for l in lint), c
    positions = [
        c.index("*/"),
        c.index('#include "my-map.v2.h"'),
        c.index("//lint"),
        c.index("my_map_v2_rows_fit"),
        c.index("my_map_v2_cols_fit"),
        c.index("const FLASH asdf_key_t m1[R][C] = {"),
        c.index("const FLASH asdf_key_t m2[R][C] = {"),
    ]
    assert positions == sorted(positions)
    assert re.search(r"typedef char my_map_v2_rows_fit\[", c)
    assert re.search(r"typedef char my_map_v2_cols_fit\[", c)


# Compiling the generated code

STUB_ARCH = "#define FLASH\n"
STUB_ACTIONS = """\
#include <stdint.h>
#define ASDF_MAX_ROWS 16u
#define ASDF_MAX_COLS 8u
typedef struct { uint8_t press, press_param, release, release_param; } asdf_key_t;
#define KEY_NOTHING(p) { 0, (uint8_t) (p), 0, 0 }
#define KEY_SEND(p) { 1, (uint8_t) (p), 0, 0 }
#define KEY_SHIFT(p) { 2, (uint8_t) (p), 0, 0 }
"""


@pytest.fixture
def compile_c(tmp_path):
    cc = shutil.which("cc") or shutil.which("gcc")
    if cc is None:
        pytest.skip("no C compiler")
    stubs = tmp_path / "stubs"
    stubs.mkdir()
    (stubs / "asdf_arch.h").write_text(STUB_ARCH)
    (stubs / "asdf_actions.h").write_text(STUB_ACTIONS)

    def run(result):
        assert result.status == 0, result.stderr
        return subprocess.run(
            [cc, "-std=c99", "-Wall", "-Werror", "-c", "-o", str(tmp_path / "km.o"),
             "-I", str(stubs), "-I", str(result.c_path.parent), str(result.c_path)],
            capture_output=True, text=True,
        )

    return run


def test_generated_code_compiles(gen, compile_c, tmp_path):
    maps = ("  m:\n    0: ['a', 0x41, 65, ~, KEY_SHIFT]\n    3: [$SYM]\n"
            "  n:\n    ROWSYM: [KEY_SEND(',')]\n")
    text = "#define SYM 1\n#define ROWSYM 2\n"
    result = gen(keymap("4", "8", maps.replace("KEY_SEND(',')", q("KEY_SEND(',')")),
                        extra="include: syms.h\n"))
    (tmp_path / "stubs" / "syms.h").write_text(text)
    proc = compile_c(result)
    assert proc.returncode == 0, proc.stderr


@pytest.mark.parametrize("rows,cols", [("16", "8"), ("0x10", "0x8")])
def test_size_checks_pass_at_the_limit(gen, compile_c, rows, cols):
    proc = compile_c(gen(keymap(rows, cols)))
    assert proc.returncode == 0, proc.stderr


@pytest.mark.parametrize("rows,cols", [("17", "8"), ("16", "9")])
def test_size_checks_fail_beyond_the_limit(gen, compile_c, rows, cols):
    proc = compile_c(gen(keymap(rows, cols)))
    assert proc.returncode != 0
    assert "_fit" in proc.stderr
