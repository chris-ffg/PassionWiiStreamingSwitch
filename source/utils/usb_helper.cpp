// ============================================================================
// DangoPlayer Switch — USB Storage Helper Implementation
// ============================================================================

#include "utils/usb_helper.h"
#include "utils/logger.h"
#include <sys/stat.h>

#if __has_include(<usbhsfs.h>)
#include <usbhsfs.h>
#define HAS_LIBUSBHSFS 1
#else
#define HAS_LIBUSBHSFS 0
#endif

namespace UsbHelper {

static bool sInitialized = false;

bool Initialize() {
    if (sInitialized) return true;

#if HAS_LIBUSBHSFS
    Result rc = usbhsfsInitialize(0);
    if (R_FAILED(rc)) {
        Logger::Warn("UsbHelper: usbhsfsInitialize failed (0x%x)", rc);
    } else {
        Logger::Info("UsbHelper: libusbhsfs initialized successfully");
    }
#else
    Logger::Info("UsbHelper: Initialized with virtual UMS mount detector");
#endif

    sInitialized = true;
    return true;
}

void Shutdown() {
    if (!sInitialized) return;

#if HAS_LIBUSBHSFS
    usbhsfsExit();
#endif
    sInitialized = false;
    Logger::Info("UsbHelper: Cleaned up");
}

bool IsMounted(const std::string& mountPoint) {
    struct stat st;
    return (stat(mountPoint.c_str(), &st) == 0);
}

std::vector<StorageDevice> GetAvailableDevices() {
    std::vector<StorageDevice> devices;

    // SD Card is always primary
    StorageDevice sd;
    sd.mountPoint = "sdmc:/";
    sd.label = "Tarjeta SD (sdmc:)";
    sd.isUsb = false;
    devices.push_back(sd);

    // Check USB Mass Storage devices (ums0: to ums3:)
    const char* umsPoints[] = {"ums0:/", "ums1:/", "ums2:/", "ums3:/"};
    for (int i = 0; i < 4; i++) {
        if (IsMounted(umsPoints[i])) {
            StorageDevice usb;
            usb.mountPoint = umsPoints[i];
            usb.label = "Unidad USB " + std::to_string(i + 1) + " (" + std::string(umsPoints[i]) + ")";
            usb.isUsb = true;
            devices.push_back(usb);
        }
    }

    return devices;
}

}  // namespace UsbHelper
