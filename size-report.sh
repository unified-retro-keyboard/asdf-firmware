#!/bin/bash
#
# Report flash and RAM use of built firmware targets.
#
# usage: size-report.sh [-b BASELINE_CSV] [TARGET...]
#
# With no TARGET, reports every avr and arm_m0+ target in targets.csv whose ELF
# has been built (build-<target>/src/asdf-v<version>-<target>.elf); targets
# that have not been built are skipped. Output is CSV:
#
#   target,text,data,bss,reserved,flash,ram
#
#   text     : code and constants in flash
#   data     : initialized data (stored in flash, copied to RAM at startup)
#   bss      : zero-initialized RAM
#   reserved : RAM reserved by the linker script for heap and stack (ARM only)
#   flash    : text + data
#   ram      : data + bss (static use, excluding reserved)
#
# With -b, each row is followed by its change from the matching row of the
# baseline file (a previous report). The report never fails on growth.

ROOT="$(dirname "${BASH_SOURCE[0]}")"
source "$ROOT/version.sh"
VERSION="$(asdf_version "$ROOT/CMakeLists.txt")"
BASELINE=""

usage() {
    echo "usage: $0 [-b BASELINE_CSV] [TARGET...]" >&2
    exit 2
}

while getopts "b:h" opt; do
    case $opt in
        b) BASELINE="$OPTARG" ;;
        *) usage ;;
    esac
done
shift $((OPTIND - 1))

# name,category for each firmware target in targets.csv
firmware_targets() {
    local name category rest
    while IFS=, read -r name category rest; do
        name="${name//[[:space:]]/}"
        category="${category//[[:space:]]/}"
        [[ -z $name || $name == \#* ]] && continue
        [[ $category == avr || $category == arm_m0+ ]] && echo "$name,$category"
    done < "$ROOT/targets.csv"
}

# Print "text,data,bss,reserved" for an ELF, using the size tool for its family.
section_sizes() {
    local category=$1 elf=$2
    if [[ $category == avr ]]; then
        avr-size -A "$elf" | awk '
            $1 == ".text"   { t = $2 }
            $1 == ".data"   { d = $2 }
            $1 == ".bss"    { b += $2 }
            $1 == ".noinit" { b += $2 }
            END { printf "%d,%d,%d,0\n", t, d, b }'
    else
        arm-none-eabi-size -A "$elf" | awk '
            $1 == ".text" || $1 == ".copy.table" { t += $2 }
            $1 == ".relocate" { d = $2 }
            $1 == ".bss"      { b = $2 }
            $1 == ".heap" || $1 == ".stack" { r += $2 }
            END { printf "%d,%d,%d,%d\n", t, d, b, r }'
    fi
}

report() {
    local target category elf sizes
    echo "target,text,data,bss,reserved,flash,ram"
    while IFS=, read -r target category; do
        if [[ $# -gt 0 ]] && ! [[ " $* " == *" $target "* ]]; then
            continue
        fi
        elf="$ROOT/build-$target/src/asdf-v$VERSION-$target.elf"
        [[ -f $elf ]] || continue
        sizes="$(section_sizes "$category" "$elf")"
        echo "$target,$sizes" | awk -F, -v OFS=, '{ print $0, $2 + $3, $3 + $4 }'
    done < <(firmware_targets)
}

if [[ -z $BASELINE ]]; then
    report "$@"
    exit 0
fi

[[ -f $BASELINE ]] || { echo "baseline not found: $BASELINE" >&2; exit 2; }

report "$@" | awk -F, -v OFS=, '
    NR == FNR { if (FNR > 1) base[$1] = $0; next }
    FNR == 1  { print; next }
    {
        print
        if (!($1 in base)) { print "  (no baseline)"; next }
        split(base[$1], b, ",")
        line = "  delta"
        for (i = 2; i <= NF; i++) line = line OFS sprintf("%+d", $i - b[i])
        print line
    }' "$BASELINE" -
