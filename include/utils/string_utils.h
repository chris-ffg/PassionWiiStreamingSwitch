// ============================================================================
// DangoPlayer Switch — String Utilities
// ============================================================================

#pragma once

#include <string>
#include <vector>

namespace StringUtils {
    /// Convert string to lowercase.
    std::string ToLower(const std::string& str);

    /// Extract file extension (including dot, e.g., ".mp4"). Returns lowercase.
    std::string GetExtension(const std::string& path);

    /// Extract filename from a full path (e.g., "sdmc:/videos/movie.mp4" -> "movie.mp4").
    std::string GetFilename(const std::string& path);

    /// Extract parent directory path (e.g., "sdmc:/videos/sub" -> "sdmc:/videos").
    std::string GetParentDirectory(const std::string& path);

    /// Format byte size to human-readable string (e.g. "4.2 MB", "1.35 GB").
    std::string FormatBytes(uint64_t bytes);

    /// Format seconds to mm:ss or hh:mm:ss string.
    std::string FormatTime(double seconds);

    /// Truncate string with ellipsis ("...") if it exceeds maxChars.
    std::string Truncate(const std::string& str, size_t maxChars);

    /// Decode percent-encoded characters (e.g. "%20" -> " ", "%28" -> "(", "%29" -> ")").
    std::string UrlDecode(const std::string& str);
}
