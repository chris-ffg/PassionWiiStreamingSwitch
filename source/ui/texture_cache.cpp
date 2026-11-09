// ============================================================================
// DangoPlayer Switch — Texture & Poster Image Cache Implementation
// ============================================================================

#include "ui/texture_cache.h"
#include "network/http_client.h"
#include "utils/logger.h"
#include <SDL2/SDL_image.h>
#include <sys/stat.h>
#include <unistd.h>
#include <chrono>

static const char* CACHE_DIR = "sdmc:/switch/PassionWiiStreaming/cache";

TextureCache& TextureCache::Instance() {
    static TextureCache instance;
    return instance;
}

TextureCache::TextureCache() {
    Initialize();
}

TextureCache::~TextureCache() {
    Shutdown();
}

void TextureCache::Initialize() {
    mkdir("sdmc:/switch", 0777);
    mkdir("sdmc:/switch/PassionWiiStreaming", 0777);
    mkdir(CACHE_DIR, 0777);

    if (!workerRunning) {
        workerRunning = true;
        workerThread = std::thread(&TextureCache::WorkerLoop, this);
    }
}

void TextureCache::Shutdown() {
    if (workerRunning) {
        workerRunning = false;
        if (workerThread.joinable()) {
            workerThread.join();
        }
    }
    ClearRamCache();
}

uint32_t TextureCache::HashUrl(const std::string& url) const {
    uint32_t hash = 2166136261u;
    for (char c : url) {
        hash ^= (uint8_t)c;
        hash *= 16777619u;
    }
    return hash;
}

std::string TextureCache::GetCacheFilePath(const std::string& url) const {
    char buf[128];
    snprintf(buf, sizeof(buf), "%s/p_%08x.png", CACHE_DIR, HashUrl(url));
    return std::string(buf);
}

void TextureCache::ClearRamCache() {
    for (auto& pair : textureMap) {
        if (pair.second) {
            SDL_DestroyTexture(pair.second);
        }
    }
    textureMap.clear();
    lruList.clear();
}

SDL_Texture* TextureCache::GetTexture(SDL_Renderer* renderer, const std::string& imageUrl) {
    if (imageUrl.empty() || !renderer) return nullptr;

    // 1. Check RAM cache
    auto it = textureMap.find(imageUrl);
    if (it != textureMap.end()) {
        // Promote to front of LRU
        lruList.remove(imageUrl);
        lruList.push_front(imageUrl);
        return it->second;
    }

    // 2. Check if cached on SD card
    std::string localPath = GetCacheFilePath(imageUrl);
    struct stat st;
    if (stat(localPath.c_str(), &st) == 0 && st.st_size > 500) {
        // Load texture into RAM
        SDL_Texture* tex = IMG_LoadTexture(renderer, localPath.c_str());
        if (tex) {
            // Evict oldest texture if limit exceeded
            if (textureMap.size() >= MAX_RAM_TEXTURES && !lruList.empty()) {
                std::string oldest = lruList.back();
                lruList.pop_back();
                auto oldIt = textureMap.find(oldest);
                if (oldIt != textureMap.end()) {
                    if (oldIt->second) SDL_DestroyTexture(oldIt->second);
                    textureMap.erase(oldIt);
                }
            }

            textureMap[imageUrl] = tex;
            lruList.push_front(imageUrl);
            return tex;
        }
    }

    // 3. Not on SD: Queue background download if remote HTTP
    if (imageUrl.rfind("http://", 0) == 0 || imageUrl.rfind("https://", 0) == 0) {
        std::lock_guard<std::mutex> lock(queueMutex);
        if (pendingUrls.find(imageUrl) == pendingUrls.end()) {
            pendingUrls.insert(imageUrl);
            downloadQueue.push(imageUrl);
        }
    }

    return nullptr;
}

void TextureCache::WorkerLoop() {
    Logger::Info("TextureCache: Background poster downloader thread started");

    while (workerRunning) {
        std::string targetUrl;
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            if (!downloadQueue.empty()) {
                targetUrl = downloadQueue.front();
                downloadQueue.pop();
            }
        }

        if (targetUrl.empty()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        std::string destPath = GetCacheFilePath(targetUrl);
        struct stat st;
        if (stat(destPath.c_str(), &st) != 0 || st.st_size < 500) {
            HttpClient::DownloadFile(targetUrl, destPath, nullptr, 15);
        }

        {
            std::lock_guard<std::mutex> lock(queueMutex);
            pendingUrls.erase(targetUrl);
        }

        // Slight throttle between network image downloads to prevent network congestion
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
    }

    Logger::Info("TextureCache: Background worker thread stopped");
}
