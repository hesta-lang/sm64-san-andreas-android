# SM64 San Andreas — Android AML port

Android port of [sm64-san-andreas](https://github.com/headshot2017/sm64-san-andreas) for GTA: San Andreas Android using Android Mod Loader (AML) and the upstream [libsm64](https://github.com/libsm64/libsm64) runtime.

## Target

- GTA: San Andreas Android **v2.00 (32-bit / armeabi-v7a)**
- Android NDK r27c
- C++17 / C99
- AML + Android GTA SA Plugin SDK
- Upstream libsm64, compiled into the plugin as a static library

GTA SA v2.00 is the 32-bit target. This project intentionally does not build an arm64 binary for v2.00.

## Current status — 0.3.0-alpha-renderware

Implemented:
- Builds upstream libsm64 for `armeabi-v7a`.
- Validates the user-provided US 1.0 ROM by size, big-endian header, and SHA-1 prefix.
- Initializes the libsm64 runtime, creates Mario, and updates its mesh every GTA game-process frame.
- Converts libsm64's world-space Y-up geometry to a local Z-up RenderWare mesh.
- Creates a RenderWare geometry, material, frame, and atomic, then draws Mario from the post-ped render-effects callback at the local player's position.
- Releases RenderWare objects before the RenderWare shutdown event.
- Pins libsm64 and the Android plugin SDK source revisions in GitHub Actions for reproducible builds.

Current limitations:
- This first renderer milestone uses vertex colors and a plain material; Mario's texture atlas is not yet uploaded to RenderWare.
- The normal CJ ped is not hidden or replaced yet, so this is a render visibility test at the player's location.
- GTA collision surfaces, player input, camera orientation, and gameplay physics are not connected yet.

A successful CI build proves compilation/linking. It does not guarantee the model will display correctly on the device; in-game render output still needs to be tested.

## ROM placement

The plugin does not include a ROM. Provide your own legally obtained **Super Mario 64 US 1.0** ROM named exactly:

`sm64.us.z64`

Preferred location:

`Android/data/com.rockstargames.gtasa/files/sm64.us.z64`

Fallbacks checked: `/sdcard/AMLMods/GTASA/` and `/sdcard/`.

The ROM must be the standard big-endian `.z64` format, exactly 8 MiB, and match the US 1.0 SHA-1 check.

## Build and install

GitHub Actions builds the plugin for `armeabi-v7a` and uploads:

`sm64-san-andreas-android-0.3.0-alpha-renderware-armeabi-v7a`

Install `libsm64sanandreas.so` into:

`Android/data/com.rockstargames.gtasa/mods/`

Keep the ROM at the location listed above. For diagnostics, inspect AML logs and Android logcat using the tag `SM64SanAndreas`.

## Next stages

1. Verify that Mario is visible and correctly positioned with the RenderWare test model.
2. Add texture-atlas upload and match player heading.
3. Replace CJ's render safely once the standalone model is verified.
4. Connect GTA collision surfaces, controls, and gameplay physics.
5. Add camera, audio, animations, and interactions.
