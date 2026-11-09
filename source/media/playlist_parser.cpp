// ============================================================================
// DangoPlayer Switch — Playlist Parser Implementation
// ============================================================================

#include "media/playlist_parser.h"
#include "network/http_client.h"
#include "utils/string_utils.h"
#include "utils/logger.h"
#include <sstream>
#include <fstream>
#include <set>
#include <algorithm>

static std::string ExtractAttribute(const std::string& line, const std::string& attr) {
    // 1. Try double quotes attr="..."
    std::string needleDouble = attr + "=\"";
    size_t start = line.find(needleDouble);
    if (start != std::string::npos) {
        start += needleDouble.length();
        size_t end = line.find('"', start);
        if (end != std::string::npos) {
            return line.substr(start, end - start);
        }
    }

    // 2. Try single quotes attr='...'
    std::string needleSingle = attr + "=\'";
    start = line.find(needleSingle);
    if (start != std::string::npos) {
        start += needleSingle.length();
        size_t end = line.find('\'', start);
        if (end != std::string::npos) {
            return line.substr(start, end - start);
        }
    }

    // 3. Try case-insensitive attribute match
    std::string lineLower = StringUtils::ToLower(line);
    std::string attrLower = StringUtils::ToLower(attr) + "=\"";
    start = lineLower.find(attrLower);
    if (start != std::string::npos) {
        start += attrLower.length();
        size_t end = line.find('"', start);
        if (end != std::string::npos) {
            return line.substr(start, end - start);
        }
    }

    return "";
}

std::vector<ChannelItem> Playlist::GetFilteredChannels(const std::string& group,
                                                      const std::string& searchQuery) const {
    std::vector<ChannelItem> result;
    std::string queryLower = StringUtils::ToLower(searchQuery);
    bool filterGroup = (!group.empty() && group != "Todos");

    for (const auto& ch : channels) {
        if (filterGroup && ch.groupTitle != group) {
            continue;
        }

        if (!queryLower.empty()) {
            std::string nameLower = StringUtils::ToLower(ch.name);
            if (nameLower.find(queryLower) == std::string::npos) {
                continue;
            }
        }

        result.push_back(ch);
    }
    return result;
}

