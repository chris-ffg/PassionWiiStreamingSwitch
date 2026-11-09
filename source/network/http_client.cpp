// ============================================================================
// DangoPlayer Switch — HTTP Client Implementation
// ============================================================================

#include "network/http_client.h"
#include "utils/logger.h"
#include <curl/curl.h>
#include <cstdio>

namespace HttpClient {

static bool sInitialized = false;

std::string SanitizeUrl(const std::string& url) {
    if (url.empty()) return url;

    // Trim whitespace
    size_t start = url.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = url.find_last_not_of(" \t\r\n");
    std::string trimmed = url.substr(start, end - start + 1);

    // Replace unencoded spaces with %20
    std::string clean;
    clean.reserve(trimmed.size() + 16);
    for (size_t i = 0; i < trimmed.size(); i++) {
        if (trimmed[i] == ' ') {
            clean += "%20";
        } else {
            clean += trimmed[i];
        }
    }
    return clean;
}

static size_t WriteStringCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t totalSize = size * nmemb;
    std::string* str = static_cast<std::string*>(userp);
    str->append(static_cast<char*>(contents), totalSize);
    return totalSize;
}

static size_t WriteFileCallback(void* ptr, size_t size, size_t nmemb, FILE* stream) {
    return fwrite(ptr, size, nmemb, stream);
}

bool Initialize() {
    if (sInitialized) return true;
    CURLcode res = curl_global_init(CURL_GLOBAL_DEFAULT);
    if (res != CURLE_OK) {
        Logger::Error("HttpClient: curl_global_init failed: %s", curl_easy_strerror(res));
        return false;
    }
    sInitialized = true;
    Logger::Info("HttpClient: Initialized with TLS/mbedtls support");
    return true;
}

void Shutdown() {
    if (sInitialized) {
        curl_global_cleanup();
        sInitialized = false;
        Logger::Info("HttpClient: Cleaned up");
    }
}

bool Get(const std::string& url, std::string& outResponse, long timeoutSec) {
    if (!sInitialized) {
        if (!Initialize()) return false;
    }

    CURL* curl = curl_easy_init();
    if (!curl) {
        Logger::Error("HttpClient: curl_easy_init failed");
        return false;
    }

    outResponse.clear();
    std::string safeUrl = SanitizeUrl(url);

    curl_easy_setopt(curl, CURLOPT_URL, safeUrl.c_str());
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "PassionWiiStreamingSwitch/1.0.0");
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeoutSec);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteStringCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &outResponse);

    // Disable strict SSL verification if system CA cert bundle is not present on Horizon OS
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

    Logger::Info("HttpClient: Fetching %s", safeUrl.c_str());
    CURLcode res = curl_easy_perform(curl);

    long httpCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);

    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        Logger::Error("HttpClient: Request failed for %s: %s", safeUrl.c_str(), curl_easy_strerror(res));
        return false;
    }

    if (httpCode >= 200 && httpCode < 300) {
        Logger::Info("HttpClient: Received %zu bytes (HTTP %ld)", outResponse.size(), httpCode);
        return true;
    } else {
        Logger::Warn("HttpClient: HTTP Error code %ld for %s", httpCode, safeUrl.c_str());
        return false;
    }
}

bool ProbeIsPlaylist(const std::string& url, long timeoutSec) {
    if (!sInitialized) {
        if (!Initialize()) return false;
    }

    std::string safeUrl = SanitizeUrl(url);
    CURL* curl = curl_easy_init();
    if (!curl) return false;

    std::string buffer;
    buffer.reserve(2048);

    curl_easy_setopt(curl, CURLOPT_URL, safeUrl.c_str());
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "PassionWiiStreamingSwitch/1.0.0");
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeoutSec);
    curl_easy_setopt(curl, CURLOPT_RANGE, "0-1023");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteStringCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

    CURLcode res = curl_easy_perform(curl);
    long httpCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);

    char* ct = nullptr;
    curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &ct);
    std::string contentType = ct ? ct : "";

    curl_easy_cleanup(curl);

    if (res != CURLE_OK && res != CURLE_PARTIAL_FILE) {
        Logger::Warn("HttpClient::ProbeIsPlaylist failed for %s: %s", safeUrl.c_str(), curl_easy_strerror(res));
        return false;
    }

    // Check Content-Type header
    bool ctPlaylist = (contentType.find("mpegurl") != std::string::npos ||
                       contentType.find("x-scpls") != std::string::npos ||
                       contentType.find("application/x-mpegURL") != std::string::npos ||
                       contentType.find("vnd.apple.mpegurl") != std::string::npos ||
                       contentType.find("text/plain") != std::string::npos);

    // Check payload magic markers (M3U or PLS)
    bool hasExtM3U = (buffer.find("#EXTM3U") != std::string::npos ||
                      buffer.find("#extm3u") != std::string::npos ||
                      buffer.find("#EXTINF") != std::string::npos);
    bool hasPLS = (buffer.find("[playlist]") != std::string::npos ||
                   buffer.find("[Playlist]") != std::string::npos);

    if (!hasExtM3U && !hasPLS && !ctPlaylist) {
        return false;
    }

    // If it's an HLS video stream, it contains EXT-X-TARGETDURATION or EXT-X-MEDIA-SEQUENCE
    if (buffer.find("#EXT-X-TARGETDURATION") != std::string::npos ||
        buffer.find("#EXT-X-MEDIA-SEQUENCE") != std::string::npos) {
        // This is a direct playable HLS media stream, not an IPTV category folder
        return false;
    }

    // If it has #EXTINF: or #EXTM3U or [playlist], it is an IPTV playlist/folder container
    return true;
}

bool DownloadFile(const std::string& url, const std::string& destPath,
                  std::function<void(double total, double current)> progressCb,
                  long timeoutSec) {
    if (!sInitialized) {
        if (!Initialize()) return false;
    }

    std::string safeUrl = SanitizeUrl(url);
    FILE* fp = fopen(destPath.c_str(), "wb");
    if (!fp) {
        Logger::Error("HttpClient: Cannot open destination file %s", destPath.c_str());
        return false;
    }

    CURL* curl = curl_easy_init();
    if (!curl) {
        fclose(fp);
        return false;
    }

    curl_easy_setopt(curl, CURLOPT_URL, safeUrl.c_str());
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "PassionWiiStreamingSwitch/1.0.0");
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeoutSec);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteFileCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

    CURLcode res = curl_easy_perform(curl);
    fclose(fp);

    long httpCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK || (httpCode < 200 || httpCode >= 300)) {
        remove(destPath.c_str());
        return false;
    }

    return true;
}

}  // namespace HttpClient
