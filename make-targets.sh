#!/bin/bash

source "$(dirname "${BASH_SOURCE[0]}")/version.sh"

GENERATOR="Unix Makefiles"
NUM_VALID_TARGETS=0
BUILD_TYPE=RELEASE
INSTALL_DIR="dist"
MAKE_TARGETS="all"
DOC_DIR=docs
LINKS_DIR="$DOC_DIR/source"

# Each target belongs to a category. The category determines how the target is
# built (see build_arch) and lets -a select a whole category at once.
#   avr     - 8-bit AVR firmware (avr-gcc); compiled/installed via deploy (-i)
#   test    - host unit tests (native gcc + ctest)
#   arm_m0+ - Arm Cortex-M0+ firmware (arm-none-eabi-gcc); build-verified
#   sim     - simavr integration tests (needs the AVR ELFs + simavr)
add_valid_target() {
    VALID_TARGETS[$NUM_VALID_TARGETS]=$1
    TARGET_CATEGORY[$NUM_VALID_TARGETS]=$2
    ((NUM_VALID_TARGETS++))
}

#                name                 category
add_valid_target test                test
add_valid_target atmega328p          avr
add_valid_target atmega168p          avr
add_valid_target atmega88p           avr
add_valid_target atmega2560          avr
add_valid_target atmega1280          avr
add_valid_target atmega640           avr
add_valid_target pic32cm_pl10_q64    arm_m0+
add_valid_target pic32cm_pl10_dip28  arm_m0+
add_valid_target simavr_test         sim


check_valid_target() {
    result="false"

    for (( i = 0; i < NUM_VALID_TARGETS; i++ ))
    do
        if [[ ${VALID_TARGETS[$i]} == $1 ]]
        then
            result=$i
        fi
    done
    echo $result
}


# Return success if $1 is a category used by at least one registered target.
is_known_category() {
    local i
    for (( i = 0; i < NUM_VALID_TARGETS; i++ ))
    do
        [[ ${TARGET_CATEGORY[$i]} == "$1" ]] && return 0
    done
    return 1
}


do_pipenv_clean() {
    echo removing old python virtual environment...
    pipenv --rm
}


do_pipenv_install() {
    echo installing python virtual environment...
    pipenv install
}

