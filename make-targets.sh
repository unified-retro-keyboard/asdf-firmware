#!/bin/bash
#
# Build several targets in one command. Each target is built by its CMake
# workflow preset (configure, build, test); see CMakePresets.json and README.md.
# For a single target, the native commands are enough:
#
#   cmake --workflow --preset atmega328p
#
# Targets and their categories are listed in targets.csv.

# cmake --workflow reads CMakePresets.json from the current directory.
cd "$(dirname "${BASH_SOURCE[0]}")" || exit 1
TARGETS_FILE=targets.csv

usage() {
    cat <<EOF
Usage:
  $0 [-x] [-i] [-c] -t PRESET [-t PRESET] ...
  $0 [-x] [-i] [-c] -a [CATEGORY]
  $0 -l [CATEGORY | simavr]

Options:
  -t PRESET  Run a workflow preset (configure, build, test); may repeat
  -a         Run every target in targets.csv, or only those in CATEGORY
  -l         List targets, optionally filtered by CATEGORY or 'simavr'
  -x         Remove each target's build directory first
  -i         Install each target (hex files to dist/ and docs/source/)
  -c         Remove installed artifacts from dist/ and docs/source/
  -h         Display this help message

Categories: avr, test, arm_m0+, sim
Presets:    cmake --list-presets=workflow
EOF
}

# Print target names from targets.csv matching a filter: empty = all, a
# category name, or the keyword "simavr" for the simavr-tested targets.
list_targets() {
    local name category simavr
    while IFS=, read -r name category simavr; do
        name="${name//[[:space:]]/}"
        category="${category//[[:space:]]/}"
        simavr="${simavr//[[:space:]]/}"
        [[ -z $name || $name == \#* ]] && continue
        if [[ -z $1 || $category == "$1" || ($1 == simavr && $simavr == yes) ]]; then
            echo "$name"
        fi
    done < "$TARGETS_FILE"
}

clean_installed() {
    rm -rvf dist docs/source/toc_*.rst docs/source/*.hex docs/source/index.rst
}

PRESETS=()
CLEAN_FIRST=""
INSTALL=""
CLEAN_INSTALLED=""

while getopts "t:alxich" opt; do
    case $opt in
        t) PRESETS+=("$OPTARG") ;;
        a | l)
            # -a and -l take an optional filter argument
            filter=""
            next="${!OPTIND}"
            if [[ -n $next && $next != -* ]]; then
                filter="$next"
                ((OPTIND++))
            fi
            if [[ $opt == l ]]; then
                list_targets "$filter"
                exit 0
            fi
            selected=$(list_targets "$filter")
            if [[ -z $selected ]]; then
                echo "No targets in category \"$filter\"" >&2
                exit 2
            fi
            PRESETS+=($selected)
            ;;
        x) CLEAN_FIRST="yes" ;;
        i) INSTALL="yes" ;;
        c) CLEAN_INSTALLED="yes" ;;
        *) usage; exit 2 ;;
    esac
done

if [[ ${#PRESETS[@]} -eq 0 && -z $CLEAN_INSTALLED ]]; then
    usage
    exit 2
fi

[[ -n $CLEAN_INSTALLED ]] && clean_installed

for preset in "${PRESETS[@]}"; do
    [[ -n $CLEAN_FIRST ]] && rm -rf "build-$preset"
    cmake --workflow --preset "$preset" || exit 1
    if [[ -n $INSTALL ]]; then
        cmake --install "build-$preset" || exit 1
    fi
done
