##########################################################################
# Warning flags shared by every first-party build: host tests, AVR, and ARM.
# Toolchain-specific code-generation flags stay in each toolchain file.
#
# ASDF_WARNING_FLAGS apply to all first-party code. ASDF_STRICT_WARNING_FLAGS
# apply to the firmware and the host build of the core library, but not to the
# host test programs, whose int loop counters and test functions would only
# add casts and prototypes without making the tests better.
#
# With ASDF_WERROR set, as a CMake option or an environment variable (CI sets
# it), warnings are errors.
##########################################################################

set(ASDF_WARNING_FLAGS
  -Wall
  -Wextra
  -Wpedantic
  -Wpointer-arith
  -Wcast-align
  -Wwrite-strings
  -Wswitch-default
  -Wunreachable-code
  -Winit-self
  -Wmissing-field-initializers
  -Wno-unknown-pragmas
  -Wstrict-prototypes
  -Wundef
  -Wold-style-definition
  -Wcast-function-type
  -Wshadow
  -Wimplicit-fallthrough
  -Wnull-dereference
  -Wredundant-decls
  )

set(ASDF_STRICT_WARNING_FLAGS
  -Wconversion
  -Wsign-conversion
  -Wmissing-prototypes
  )

if(ASDF_WERROR OR "$ENV{ASDF_WERROR}")
  list(APPEND ASDF_WARNING_FLAGS -Werror)
endif()
