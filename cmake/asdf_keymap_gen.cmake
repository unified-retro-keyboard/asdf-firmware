# Generation of key matrices from YAML (src/asdf_keymap_gen.py).
#
# asdf_generate_keymaps(<target> <yaml>...) generates <stem>.c and <stem>.h in
# the current binary directory for each YAML file, and makes a custom target
# <target> that builds them. Targets that compile or include the generated
# files depend on <target>. The generated .c files are returned in
# ASDF_GENERATED_KEYMAP_SOURCES.
#
# The generator's Python dependencies are managed by uv (inline script
# metadata in the script).

find_program(ASDF_UV uv REQUIRED)
set(ASDF_KEYMAP_GEN ${CMAKE_SOURCE_DIR}/src/asdf_keymap_gen.py)

function(asdf_generate_keymaps target)
  set(sources)
  set(outputs)
  foreach(yaml ${ARGN})
    get_filename_component(stem ${yaml} NAME_WE)
    set(c_file ${CMAKE_CURRENT_BINARY_DIR}/${stem}.c)
    set(h_file ${CMAKE_CURRENT_BINARY_DIR}/${stem}.h)
    add_custom_command(
      OUTPUT ${c_file} ${h_file}
      COMMAND ${ASDF_UV} run --quiet --script ${ASDF_KEYMAP_GEN} ${yaml} ${CMAKE_CURRENT_BINARY_DIR}
      DEPENDS ${yaml} ${ASDF_KEYMAP_GEN}
      COMMENT "Generating key matrices from ${stem}.yaml"
      VERBATIM)
    list(APPEND sources ${c_file})
    list(APPEND outputs ${c_file} ${h_file})
  endforeach()
  add_custom_target(${target} DEPENDS ${outputs})
  set(ASDF_GENERATED_KEYMAP_SOURCES ${sources} PARENT_SCOPE)
endfunction()
