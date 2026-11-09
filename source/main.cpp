// ============================================================================
// DangoPlayer Switch — Entry Point
// ============================================================================
// Initializes libnx services (networking for nxlink debug, romfs for assets),
// creates the App instance, and runs the main loop.
// ============================================================================

#include <switch.h>
#include "app.h"
#include "utils/logger.h"
#include "utils/usb_helper.h"

int main(int argc, char* argv[]) {
    // --- Initialize libnx services ---

    // Network socket for nxlink debugging (stdout/stderr over WiFi)
    socketInitializeDefault();
    nxlinkStdio();

    // RomFS for embedded assets (fonts, icons)
    romfsInit();

    // USB mass storage driver
    UsbHelper::Initialize();

    Logger::Info("=== DangoPlayer Switch ===");
    Logger::Info("Starting up...");

    // --- Run the application ---
    {
        App app;
        if (app.Initialize()) {
            app.Run();
        } else {
            Logger::Error("Failed to initialize application!");
        }
        app.Shutdown();
    }

    // --- Cleanup libnx services ---
    Logger::Info("Exiting...");
    UsbHelper::Shutdown();
    romfsExit();
    socketExit();

    return 0;
}