namespace PlaylistParser {

PlaylistItemType DetectItemType(const std::string& extInfLine, const std::string& url) {
    std::string extLower = StringUtils::ToLower(StringUtils::GetExtension(url));
    std::string urlLower = StringUtils::ToLower(url);
    std::string lineLower = StringUtils::ToLower(extInfLine);

    // 1. Explicit metadata container flags in #EXTINF tag
    if (lineLower.find("is-container=\"true\"") != std::string::npos ||
        lineLower.find("is-container=\"1\"") != std::string::npos ||
        lineLower.find("is-folder=\"true\"") != std::string::npos ||
        lineLower.find("is-folder=\"1\"") != std::string::npos ||
        lineLower.find("folder=\"true\"") != std::string::npos ||
        lineLower.find("type=\"playlist\"") != std::string::npos ||
        lineLower.find("type=\"folder\"") != std::string::npos ||
        lineLower.find("type=\"category\"") != std::string::npos ||
        lineLower.find("type=\"sublist\"") != std::string::npos) {
        return PlaylistItemType::FolderContainer;
    }

    // 2. Folder indicators in item title or group-title
    if (lineLower.find("[carpeta]") != std::string::npos ||
        lineLower.find("[folder]") != std::string::npos ||
        lineLower.find("[dir]") != std::string::npos ||
        lineLower.find("[cat]") != std::string::npos ||
        lineLower.find("[lista]") != std::string::npos ||
        lineLower.find("(carpeta)") != std::string::npos ||
        lineLower.find("(folder)") != std::string::npos ||
        lineLower.find("group-title=\"carpetas\"") != std::string::npos ||
        lineLower.find("group-title=\"[carpetas]\"") != std::string::npos) {
        return PlaylistItemType::FolderContainer;
    }

    // 3. Known direct media stream extensions (high priority: definitely playable media files)
    static const std::set<std::string> mediaExtensions = {
        ".mp4", ".mkv", ".ts", ".avi", ".flv", ".mov", ".wmv",
        ".webm", ".m4v", ".3gp", ".mpg", ".mpeg", ".vob",
        ".mp3", ".aac", ".flac", ".ogg", ".wav", ".m4a", ".opus", ".ac3"
    };
    if (mediaExtensions.find(extLower) != mediaExtensions.end()) {
        return PlaylistItemType::MediaStream;
    }

    // 4. Live streaming protocols
    if (urlLower.rfind("rtsp://", 0) == 0 ||
        urlLower.rfind("rtmp://", 0) == 0 ||
        urlLower.rfind("udp://", 0) == 0 ||
        urlLower.rfind("rtp://", 0) == 0 ||
        urlLower.rfind("mms://", 0) == 0) {
        return PlaylistItemType::MediaStream;
    }

    // 5. Playlist container extensions (.m3u, .pls, .xspf)
    if (extLower == ".m3u" || extLower == ".pls" || extLower == ".xspf") {
        return PlaylistItemType::FolderContainer;
    }

    // 6. Query parameters designating playlist downloads/indexes
    if (urlLower.find("type=m3u") != std::string::npos ||
        urlLower.find("output=m3u") != std::string::npos ||
        urlLower.find("output=m3u8") != std::string::npos ||
        urlLower.find("action=get_live_categories") != std::string::npos ||
        urlLower.find("action=get_vod_categories") != std::string::npos ||
        urlLower.find("action=get_series_categories") != std::string::npos) {
        return PlaylistItemType::FolderContainer;
    }

    // 7. Passion Wii Streaming specific domain detection
    // In PassionWii, all category and sub-playlist endpoints are hosted on
    // passionstreamingwii / myartsonline / listas-iptv-wii and do not have video file extensions.
    if (urlLower.find("passionstreamingwii") != std::string::npos ||
        urlLower.find("myartsonline.com") != std::string::npos ||
        urlLower.find("listas-iptv-wii") != std::string::npos) {
        return PlaylistItemType::FolderContainer;
    }

    // 8. Category / Section title keywords when URL has NO video file extension
    // Entries like "Streaming General", "Películas", "Series", "Zona Kids", "Novedades"
    if (lineLower.find("streaming") != std::string::npos ||
        lineLower.find("series") != std::string::npos ||
        lineLower.find("película") != std::string::npos ||
        lineLower.find("pelicula") != std::string::npos ||
        lineLower.find("novela") != std::string::npos ||
        lineLower.find("dorama") != std::string::npos ||
        lineLower.find("anime") != std::string::npos ||
        lineLower.find("temporada") != std::string::npos ||
        lineLower.find("season") != std::string::npos ||
        lineLower.find("zona kids") != std::string::npos ||
        lineLower.find("novedades") != std::string::npos ||
        lineLower.find("aportadores") != std::string::npos ||
        lineLower.find("música") != std::string::npos ||
        lineLower.find("musica") != std::string::npos ||
        lineLower.find("studio") != std::string::npos) {
        return PlaylistItemType::FolderContainer;
    }

    // 9. General rule: Any HTTP URL with NO extension (extLower.empty())
    // Direct video streams virtually always have a file extension (.mp4, .mkv, .ts, .m3u8).
    // An HTTP link without any extension is almost always an M3U/API sub-playlist container!
    if (extLower.empty()) {
        return PlaylistItemType::FolderContainer;
    }

    // 10. Directory or category path segments without media extensions
    if (urlLower.find("/category/") != std::string::npos ||
        urlLower.find("/categories/") != std::string::npos ||
        urlLower.find("/sublist/") != std::string::npos ||
        urlLower.find("/playlists/") != std::string::npos ||
        urlLower.find("/folder/") != std::string::npos) {
        if (extLower != ".ts" && extLower != ".mp4" && extLower != ".m3u8") {
            return PlaylistItemType::FolderContainer;
        }
    }

    // Default to playable media stream
    return PlaylistItemType::MediaStream;
}

std::string ResolveUrl(const std::string& baseUrl, const std::string& targetUrl) {
    if (targetUrl.empty()) return "";

    std::string safeTarget = HttpClient::SanitizeUrl(targetUrl);

    // Already an absolute URL / URI?
    if (safeTarget.rfind("http://", 0) == 0 ||
        safeTarget.rfind("https://", 0) == 0 ||
        safeTarget.rfind("sdmc:/", 0) == 0 ||
        safeTarget.rfind("romfs:/", 0) == 0 ||
        safeTarget.rfind("rtsp://", 0) == 0 ||
        safeTarget.rfind("rtmp://", 0) == 0 ||
        safeTarget.rfind("file://", 0) == 0) {
        return safeTarget;
    }

    if (baseUrl.empty()) return safeTarget;

    std::string safeBase = HttpClient::SanitizeUrl(baseUrl);

    // Root-relative path
    if (safeTarget[0] == '/') {
        size_t protoEnd = safeBase.find("://");
        if (protoEnd != std::string::npos) {
            size_t hostEnd = safeBase.find('/', protoEnd + 3);
            if (hostEnd != std::string::npos) {
                return safeBase.substr(0, hostEnd) + safeTarget;
            } else {
                return safeBase + safeTarget;
            }
        }
        if (safeBase.rfind("sdmc:/", 0) == 0) {
            return "sdmc:" + safeTarget;
        }
        return safeTarget;
    }

    // Directory-relative path
    size_t lastSlash = safeBase.find_last_of('/');
    if (lastSlash != std::string::npos) {
        return safeBase.substr(0, lastSlash + 1) + safeTarget;
    }
    return safeBase + "/" + safeTarget;
}

bool ParseM3U(const std::string& content, Playlist& outPlaylist, const std::string& baseUrl) {
    outPlaylist.channels.clear();
    outPlaylist.groups.clear();

    std::stringstream ss(content);
    std::string line;
    std::set<std::string> groupSet;

    ChannelItem currentItem;
    std::string pendingExtInfLine;
    bool hasPendingItem = false;

    while (std::getline(ss, line)) {
        // Strip carriage return and whitespace
        while (!line.empty() && (line.back() == '\r' || line.back() == ' ' || line.back() == '\t')) {
            line.pop_back();
        }
        size_t start = line.find_first_not_of(" \t");
        if (start != std::string::npos) {
            line = line.substr(start);
        }

        if (line.empty()) continue;

        // Skip UTF-8 BOM if at very beginning
        if (line.rfind("\xEF\xBB\xBF", 0) == 0) {
            line = line.substr(3);
        }

        std::string lineHead = (line.length() >= 8) ? StringUtils::ToLower(line.substr(0, 8)) : "";
        if (lineHead == "#extinf:") {
            currentItem = ChannelItem();
            pendingExtInfLine = line;
            hasPendingItem = true;

            // Extract tags
            std::string group = ExtractAttribute(line, "group-title");
            if (group.empty()) group = ExtractAttribute(line, "group");
            if (!group.empty()) currentItem.groupTitle = group;

            std::string logo = ExtractAttribute(line, "tvg-logo");
            if (logo.empty()) logo = ExtractAttribute(line, "logo");
            if (logo.empty()) logo = ExtractAttribute(line, "tvg_logo");
            if (!logo.empty()) currentItem.logoUrl = logo;

            std::string tvgId = ExtractAttribute(line, "tvg-id");
            if (tvgId.empty()) tvgId = ExtractAttribute(line, "tvg-name");
            if (!tvgId.empty()) currentItem.tvgId = tvgId;

            // Channel name is after the last comma
            size_t commaPos = line.find_last_of(',');
            if (commaPos != std::string::npos && commaPos + 1 < line.length()) {
                currentItem.name = line.substr(commaPos + 1);
                // Trim leading spaces
                size_t nStart = currentItem.name.find_first_not_of(" \t");
                if (nStart != std::string::npos) {
                    currentItem.name = currentItem.name.substr(nStart);
                }
            } else {
                currentItem.name = "Canal Desconocido";
            }
        } else if (line.rfind("#EXTGRP:", 0) == 0) {
            // Alternative IPTV group tag (#EXTGRP:Category)
            std::string grp = line.substr(8);
            size_t gStart = grp.find_first_not_of(" \t");
            if (gStart != std::string::npos) {
                currentItem.groupTitle = grp.substr(gStart);
            }
        } else if (line[0] != '#') {
            // This is the target URL line (Media stream or nested folder list)
            if (hasPendingItem) {
                std::string finalUrl = ResolveUrl(baseUrl, line);
                currentItem.url = finalUrl;

                // Detect whether this item is a playable stream or a subfolder container
                currentItem.type = DetectItemType(pendingExtInfLine, finalUrl);

                if (currentItem.type == PlaylistItemType::FolderContainer && currentItem.groupTitle == "General") {
                    currentItem.groupTitle = "Carpetas";
                }

                if (currentItem.name.empty()) {
                    currentItem.name = StringUtils::GetFilename(finalUrl);
                }

                outPlaylist.channels.push_back(currentItem);
                groupSet.insert(currentItem.groupTitle);
                hasPendingItem = false;
                pendingExtInfLine.clear();
            }
        }
    }

    // Build groups list: "Todos" is always first
    outPlaylist.groups.push_back("Todos");
    for (const auto& g : groupSet) {
        outPlaylist.groups.push_back(g);
    }

    Logger::Info("PlaylistParser: Parsed %zu channels/folders, %zu groups",
                 outPlaylist.channels.size(), outPlaylist.groups.size());
    return !outPlaylist.channels.empty();
}

bool ParsePLS(const std::string& content, Playlist& outPlaylist, const std::string& baseUrl) {
    outPlaylist.channels.clear();
    outPlaylist.groups.clear();

    std::stringstream ss(content);
    std::string line;
    std::vector<std::string> urls;
    std::vector<std::string> titles;

    while (std::getline(ss, line)) {
        while (!line.empty() && (line.back() == '\r' || line.back() == ' ')) line.pop_back();

        if (line.rfind("File", 0) == 0) {
            size_t eq = line.find('=');
            if (eq != std::string::npos) {
                urls.push_back(line.substr(eq + 1));
            }
        } else if (line.rfind("Title", 0) == 0) {
            size_t eq = line.find('=');
            if (eq != std::string::npos) {
                titles.push_back(line.substr(eq + 1));
            }
        }
    }

    for (size_t i = 0; i < urls.size(); i++) {
        ChannelItem item;
        item.url = ResolveUrl(baseUrl, urls[i]);
        item.name = (i < titles.size()) ? titles[i] : ("Canal " + std::to_string(i + 1));
        item.groupTitle = "Radio / Stream";
        item.type = DetectItemType("", item.url);
        outPlaylist.channels.push_back(item);
    }

    outPlaylist.groups.push_back("Todos");
    if (!outPlaylist.channels.empty()) {
        outPlaylist.groups.push_back("Radio / Stream");
    }

    return !outPlaylist.channels.empty();
}

bool ParseWiiMC(const std::string& content, Playlist& outPlaylist, const std::string& baseUrl) {
    outPlaylist.channels.clear();
    outPlaylist.groups.clear();

    std::stringstream ss(content);
    std::string line;
    std::set<std::string> groupSet;

    ChannelItem currentItem;
    std::string rawType;
    bool hasPendingData = false;

    auto commitItem = [&]() {
        if (!hasPendingData) return;
        if (currentItem.url.empty() && currentItem.name.empty()) {
            hasPendingData = false;
            return;
        }

        if (currentItem.url.empty()) {
            hasPendingData = false;
            currentItem = ChannelItem();
            rawType.clear();
            return;
        }

        currentItem.url = ResolveUrl(baseUrl, currentItem.url);

        // Detect item type: FolderContainer vs MediaStream
        currentItem.type = DetectItemType(currentItem.name, currentItem.url);
        if (rawType == "playlist" || rawType == "folder") {
            currentItem.type = PlaylistItemType::FolderContainer;
        }

        if (currentItem.name.empty()) {
            currentItem.name = StringUtils::GetFilename(currentItem.url);
        }

        if (currentItem.groupTitle.empty() || currentItem.groupTitle == "General") {
            currentItem.groupTitle = (currentItem.type == PlaylistItemType::FolderContainer) ? "Carpetas" : "Novedades";
        }

        outPlaylist.channels.push_back(currentItem);
        groupSet.insert(currentItem.groupTitle);

        currentItem = ChannelItem();
        rawType.clear();
        hasPendingData = false;
    };

    while (std::getline(ss, line)) {
        while (!line.empty() && (line.back() == '\r' || line.back() == ' ' || line.back() == '\t')) {
            line.pop_back();
        }
        size_t start = line.find_first_not_of(" \t");
        if (start != std::string::npos) {
            line = line.substr(start);
        }
        if (line.empty()) continue;

        if (line[0] == '#') {
            // New item block separator in WiiMC / PLX
            commitItem();
            continue;
        }

        size_t eqPos = line.find('=');
        if (eqPos == std::string::npos) continue;

        std::string key = StringUtils::ToLower(line.substr(0, eqPos));
        std::string val = line.substr(eqPos + 1);
        while (!val.empty() && (val.front() == ' ' || val.front() == '\t')) val.erase(val.begin());

        hasPendingData = true;

        if (key == "name") {
            currentItem.name = val;
        } else if (key == "thumb" || key == "icon" || key == "image") {
            currentItem.logoUrl = val;
        } else if (key == "url") {
            currentItem.url = val;
        } else if (key == "xml") {
            if (currentItem.url.empty() && !val.empty()) {
                currentItem.url = val;
            }
        } else if (key == "type") {
            rawType = StringUtils::ToLower(val);
        } else if (key == "group" || key == "category") {
            currentItem.groupTitle = val;
        }
    }

    commitItem();

    outPlaylist.groups.push_back("Todos");
    for (const auto& g : groupSet) {
        outPlaylist.groups.push_back(g);
    }

    Logger::Info("PlaylistParser: Parsed WiiMC/PLX format (%zu items, %zu groups)",
                 outPlaylist.channels.size(), outPlaylist.groups.size());
    return !outPlaylist.channels.empty();
}

bool LoadFromFile(const std::string& filePath, Playlist& outPlaylist) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        Logger::Error("PlaylistParser: Failed to open file %s", filePath.c_str());
        return false;
    }

    std::string content((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());
    file.close();

    outPlaylist.name = StringUtils::GetFilename(filePath);
    outPlaylist.source = filePath;

    std::string ext = StringUtils::GetExtension(filePath);
    if (ext == ".pls") {
        return ParsePLS(content, outPlaylist, filePath);
    }

    if (content.find("#EXTM3U") != std::string::npos || content.find("#EXTINF:") != std::string::npos) {
        if (ParseM3U(content, outPlaylist, filePath) && !outPlaylist.channels.empty()) {
            return true;
        }
    }

    if (content.find("type=") != std::string::npos || content.find("URL=") != std::string::npos || content.find("url=") != std::string::npos) {
        return ParseWiiMC(content, outPlaylist, filePath);
    }

    return ParseM3U(content, outPlaylist, filePath);
}

