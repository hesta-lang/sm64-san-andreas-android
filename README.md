# SM64 San Andreas — Android AML port

Android port of [sm64-san-andreas](https://github.com/headshot2017/sm64-san-andreas) for GTA: San Andreas Android using Android Mod Loader (AML) and the upstream [libsm64](https://github.com/libsm64/libsm64) runtime.

## Target

- GTA: San Andreas Android **v2.00 (32-bit / armeabi-v7a)**
- Android NDK r27c
- C++17 / C99
- AML + Android GTA SA Plugin SDK
- Upstream libsm64, compiled into the plugin as a static library

GTA SA v2.00 is the 32-bit target. This project intentionally does not build an arm64 binary for v2.00.

## Current status — 0.2.0-alpha

This milestone adds the upstream libsm64 source to the Android build and links it into the AML plugin.

Implemented:
- Compiles libsm64 for `armeabi-v7a` instead of relying on a desktop prebuilt library.
- Generates the upstream Mario geometry/model source during GitHub Actions.
- Includes the public `libsm64.h` API in the Android plugin.
- Adds a guarded runtime initializer for `sm64_global_init`.
- Searches for a user-provided ROM named `sm64.us.z64`.
- Validates the 8 MiB size, big-endian `.z64` header, and SHA-1 prefix for the US 1.0 ROM before initialization.
- Creates a Mario simulation instance and executes one libsm64 simulation tick as an ABI/runtime smoke test.
- Writes initialization diagnostics to Android logcat and the AML logger.
- Leaves GTA SA running if the ROM is missing or invalid.

Not implemented yet:
- Mario rendered in-game or CJ replacement.
- RenderWare conversion of libsm64 geometry.
- GTA collision surfaces, live controls, camera, audio, and animation integration.

A successful CI build proves the library compiles and is linked. Runtime initialization still needs to be tested on the device with AML and the correct ROM.

## ROM placement

The plugin does not include a ROM. Provide your own legally obtained **Super Mario 64 US 1.0** ROM named exactly:

`sm64.us.z64`

Preferred location:

`Android/data/com.rockstargames.gtasa/files/sm64.us.z64`

The initializer also checks `/sdcard/AMLMods/GTASA/` and `/sdcard/` as fallback locations. The ROM must be the standard big-endian `.z64` format, exactly 8 MiB, and match the US 1.0 SHA-1 check.

## Build

GitHub Actions:
1. Downloads the upstream libsm64 source.
2. Runs its Mario geometry importer.
3. Builds libsm64 and the AML plugin for `armeabi-v7a` with NDK r27c.
4. Checks that key libsm64 symbols are linked.
5. Uploads `libsm64sanandreas.so`.

Artifact name:

`sm64-san-andreas-android-0.2.0-alpha-armeabi-v7a`

## Install

1. Open the repository's Actions tab: https://github.com/hesta-lang/sm64-san-andreas-android/actions
2. Open the latest successful build and download the artifact above.
3. Extract the ZIP.
4. Copy `libsm64sanandreas.so` to the game's AML mods folder:
   `Android/data/com.rockstargames.gtasa/mods/`
5. Place your ROM in the preferred location listed above.
6. Start GTA SA and inspect AML logs plus Android logcat for the tag `SM64SanAndreas`.

This build verifies libsm64 linking, initialization, Mario instance creation, and one simulation tick. **It does not display Mario in the game yet.**

## Porting plan

1. Validate AML loading and the v2.00 ABI.
2. **Integrate and initialize libsm64 on Android** (current milestone).
3. Port Mario geometry rendering to RenderWare on Android.
4. Connect Mario movement and collision to GTA SA and add mobile controls.
5. Integrate camera, audio, animations, CJ switching, and gameplay interactions.

The original PC plugin uses ASI Loader, plugin-sdk and Direct3D 9. Those parts cannot be copied unchanged to Android.
