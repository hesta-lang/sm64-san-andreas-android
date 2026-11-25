#pragma once

enum class SM64BridgeResult
{
    Initialized,
    AlreadyInitialized,
    RomNotFound,
    InvalidRom,
    InitializationFailed
};

namespace SM64Bridge
{
    SM64BridgeResult Initialize();
    bool IsInitialized();
    void Update();
    void Render();
    void ShutdownRenderWare();
    void Shutdown();
}
