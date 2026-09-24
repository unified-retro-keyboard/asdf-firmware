#!/bin/bash
#
# Report flash and RAM use of built firmware targets.
#
# usage: size-report.sh [-b BASELINE_CSV] [-c CEILINGS_CSV] [TARGET...]
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
# baseline file (a previous report). The baseline comparison never fails.
#
# With -c, each target's flash and ram are checked against its budget in the
# ceilings file (target,flash,ram; lines starting with # are comments). The
# script exits 1 if any target is over its budget. A built target with no
# budget is reported but does not fail.

ROOT="$(dirname "${BASH_SOURCE[0]}")"
source "$ROOT/version.sh"
VERSION="$(asdf_version "$ROOT/CMakeLists.txt")"
BASELINE=""
CEILINGS=""

usage() {
    echo "usage: $0 [-b BASELINE_CSV] [-c CEILINGS_CSV] [TARGET...]" >&2
    exit 2
}

while getopts "b:c:h" opt; do
    case $opt in
        b) BASELINE="$OPTARG" ;;
        c) CEILINGS="$OPTARG" ;;
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

REPORT="$(report "$@")"

if [[ -z $BASELINE ]]; then
    echo "$REPORT"
else
    [[ -f $BASELINE ]] || { echo "baseline not found: $BASELINE" >&2; exit 2; }
    echo "$REPORT" | awk -F, -v OFS=, '
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
fi

[[ -z $CEILINGS ]] && exit 0
[[ -f $CEILINGS ]] || { echo "ceilings not found: $CEILINGS" >&2; exit 2; }

# report columns: 1 target, 6 flash, 7 ram; ceilings columns: target,flash,ram
echo "$REPORT" | awk -F, '
    NR == FNR {
        if ($0 ~ /^#/ || $1 == "target" || NF < 3) next
        flash[$1] = $2; ram[$1] = $3; next
    }
    FNR == 1 { next }
    {
        if (!($1 in flash)) { print $1 ": no size budget" > "/dev/stderr"; next }
        if ($6 > flash[$1]) { print $1 ": flash " $6 " over budget " flash[$1] > "/dev/stderr"; over = 1 }
        if ($7 > ram[$1])   { print $1 ": ram " $7 " over budget " ram[$1] > "/dev/stderr"; over = 1 }
    }
    END { exit over }' "$CEILINGS" -