clean_arch() {
    target_arch="$1"
    echo "Removing pre-existing build directory: $target_arch..."
    rm -rf "build-$target_arch"
    echo "Removing any build artifacts"
    rm -f $INSTALL_DIR/*"$1"*
    echo "Removing any download links"
    rm -f $LINKS_DIR/*$1*
}

preflight_simavr_test() {
    local missing=0
    local ver
    ver=$(asdf_version)
    for t in atmega328p atmega168p atmega640 atmega1280 atmega2560; do
        local elf="build-$t/src/asdf-v${ver}-$t.elf"
        if [[ ! -f $elf ]]; then
            echo "ERROR: missing $elf"
            missing=1
        fi
    done
    if [[ $missing -ne 0 ]]; then
        echo
        echo "Run: bash make-targets.sh -a avr"
        echo "to build the AVR firmware before running integration tests."
        return 1
    fi

    if ! command -v simavr >/dev/null 2>&1; then
        echo "ERROR: simavr not found on PATH."
        echo "Install with: sudo apt-get install simavr libsimavr-dev pkg-config"
        echo "          or: sudo port install simavr  (MacPorts)"
        echo "          or: brew install simavr  (Homebrew)"
        return 1
    fi
    local header_found=
    for prefix in /usr/include /usr/local/include /opt/local/include /opt/homebrew/include; do
        if [[ -f "$prefix/simavr/sim_avr.h" ]]; then
            header_found="$prefix/simavr/sim_avr.h"
            break
        fi
    done
    if [[ -z $header_found ]]; then
        echo "ERROR: libsimavr headers not found."
        echo "Install with: sudo apt-get install libsimavr-dev"
        echo "          or: sudo port install simavr"
        echo "          or: brew install simavr"
        echo "Or build from source and pass -DSIMAVR_ROOT=<prefix> via cmake."
        return 1
    fi
    return 0
}

# Build a target. Behavior is derived from the target's category:
#   avr     -> configure only (firmware is compiled and installed by deploy, -i)
#   test    -> make + ctest (host unit tests)
#   sim     -> preflight + make + ctest (simavr integration tests)
#   arm_m0+ -> make (build-verified firmware; not installed/deployed)
build_arch() {
    local target_arch="$1"
    local category="$2"

    if [[ $category == sim ]]; then
        preflight_simavr_test || exit 1
    fi

    cmake -S . -B "build-$target_arch" -G "$GENERATOR" \
        -DCMAKE_INSTALL_PREFIX="." -DARCH="$target_arch" \
        -DCMAKE_BUILD_TYPE="$BUILD_TYPE" || exit 1

    case "$category" in
        test | sim)
            (cd "build-$target_arch" && make && ctest --output-on-failure) || exit 1
            ;;
        "arm_m0+")
            (cd "build-$target_arch" && make) || exit 1
            ;;
    esac

}

deploy_arch() {
    local target_arch="$1"

    if [[ -d "build-$target_arch" ]]
    then
        (cd "build-$target_arch" \
        && make install) || exit 1
    fi
}

clean_all() {
    echo "- Removing install dir \"$INSTALL_DIR\"..."
    rm -rvf "$INSTALL_DIR"
    echo "- Removing download links from doc dir..."
    rm -vf $DOC_DIR/source/toc_*
    echo "- Removing hex files from doc dir..."
    rm -vf $DOC_DIR/source/*.hex
    echo "- Removing versioned index file from doc dir"
    rm -vf "$DOC_DIR/source/index.rst"
}

syntax() {
    echo "Usage:"
    echo "  $0 -t target [-t target] ..."
    echo "  $0 -a [category]"
    echo "  $0 -h"
    echo ""
    echo "Options:"
    echo "  -h.  Display this help message"
    echo "  -x   Before creating a build directgory or virtual env, remove"
    echo "       any pre-existing version"
    echo "  -t   add an architecture target"
    echo "  -a   Build all targets; with a category argument, only that category"
    echo "  -i   Build each specified target and install to dist directory"
    echo "  -p   Install pipenv virtual environment for python scripts"
    echo "  -c   Clean all artifacts"
    echo "  -s   Copy dist files to sphinx directory"
    echo "Valid targets: ${VALID_TARGETS[*]}"
    echo "Categories:    avr, test, arm_m0+, sim"
}

parse() {
    local SYNTAX=""
    local ALL_CATEGORY=""
    local i
    local valid_index
    local next

    NUM_CMAKE_TARGETS=0
    CLEAN_BEFORE_BUILD=""
    DO_PIPENV_INSTALL=""
    DEPLOY=""
    CLEAN_ALL=""
    COPY_DIST_TO_DOCS=""

    while getopts "t:ahipxcs" optname
    do
        case "$optname" in
            h)
                SYNTAX="yes"
                ;;
            a)
                # -a takes an OPTIONAL category argument. With a known category,
                # build that category; a bare -a builds every target.
                next="${!OPTIND}"
                if [[ -n $next && $next != -* ]]; then
                    if is_known_category "$next"; then
                        ALL_CATEGORY="$next"
                        ((OPTIND++))
                    else
                        echo "Unknown category \"$next\""
                        SYNTAX="yes"
                    fi
                else
                    ALL_CATEGORY="__all__"
                fi
                ;;
            t)
                # Test that target is valid
                valid_index=$(check_valid_target $OPTARG)
                if [[ $valid_index != "false" ]]
                then
                    CMAKE_TARGETS[$NUM_CMAKE_TARGETS]=$valid_index
                    ((NUM_CMAKE_TARGETS++))
                else
                    echo Unknown target \"$OPTARG\"
                    SYNTAX="yes"
                fi
                ;;
            p)
                DO_PIPENV_INSTALL="yes"
                ;;
            x)
                CLEAN_BEFORE_BUILD="yes"
                ;;
            i)  DEPLOY="yes"
                ;;
            s)  COPY_DIST_TO_DOCS="yes"
                ;;
            c)  CLEAN_ALL="yes"

    esac
    done

    if [[ "$SYNTAX" == "yes" ]]; then
        syntax && die
    fi

    if [[ -n "$ALL_CATEGORY" ]]
    then
        for (( i=0; i<NUM_VALID_TARGETS; i++ ))
        do
            if [[ "$ALL_CATEGORY" == "__all__" || ${TARGET_CATEGORY[$i]} == "$ALL_CATEGORY" ]]; then
                CMAKE_TARGETS[$NUM_CMAKE_TARGETS]=$i
                ((NUM_CMAKE_TARGETS++))
            fi
        done
    fi

}

die() {
    builtin echo $@
    exit 1
}

main() {
     local TARGET
     local i
     parse $@

     if [[ "$DO_PIPENV_INSTALL" == "yes" ]]
     then
         if [[ "$CLEAN_BEFORE_BUILD" == "yes" ]]
         then
             do_pipenv_clean
         fi

         do_pipenv_install
     fi

     if [[ "$CLEAN_ALL" == "yes" ]]
     then
         clean_all
     fi

     echo Valid Targets: "${VALID_TARGETS[*]}"

     for (( i=0; $i<$NUM_CMAKE_TARGETS; i++ ))
     do
         TARGET=${CMAKE_TARGETS[$i]}
         if [[ "$CLEAN_BEFORE_BUILD" == "yes" ]]
         then
             clean_arch ${VALID_TARGETS[$TARGET]}
         fi
         build_arch ${VALID_TARGETS[$TARGET]} ${TARGET_CATEGORY[$TARGET]}
         if [[ "$DEPLOY" == "yes" ]]
         then
             deploy_arch ${VALID_TARGETS[$TARGET]}
         fi
     done

     if [[ $COPY_DIST_TO_DOCS == "yes" && -d "$INSTALL_DIR" ]]
     then
         cp -av "$INSTALL_DIR"/* docs/source
     fi

}


main $@

exit 0
