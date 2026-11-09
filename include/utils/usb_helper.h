// ============================================================================
// DangoPlayer Switch — USB Storage Helper (libusbhsfs integration)
// ============================================================================
// Handles detection and mounting of external USB drives (FAT32, exFAT, NTFS, EXT)
// connected to the Switch dock or USB-C port via OTG.
// ============================================================================

#pragma once

#include <string>
#include <vector>

struct StorageDevice {
    std::string mountPoint; // e.g. "sdmc:/", "ums0:/"
    std::string label;      // e.g. "Tarjeta SD", "USB 1 (FAT32)"
    bool isUsb = false;
};

namespace UsbHelper {
    /// Initialize the USB mass storage service
    bool Initialize();

    /// Unmount and cleanup USB service
    void Shutdown();

    /// Retrieve list of all available storage devices (always includes sdmc:/)
    std::vector<StorageDevice> GetAvailableDevices();

    /// Check if a given mount point is currently mounted and accessible
    bool IsMounted(const std::string& mountPoint);
}
