#include "sm64_bridge.h"

#include <android/log.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <vector>

#include <libsm64.h>
#include <plugin-sdk/game_sa/plugin.h>
#include <plugin-sdk/renderware/RpAtomic.h>
#include <plugin-sdk/renderware/RpGeometry.h>
#include <plugin-sdk/renderware/RpMaterial.h>
#include <plugin-sdk/renderware/RwFrame.h>
#include <plugin-sdk/game_sa/entity/PlayerPed.h>
#include "TinySHA1.hpp"

namespace
{
    constexpr const char* kLogTag = "SM64SanAndreas";
    constexpr size_t kExpectedRomSize = 8u * 1024u * 1024u;
    constexpr uint32_t kExpectedRomSha1FirstWord = 0x9bef1128u;
    constexpr size_t kTextureSize =
        static_cast<size_t>(SM64_TEXTURE_WIDTH) *
        static_cast<size_t>(SM64_TEXTURE_HEIGHT) * 4u;
    constexpr uint32_t kMaxTriangles = SM64_GEO_MAX_TRIANGLES;
    constexpr uint32_t kMaxVertices = 3u * kMaxTriangles;
    constexpr float kMarioScale = 0.01f;

    bool g_initialized = false;
    bool g_renderLogged = false;
    uint8_t* g_marioTexture = nullptr;
    int32_t g_marioId = -1;

    std::vector<float> g_positions;
    std::vector<float> g_normals;
    std::vector<float> g_colors;
    std::vector<float> g_uvs;

    SM64MarioInputs g_inputs{};
    SM64MarioState g_state{};
    SM64MarioGeometryBuffers g_geometry{};

    RpGeometry* g_rwGeometry = nullptr;
    RpAtomic* g_rwAtomic = nullptr;
    RwFrame* g_rwFrame = nullptr;

    void DebugPrint(const char* message)
    {
        if (message && *message)
            __android_log_print(ANDROID_LOG_INFO, kLogTag, "%s", message);
    }

