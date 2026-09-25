##########################################################################
# Cppcheck static analysis.
#
# asdf_add_cppcheck(<target>) adds a `cppcheck` target that runs Cppcheck over
# the first-party C sources of <target>, with its include directories and
# definitions, with every check enabled. Any finding fails the target. False
# positives for how the check is run are suppressed, with the reasons, in
# lint/cppcheck-suppressions.txt. Unlike PC-lint Plus, Cppcheck is free, so CI
# runs it on every push and pull request.
#
# Cppcheck 2.17 or later is needed: an earlier one reports the staticFunction
# suppression as unmatched. There is no --cppcheck-build-dir, because with one
# Cppcheck skips the whole-program staticFunction check.
#
# ASDF_CPPCHECK is the Cppcheck executable, found on the PATH or set in the
# CMake cache. Without it, the cppcheck target reports that Cppcheck is missing
# and fails.
##########################################################################

find_program(ASDF_CPPCHECK cppcheck)

function(asdf_add_cppcheck target)
  if(NOT ASDF_CPPCHECK)
    add_custom_target(cppcheck
      COMMAND ${CMAKE_COMMAND} -E echo "cppcheck: install Cppcheck, or set ASDF_CPPCHECK"
      COMMAND ${CMAKE_COMMAND} -E false
      VERBATIM)
    return()
  endif()

  set(includes "$<TARGET_PROPERTY:${target},INCLUDE_DIRECTORIES>")
  set(defines "$<TARGET_PROPERTY:${target},COMPILE_DEFINITIONS>")
  # First-party C sources; vendored code is not checked.
  set(sources "$<FILTER:$<FILTER:$<TARGET_PROPERTY:${target},SOURCES>,INCLUDE,\\.c$>,EXCLUDE,/third_party/>")

  # Relative source paths are relative to the target's source directory.
  get_target_property(source_dir ${target} SOURCE_DIR)
  add_custom_target(cppcheck
    COMMAND ${ASDF_CPPCHECK}
            --std=c99 --language=c
            --enable=all --check-level=exhaustive
            --suppressions-list=${CMAKE_SOURCE_DIR}/lint/cppcheck-suppressions.txt
            --inline-suppr
            --error-exitcode=1 --quiet
            "$<$<BOOL:${includes}>:-I$<JOIN:${includes},;-I>>"
            "$<$<BOOL:${defines}>:-D$<JOIN:${defines},;-D>>"
            ${sources}
    WORKING_DIRECTORY ${source_dir}
    COMMENT "Running Cppcheck on ${target}"
    COMMAND_EXPAND_LISTS
    VERBATIM)
  add_dependencies(cppcheck ${target})
endfunction()
