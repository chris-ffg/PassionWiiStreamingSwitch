// ============================================================================
// DangoPlayer Switch — Favorites Manager Implementation
// ============================================================================

#include "media/favorites_manager.h"
#include "utils/logger.h"
#include <fstream>
#include <sstream>
#include <algorithm>

static const char* FAVORITES_FILE = "sdmc:/switch/PassionWiiStreaming/favorites.txt";

FavoritesManager& FavoritesManager::Instance() {
    static FavoritesManager instance;
    return instance;
}

FavoritesManager::FavoritesManager() {
    Load();
}

void FavoritesManager::Load() {
    favorites.clear();
    std::ifstream file(FAVORITES_FILE);
    if (!file.is_open()) return;

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        std::stringstream ss(line);
        std::string title, path, group, isStreamStr;

        if (std::getline(ss, title, '|') &&
            std::getline(ss, path, '|')) {
            FavoriteItem item;
            item.title = title;
            item.pathOrUrl = path;

            if (std::getline(ss, group, '|')) item.group = group;
            if (std::getline(ss, isStreamStr, '|')) item.isStream = (isStreamStr == "1");

            favorites.push_back(item);
        }
    }
    file.close();
    Logger::Info("FavoritesManager: Loaded %zu favorites", favorites.size());
}

void FavoritesManager::Save() {
    std::ofstream file(FAVORITES_FILE);
    if (!file.is_open()) return;

    for (const auto& f : favorites) {
        file << f.title << "|" << f.pathOrUrl << "|"
             << f.group << "|" << (f.isStream ? "1" : "0") << "\n";
    }
    file.close();
    Logger::Info("FavoritesManager: Saved favorites");
}

bool FavoritesManager::IsFavorite(const std::string& pathOrUrl) const {
    for (const auto& f : favorites) {
        if (f.pathOrUrl == pathOrUrl) return true;
    }
    return false;
}

bool FavoritesManager::ToggleFavorite(const std::string& title, const std::string& pathOrUrl,
                                      const std::string& group, bool isStream) {
    for (auto it = favorites.begin(); it != favorites.end(); ++it) {
        if (it->pathOrUrl == pathOrUrl) {
            favorites.erase(it);
            Save();
            return false; // Removed
        }
    }

    FavoriteItem item;
    item.title = title;
    item.pathOrUrl = pathOrUrl;
    item.group = group;
    item.isStream = isStream;
    favorites.push_back(item);
    Save();
    return true; // Added
}

void FavoritesManager::RemoveFavorite(const std::string& pathOrUrl) {
    auto it = std::remove_if(favorites.begin(), favorites.end(), [&](const FavoriteItem& item) {
        return item.pathOrUrl == pathOrUrl;
    });
    if (it != favorites.end()) {
        favorites.erase(it, favorites.end());
        Save();
    }
}
