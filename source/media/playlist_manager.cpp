// ============================================================================
// DangoPlayer Switch — Playlist Manager Implementation
// ============================================================================

#include "media/playlist_manager.h"
#include "utils/logger.h"
#include <fstream>

static const char* PLAYLIST_INDEX_FILE = "sdmc:/switch/PassionWiiStreaming/playlists.txt";

PlaylistManager& PlaylistManager::Instance() {
    static PlaylistManager instance;
    return instance;
}

PlaylistManager::PlaylistManager() {
    LoadSaved();
}

void PlaylistManager::LoadSaved() {
    savedList.clear();
    std::ifstream file(PLAYLIST_INDEX_FILE);
    if (!file.is_open()) return;

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        size_t pipe = line.find('|');
        if (pipe != std::string::npos) {
            SavedPlaylistEntry entry;
            entry.name = line.substr(0, pipe);
            entry.pathOrUrl = line.substr(pipe + 1);
            entry.isRemote = (entry.pathOrUrl.rfind("http://", 0) == 0 ||
                              entry.pathOrUrl.rfind("https://", 0) == 0);
            savedList.push_back(entry);
        }
    }
    file.close();
    Logger::Info("PlaylistManager: Loaded %zu saved playlists", savedList.size());
}

void PlaylistManager::Save() {
    std::ofstream file(PLAYLIST_INDEX_FILE);
    if (!file.is_open()) return;

    for (const auto& entry : savedList) {
        file << entry.name << "|" << entry.pathOrUrl << "\n";
    }
    file.close();
    Logger::Info("PlaylistManager: Saved playlist index");
}

void PlaylistManager::AddEntry(const std::string& name, const std::string& pathOrUrl) {
    SavedPlaylistEntry entry;
    entry.name = name;
    entry.pathOrUrl = pathOrUrl;
    entry.isRemote = (pathOrUrl.rfind("http://", 0) == 0 ||
                      pathOrUrl.rfind("https://", 0) == 0);

    // Avoid duplicate URLs
    for (auto& item : savedList) {
        if (item.pathOrUrl == pathOrUrl) {
            item.name = name;
            Save();
            return;
        }
    }

    savedList.push_back(entry);
    Save();
}

void PlaylistManager::RemoveEntry(int index) {
    if (index >= 0 && index < (int)savedList.size()) {
        savedList.erase(savedList.begin() + index);
        Save();
    }
}
