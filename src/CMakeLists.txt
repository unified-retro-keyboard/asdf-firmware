message("C compiler: ${CMAKE_C_COMPILER}")

function(create_keymap_table keymaps keymap_table)
  # one "[number] = &name_keymap" initializer per keymap, comma separated
  list(TRANSFORM keymaps REPLACE "<\(.+\):\(.+\)>"  "\n  [\\2] = &\\1_keymap" OUTPUT_VARIABLE temp_list)
  list(JOIN temp_list "," temp_string)
  set(${keymap_table} "${temp_string}" PARENT_SCOPE)
endfunction(create_keymap_table)

function(create_keymap_declarations keymaps keymap_decl)
  list(TRANSFORM keymaps REPLACE "<\(.+\):\(.+\)>"  "\nextern const asdf_keymap_t \\1_keymap" OUTPUT_VARIABLE temp_list)
  # we can keep the ';' cmake list separators as the C statement separators.
  # However, we need to append an extra ';' at the end.
  string(APPEND temp_list ";")
  set(${keymap_decl} "${temp_list}" PARENT_SCOPE)
endfunction(create_keymap_declarations)

function(create_keymap_report keymaps keymap_report)
  list(TRANSFORM keymaps REPLACE "<\(.+\):\(.+\)>"  "\nkeymap [\\2]: \\1" OUTPUT_VARIABLE temp_list)
  string(REPLACE ";" "" temp_list2 "${temp_list}")
  set(${keymap_report} "${temp_list2}" PARENT_SCOPE)
endfunction(create_keymap_report)

include(../keymap_list.cmake)

create_keymap_table("${keymap_list}" keymap_table)
create_keymap_report("${keymap_list}" keymap_report)
create_keymap_declarations("${keymap_list}" keymap_declarations)

message("**********************")
message("Keymap table: ${keymap_report}")
message("**********************\n\n")

set (PROJECT_BIN_DIR ${CMAKE_CURRENT_BINARY_DIR})

configure_file(${PROJECT_SRC_DIR}/Arch/asdf_arch_${ARCH_FAMILY}.h ${CMAKE_CURRENT_BINARY_DIR}/asdf_arch.h COPYONLY)
configure_file(${PROJECT_SRC_DIR}/Arch/asdf_arch_${ARCH_FAMILY}.c ${CMAKE_CURRENT_BINARY_DIR}/asdf_arch.c COPYONLY)
configure_file(${PROJECT_SRC_DIR}/asdf_keymap_setup.c.in  ${CMAKE_CURRENT_BINARY_DIR}/asdf_keymap_setup.c)
configure_file(${PROJECT_SRC_DIR}/asdf_keymap_setup.h.in  ${CMAKE_CURRENT_BINARY_DIR}/asdf_keymap_setup.h)


c_toolchain_flags()

# Key matrices, generated from YAML into the build directory.
include(${CMAKE_SOURCE_DIR}/cmake/asdf_keymap_gen.cmake)
asdf_generate_keymaps(keymaps_gen
  ${PROJECT_SRC_DIR}/Keymaps/asdf_keymap_classic_maps.yaml
  ${PROJECT_SRC_DIR}/Keymaps/asdf_keymap_apple2_maps.yaml
  ${PROJECT_SRC_DIR}/Keymaps/asdf_keymap_sol_maps.yaml
  ${PROJECT_SRC_DIR}/Keymaps/asdf_keymap_ace1000_maps.yaml
  )

list (APPEND SOURCES
  asdf.c
  ${CMAKE_CURRENT_BINARY_DIR}/asdf_arch.c
  ${CMAKE_CURRENT_BINARY_DIR}/asdf_keymap_setup.c
  asdf_ring.c
  asdf_actions.c
  asdf_keymaps.c
  asdf_modifiers.c
  asdf_physical.c
  asdf_repeat.c
  asdf_virtual.c
  asdf_print.c
  Keymaps/asdf_keymap_classic.c
  Keymaps/asdf_keymap_classic_caps.c
  Keymaps/asdf_keymap_apple2.c
  Keymaps/asdf_keymap_apple2_caps.c
  Keymaps/asdf_keymap_apple2_add_map.c
  Keymaps/asdf_keymap_sol.c
  Keymaps/asdf_keymap_ace1000.c
  Keymaps/asdf_keymap_ace1000_add_map.c
  Keymaps/asdf_keymap_actions.c
  ${ASDF_GENERATED_KEYMAP_SOURCES}
  main.c
  )

