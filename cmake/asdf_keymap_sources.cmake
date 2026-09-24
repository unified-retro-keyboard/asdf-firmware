# The production keymaps: their YAML key matrices and C sources, used by the
# firmware build (src/) and by the host test of the production keymaps (test/).
# Paths are relative to src/.

set(ASDF_KEYMAP_YAML
  Keymaps/asdf_keymap_classic_maps.yaml
  Keymaps/asdf_keymap_apple2_maps.yaml
  Keymaps/asdf_keymap_sol_maps.yaml
  Keymaps/asdf_keymap_ace1000_maps.yaml
  )

set(ASDF_KEYMAP_SOURCES
  Keymaps/asdf_keymap_classic.c
  Keymaps/asdf_keymap_classic_caps.c
  Keymaps/asdf_keymap_apple2.c
  Keymaps/asdf_keymap_apple2_caps.c
  Keymaps/asdf_keymap_apple2_add_map.c
  Keymaps/asdf_keymap_sol.c
  Keymaps/asdf_keymap_ace1000.c
  Keymaps/asdf_keymap_ace1000_add_map.c
  Keymaps/asdf_keymap_actions.c
  )
