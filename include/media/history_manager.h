// ============================================================================
// DangoPlayer Switch — Playback History Manager
// ============================================================================

#pragma once

#include <string>
#include <vector>
#include <cstdint>

struct HistoryItem {
    std::string title;
    std::string pathOrUrl;
    double lastPosition = 0.0;
    double duration = 0.0;
    uint64_t timestamp = 0;
    bool isStream = false;
};

class HistoryManager {
public:
    static HistoryManager& Instance();

    void Load();
    void Save();

    void AddEntry(const std::string& title, const std::string& pathOrUrl,
                  double pos, double dur, bool isStream = false);
    void Clear();

    const std::vector<HistoryItem>& GetEntries() const { return entries; }
    size_t GetCount() const { return entries.size(); }

private:
    HistoryManager();
    std::vector<HistoryItem> entries;
    static constexpr size_t MAX_HISTORY_ENTRIES = 50;
};
