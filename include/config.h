// ============================================================================
// DangoPlayer Switch — Configuration Manager
// ============================================================================
// Handles loading and saving user preferences to the SD card.
// Config file: sdmc:/switch/DangoPlayer/config.txt
//
// Phase 1: Simple key=value format.
// Phase 5+: Will migrate to JSON (switch-jansson).
// ============================================================================

#pragma once

#include <string>

// ============================================================================
// Configuration data
// ============================================================================
struct AppConfig {
    // Player
    int  defaultVolume   = 100;
    bool resumePlayback  = true;

    // UI
    int themeIndex = 0;  // 0 = dark (default), 1 = light

    // IPTV
    std::string lastPlaylistPath = "https://listas-iptv-wii.github.io/dango/passionwii.m3u";

    // Filesystem paths
    static constexpr const char* APP_DIR     = "sdmc:/switch/PassionWiiStreaming";
    static constexpr const char* CONFIG_FILE = "sdmc:/switch/PassionWiiStreaming/config.txt";
    static constexpr const char* FONTS_DIR   = "sdmc:/switch/PassionWiiStreaming/fonts";
    static constexpr const char* CACHE_DIR   = "sdmc:/switch/PassionWiiStreaming/cache";
};

// ============================================================================
// Singleton config manager
// ============================================================================
class Config {
public:
    static Config& Instance();

    /// Load configuration from SD card. Returns false if file not found.
    bool Load();

    /// Save current configuration to SD card.
    bool Save();

    /// Access the config data.
    AppConfig&       Get()       { return config; }
    const AppConfig& Get() const { return config; }

private:
    Config() = default;
    AppConfig config;
};
