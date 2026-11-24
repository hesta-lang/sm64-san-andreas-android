#include <mod/amlmod.h>
#include <mod/logger.h>
#include <plugin-sdk/game_sa/Events.h>

MYMOD(hesta.sm64sanandreas.android, SM64 San Andreas Android, 0.1.0-alpha, hesta-lang)

ON_MOD_LOAD()
{
    logger->SetTag("SM64 San Andreas");
    logger->Info("SM64 San Andreas Android 0.1.0-alpha: AML plugin loaded.");

    // Harmless callback used only to confirm that the GTA SA v2.00 game loop is reachable.
    Events::gameProcessEvent += []()
    {
        static bool reported = false;
        if (!reported)
        {
            reported = true;
            logger->Info("GTA SA v2.00 game-process callback reached.");
        }
    };
}
