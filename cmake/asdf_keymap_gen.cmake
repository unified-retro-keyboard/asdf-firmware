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

# Keymap registry helpers: from a keymap list (keymap_list.cmake), make the
# initializers and declarations for asdf_keymap_setup.c.in, and a report.
function(create_keymap_table keymaps keymap_table)
  # one "[number] = &name_keymap" initializer per keymap, comma separated
  list(TRANSFORM keymaps REPLACE "<\(.+\):\(.+\)>"  "\n  [\\2] = &\\1_keymap" OUTPUT_VARIABLE temp_list)
  list(JOIN temp_list "," temp_string)
  set(${keymap_table} "${temp_string}" PARENT_SCOPE)
endfunction(create_keymap_table)

function(create_keymap_slots keymaps keymap_slots)
  # the registry's size: one past the highest keymap number
  set(slots 0)
  foreach(keymap IN LISTS keymaps)
    string(REGEX REPLACE "<.+:(.+)>" "\\1" number "${keymap}")
    if(number GREATER_EQUAL slots)
      math(EXPR slots "${number} + 1")
    endif()
  endforeach()
  set(${keymap_slots} ${slots} PARENT_SCOPE)
endfunction(create_keymap_slots)

function(create_keymap_declarations keymaps keymap_decl)
  # each keymap's header declares its descriptor
  list(TRANSFORM keymaps REPLACE "<\(.+\):\(.+\)>" "#include \"asdf_keymap_\\1.h\"" OUTPUT_VARIABLE temp_list)
  list(JOIN temp_list "\n" temp_list)
  set(${keymap_decl} "${temp_list}" PARENT_SCOPE)
endfunction(create_keymap_declarations)

function(create_keymap_report keymaps keymap_report)
  list(TRANSFORM keymaps REPLACE "<\(.+\):\(.+\)>"  "\nkeymap [\\2]: \\1" OUTPUT_VARIABLE temp_list)
  string(REPLACE ";" "" temp_list2 "${temp_list}")
  set(${keymap_report} "${temp_list2}" PARENT_SCOPE)
endfunction(create_keymap_report)
