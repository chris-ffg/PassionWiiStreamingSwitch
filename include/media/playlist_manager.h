// ============================================================================
// DangoPlayer Switch — Playlist Manager
// ============================================================================

#pragma once

#include "media/playlist_parser.h"
#include <string>
#include <vector>

struct SavedPlaylistEntry {
    std::string name;
    std::string pathOrUrl;
    bool isRemote = false;
};

class PlaylistManager {
public:
    static PlaylistManager& Instance();

    void LoadSaved();
    void Save();

    const std::vector<SavedPlaylistEntry>& GetSaved() const { return savedList; }
    void AddEntry(const std::string& name, const std::string& pathOrUrl);
    void RemoveEntry(int index);

    Playlist& GetCurrentPlaylist() { return currentPlaylist; }
    const Playlist& GetCurrentPlaylist() const { return currentPlaylist; }
    void SetCurrentPlaylist(const Playlist& pl) { currentPlaylist = pl; }
    bool HasActivePlaylist() const { return !currentPlaylist.channels.empty(); }

private:
    PlaylistManager();
    std::vector<SavedPlaylistEntry> savedList;
    Playlist currentPlaylist;
};
