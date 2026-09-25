configure_file(${PROJECT_SRC_DIR}/Arch/asdf_arch_test.h ${CMAKE_CURRENT_BINARY_DIR}/asdf_arch.h COPYONLY)
configure_file(${PROJECT_SRC_DIR}/asdf_keymap_setup.h.in ${CMAKE_CURRENT_BINARY_DIR}/asdf_keymap_setup.h COPYONLY)

include(${CMAKE_SOURCE_DIR}/cmake/asdf_warnings.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/asdf_keymap_gen.cmake)

asdf_generate_keymaps(test_keymaps_gen ${CMAKE_CURRENT_SOURCE_DIR}/test_keymaps.yaml)

# With ASDF_SANITIZE set (CMake option or environment variable), the host tests
# run under AddressSanitizer and UndefinedBehaviorSanitizer, and any finding
# fails the test.
option(ASDF_SANITIZE "Build host tests with ASan and UBSan" OFF)
if(ASDF_SANITIZE OR "$ENV{ASDF_SANITIZE}")
  set(ASDF_SANITIZE_FLAGS -fsanitize=address,undefined -fno-sanitize-recover=all
    -fno-omit-frame-pointer)
  add_compile_options(${ASDF_SANITIZE_FLAGS})
  add_link_options(${ASDF_SANITIZE_FLAGS})
endif()

# With ASDF_COVERAGE set (CMake option or environment variable), the host tests
# record line and branch coverage for gcov. After the tests run, the coverage
# target reports the core's coverage with gcovr (run through uv).
option(ASDF_COVERAGE "Build host tests with coverage instrumentation" OFF)
if(ASDF_COVERAGE OR "$ENV{ASDF_COVERAGE}")
  add_compile_options(--coverage -O0)
  add_link_options(--coverage)
  add_custom_target(coverage
    COMMAND ${ASDF_UV} tool run gcovr --root ${CMAKE_SOURCE_DIR}
            --filter src/ --exclude src/Arch/ --exclude src/Keymaps/
            ${CMAKE_BINARY_DIR}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Reporting host test coverage of the core"
    VERBATIM)
endif()

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


set(ASDF_CORE_SOURCES
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

# The core with the test keymaps, used by most tests.
add_library(asdf_core
  STATIC
  asdf_keymap_table.c
  ${ASDF_CORE_SOURCES}
  )

target_compile_options(asdf_core
  PRIVATE
  ${C_FLAGS}
  )
target_compile_options(asdf_core PRIVATE ${ASDF_STRICT_WARNING_FLAGS})
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
setup_test(test_asdf_fuzz)
target_sources(test_asdf_simple PRIVATE ${PROJECT_SRC_DIR}/asdf_simple.c)

# The core with the production keymaps (the firmware's keymap list), to check
# that every production keymap applies cleanly on the host.
include(${CMAKE_SOURCE_DIR}/cmake/asdf_keymap_sources.cmake)
include(${CMAKE_SOURCE_DIR}/keymap_list.cmake)
list(TRANSFORM ASDF_KEYMAP_YAML PREPEND ${PROJECT_SRC_DIR}/ OUTPUT_VARIABLE production_keymap_yaml)
asdf_generate_keymaps(production_keymaps_gen ${production_keymap_yaml})
set(PRODUCTION_KEYMAP_SOURCES ${ASDF_GENERATED_KEYMAP_SOURCES})
create_keymap_table("${keymap_list}" keymap_table)
create_keymap_declarations("${keymap_list}" keymap_declarations)
configure_file(${PROJECT_SRC_DIR}/asdf_keymap_setup.c.in
  ${CMAKE_CURRENT_BINARY_DIR}/production_keymap_setup.c)
list(TRANSFORM ASDF_KEYMAP_SOURCES PREPEND ${PROJECT_SRC_DIR}/ OUTPUT_VARIABLE production_keymap_c)

add_library(asdf_production
  STATIC
  ${ASDF_CORE_SOURCES}
  ${production_keymap_c}
  ${PRODUCTION_KEYMAP_SOURCES}
  ${CMAKE_CURRENT_BINARY_DIR}/production_keymap_setup.c
  )
target_compile_options(asdf_production PRIVATE ${C_FLAGS} ${ASDF_STRICT_WARNING_FLAGS})
target_include_directories(asdf_production
  PRIVATE
  ${PROJECT_SRC_DIR}
  ${PROJECT_SRC_DIR}/Arch
  ${PROJECT_SRC_DIR}/Keymaps
  ${CMAKE_CURRENT_BINARY_DIR}
  )
add_dependencies(asdf_production production_keymaps_gen)

add_executable(test_production_keymaps
  test_production_keymaps.c
  ${CMAKE_CURRENT_SOURCE_DIR}/fake_platform.c
  )
target_compile_options(test_production_keymaps PRIVATE ${C_FLAGS})
target_include_directories(test_production_keymaps PRIVATE ${TEST_INCLUDES})
target_link_libraries(test_production_keymaps unity asdf_production)
add_test(NAME test_production_keymaps COMMAND ${CMAKE_BINARY_DIR}/test/test_production_keymaps)

# PC-lint Plus runs over the same sources: the core and the production keymaps.
include(${CMAKE_SOURCE_DIR}/cmake/asdf_lint.cmake)
asdf_add_lint(asdf_production)

# CMakePresets.json must have a preset for every target in targets.csv.
add_test(NAME presets_match_targets
  COMMAND ${CMAKE_COMMAND} -P ${CMAKE_SOURCE_DIR}/cmake/asdf_check_presets.cmake)
