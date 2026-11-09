// ============================================================================
// DangoPlayer Switch — Configuration Implementation
// ============================================================================

#include "config.h"
#include "utils/logger.h"
#include <cstdio>
#include <cstring>
#include <sys/stat.h>

// ============================================================================
// Singleton
// ============================================================================

Config& Config::Instance() {
    static Config instance;
    return instance;
}

// ============================================================================
// Load — Read key=value config from SD card
// ============================================================================

bool Config::Load() {
    FILE* f = fopen(AppConfig::CONFIG_FILE, "r");
    if (!f) {
        Logger::Info("Config: No config file found at %s — using defaults", AppConfig::CONFIG_FILE);
        config.lastPlaylistPath = "https://listas-iptv-wii.github.io/dango/passionwii.m3u";
        return false;
    }

    char line[512];
    while (fgets(line, sizeof(line), f)) {
        // Strip newline
        char* nl = strchr(line, '\n');
        if (nl) *nl = '\0';
        char* cr = strchr(line, '\r');
        if (cr) *cr = '\0';

        // Parse key=value
        char* eq = strchr(line, '=');
        if (!eq) continue;

        *eq = '\0';
        const char* key   = line;
        const char* value = eq + 1;

        if (strcmp(key, "volume") == 0) {
            config.defaultVolume = atoi(value);
        } else if (strcmp(key, "resume") == 0) {
            config.resumePlayback = (atoi(value) != 0);
        } else if (strcmp(key, "theme") == 0) {
            config.themeIndex = atoi(value);
        } else if (strcmp(key, "playlist") == 0) {
            config.lastPlaylistPath = value;
        }
    }

    fclose(f);
    Logger::Info("Config: Loaded from %s", AppConfig::CONFIG_FILE);
    return true;
}

// ============================================================================
// Save — Write config to SD card
// ============================================================================

bool Config::Save() {
    // Ensure directories exist
    mkdir(AppConfig::APP_DIR, 0777);
    mkdir(AppConfig::FONTS_DIR, 0777);
    mkdir(AppConfig::CACHE_DIR, 0777);

    FILE* f = fopen(AppConfig::CONFIG_FILE, "w");
    if (!f) {
        Logger::Error("Config: Failed to save to %s", AppConfig::CONFIG_FILE);
        return false;
    }

    fprintf(f, "volume=%d\n", config.defaultVolume);
    fprintf(f, "resume=%d\n", config.resumePlayback ? 1 : 0);
    fprintf(f, "theme=%d\n", config.themeIndex);
    if (!config.lastPlaylistPath.empty()) {
        fprintf(f, "playlist=%s\n", config.lastPlaylistPath.c_str());
    }

    fclose(f);
    Logger::Info("Config: Saved to %s", AppConfig::CONFIG_FILE);
    return true;
}
