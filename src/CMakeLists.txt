message("C compiler: ${CMAKE_C_COMPILER}")

function(create_keymap_setups keymaps keymap_table)
  list(TRANSFORM keymaps REPLACE "<\(.+\):\(.+\)>"  "\n    case \\2: setup_\\1_keymap();break" OUTPUT_VARIABLE temp_list)
  # we can keep the ';' cmake list separators as the C statement separators.
  # However, we need to append an extra ';' at the end.
  string(APPEND temp_list ";")
  set(${keymap_table} "${temp_list}" PARENT_SCOPE)
endfunction(create_keymap_setups)

function(create_keymap_valid keymaps keymap_valid)
  list(TRANSFORM keymaps REPLACE "<\(.+\):\(.+\)>"  "\n    case \\2:" OUTPUT_VARIABLE temp_list)
  # we can keep the ';' cmake list separators as the C statement separators.
  # However, we need to append an extra ';' at the end.
  string(APPEND temp_list ";")
  set(${keymap_valid} "${temp_list}" PARENT_SCOPE)
endfunction(create_keymap_valid)

function(create_keymap_declarations keymaps keymap_decl)
  list(TRANSFORM keymaps REPLACE "<\(.+\):\(.+\)>"  "\n void setup_\\1_keymap(void)" OUTPUT_VARIABLE temp_list)
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

create_keymap_setups("${keymap_list}" keymap_table)
create_keymap_valid("${keymap_list}" keymap_valid)
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

list (APPEND SOURCES
  asdf.c
  ${CMAKE_CURRENT_BINARY_DIR}/asdf_arch.c
  ${CMAKE_CURRENT_BINARY_DIR}/asdf_keymap_setup.c
  asdf_ring.c
  asdf_hook.c
  asdf_keymap_setup.c
  asdf_keymaps.c
  asdf_modifiers.c
  asdf_physical.c
  asdf_repeat.c
  asdf_virtual.c
  asdf_print.c
  Keymaps/asdf_keymap_classic.c
  Keymaps/asdf_keymap_classic_caps.c
  Keymaps/asdf_keymap_classic_add_map.c
  Keymaps/asdf_keymap_apple2.c
  Keymaps/asdf_keymap_apple2_caps.c
  Keymaps/asdf_keymap_apple2_add_map.c
  Keymaps/asdf_keymap_sol.c
  Keymaps/asdf_keymap_ace1000.c
  Keymaps/asdf_keymap_ace1000_add_map.c
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

#target_link_libraries(${PROJECT_EXECUTABLE_TARGET_NAME}
#  keymaps
#  )
