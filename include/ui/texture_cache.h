// ============================================================================
// DangoPlayer Switch — Texture & Poster Image Cache
// ============================================================================
// Asynchronous LRU poster texture cache with SD card persistent caching.
// Ensures that massive M3U playlists with thousands of poster URLs do not
// leak memory or exceed the Nintendo Switch RAM limits.
// ============================================================================

#pragma once

#include <SDL2/SDL.h>
#include <string>
#include <map>
#include <list>
#include <queue>
#include <set>
#include <mutex>
#include <thread>
#include <atomic>

class TextureCache {
public:
    static TextureCache& Instance();

    /// Initialize the cache directory on SD card
    void Initialize();

    /// Shutdown the background download thread and free all textures
    void Shutdown();

    /// Get or queue-load a texture for the given image URL.
    /// Returns SDL_Texture* if already loaded in RAM, or nullptr if pending download/load.
    SDL_Texture* GetTexture(SDL_Renderer* renderer, const std::string& imageUrl);

    /// Clear all loaded textures from RAM (e.g. when changing screen or memory pressure)
    void ClearRamCache();

private:
    TextureCache();
    ~TextureCache();

    void WorkerLoop();
    std::string GetCacheFilePath(const std::string& url) const;
    uint32_t HashUrl(const std::string& url) const;

    static constexpr size_t MAX_RAM_TEXTURES = 36;  // ~15MB RAM for 200x300 posters

    std::map<std::string, SDL_Texture*> textureMap;
    std::list<std::string> lruList;

    std::queue<std::string> downloadQueue;
    std::set<std::string> pendingUrls;
    std::mutex queueMutex;

    std::thread workerThread;
    std::atomic<bool> workerRunning{false};
};
