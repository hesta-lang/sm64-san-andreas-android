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

## Install (once the build artifact exists)

1. Open the repository's **Actions** tab: https://github.com/hesta-lang/sm64-san-andreas-android/actions
2. Open the latest successful **Build Android AML plugin** run.
3. In **Artifacts**, download `sm64-san-andreas-android-0.1.0-alpha-armeabi-v7a` and extract the ZIP.
4. Copy `libsm64sanandreas.so` to the game's AML mods folder:
   `Android/data/com.rockstargames.gtasa/mods/`
   Create the `mods` folder if it does not exist. If your game installation uses a different package ID, use that package's `Android/data/<package>/mods/` folder instead.
5. Start GTA SA. Check the AML log for the tag **SM64 San Andreas** and the message **GTA SA v2.00 game-process callback reached**.

If Android's file manager blocks access to `Android/data`, use its system document picker or the internal AML directory `/sdcard/AMLMods/<game>/` if your AML build exposes it.

**Important:** this alpha only verifies plugin loading. It does not add Mario to the game yet. Do not install an ARM64 binary; this build is for `armeabi-v7a`. Back up game data before testing native mods.

## Porting plan

1. Validate AML loading and the v2.00 ABI.
2. Port libsm64 initialization and ROM loading using an Android-safe file path.
3. Replace Direct3D 9 rendering with RenderWare on Android.
4. Port Mario/CJ model substitution, animation integration, collision, audio, and mobile controls.

The original PC plugin uses ASI Loader, plugin-sdk and Direct3D 9. Those parts cannot be copied unchanged to Android.
