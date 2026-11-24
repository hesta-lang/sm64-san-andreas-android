#include "sm64_bridge.h"

#include <android/log.h>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <vector>

#include <libsm64.h>

namespace
{
    constexpr const char* kLogTag = "SM64SanAndreas";
    constexpr size_t kExpectedRomSize = 8u * 1024u * 1024u;
    constexpr size_t kTextureSize =
        static_cast<size_t>(SM64_TEXTURE_WIDTH) *
        static_cast<size_t>(SM64_TEXTURE_HEIGHT) * 4u;

    bool g_initialized = false;
    uint8_t* g_marioTexture = nullptr;

    void DebugPrint(const char* message)
    {
        if (message && *message)
            __android_log_print(ANDROID_LOG_INFO, kLogTag, "%s", message);
    }

    FILE* OpenRom()
    {
        // The first path is inside the game's own app-specific external files
        // directory, which the game process can access without broad storage access.
        const char* paths[] = {
            "/sdcard/Android/data/com.rockstargames.gtasa/files/sm64.us.z64",
            "/storage/emulated/0/Android/data/com.rockstargames.gtasa/files/sm64.us.z64",
            "/sdcard/AMLMods/GTASA/sm64.us.z64",
            "/sdcard/sm64.us.z64"
        };

        for (const char* path : paths)
        {
            FILE* file = std::fopen(path, "rb");
            if (file)
            {
                __android_log_print(ANDROID_LOG_INFO, kLogTag,
                                    "Found SM64 ROM at %s", path);
                return file;
            }
        }
        return nullptr;
    }
}

SM64BridgeResult SM64Bridge::Initialize()
{
    __android_log_print(ANDROID_LOG_INFO, kLogTag, "libsm64 initialization requested.");
    if (g_initialized)
        return SM64BridgeResult::AlreadyInitialized;

    FILE* romFile = OpenRom();
    if (!romFile)
    {
        __android_log_print(ANDROID_LOG_WARN, kLogTag, "SM64 ROM not found in configured paths.");
        return SM64BridgeResult::RomNotFound;
    }

    if (std::fseek(romFile, 0, SEEK_END) != 0)
    {
        std::fclose(romFile);
        return SM64BridgeResult::InvalidRom;
    }

    const long romLength = std::ftell(romFile);
    if (romLength != static_cast<long>(kExpectedRomSize) ||
        std::fseek(romFile, 0, SEEK_SET) != 0)
    {
        __android_log_print(ANDROID_LOG_ERROR, kLogTag, "SM64 ROM size/seek validation failed.");
        std::fclose(romFile);
        return SM64BridgeResult::InvalidRom;
    }

    std::vector<uint8_t> rom(kExpectedRomSize);
    if (std::fread(rom.data(), 1, rom.size(), romFile) != rom.size())
    {
        std::fclose(romFile);
        return SM64BridgeResult::InvalidRom;
    }
    std::fclose(romFile);

    // libsm64 expects the standard big-endian .z64 byte order.
    const uint8_t z64Magic[4] = { 0x80, 0x37, 0x12, 0x40 };
    if (std::memcmp(rom.data(), z64Magic, sizeof(z64Magic)) != 0)
    {
        __android_log_print(ANDROID_LOG_ERROR, kLogTag, "SM64 ROM is not big-endian .z64 format.");
        return SM64BridgeResult::InvalidRom;
    }

    g_marioTexture = static_cast<uint8_t*>(std::malloc(kTextureSize));
    if (!g_marioTexture)
        return SM64BridgeResult::InitializationFailed;

    sm64_register_debug_print_function(DebugPrint);
    sm64_global_init(rom.data(), g_marioTexture);

    g_initialized = true;
    __android_log_print(ANDROID_LOG_INFO, kLogTag,
                        "libsm64 global initialization completed (ARMv7).");
    return SM64BridgeResult::Initialized;
}

bool SM64Bridge::IsInitialized()
{
    return g_initialized;
}

void SM64Bridge::Shutdown()
{
    if (!g_initialized)
        return;

    sm64_global_terminate();
    g_initialized = false;

    std::free(g_marioTexture);
    g_marioTexture = nullptr;

    __android_log_print(ANDROID_LOG_INFO, kLogTag,
                        "libsm64 global runtime terminated.");
}
