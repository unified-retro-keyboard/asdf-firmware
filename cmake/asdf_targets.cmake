##########################################################################
# Reader for targets.csv, the build-target registry.
#
# asdf_read_targets(<prefix>) sets, in the caller's scope:
#   <prefix>_NAMES           every target name, in file order
#   <prefix>_CATEGORY_<name> the target's category (avr, test, arm_m0+, sim)
#   <prefix>_SIMAVR          the targets run by the simavr integration tests
#
# Usable from a project and from a script run with cmake -P.
##########################################################################

set(ASDF_TARGETS_FILE ${CMAKE_CURRENT_LIST_DIR}/../targets.csv)

function(asdf_read_targets prefix)
  set(names "")
  set(simavr "")
  file(STRINGS ${ASDF_TARGETS_FILE} rows ENCODING UTF-8)
  foreach(row IN LISTS rows)
    if(row MATCHES "^[ \t]*(#|$)")
      continue()
    endif()
    string(REPLACE "," ";" cols "${row}")
    list(TRANSFORM cols STRIP)
    list(GET cols 0 name)
    list(GET cols 1 category)
    list(GET cols 2 in_simavr)
    list(APPEND names ${name})
    set(${prefix}_CATEGORY_${name} ${category} PARENT_SCOPE)
    if(in_simavr STREQUAL "yes")
      list(APPEND simavr ${name})
    endif()
  endforeach()
  set(${prefix}_NAMES ${names} PARENT_SCOPE)
  set(${prefix}_SIMAVR ${simavr} PARENT_SCOPE)
endfunction()
