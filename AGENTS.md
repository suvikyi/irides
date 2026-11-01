# Agent guidance

## Versioning and releases

Use Semantic Versioning 2.0.0 for Irides. Keep releases pre-1.0 until the QML API, compositor bridge, and fallback behavior are considered stable.

The consumer-facing contract includes:

- the `Irides` QML module URI and its import version,
- the public `Grade`, `Shadow`, and `Oklab` types and their properties,
- the `hyprion_grade_manager_v1` client bridge and its compatibility with Hyprion,
- the documented compositor-path and local-fallback behavior.

### Before 1.0.0

Keep the major version at `0`:

- **Patch** (`0.y.Z`): backwards-compatible bug fixes and corrections.
- **Minor** (`0.Y.0`): new features or properties, and any breaking change to the consumer-facing contract. Reset the patch number to zero.

Prefer additive QML API changes. Because the project is pre-1.0, do not add compatibility aliases for removed or renamed properties by default; update examples and migration instructions when a breaking change is made.

### From 1.0.0 onward

- **Patch**: backwards-compatible fixes.
- **Minor**: backwards-compatible features and properties.
- **Major**: incompatible changes to the consumer-facing contract.

Use `1.0.0` when the QML API, compositor bridge, and documented fallback behavior are ready to be treated as stable, not merely because a date or feature count was reached.

### Release procedure

- Keep `project(irides VERSION ...)` in `CMakeLists.txt` and the major/minor of `qt_add_qml_module(... VERSION ...)` consistent. The project version is the package version; the QML module version is its `MAJOR.MINOR` form.
- Update the import version in `README.md` and examples in the same release change.
- Regenerate `grade.frag.qsb` and `shadow.frag.qsb` whenever their GLSL sources change.
- Bump for user-visible QML, bridge, shader, or packaging changes; documentation-only edits and internal refactors with no user-visible change do not require a bump.
- Before release, build the QML module, verify the requested `import Irides MAJOR.MINOR` resolves, and test both the compositor path and the local fallback.
- Tag releases `vMAJOR.MINOR.PATCH` (for example, `v0.2.0`) and publish GitHub release notes against that tag. Do not move or rewrite a published version tag; correct a released mistake with a subsequent release.

## Local QML verification

- Build with `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release` and `cmake --build build --parallel`.
- Verify the module with the built import path:
  `QML_IMPORT_PATH="$PWD/build/qml" qml6 ...`
  and confirm `import Irides MAJOR.MINOR` resolves.
- Verify the local `Grade` fallback on a compositor without the Hyprion global, including a multi-stop colorize gradient.
- Verify the compositor path in an isolated nested Hyprland session with Hyprion loaded, including the `hyprion_grade_manager_v1` global and `set_colorize_gradient`.
- Keep `protocols/hyprion-grade-v1.xml` synchronized with Hyprion; the bridge is a shared wire contract.
- Do not overwrite a loaded `libiridesplugin.so` in place. Rebuild to a temporary file on the same filesystem and restart the consuming application, or use a fresh build path.
- QML modules and shader packages are trusted content. Do not load untrusted `.qsb` files or untrusted QML through the module.
