#include <mod/amlmod.h>
#include <mod/logger.h>
#include <plugin-sdk/game_sa/Events.h>

#include "sm64_bridge.h"

MYMOD(hesta.sm64sanandreas.android, SM64 San Andreas Android, 0.3.0-alpha, hesta-lang)

ON_MOD_LOAD()
{
    logger->SetTag("SM64 San Andreas");
    logger->Info("SM64 San Andreas Android 0.3.0-alpha: AML plugin loaded.");

    static bool initializationAttempted = false;

    Events::gameProcessEvent += []()
    {
        if (!initializationAttempted)
        {
            initializationAttempted = true;
            logger->Info("GTA SA game-process callback reached; initializing libsm64.");

            const SM64BridgeResult result = SM64Bridge::Initialize();
            switch (result)
            {
                case SM64BridgeResult::Initialized:
                    logger->Info("libsm64 initialized; Mario geometry is ready for RenderWare.");
                    break;
                case SM64BridgeResult::RomNotFound:
                    logger->Info("SM64 ROM not found. Place sm64.us.z64 in Android/data/com.rockstargames.gtasa/files/.");
                    break;
                case SM64BridgeResult::InvalidRom:
                    logger->Error("SM64 ROM rejected: expected the 8 MiB big-endian US 1.0 ROM.");
                    break;
                case SM64BridgeResult::AlreadyInitialized:
                    logger->Info("libsm64 was already initialized.");
                    break;
                case SM64BridgeResult::InitializationFailed:
                    logger->Error("libsm64 initialization failed.");
                    break;
            }
            return;
        }

        SM64Bridge::Update();
    };

    Events::renderEffectsEvent += []()
    {
        SM64Bridge::Render();
    };

    Events::shutdownRwEvent += []()
    {
        SM64Bridge::ShutdownRenderWare();
    };
}
