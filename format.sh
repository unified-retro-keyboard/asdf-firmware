#!/bin/bash
#
# Format the first-party C sources with clang-format, in the style set by
# .clang-format. The clang-format version is pinned in pyproject.toml (the
# "format" group) and run through uv, so every machine formats alike.
# Vendored code (src/third_party, test/unity) is left alone.

cd "$(dirname "${BASH_SOURCE[0]}")" || exit 1

usage() {
    cat <<EOF
Usage:
  $0 [-c]

Options:
  -c  Check only: list what would change and fail, without editing files
  -h  Display this help message
EOF
}

mode=(-i)
while getopts "ch" opt; do
    case $opt in
        c) mode=(--dry-run --Werror) ;;
        h) usage; exit 0 ;;
        *) usage; exit 1 ;;
    esac
done

mapfile -t files < <(git ls-files '*.c' '*.h' ':!:src/third_party/' ':!:test/unity/')
uv run --only-group format clang-format "${mode[@]}" "${files[@]}"