# ARM targets need the device startup, SystemInit, and shared arch-common source.
if(ARCH_TYPE STREQUAL "ARM")
  list(APPEND SOURCES
    ${CMAKE_SOURCE_DIR}/src/third_party/cmsis/startup_${ARM_STARTUP_BASE}.c
    ${CMAKE_SOURCE_DIR}/src/third_party/cmsis/system_${ARM_STARTUP_BASE}.c
    ${CMAKE_SOURCE_DIR}/src/Arch/asdf_arch_pic32cm_common.c
    ${CMAKE_SOURCE_DIR}/src/Arch/asdf_arch_pic32cm_syscalls.c)
  # Vendored Microchip startup/system sources: do not warn on them (they call
  # their own deprecated legacy handler aliases). Our code keeps -Wall -Wextra.
  set_source_files_properties(
    ${CMAKE_SOURCE_DIR}/src/third_party/cmsis/startup_${ARM_STARTUP_BASE}.c
    ${CMAKE_SOURCE_DIR}/src/third_party/cmsis/system_${ARM_STARTUP_BASE}.c
    PROPERTIES COMPILE_OPTIONS "-w")
endif()

# create a library for the keymap modules
#add_subdirectory(Keymaps)

# add the executable
if (COMMAND custom_add_executable)
  custom_add_executable(${PROJECT_TARGET_NAME}
  ${SOURCES}
  )
else()
  add_executable(${PROJECT_TARGET_NAME}
    ${SOURCES}
    )
endif()

target_include_directories(${PROJECT_EXECUTABLE_TARGET_NAME}
  PRIVATE
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/Keymaps

  )

target_compile_options(${PROJECT_EXECUTABLE_TARGET_NAME}
  PRIVATE
  ${CFLAGS}
  )
add_dependencies(${PROJECT_EXECUTABLE_TARGET_NAME} keymaps_gen)

if(ARCH_TYPE STREQUAL "ARM")
  target_include_directories(${PROJECT_EXECUTABLE_TARGET_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src/third_party/cmsis
    ${CMAKE_SOURCE_DIR}/src/Arch)
  target_compile_definitions(${PROJECT_EXECUTABLE_TARGET_NAME} PRIVATE ${PART_DEFINE})
  set_target_properties(${PROJECT_EXECUTABLE_TARGET_NAME} PROPERTIES SUFFIX ".elf")
  target_link_options(${PROJECT_EXECUTABLE_TARGET_NAME} PRIVATE
    -mcpu=cortex-m0plus -mthumb
    -T${LINKER_SCRIPT}
    --specs=nano.specs
    -Wl,--gc-sections
    -Wl,-Map=${PROJECT_TARGET_NAME}.map)
  add_custom_command(TARGET ${PROJECT_EXECUTABLE_TARGET_NAME} POST_BUILD
    COMMAND arm-none-eabi-objcopy -O binary $<TARGET_FILE:${PROJECT_EXECUTABLE_TARGET_NAME}> ${PROJECT_TARGET_NAME}.bin
    COMMAND arm-none-eabi-objcopy -O ihex   $<TARGET_FILE:${PROJECT_EXECUTABLE_TARGET_NAME}> ${PROJECT_TARGET_NAME}.hex
    COMMAND arm-none-eabi-size $<TARGET_FILE:${PROJECT_EXECUTABLE_TARGET_NAME}>)
  # Deploy the flashable images to dist/ on `make install` (-i). This also gives
  # the build an install rule, so deploy_arch does not fail on ARM targets.
  # Note: unlike the AVR path, no Sphinx download-page link is generated, since
  # these targets are build-verified only and not part of the release artifacts.
  install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_TARGET_NAME}.hex
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_TARGET_NAME}.bin
    DESTINATION dist)
endif()

# The simple wrapper (asdf_simple.c) is not part of this firmware: main.c owns
# the keyboard and the tick interrupt. Compile it anyway, to check that it
# builds for the architecture.
add_library(asdf_simple_check OBJECT asdf_simple.c)
add_dependencies(asdf_simple_check keymaps_gen)
target_include_directories(asdf_simple_check PRIVATE
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/Keymaps)
target_compile_options(asdf_simple_check PRIVATE ${CFLAGS})
if(DEFINED AVR_MCU)
  # the AVR toolchain sets the device per target (add_avr_executable)
  target_compile_options(asdf_simple_check PRIVATE -mmcu=${AVR_MCU})
endif()
if(ARCH_TYPE STREQUAL "ARM")
  target_include_directories(asdf_simple_check PRIVATE
    ${CMAKE_SOURCE_DIR}/src/third_party/cmsis
    ${CMAKE_SOURCE_DIR}/src/Arch)
  target_compile_definitions(asdf_simple_check PRIVATE ${PART_DEFINE})
endif()

#target_link_libraries(${PROJECT_EXECUTABLE_TARGET_NAME}
#  keymaps
#  )
