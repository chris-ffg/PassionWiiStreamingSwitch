// ============================================================================
// DangoPlayer Switch — HTTP Client
// ============================================================================
// Network fetcher based on libcurl for downloading remote playlists, channel
// logos, and verifying stream reachability.
// ============================================================================

#pragma once

#include <string>
#include <functional>

namespace HttpClient {
    /// Initialize the curl subsystem
    bool Initialize();

    /// Cleanup the curl subsystem
    void Shutdown();

    /// Sanitize URL (encode spaces to %20, trim whitespace)
    std::string SanitizeUrl(const std::string& url);

    /// Download a remote URL into a string. Returns true on HTTP 200 OK.
    bool Get(const std::string& url, std::string& outResponse, long timeoutSec = 15);

    /// Probe whether a URL points to an M3U/PLS playlist container rather than a media stream
    bool ProbeIsPlaylist(const std::string& url, long timeoutSec = 4);

    /// Download a remote file to a destination path on SD card.
    bool DownloadFile(const std::string& url, const std::string& destPath,
                      std::function<void(double total, double current)> progressCb = nullptr,
                      long timeoutSec = 30);
}
