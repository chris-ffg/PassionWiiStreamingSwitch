// ============================================================================
// DangoPlayer Switch — Playback History Manager Implementation
// ============================================================================

#include "media/history_manager.h"
#include "utils/logger.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <ctime>

static const char* HISTORY_FILE = "sdmc:/switch/PassionWiiStreaming/history.txt";

HistoryManager& HistoryManager::Instance() {
    static HistoryManager instance;
    return instance;
}

HistoryManager::HistoryManager() {
    Load();
}

void HistoryManager::Load() {
    entries.clear();
    std::ifstream file(HISTORY_FILE);
    if (!file.is_open()) return;

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        std::stringstream ss(line);
        std::string title, path, posStr, durStr, timeStr, streamStr;

        if (std::getline(ss, title, '|') &&
            std::getline(ss, path, '|')) {
            HistoryItem item;
            item.title = title;
            item.pathOrUrl = path;

            if (std::getline(ss, posStr, '|')) {
                item.lastPosition = strtod(posStr.c_str(), nullptr);
            }
            if (std::getline(ss, durStr, '|')) {
                item.duration = strtod(durStr.c_str(), nullptr);
            }
            if (std::getline(ss, timeStr, '|')) {
                item.timestamp = strtoull(timeStr.c_str(), nullptr, 10);
            }
            if (std::getline(ss, streamStr, '|')) {
                item.isStream = (streamStr == "1");
            }

            entries.push_back(item);
        }
    }
    file.close();
    Logger::Info("HistoryManager: Loaded %zu history items", entries.size());
}

void HistoryManager::Save() {
    std::ofstream file(HISTORY_FILE);
    if (!file.is_open()) return;

    for (const auto& h : entries) {
        file << h.title << "|" << h.pathOrUrl << "|"
             << h.lastPosition << "|" << h.duration << "|"
             << h.timestamp << "|" << (h.isStream ? "1" : "0") << "\n";
    }
    file.close();
}

void HistoryManager::AddEntry(const std::string& title, const std::string& pathOrUrl,
                             double pos, double dur, bool isStream) {
    if (pathOrUrl.empty()) return;

    // Remove older duplicate if exists
    entries.erase(std::remove_if(entries.begin(), entries.end(), [&](const HistoryItem& h) {
        return h.pathOrUrl == pathOrUrl;
    }), entries.end());

    HistoryItem item;
    item.title = title;
    item.pathOrUrl = pathOrUrl;
    item.lastPosition = pos;
    item.duration = dur;
    item.timestamp = (uint64_t)std::time(nullptr);
    item.isStream = isStream;

    // Insert at front (most recent first)
    entries.insert(entries.begin(), item);

    // Limit maximum entries
    if (entries.size() > MAX_HISTORY_ENTRIES) {
        entries.resize(MAX_HISTORY_ENTRIES);
    }

    Save();
}

void HistoryManager::Clear() {
    entries.clear();
    remove(HISTORY_FILE);
    Logger::Info("HistoryManager: History cleared");
}
