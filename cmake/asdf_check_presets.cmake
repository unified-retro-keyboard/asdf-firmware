##########################################################################
# Check that CMakePresets.json has a workflow preset for every target in
# targets.csv, whose configure preset sets ARCH to the target name and builds
# in build-<target> (the layout the simavr tests and size-report.sh expect).
#
# usage: cmake -P cmake/asdf_check_presets.cmake
##########################################################################

cmake_minimum_required(VERSION 3.25)

include(${CMAKE_CURRENT_LIST_DIR}/asdf_targets.cmake)
asdf_read_targets(T)

file(READ ${CMAKE_CURRENT_LIST_DIR}/../CMakePresets.json presets)

# name -> ARCH of each configure preset, and the names of the workflow presets
string(JSON n LENGTH "${presets}" configurePresets)
math(EXPR last "${n} - 1")
foreach(i RANGE ${last})
  string(JSON name GET "${presets}" configurePresets ${i} name)
  string(JSON arch ERROR_VARIABLE err GET "${presets}"
    configurePresets ${i} cacheVariables ARCH)
  if(NOT err)
    set(arch_${name} ${arch})
  endif()
endforeach()
string(JSON binary_dir GET "${presets}" configurePresets 0 binaryDir)

set(workflows "")
string(JSON n LENGTH "${presets}" workflowPresets)
math(EXPR last "${n} - 1")
foreach(i RANGE ${last})
  string(JSON name GET "${presets}" workflowPresets ${i} name)
  list(APPEND workflows ${name})
endforeach()

set(errors "")
if(NOT binary_dir STREQUAL "\${sourceDir}/build-\${presetName}")
  string(APPEND errors "  base binaryDir is not \${sourceDir}/build-\${presetName}\n")
endif()
foreach(target IN LISTS T_NAMES)
  if(NOT arch_${target} STREQUAL target)
    string(APPEND errors "  ${target}: no configure preset with ARCH=${target}\n")
  endif()
  if(NOT target IN_LIST workflows)
    string(APPEND errors "  ${target}: no workflow preset\n")
  endif()
endforeach()

if(errors)
  message(FATAL_ERROR "CMakePresets.json does not match targets.csv:\n${errors}")
endif()
