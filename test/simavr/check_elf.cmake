# Fixture setup for the simavr tests of one target: fail, with the command
# that builds it, if the target's firmware ELF is missing.
#
# usage: cmake -DELF=<path> -DTARGET=<name> -P check_elf.cmake

cmake_minimum_required(VERSION 3.25)

if(NOT EXISTS ${ELF})
  message(FATAL_ERROR "missing ${ELF}\n"
    "Build the firmware first: cmake --workflow --preset ${TARGET}")
endif()
