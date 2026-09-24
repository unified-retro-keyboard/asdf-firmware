configure_file(${PROJECT_SRC_DIR}/Arch/asdf_arch_test.h ${CMAKE_CURRENT_BINARY_DIR}/asdf_arch.h COPYONLY)
configure_file(${PROJECT_SRC_DIR}/asdf_keymap_setup.h.in ${CMAKE_CURRENT_BINARY_DIR}/asdf_keymap_setup.h COPYONLY)

include(${CMAKE_SOURCE_DIR}/cmake/asdf_warnings.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/asdf_keymap_gen.cmake)

asdf_generate_keymaps(test_keymaps_gen ${CMAKE_CURRENT_SOURCE_DIR}/test_keymaps.yaml)

# Match the firmware's unsigned char (set by -funsigned-char on AVR, the default
# on ARM) so host tests see the same char semantics.
list(APPEND C_FLAGS
  "-std=c99"
  "-funsigned-char"
  ${ASDF_WARNING_FLAGS}
  "-g"
  )

list(APPEND TEST_INCLUDES
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${PROJECT_SRC_DIR}
  ${PROJECT_SRC_DIR}/Arch
  )

add_library(unity
  STATIC
  ${CMAKE_CURRENT_SOURCE_DIR}/unity/unity.c
  )

target_include_directories(unity
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/unity
  )


add_library(asdf_core
  STATIC
  asdf_keymap_table.c
  ${PROJECT_SRC_DIR}/asdf.c
  ${PROJECT_SRC_DIR}/asdf_ring.c
  ${PROJECT_SRC_DIR}/asdf_actions.c
  ${PROJECT_SRC_DIR}/asdf_keymaps.c
  ${PROJECT_SRC_DIR}/asdf_modifiers.c
  ${PROJECT_SRC_DIR}/asdf_physical.c
  ${PROJECT_SRC_DIR}/asdf_repeat.c
  ${PROJECT_SRC_DIR}/asdf_virtual.c
  ${PROJECT_SRC_DIR}/asdf_print.c
  ${PROJECT_SRC_DIR}/Arch/asdf_arch_test.c
  )

target_compile_options(asdf_core
  PRIVATE
  ${C_FLAGS}
  )
add_dependencies(asdf_core test_keymaps_gen)

target_include_directories(asdf_core
  PRIVATE
  ${PROJECT_SRC_DIR}
  ${PROJECT_SRC_DIR}/Arch
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  )

add_library(test_helpers
  STATIC
  ${CMAKE_CURRENT_SOURCE_DIR}/test_asdf_lib.c
  ${CMAKE_CURRENT_SOURCE_DIR}/fake_platform.c
  ${CMAKE_CURRENT_SOURCE_DIR}/test_asdf_keymap_defs.c
  ${CMAKE_CURRENT_SOURCE_DIR}/asdf_keymap_table.c
  ${ASDF_GENERATED_KEYMAP_SOURCES}
  )
add_dependencies(test_helpers test_keymaps_gen)

target_include_directories(test_helpers
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${PROJECT_SRC_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  )


target_compile_options(test_helpers
  PRIVATE
  ${C_FLAGS}
  )

# The test keymaps' alternate platform uses the test adapter's platform, and
# the core uses the test keymaps, so the two libraries depend on each other.
target_link_libraries(test_helpers asdf_core)

function(setup_test testname)

  add_executable(${testname}
    ${testname}.c
    )

  target_compile_options(${testname}
    PRIVATE
    ${C_FLAGS}
    )

  target_include_directories(${testname}
    PRIVATE
    ${TEST_INCLUDES}
    )

  target_link_libraries(${testname} unity asdf_core test_helpers)
  add_dependencies(${testname} test_keymaps_gen)

  add_test(
    NAME ${testname}
    COMMAND ${CMAKE_BINARY_DIR}/test/${testname}
    )
endfunction()

enable_testing()

setup_test(test_asdf_repeat)
setup_test(test_asdf_modifiers)
setup_test(test_asdf_keymaps)
setup_test(test_asdf_ring)
setup_test(test_asdf_physical)
setup_test(test_asdf_platform)
setup_test(test_asdf_instances)
setup_test(test_asdf_output)
setup_test(test_asdf_actions)
setup_test(test_asdf_print)
setup_test(test_asdf_virtual)
setup_test(test_asdf_descriptor)
setup_test(test_keymap_lookup)
setup_test(test_asdf)
setup_test(test_asdf_characterization)
setup_test(test_asdf_simple)
target_sources(test_asdf_simple PRIVATE ${PROJECT_SRC_DIR}/asdf_simple.c)
