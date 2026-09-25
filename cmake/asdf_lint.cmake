##########################################################################
# PC-lint Plus static analysis.
#
# asdf_add_lint(<target> [<lnt>...]) adds a `lint` target that runs PC-lint Plus
# over the sources of <target>, with its include directories and definitions,
# and with any extra option files <lnt> after lint/asdf.lnt. The
# compiler configuration (type sizes, predefined macros, system include paths)
# is generated at build time by the pclp_config.py shipped with PC-lint Plus,
# from the compiler and language options <target> is built with, so nothing
# machine-specific is committed. Project policy is in lint/asdf.lnt; MISRA C
# checking and its deviations are in lint/misra.lnt.
#
# ASDF_PCLP is the PC-lint Plus executable, from the CMake cache or the PCLP
# environment variable. Its license file must be in the same directory. Without
# it, the lint target reports that PC-lint Plus is missing and fails.
##########################################################################

set(ASDF_PCLP "$ENV{PCLP}" CACHE FILEPATH "PC-lint Plus executable (pclp64_linux)")
find_package(Python3 COMPONENTS Interpreter)

function(asdf_add_lint target)
  set(lint_dir ${CMAKE_BINARY_DIR}/lint)
  set(policy ${CMAKE_SOURCE_DIR}/lint/asdf.lnt ${ARGN} ${CMAKE_SOURCE_DIR}/lint/misra.lnt)

  if(NOT EXISTS "${ASDF_PCLP}" OR NOT Python3_Interpreter_FOUND)
    add_custom_target(lint
      COMMAND ${CMAKE_COMMAND} -E echo
              "lint: set PCLP (or ASDF_PCLP) to the PC-lint Plus executable, and install python3"
      COMMAND ${CMAKE_COMMAND} -E false
      VERBATIM)
    return()
  endif()

  get_filename_component(pclp_home ${ASDF_PCLP} DIRECTORY)

  # Language and device options that change what the compiler predefines.
  # The AVR toolchain passes -mmcu in the COMPILE_FLAGS string.
  set(options "$<FILTER:$<TARGET_PROPERTY:${target},COMPILE_OPTIONS>,INCLUDE,^-(std=|f|m)>")
  set(flags "$<TARGET_PROPERTY:${target},COMPILE_FLAGS>")

  add_custom_command(
    OUTPUT ${lint_dir}/co-compiler.lnt ${lint_dir}/co-compiler.h
    COMMAND ${Python3_EXECUTABLE} ${pclp_home}/config/pclp_config.py
            --compiler=gcc
            --compiler-bin=${CMAKE_C_COMPILER}
            "--compiler-options=$<JOIN:${options}, > ${flags}"
            --config-output-lnt-file=${lint_dir}/co-compiler.lnt
            --config-output-header-file=${lint_dir}/co-compiler.h
            --generate-compiler-config
    WORKING_DIRECTORY ${lint_dir}
    COMMENT "Generating the PC-lint Plus configuration for ${CMAKE_C_COMPILER}"
    VERBATIM)

  set(includes "$<TARGET_PROPERTY:${target},INCLUDE_DIRECTORIES>")
  set(defines "$<TARGET_PROPERTY:${target},COMPILE_DEFINITIONS>")
  # First-party C sources; vendored code is not linted.
  set(sources "$<FILTER:$<FILTER:$<TARGET_PROPERTY:${target},SOURCES>,INCLUDE,\\.c$>,EXCLUDE,/third_party/>")
  file(GENERATE OUTPUT ${lint_dir}/project.lnt CONTENT
"$<$<BOOL:${includes}>:-i\"$<JOIN:${includes},\"\n-i\">\"\n>\
$<$<BOOL:${defines}>:-d$<JOIN:${defines},\n-d>\n>\
\"$<JOIN:${sources},\"\n\">\"
")

  # Relative source paths are relative to the target's source directory.
  get_target_property(source_dir ${target} SOURCE_DIR)
  add_custom_target(lint
    COMMAND ${ASDF_PCLP} -i${pclp_home}/lnt ${lint_dir}/co-compiler.lnt ${policy} ${lint_dir}/project.lnt
    DEPENDS ${lint_dir}/co-compiler.lnt ${policy}
    WORKING_DIRECTORY ${source_dir}
    COMMENT "Running PC-lint Plus on ${target}"
    VERBATIM)
  add_dependencies(lint ${target})
endfunction()
