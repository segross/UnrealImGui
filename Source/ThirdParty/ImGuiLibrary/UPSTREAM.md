# Dear ImGui provenance

- Upstream: https://github.com/ocornut/imgui
- Stable release: `v1.92.9b` (non-docking branch)
- Commit: `f1cc2ae15e53a861a874c3034aae6798fde194ab`
- License: MIT, preserved in `LICENSE.txt`

The `Include/` headers, `Private/` core and stb files, and `Docs/` documentation are copied unchanged from this tag. Unreal-specific definitions and the Slate/input backend live in `Source/ImGui`; they are not patched into the upstream sources. `imgui_tables.cpp` is compiled along with the other core sources by `ImGuiImplementation.cpp`.
