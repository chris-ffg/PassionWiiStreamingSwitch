// ============================================================================
// DangoPlayer Switch — Playlist Parser (M3U / M3U8 / PLS)
// ============================================================================

#pragma once

#include <string>
#include <vector>

/// Classification of playlist entries: playable stream vs hierarchical container
enum class PlaylistItemType {
    MediaStream,     // Direct video/audio stream (.ts, .mp4, live HLS m3u8, etc.)
    FolderContainer  // Sub-playlist, category or nested directory (.m3u, Xtream category, etc.)
};

struct ChannelItem {
    std::string name;
    std::string url;
    std::string groupTitle = "General";
    std::string logoUrl;
    std::string tvgId;
    int duration = -1;  // -1 indicates a live TV stream or indefinite container
    PlaylistItemType type = PlaylistItemType::MediaStream;
};

struct Playlist {
    std::string name;
    std::string source;
    std::vector<ChannelItem> channels;
    std::vector<std::string> groups;  // Unique categories ("Todos", "Noticias", etc.)

    /// Return channels filtered by category group and optional search text
    std::vector<ChannelItem> GetFilteredChannels(const std::string& group = "Todos",
                                                const std::string& searchQuery = "") const;
};

namespace PlaylistParser {
    /// Detect whether an entry is a media stream or a folder/sub-playlist container
    PlaylistItemType DetectItemType(const std::string& extInfLine, const std::string& url);

    /// Resolve relative playlist URLs against base playlist URL/path
    std::string ResolveUrl(const std::string& baseUrl, const std::string& targetUrl);

    /// Parse an M3U/M3U8 string
    bool ParseM3U(const std::string& content, Playlist& outPlaylist, const std::string& baseUrl = "");

    /// Parse a PLS playlist string
    bool ParsePLS(const std::string& content, Playlist& outPlaylist, const std::string& baseUrl = "");

    /// Parse a WiiMC / Navi-X / PLX key-value playlist (used by PassionWii news/novedades)
    bool ParseWiiMC(const std::string& content, Playlist& outPlaylist, const std::string& baseUrl = "");

    /// Load and parse a playlist from a local file (.m3u, .m3u8, .pls)
    bool LoadFromFile(const std::string& filePath, Playlist& outPlaylist);

    /// Download and parse a playlist from a remote HTTP/HTTPS URL
    bool LoadFromUrl(const std::string& url, Playlist& outPlaylist);
}