    uint8_t FloatToColor(float value)
    {
        if (!std::isfinite(value))
            return 255;

        value = std::max(0.0f, std::min(1.0f, value));
        return static_cast<uint8_t>(value * 255.0f + 0.5f);
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

    void DestroyRenderObjects()
    {
        if (g_rwAtomic)
        {
            RpAtomicDestroy(g_rwAtomic);
            g_rwAtomic = nullptr;
        }

        if (g_rwFrame)
        {
            RwFrameDestroy(g_rwFrame);
            g_rwFrame = nullptr;
        }

        if (g_rwGeometry)
        {
            RpGeometryDestroy(g_rwGeometry);
            g_rwGeometry = nullptr;
        }
    }

    bool CopyMarioGeometryToRenderWare()
    {
        if (!g_rwGeometry || !g_geometry.position || !g_geometry.normal ||
            !g_geometry.color || g_geometry.numTrianglesUsed == 0)
            return false;

        const RwInt32 lockFlags =
            rpGEOMETRYLOCKVERTICES |
            rpGEOMETRYLOCKNORMALS |
            rpGEOMETRYLOCKPRELIGHT;

        if (!RpGeometryLock(g_rwGeometry, lockFlags))
        {
            __android_log_print(ANDROID_LOG_ERROR, kLogTag,
                                "RpGeometryLock failed while updating Mario.");
            return false;
        }

        RpMorphTarget* morphTarget = &g_rwGeometry->morphTarget[0];
        RwV3d* vertices = morphTarget->verts;
        RwV3d* normals = morphTarget->normals;
        RwRGBA* colors = g_rwGeometry->preLitLum;

        const uint32_t usedTriangles =
            std::min<uint32_t>(g_geometry.numTrianglesUsed, kMaxTriangles);

        for (uint32_t triangle = 0; triangle < kMaxTriangles; ++triangle)
        {
            for (uint32_t corner = 0; corner < 3; ++corner)
            {
                const uint32_t vertexIndex = triangle * 3u + corner;
                const uint32_t component = vertexIndex * 3u;

                if (triangle < usedTriangles)
                {
                    // libsm64 emits world-space vertices. Convert them to a
                    // local, right-handed RenderWare model (GTA uses Z-up).
                    const float x = g_positions[component + 0] - g_state.position[0];
                    const float y = g_positions[component + 1] - g_state.position[1];
                    const float z = g_positions[component + 2] - g_state.position[2];

                    vertices[vertexIndex].x = x * kMarioScale;
                    vertices[vertexIndex].y = -z * kMarioScale;
                    vertices[vertexIndex].z = y * kMarioScale;

                    const float nx = g_normals[component + 0];
                    const float ny = g_normals[component + 1];
                    const float nz = g_normals[component + 2];

                    normals[vertexIndex].x = nx;
                    normals[vertexIndex].y = -nz;
                    normals[vertexIndex].z = ny;

                    colors[vertexIndex].r = FloatToColor(g_colors[component + 0]);
                    colors[vertexIndex].g = FloatToColor(g_colors[component + 1]);
                    colors[vertexIndex].b = FloatToColor(g_colors[component + 2]);
                    colors[vertexIndex].a = 255;
                }
                else
                {
                    // Unused capacity becomes degenerate triangles at origin.
                    vertices[vertexIndex].x = 0.0f;
                    vertices[vertexIndex].y = 0.0f;
                    vertices[vertexIndex].z = 0.0f;
                    normals[vertexIndex].x = 0.0f;
                    normals[vertexIndex].y = 0.0f;
                    normals[vertexIndex].z = 1.0f;
                    colors[vertexIndex].r = 255;
                    colors[vertexIndex].g = 255;
                    colors[vertexIndex].b = 255;
                    colors[vertexIndex].a = 255;
                }
            }
        }

        RpGeometryUnlock(g_rwGeometry);

        RwSphere bounds{};
        RpMorphTargetCalcBoundingSphere(morphTarget, &bounds);
        morphTarget->boundingSphere = bounds;
        if (g_rwAtomic)
            g_rwAtomic->boundingSphere = bounds;

        return true;
    }

    bool CreateRenderObjects()
    {
        if (g_rwAtomic)
            return true;

        g_rwGeometry = RpGeometryCreate(
            static_cast<RwInt32>(kMaxVertices),
            static_cast<RwInt32>(kMaxTriangles),
            rpGEOMETRYPOSITIONS | rpGEOMETRYNORMALS | rpGEOMETRYPRELIT);

        if (!g_rwGeometry)
        {
            __android_log_print(ANDROID_LOG_ERROR, kLogTag,
                                "RpGeometryCreate failed for Mario.");
            DestroyRenderObjects();
            return false;
        }

        RpMaterial* material = RpMaterialCreate();
        if (!material)
        {
            __android_log_print(ANDROID_LOG_ERROR, kLogTag,
                                "RpMaterialCreate failed for Mario.");
            DestroyRenderObjects();
            return false;
        }

        CRGBA materialColor(255, 255, 255, 255);
        RpMaterialSetColor(material, &materialColor);

        if (_rpMaterialListAppendMaterial(&g_rwGeometry->matList, material) < 0)
        {
            RpMaterialDestroy(material);
            __android_log_print(ANDROID_LOG_ERROR, kLogTag,
                                "Could not attach the Mario material to geometry.");
            DestroyRenderObjects();
            return false;
        }

        if (!RpGeometryLock(g_rwGeometry,
                            rpGEOMETRYLOCKPOLYGONS |
                            rpGEOMETRYLOCKVERTICES |
                            rpGEOMETRYLOCKNORMALS |
                            rpGEOMETRYLOCKPRELIGHT))
        {
            __android_log_print(ANDROID_LOG_ERROR, kLogTag,
                                "Initial RpGeometryLock failed for Mario.");
            DestroyRenderObjects();
            return false;
        }

        for (uint32_t triangle = 0; triangle < kMaxTriangles; ++triangle)
        {
            RpTriangle& rwTriangle = g_rwGeometry->triangles[triangle];
            rwTriangle.vertIndex[0] = static_cast<RwUInt16>(triangle * 3u);
            rwTriangle.vertIndex[1] = static_cast<RwUInt16>(triangle * 3u + 1u);
            rwTriangle.vertIndex[2] = static_cast<RwUInt16>(triangle * 3u + 2u);
            rwTriangle.matIndex = 0;
        }

        RpGeometryUnlock(g_rwGeometry);

        // The first libsm64 tick is available before the first render call.
        if (!CopyMarioGeometryToRenderWare())
        {
            __android_log_print(ANDROID_LOG_ERROR, kLogTag,
                                "Could not upload the first Mario geometry frame.");
            DestroyRenderObjects();
            return false;
        }

        g_rwFrame = RwFrameCreate();
        g_rwAtomic = RpAtomicCreate();
        if (!g_rwFrame || !g_rwAtomic)
        {
            __android_log_print(ANDROID_LOG_ERROR, kLogTag,
                                "Could not create Mario's RenderWare frame/atomic.");
            DestroyRenderObjects();
            return false;
        }

        if (!RpAtomicSetGeometry(g_rwAtomic, g_rwGeometry, 0) ||
            !RpAtomicSetFrame(g_rwAtomic, g_rwFrame))
        {
            __android_log_print(ANDROID_LOG_ERROR, kLogTag,
                                "Could not bind Mario geometry to its RenderWare atomic.");
            DestroyRenderObjects();
            return false;
        }

        RpAtomicSetRenderCallBack(g_rwAtomic, nullptr);
        __android_log_print(ANDROID_LOG_INFO, kLogTag,
                            "Mario RenderWare model created (%u vertices, %u triangles capacity).",
                            kMaxVertices, kMaxTriangles);
        return true;
    }

    void ReleaseRuntime()
    {
        DestroyRenderObjects();

        if (g_marioId >= 0)
        {
            sm64_mario_delete(g_marioId);
            g_marioId = -1;
        }

        sm64_global_terminate();
        g_initialized = false;
        g_renderLogged = false;
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
        __android_log_print(ANDROID_LOG_WARN, kLogTag,
                            "SM64 ROM not found in configured paths.");
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
        __android_log_print(ANDROID_LOG_ERROR, kLogTag,
                            "SM64 ROM size/seek validation failed.");
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
        __android_log_print(ANDROID_LOG_ERROR, kLogTag,
                            "SM64 ROM is not big-endian .z64 format.");
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

    g_marioId = sm64_mario_create(0.0f, 0.0f, 0.0f);
    if (g_marioId < 0)
    {
        ReleaseRuntime();
        return SM64BridgeResult::InitializationFailed;
    }

    sm64_mario_tick(g_marioId, &g_inputs, &g_state, &g_geometry);
    g_initialized = true;

    __android_log_print(ANDROID_LOG_INFO, kLogTag,
                        "libsm64 initialized; Mario instance %d completed its first simulation tick with %u triangles.",
                        g_marioId, static_cast<unsigned int>(g_geometry.numTrianglesUsed));
    return SM64BridgeResult::Initialized;
}

bool SM64Bridge::IsInitialized()
{
    return g_initialized;
}

void SM64Bridge::Update()
{
    if (!g_initialized || g_marioId < 0)
        return;

    // Until GTA collision surfaces and input are connected, keep the visual
    // libsm64 instance at its local origin instead of letting it fall away.
    sm64_set_mario_position(g_marioId, 0.0f, 0.0f, 0.0f);
    sm64_mario_tick(g_marioId, &g_inputs, &g_state, &g_geometry);
}

void SM64Bridge::Render()
{
    if (!g_initialized || g_geometry.numTrianglesUsed == 0)
        return;

    CPlayerPed* player = FindPlayerPed(0);
    if (!player)
        return;

    if (!g_rwAtomic && !CreateRenderObjects())
        return;

    if (!CopyMarioGeometryToRenderWare())
        return;

    RwFrameSetIdentity(g_rwFrame);
    RwMatrix* matrix = RwFrameGetMatrix(g_rwFrame);
    const CVector& playerPosition = player->GetPosition();
    matrix->pos.x = playerPosition.x;
    matrix->pos.y = playerPosition.y;
    matrix->pos.z = playerPosition.z;
    RwFrameUpdateObjects(g_rwFrame);

    RpAtomicRender(g_rwAtomic);

    if (!g_renderLogged)
    {
        g_renderLogged = true;
        __android_log_print(ANDROID_LOG_INFO, kLogTag,
                            "Mario RenderWare draw callback executed at the local player's position.");
    }
}

void SM64Bridge::ShutdownRenderWare()
{
    DestroyRenderObjects();
    g_renderLogged = false;
}

void SM64Bridge::Shutdown()
{
    if (!g_initialized && !g_marioTexture && !g_rwAtomic && !g_rwGeometry && !g_rwFrame)
        return;

    ReleaseRuntime();
    __android_log_print(ANDROID_LOG_INFO, kLogTag,
                        "libsm64 and RenderWare runtime terminated.");
}
