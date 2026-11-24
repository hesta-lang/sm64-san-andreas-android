#include <mod/amlmod.h>
#include <mod/logger.h>
#include <plugin-sdk/game_sa/Events.h>

#include "sm64_bridge.h"

MYMOD(hesta.sm64sanandreas.android, SM64 San Andreas Android, 0.2.0-alpha, hesta-lang)

ON_MOD_LOAD()
{
    logger->SetTag("SM64 San Andreas");
    logger->Info("SM64 San Andreas Android 0.2.0-alpha: AML plugin loaded.");
    logger->Info("libsm64 Android ARMv7 integration is present.");

    Events::gameProcessEvent += []()
    {
        static bool reported = false;
        if (!reported)
        {
            reported = true;
            logger->Info("GTA SA v2.00 game-process callback reached.");
            const SM64BridgeResult result = SM64Bridge::Initialize();
            switch (result)
            {
                case SM64BridgeResult::Initialized:
                    logger->Info("libsm64 initialized successfully. Mario runtime is ready.");
                    break;
                case SM64BridgeResult::RomNotFound:
                    logger->Warn("libsm64 is built in, but sm64.us.z64 was not found. Place your legally obtained US ROM in Android/data/com.rockstargames.gtasa/files/.");
                    break;
                case SM64BridgeResult::InvalidRom:
                    logger->Error("SM64 ROM rejected: expected an 8 MiB big-endian US .z64 ROM.");
                    break;
                case SM64BridgeResult::AlreadyInitialized:
                    logger->Info("libsm64 was already initialized.");
                    break;
                case SM64BridgeResult::InitializationFailed:
                    logger->Error("libsm64 initialization failed.");
                    break;
            }
        }
    };
}
