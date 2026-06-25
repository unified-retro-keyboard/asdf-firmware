##########################################################################
# ARM bare-metal toolchain for Cortex-M0+ (PIC32CM PL10).
#
# Mirrors the contract of cmake/generic-gcc-avr.cmake: it selects the
# arm-none-eabi compiler and defines c_toolchain_flags(), which
# src/CMakeLists.txt calls to populate CFLAGS. Link options, the linker
# script, and the objcopy/size post-build step are applied in
# src/CMakeLists.txt for ARCH_TYPE == ARM.
##########################################################################

find_program(ARM_CC arm-none-eabi-gcc)
find_program(ARM_OBJCOPY arm-none-eabi-objcopy)
find_program(ARM_SIZE_TOOL arm-none-eabi-size)

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMAKE_C_COMPILER ${ARM_CC})

# Bare-metal: do not attempt a full link during the compiler-id probe.
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(CMAKE_C_FLAGS_RELEASE    "-Os -DNDEBUG")
set(CMAKE_C_FLAGS_DEBUG      "-O0 -g")
set(CMAKE_C_FLAGS_MINSIZEREL "-Os -DNDEBUG")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "-Os -g -DNDEBUG")

set(ARM_MCU_FLAGS "-mcpu=cortex-m0plus" "-mthumb")

# Contract: src/CMakeLists.txt calls c_toolchain_flags() to populate CFLAGS.
function(c_toolchain_flags)
  set(CFLAGS
    ${ARM_MCU_FLAGS}
    -std=gnu99
    -ffunction-sections -fdata-sections
    -Wall -Wextra
    PARENT_SCOPE)
endfunction(c_toolchain_flags)