bool LoadFromUrl(const std::string& url, Playlist& outPlaylist) {
    std::string content;
    if (!HttpClient::Get(url, content, 25)) {
        Logger::Error("PlaylistParser: Failed to download playlist from %s", url.c_str());
        return false;
    }

    outPlaylist.name = StringUtils::GetFilename(url);
    if (outPlaylist.name.empty() || outPlaylist.name.find('.') == std::string::npos) {
        outPlaylist.name = "Lista IPTV Remota";
    }
    outPlaylist.source = url;

    if (content.find("[playlist]") != std::string::npos) {
        return ParsePLS(content, outPlaylist, url);
    }

    if (content.find("#EXTM3U") != std::string::npos || content.find("#EXTINF:") != std::string::npos) {
        if (ParseM3U(content, outPlaylist, url) && !outPlaylist.channels.empty()) {
            return true;
        }
    }

    if (content.find("type=") != std::string::npos ||
        content.find("name=") != std::string::npos ||
        content.find("URL=") != std::string::npos ||
        content.find("url=") != std::string::npos ||
        content.find("thumb=") != std::string::npos) {
        return ParseWiiMC(content, outPlaylist, url);
    }

    if (ParseM3U(content, outPlaylist, url) && !outPlaylist.channels.empty()) {
        return true;
    }

    return ParseWiiMC(content, outPlaylist, url);
}

}  // namespace PlaylistParser
