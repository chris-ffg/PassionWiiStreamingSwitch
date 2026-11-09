// ============================================================================
// DangoPlayer Switch — Favorites Manager
// ============================================================================

#pragma once

#include <string>
#include <vector>

struct FavoriteItem {
    std::string title;
    std::string pathOrUrl;
    std::string group = "General";
    bool isStream = false;
};

class FavoritesManager {
public:
    static FavoritesManager& Instance();

    void Load();
    void Save();

    bool IsFavorite(const std::string& pathOrUrl) const;
    bool ToggleFavorite(const std::string& title, const std::string& pathOrUrl,
                        const std::string& group = "General", bool isStream = false);
    void RemoveFavorite(const std::string& pathOrUrl);

    const std::vector<FavoriteItem>& GetFavorites() const { return favorites; }
    size_t GetCount() const { return favorites.size(); }

private:
    FavoritesManager();
    std::vector<FavoriteItem> favorites;
};
