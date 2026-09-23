##########################################################################
# Warning flags shared by every first-party build: host tests, AVR, and ARM.
# Toolchain-specific code-generation flags stay in each toolchain file.
##########################################################################

set(ASDF_WARNING_FLAGS
  -Wall
  -Wextra
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
  )
