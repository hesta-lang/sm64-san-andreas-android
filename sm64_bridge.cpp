#include "sm64_bridge.h"

#include <android/log.h>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <vector>

#include <libsm64.h>
#include "TinySHA1.hpp"

namespace
{
    constexpr const char* kLogTag = "SM64SanAndreas";
    constexpr size_t kExpectedRomSize = 8u * 1024u * 1024u;
    constexpr uint32_t kExpectedRomSha1FirstWord = 0x9bef1128u;
    constexpr size_t kTextureSize =
        static_cast<size_t>(SM64_TEXTURE_WIDTH) *
        static_cast<size_t>(SM64_TEXTURE_HEIGHT) * 4u;

    bool g_initialized = false;
    uint8_t* g_marioTexture = nullptr;
    int32_t g_marioId = -1;

    std::vector<float> g_positions;
    std::vector<float> g_normals;
    std::vector<float> g_colors;
    std::vector<float> g_uvs;

    SM64MarioInputs g_inputs{};
    SM64MarioState g_state{};
    SM64MarioGeometryBuffers g_geometry{};

    void DebugPrint(const char* message)
    {
        if (message && *message)
            __android_log_print(ANDROID_LOG_INFO, kLogTag, "%s", message);
    }

    FILE* OpenRom()
    {
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

    void ReleaseRuntime()
    {
        if (g_marioId >= 0)
        {
            sm64_mario_delete(g_marioId);
            g_marioId = -1;
        }

        sm64_global_terminate();
        g_initialized = false;
        std::free(g_marioTexture);
        g_marioTexture = nullptr;

        g_positions.clear();
        g_normals.clear();
        g_colors.clear();
        g_uvs.clear();
        g_geometry = {};
        g_state = {};
        g_inputs = {};
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

    const uint8_t z64Magic[4] = { 0x80, 0x37, 0x12, 0x40 };
    if (std::memcmp(rom.data(), z64Magic, sizeof(z64Magic)) != 0)
    {
        __android_log_print(ANDROID_LOG_ERROR, kLogTag, "SM64 ROM is not big-endian .z64 format.");
        return SM64BridgeResult::InvalidRom;
    }

    sha1::SHA1 sha;
    uint32_t digest[5]{};
    sha.processBytes(rom.data(), rom.size());
    sha.getDigest(digest);
    if (digest[0] != kExpectedRomSha1FirstWord)
    {
        __android_log_print(ANDROID_LOG_ERROR, kLogTag,
                            "ROM SHA-1 mismatch; expected Super Mario 64 US 1.0.");
        return SM64BridgeResult::InvalidRom;
    }

    g_marioTexture = static_cast<uint8_t*>(std::malloc(kTextureSize));
    if (!g_marioTexture)
        return SM64BridgeResult::InitializationFailed;

    sm64_register_debug_print_function(DebugPrint);
    sm64_global_init(rom.data(), g_marioTexture);

    g_positions.resize(9u * SM64_GEO_MAX_TRIANGLES);
    g_normals.resize(9u * SM64_GEO_MAX_TRIANGLES);
    g_colors.resize(9u * SM64_GEO_MAX_TRIANGLES);
    g_uvs.resize(6u * SM64_GEO_MAX_TRIANGLES);
    g_geometry.position = g_positions.data();
    g_geometry.normal = g_normals.data();
    g_geometry.color = g_colors.data();
    g_geometry.uv = g_uvs.data();
    g_geometry.numTrianglesUsed = 0;

    // Smoke-test the real Mario simulation API; RenderWare and GTA collision
    // are intentionally not enabled until their conversion layer is ready.
    g_marioId = sm64_mario_create(0.0f, 1000.0f, 0.0f);
    if (g_marioId < 0)
    {
        ReleaseRuntime();
        return SM64BridgeResult::InitializationFailed;
    }

    sm64_mario_tick(g_marioId, &g_inputs, &g_state, &g_geometry);
    g_initialized = true;

    __android_log_print(ANDROID_LOG_INFO, kLogTag,
                        "libsm64 initialized; Mario instance %d completed its first simulation tick.",
                        g_marioId);
    return SM64BridgeResult::Initialized;
}

bool SM64Bridge::IsInitialized()
{
    return g_initialized;
}

void SM64Bridge::Shutdown()
{
    if (!g_initialized && !g_marioTexture)
        return;

    ReleaseRuntime();
    __android_log_print(ANDROID_LOG_INFO, kLogTag, "libsm64 runtime terminated.");
}
