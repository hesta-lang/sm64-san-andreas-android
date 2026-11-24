# SM64 San Andreas — Android AML port

Port of [sm64-san-andreas](https://github.com/headshot2017/sm64-san-andreas) for GTA: San Andreas Android using Android Mod Loader (AML).

## Target

- GTA: San Andreas Android **v2.00 (32-bit / armeabi-v7a)**
- Android NDK r27c
- C++17
- AML + the Android GTA SA Plugin SDK

GTA SA v2.00 is the 32-bit target. The separate v2.10 release is 64-bit; this project does not build an arm64 binary for v2.00.

## Current status — 0.1.0-alpha

This is the first compatibility milestone:
- Builds an AML plugin for the v2.00 ABI.
- Registers a harmless game-process callback and writes a load confirmation to the AML log.
- Does **not** yet initialize libsm64, render Mario, replace CJ, or implement controls.

A successful CI build only proves that the native plugin compiles. In-game loading must still be verified on a device with GTA SA v2.00 and AML installed.

## Build

GitHub Actions builds the plugin on pushes, pull requests, and manual runs. The artifact is:

`libsm64sanandreas.so`

Install it in the AML mods directory for the GTA SA package used by your installation:

`Android/data/<game-package>/mods/`

Use only the `armeabi-v7a` binary with GTA SA v2.00. Back up game data before testing native mods.

## Porting plan

1. Validate AML loading and the v2.00 ABI.
2. Port libsm64 initialization and ROM loading using an Android-safe file path.
3. Replace Direct3D 9 rendering with RenderWare on Android.
4. Port Mario/CJ model substitution, animation integration, collision, audio, and mobile controls.

The original PC plugin uses ASI Loader, plugin-sdk and Direct3D 9. Those parts cannot be copied unchanged to Android.
