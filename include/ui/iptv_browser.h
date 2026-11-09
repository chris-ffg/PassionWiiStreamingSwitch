// ============================================================================
// DangoPlayer Switch — IPTV Horizontal Shelf & Sidebar Browser
// ============================================================================

#pragma once

#include "ui/screen.h"
#include "media/playlist_manager.h"
#include "ui/widgets/toast.h"
#include "ui/texture_cache.h"
#include <vector>
#include <string>

enum class FocusZone {
    Sidebar,
    Shelf,
    Header
};

enum class SidebarItem {
    Home = 0,
    IPTV = 1,
    Favorites = 2,
    Search = 3,
    Settings = 4,
    Count = 5
};

class IPTVBrowser : public Screen {
public:
    explicit IPTVBrowser(App* app, const std::string& autoLoadPath = "");
    /// Sub-folder navigation constructor
    IPTVBrowser(App* app, const Playlist& subPlaylist, const std::string& folderTitle, bool isSubFolder = true);

    void OnEnter() override;
    void HandleInput(u64 kDown, u64 kHeld) override;
    void HandleTouch(const TouchEvent& touch) override;
    void Update(float dt) override;
    void Render(SDL_Renderer* renderer) override;

    void LoadPlaylist(const std::string& pathOrUrl);

private:
    void HandlePlaylistsInput(u64 kDown);
    void HandleShelfInput(u64 kDown);
    void HandleSidebarInput(u64 kDown);
    void RefreshChannelList(bool resetSelection = false);
    void CycleGroup(int direction);
    void PromptAddUrl();
    void PromptSearch();

    // Bifurcation & Nested Folder Navigation
    void HandleItemClick(const ChannelItem& ch);
    void NavigateToFolder(const ChannelItem& ch);
    bool MightBeSubPlaylist(const ChannelItem& ch) const;
    void RenderLoadingFrame(const std::string& message);
    const Playlist& GetActivePlaylist() const;

    // View Renderers
    void RenderPlaylistsView(SDL_Renderer* renderer);
    void RenderSidebar(SDL_Renderer* renderer);
    void RenderShelfView(SDL_Renderer* renderer);
    void RenderPosterCard(SDL_Renderer* renderer, const ChannelItem& ch, int index, int x, int y, int w, int h, bool isFocused);

    bool isSubFolder = false;
    std::string folderTitle;
    Playlist currentPlaylist;
    bool hasLocalPlaylist = false;

    bool showPlaylistsView = true;
    int playlistIndex = 0;

    FocusZone focusZone = FocusZone::Shelf;
    int sidebarIndex = 1;  // Default to IPTV

    int channelIndex = 0;
    float currentScrollX = 0.0f;
    float targetScrollX = 0.0f;
    int groupIndex = 0;
    std::string searchQuery;

    // Touch dragging
    bool isDraggingShelf = false;
    int dragStartX = 0;
    float dragStartScrollX = 0.0f;

    std::vector<ChannelItem> displayedChannels;
    Toast toast;

    // Layout constants for 720p / 1080p
    static constexpr int SIDEBAR_W = 96;
    static constexpr int POSTER_W = 200;
    static constexpr int POSTER_H = 345;
    static constexpr int POSTER_GAP = 22;
    static constexpr int POSTER_FOCUSED_W = 216;
    static constexpr int POSTER_FOCUSED_H = 372;
    static constexpr int SHELF_Y = 96;
    static constexpr int SHELF_H = 385;
};
