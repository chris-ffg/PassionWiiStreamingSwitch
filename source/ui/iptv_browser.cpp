// ============================================================================
// DangoPlayer Switch — IPTV Horizontal Shelf & Sidebar Implementation
// ============================================================================

#include "ui/iptv_browser.h"
#include "ui/player_screen.h"
#include "ui/file_browser.h"
#include "ui/history_screen.h"
#include "ui/settings_screen.h"
#include "media/favorites_manager.h"
#include "network/http_client.h"
#include "config.h"
#include "app.h"
#include "ui/ui_common.h"
#include "utils/swkbd_helper.h"
#include "utils/string_utils.h"
#include "utils/logger.h"
#include <algorithm>
#include <set>
#include <cmath>
#include <cstdio>

// Root Browser Constructor
IPTVBrowser::IPTVBrowser(App* app, const std::string& autoLoadPath)
    : Screen(app), isSubFolder(false), hasLocalPlaylist(false),
      focusZone(FocusZone::Shelf), sidebarIndex(1) {
    std::string path = autoLoadPath;
    if (path.empty() && !PlaylistManager::Instance().HasActivePlaylist()) {
        const std::string& last = Config::Instance().Get().lastPlaylistPath;
        if (!last.empty()) {
            path = last;
        } else {
            path = "https://listas-iptv-wii.github.io/dango/passionwii.m3u";
        }
    }
    if (!path.empty()) {
        LoadPlaylist(path);
    } else if (PlaylistManager::Instance().HasActivePlaylist()) {
        currentPlaylist = PlaylistManager::Instance().GetCurrentPlaylist();
        hasLocalPlaylist = true;
        showPlaylistsView = false;
        RefreshChannelList(true);
    }
}

// Sub-Folder Navigation Constructor
IPTVBrowser::IPTVBrowser(App* app, const Playlist& subPlaylist, const std::string& title, bool isSub)
    : Screen(app), isSubFolder(isSub), folderTitle(title), currentPlaylist(subPlaylist), hasLocalPlaylist(true),
      focusZone(FocusZone::Shelf), sidebarIndex(1) {
    showPlaylistsView = false;
    RefreshChannelList(true);
}

const Playlist& IPTVBrowser::GetActivePlaylist() const {
    if (hasLocalPlaylist) {
        return currentPlaylist;
    }
    return PlaylistManager::Instance().GetCurrentPlaylist();
}

void IPTVBrowser::OnEnter() {
    Logger::Info("IPTVBrowser: OnEnter (isSubFolder=%d)", isSubFolder);
    // Only initialize channel list if empty. When returning from video playback,
    // preserve the user's selected card and scroll position!
    if (displayedChannels.empty()) {
        if (!showPlaylistsView && (hasLocalPlaylist || PlaylistManager::Instance().HasActivePlaylist())) {
            RefreshChannelList(true);
        }
    }
}

void IPTVBrowser::LoadPlaylist(const std::string& pathOrUrl) {
    toast.Show("Cargando lista IPTV...", 2.0f);
    RenderLoadingFrame("Cargando lista IPTV...");

    Playlist pl;
    bool ok = false;
    std::string safeUrl = HttpClient::SanitizeUrl(pathOrUrl);
    if (safeUrl.rfind("http://", 0) == 0 || safeUrl.rfind("https://", 0) == 0) {
        ok = PlaylistParser::LoadFromUrl(safeUrl, pl);
    } else {
        ok = PlaylistParser::LoadFromFile(safeUrl, pl);
    }

    if (ok && !pl.channels.empty()) {
        currentPlaylist = pl;
        hasLocalPlaylist = true;
        PlaylistManager::Instance().SetCurrentPlaylist(pl);
        PlaylistManager::Instance().AddEntry(pl.name, pathOrUrl);

        showPlaylistsView = false;
        groupIndex = 0;
        channelIndex = 0;
        currentScrollX = 0.0f;
        targetScrollX = 0.0f;
        searchQuery.clear();

        RefreshChannelList(true);
        toast.Show("Lista cargada: " + std::to_string(pl.channels.size()) + " elementos", 2.2f);
    } else {
        toast.Show("Error al cargar la lista", 2.2f);
    }
}

void IPTVBrowser::RefreshChannelList(bool resetSelection) {
    const auto& pl = GetActivePlaylist();
    std::string currentGroup = "Todos";
    if (groupIndex >= 0 && groupIndex < (int)pl.groups.size()) {
        currentGroup = pl.groups[groupIndex];
    }
    displayedChannels = pl.GetFilteredChannels(currentGroup, searchQuery);

    if (resetSelection) {
        channelIndex = 0;
        currentScrollX = 0.0f;
        targetScrollX = 0.0f;
    } else {
        if (!displayedChannels.empty()) {
            channelIndex = std::clamp(channelIndex, 0, (int)displayedChannels.size() - 1);
        } else {
            channelIndex = 0;
        }
    }
}

void IPTVBrowser::CycleGroup(int direction) {
    const auto& pl = GetActivePlaylist();
    if (pl.groups.empty()) return;

    int totalGroups = (int)pl.groups.size();
    groupIndex = (groupIndex + direction + totalGroups) % totalGroups;
    RefreshChannelList(true);
    toast.Show("Grupo: " + pl.groups[groupIndex], 1.2f);
}

void IPTVBrowser::PromptAddUrl() {
    std::string entered = SwkbdHelper::Show(
        "Introduce la URL de la lista IPTV (M3U / M3U8):",
        "http://", 512
    );
    if (!entered.empty() && entered != "http://" && entered != "https://") {
        LoadPlaylist(entered);
    }
}

void IPTVBrowser::PromptSearch() {
    std::string query = SwkbdHelper::Show(
        "Buscar película, serie o canal:",
        searchQuery, 64
    );
    searchQuery = query;
    RefreshChannelList(true);
    if (!searchQuery.empty()) {
        toast.Show("Resultados para: " + searchQuery, 1.5f);
    }
}

void IPTVBrowser::RenderLoadingFrame(const std::string& message) {
    SDL_Renderer* renderer = app->GetRenderer();
    if (!renderer) return;

    SDL_SetRenderDrawColor(renderer, Colors::BG_DARK.r, Colors::BG_DARK.g, Colors::BG_DARK.b, 255);
    SDL_RenderClear(renderer);

    SDL_Texture* bg = app->GetBackgroundTexture();
    if (bg) {
        SDL_RenderCopy(renderer, bg, nullptr, nullptr);
        UI::DrawRect(renderer, 0, 0, Layout::SCREEN_W, Layout::SCREEN_H, {10, 8, 18, 70});
    }

    Render(renderer);
    toast.Show(message, 3.0f);
    toast.Render(renderer, app->GetFontSmall());
    SDL_RenderPresent(renderer);
}

bool IPTVBrowser::MightBeSubPlaylist(const ChannelItem& ch) const {
    std::string urlLower = StringUtils::ToLower(ch.url);
    std::string extLower = StringUtils::ToLower(StringUtils::GetExtension(ch.url));

    // Definite direct media formats — always playable streams
    static const std::set<std::string> directMedia = {
        ".mp4", ".mkv", ".ts", ".avi", ".flv", ".mov", ".wmv",
        ".webm", ".m4v", ".3gp", ".mpg", ".mpeg", ".vob",
        ".mp3", ".aac", ".flac", ".ogg", ".wav", ".m4a", ".opus", ".ac3"
    };
    if (directMedia.find(extLower) != directMedia.end()) {
        return false;
    }

    // Direct streaming protocols
    if (urlLower.rfind("rtsp://", 0) == 0 ||
        urlLower.rfind("rtmp://", 0) == 0 ||
        urlLower.rfind("udp://", 0) == 0 ||
        urlLower.rfind("rtp://", 0) == 0) {
        return false;
    }

    // Passion Wii Streaming endpoints are always folders
    if (urlLower.find("passionstreamingwii") != std::string::npos ||
        urlLower.find("myartsonline.com") != std::string::npos ||
        urlLower.find("listas-iptv-wii") != std::string::npos) {
        return true;
    }

    // If URL has no extension or is ambiguous, it might be a sub-playlist
    return true;
}

// ============================================================================
// BIFURCATION POINT: Folder Container Navigation vs Media Stream Playback
// ============================================================================
void IPTVBrowser::HandleItemClick(const ChannelItem& ch) {
    bool isFolder = (ch.type == PlaylistItemType::FolderContainer);

    // If marked as stream but has no definite video extension, probe it before playback!
    if (!isFolder && MightBeSubPlaylist(ch)) {
        toast.Show("Verificando carpeta...", 1.2f);
        RenderLoadingFrame("Verificando carpeta...");
        if (HttpClient::ProbeIsPlaylist(ch.url)) {
            isFolder = true;
        }
    }

    if (isFolder) {
        // --------------------------------------------------------------------
        // BIFURCATION A: FOLDER / CONTAINER NAVIGATION (Stack PUSH)
        // --------------------------------------------------------------------
        // Do NOT invoke VideoPlayerEngine. Navigate into sub-directory.
        NavigateToFolder(ch);
    } else {
        // --------------------------------------------------------------------
        // BIFURCATION B: MEDIA STREAM PLAYBACK
        // --------------------------------------------------------------------
        // Direct media stream (.mp4, .mkv, .ts, live HLS): launch PlayerScreen.
        Logger::Info("IPTVBrowser: Playing stream -> %s (%s)", ch.name.c_str(), ch.url.c_str());
        app->PushScreen(std::make_unique<PlayerScreen>(app, ch.url, ch.name));
    }
}

void IPTVBrowser::NavigateToFolder(const ChannelItem& ch) {
    Logger::Info("IPTVBrowser: Navigating to sub-folder -> %s (%s)", ch.name.c_str(), ch.url.c_str());
    toast.Show("Cargando: " + ch.name + "...", 3.0f);
    RenderLoadingFrame("Cargando: " + ch.name + "...");

    Playlist subPlaylist;
    bool ok = false;
    std::string safeUrl = HttpClient::SanitizeUrl(ch.url);
    if (safeUrl.rfind("http://", 0) == 0 || safeUrl.rfind("https://", 0) == 0) {
        ok = PlaylistParser::LoadFromUrl(safeUrl, subPlaylist);
    } else {
        ok = PlaylistParser::LoadFromFile(safeUrl, subPlaylist);
    }

    if (!ok || subPlaylist.channels.empty()) {
        // If sub-folder is empty or unreachable, show alert toast and remain in current folder.
        Logger::Warn("IPTVBrowser: Sub-folder is empty or unreachable -> %s", ch.url.c_str());
        toast.Show("Carpeta vacia o no disponible", 2.5f);
        return;
    }

    if (subPlaylist.name.empty() || subPlaylist.name == "Lista IPTV Remota") {
        subPlaylist.name = ch.name;
    }

    // Push new IPTVBrowser instance onto navigation stack
    app->PushScreen(std::make_unique<IPTVBrowser>(app, subPlaylist, ch.name, true));
}

// ============================================================================
// Input Handling
// ============================================================================

void IPTVBrowser::HandleInput(u64 kDown, u64 kHeld) {
    if (focusZone == FocusZone::Sidebar) {
        HandleSidebarInput(kDown);
    } else {
        HandleShelfInput(kDown);
    }
}

void IPTVBrowser::HandleSidebarInput(u64 kDown) {
    if (kDown & HidNpadButton_AnyUp) {
        sidebarIndex = (sidebarIndex - 1 + (int)SidebarItem::Count) % (int)SidebarItem::Count;
    }
    if (kDown & HidNpadButton_AnyDown) {
        sidebarIndex = (sidebarIndex + 1) % (int)SidebarItem::Count;
    }
    if (kDown & HidNpadButton_AnyRight) {
        // Move focus back to the poster carousel shelf
        focusZone = FocusZone::Shelf;
    }

    if (kDown & HidNpadButton_A) {
        switch ((SidebarItem)sidebarIndex) {
            case SidebarItem::Home:
                // Dedicated App: Home button always returns directly to Main Menu
                app->PopScreen();
                break;
            case SidebarItem::IPTV:
                focusZone = FocusZone::Shelf;
                break;
            case SidebarItem::Favorites:
                app->PushScreen(std::make_unique<HistoryScreen>(app, 1));
                break;
            case SidebarItem::Search:
                PromptSearch();
                focusZone = FocusZone::Shelf;
                break;
            case SidebarItem::Settings:
                app->PushScreen(std::make_unique<SettingsScreen>(app));
                break;
            default:
                break;
        }
    }

    if (kDown & HidNpadButton_B) {
        // Dedicated App: [B] button always pops (returns to parent folder or Main Menu)
        app->PopScreen();
    }
}

void IPTVBrowser::HandleShelfInput(u64 kDown) {
    int total = (int)displayedChannels.size();

    // Horizontal Carousel Navigation
    if (kDown & HidNpadButton_AnyLeft) {
        if (channelIndex > 0) {
            channelIndex--;
        } else {
            // Reached left edge: switch focus to Sidebar rail
            focusZone = FocusZone::Sidebar;
        }
    }
    if (kDown & HidNpadButton_AnyRight) {
        if (total > 0 && channelIndex < total - 1) {
            channelIndex++;
        }
    }

    // Jump 5 items with ZR / ZL
    if (kDown & HidNpadButton_ZR) {
        if (total > 0) {
            channelIndex = std::min(total - 1, channelIndex + 5);
        }
    }
    if (kDown & HidNpadButton_ZL) {
        if (total > 0) {
            channelIndex = std::max(0, channelIndex - 5);
        }
    }

    // Category cycling [L / R]
    if (kDown & HidNpadButton_R) {
        CycleGroup(+1);
    }
    if (kDown & HidNpadButton_L) {
        CycleGroup(-1);
    }

    // Select / Activate Card [A]
    if (kDown & HidNpadButton_A) {
        if (total > 0 && channelIndex >= 0 && channelIndex < total) {
            HandleItemClick(displayedChannels[channelIndex]);
        }
    }

    // Toggle Favorite [X]
    if (kDown & HidNpadButton_X) {
        if (total > 0 && channelIndex >= 0 && channelIndex < total) {
            const auto& ch = displayedChannels[channelIndex];
            bool added = FavoritesManager::Instance().ToggleFavorite(ch.name, ch.url, ch.groupTitle, ch.type == PlaylistItemType::MediaStream);
            toast.Show(added ? "Anadido a Favoritos" : "Eliminado de Favoritos", 1.5f);
        }
    }

    // Search query [Y]
    if (kDown & HidNpadButton_Y) {
        PromptSearch();
    }

    // Return back [B] — Stack POP to parent directory or Main Menu
    if (kDown & HidNpadButton_B) {
        app->PopScreen();
    }
}

void IPTVBrowser::HandlePlaylistsInput(u64 kDown) {
    const auto& saved = PlaylistManager::Instance().GetSaved();
    int totalItems = 2 + (int)saved.size();

    if (kDown & HidNpadButton_AnyDown) {
        playlistIndex = (playlistIndex + 1) % totalItems;
    }
    if (kDown & HidNpadButton_AnyUp) {
        playlistIndex = (playlistIndex - 1 + totalItems) % totalItems;
    }

    if (kDown & HidNpadButton_A) {
        if (playlistIndex == 0) {
            PromptAddUrl();
        } else if (playlistIndex == 1) {
            app->PushScreen(std::make_unique<FileBrowser>(app, "sdmc:/"));
        } else {
            int savedIdx = playlistIndex - 2;
            if (savedIdx >= 0 && savedIdx < (int)saved.size()) {
                LoadPlaylist(saved[savedIdx].pathOrUrl);
            }
        }
    }

    if (kDown & HidNpadButton_X) {
        if (playlistIndex >= 2) {
            int savedIdx = playlistIndex - 2;
            PlaylistManager::Instance().RemoveEntry(savedIdx);
            playlistIndex = std::max(0, playlistIndex - 1);
            toast.Show("Lista eliminada", 1.5f);
        }
    }

    if (kDown & HidNpadButton_B) {
        app->PopScreen();
    }
}

// ============================================================================
// Touch Handling
// ============================================================================

void IPTVBrowser::HandleTouch(const TouchEvent& touch) {
    // Active horizontal shelf drag capture (even if finger drifts outside SHELF_Y vertically)
    if (isDraggingShelf && (touch.type == TouchEvent::Motion || touch.type == TouchEvent::Up)) {
        if (touch.type == TouchEvent::Motion) {
            int deltaX = touch.x - dragStartX;
            currentScrollX = dragStartScrollX - deltaX;
            if (currentScrollX < 0) currentScrollX = 0;
        } else {
            isDraggingShelf = false;
            int deltaX = touch.x - dragStartX;
            if (std::abs(deltaX) < 16 && touch.x >= SIDEBAR_W) {
                int shelfStartX = SIDEBAR_W + 20;
                int total = (int)displayedChannels.size();
                for (int i = 0; i < total; i++) {
                    int cardX = (int)(shelfStartX + i * (POSTER_W + POSTER_GAP) - currentScrollX);
                    if (touch.x >= cardX && touch.x <= cardX + POSTER_W) {
                        if (channelIndex == i) {
                            HandleItemClick(displayedChannels[i]);
                        } else {
                            channelIndex = i;
                            focusZone = FocusZone::Shelf;
                        }
                        return;
                    }
                }
            }
        }
        return;
    }

    // Touch on Sidebar Rail (X: 0 to SIDEBAR_W)
    if (touch.x < SIDEBAR_W) {
        if (touch.type != TouchEvent::Down) return;
        int btnH = 80;
        int startY = 90;
        for (int i = 0; i < (int)SidebarItem::Count; i++) {
            int y = startY + i * btnH;
            if (touch.y >= y && touch.y <= y + btnH) {
                sidebarIndex = i;
                focusZone = FocusZone::Sidebar;
                if (i == 0) {
                    app->PopScreen();
                    return;
                }
                else if (i == 1) focusZone = FocusZone::Shelf;
                else if (i == 2) app->PushScreen(std::make_unique<HistoryScreen>(app, 1));
                else if (i == 3) { PromptSearch(); focusZone = FocusZone::Shelf; }
                else if (i == 4) app->PushScreen(std::make_unique<SettingsScreen>(app));
                return;
            }
        }
        return;
    }

    // Touch on Header
    if (touch.y <= 74) {
        if (touch.type != TouchEvent::Down) return;

        // Back button: Always pops to parent folder or Main Menu
        if (touch.x >= SIDEBAR_W + 10 && touch.x <= SIDEBAR_W + 110) {
            app->PopScreen();
            return;
        }

        // Category filter tap (cycle next)
        int catBtnX = Layout::SCREEN_W - 240;
        if (touch.x >= catBtnX && touch.x <= catBtnX + 220) {
            CycleGroup(+1);
            return;
        }
        return;
    }

    // Touch down on Horizontal Carousel Shelf (Y: SHELF_Y to SHELF_Y + SHELF_H)
    if (touch.type == TouchEvent::Down && touch.y >= SHELF_Y && touch.y <= SHELF_Y + SHELF_H) {
        isDraggingShelf = true;
        dragStartX = touch.x;
        dragStartScrollX = currentScrollX;
    }
}

// ============================================================================
// Update
// ============================================================================

void IPTVBrowser::Update(float dt) {
    toast.Update(dt);

    int total = (int)displayedChannels.size();
    if (total > 0 && !isDraggingShelf) {
        int shelfStartX = SIDEBAR_W + 20;
        int viewportW = Layout::SCREEN_W - shelfStartX;

        // Auto-center target scroll on the focused poster
        targetScrollX = channelIndex * (POSTER_W + POSTER_GAP) - (viewportW / 2 - POSTER_W / 2);
        if (targetScrollX < 0) targetScrollX = 0;

        float maxScroll = std::max(0, (total * (POSTER_W + POSTER_GAP)) - viewportW + 60);
        if (targetScrollX > maxScroll) targetScrollX = maxScroll;

        // Smooth critically damped lerp
        currentScrollX += (targetScrollX - currentScrollX) * std::min(1.0f, 15.0f * dt);
    }
}

// ============================================================================
// Render
// ============================================================================

void IPTVBrowser::Render(SDL_Renderer* renderer) {
    // Draw main shelf content first (strictly clipped right of sidebar), then draw sidebar on top
    RenderShelfView(renderer);
    RenderSidebar(renderer);
    toast.Render(renderer, app->GetFontSmall());
}

// ============================================================================
// Render: Sidebar Navigation Rail (Minimum 64x64 dp touch target)
// ============================================================================

void IPTVBrowser::RenderSidebar(SDL_Renderer* renderer) {
    // Sidebar background panel (soft translucent dark veil)
    UI::DrawRect(renderer, 0, 0, SIDEBAR_W, Layout::SCREEN_H, {20, 20, 34, 215});
    UI::DrawRect(renderer, SIDEBAR_W - 1, 0, 1, Layout::SCREEN_H, Colors::DIVIDER); // Vertical separator

    // Logo / Brand Icon at top (Switch Joy-Con logo)
    UI::DrawRoundedRect(renderer, 24, 20, 48, 48, 12, Colors::BG_SELECTED);
    UI::DrawSwitchJoyconIcon(renderer, 48, 44, 18);

    static const char* navLabels[5] = {
        "Inicio", "Catalogo", "Favoritos", "Buscar", "Ajustes"
    };

    int startY = 100;
    int btnH = 80;
    int btnW = 76;
    int btnX = 10;

    for (int i = 0; i < 5; i++) {
        int y = startY + i * btnH;
        bool isFocused = (focusZone == FocusZone::Sidebar && sidebarIndex == i);
        bool isActive = (!isFocused && sidebarIndex == i);

        // Focus & active button styling
        if (isFocused) {
            UI::DrawRoundedRect(renderer, btnX, y, btnW, 68, 14, Colors::BG_SELECTED);
            // Glowing border
            UI::DrawRoundedRect(renderer, btnX, y, 4, 68, 2, Colors::DANGO_PINK);
        } else if (isActive) {
            UI::DrawRoundedRect(renderer, 0, y + 14, 4, 40, 2, Colors::DANGO_PINK);
        }

        SDL_Color iconCol = (isFocused || isActive) ? Colors::DANGO_PINK : Colors::TEXT_DISABLED;
        SDL_Color textCol = (isFocused || isActive) ? Colors::TEXT_PRIMARY : Colors::TEXT_DISABLED;

        int iconCx = btnX + btnW / 2;
        int iconCy = y + 22;

        switch (i) {
            case 0: UI::DrawIconHome(renderer, iconCx, iconCy, 26, iconCol); break;
            case 1: UI::DrawIconTV(renderer, iconCx, iconCy, 26, iconCol); break;
            case 2: UI::DrawIconStar(renderer, iconCx, iconCy, 24, iconCol); break;
            case 3: UI::DrawIconSearch(renderer, iconCx, iconCy, 24, iconCol); break;
            case 4: UI::DrawIconSettings(renderer, iconCx, iconCy, 26, iconCol); break;
        }

        // Compact title under icon
        UI::DrawTextCentered(renderer, app->GetFontTiny(), navLabels[i],
                             {btnX, y + 42, btnW, 20}, textCol);
    }
}

// ============================================================================
// Render: Single-Row Horizontal Carousel / Shelf View
// ============================================================================

void IPTVBrowser::RenderShelfView(SDL_Renderer* renderer) {
    const auto& pl = GetActivePlaylist();
    int headerX = SIDEBAR_W + 20;
    int headerY = 22;

    // Back Button [B]
    UI::DrawRoundedRect(renderer, headerX, headerY, 90, 42, 8, Colors::BG_CARD);
    UI::DrawRoundedRect(renderer, headerX + 8, headerY + 9, 24, 24, 6, Colors::DANGO_PINK);
    UI::DrawText(renderer, app->GetFontTiny(), "B",
                 headerX + 20, headerY + 12, Colors::TEXT_PRIMARY, true, false);
    UI::DrawText(renderer, app->GetFontSmall(), isSubFolder ? "Atras" : "Inicio",
                 headerX + 38, headerY + 11, Colors::TEXT_SECONDARY, false, false);

    // Title / Breadcrumbs
    std::string plTitle = pl.name;
    if (isSubFolder && !folderTitle.empty()) {
        UI::DrawIconFolder(renderer, headerX + 120, headerY + 21, 22, Colors::DANGO_ORANGE);
        UI::DrawText(renderer, app->GetFont(), StringUtils::Truncate(folderTitle, 36),
                     headerX + 138, headerY + 9, Colors::DANGO_CREAM, false, false);
    } else {
        UI::DrawText(renderer, app->GetFont(), StringUtils::Truncate(plTitle, 36),
                     headerX + 110, headerY + 9, Colors::DANGO_CREAM, false, false);
    }

    // Category Selector pill on right
    int catBtnX = Layout::SCREEN_W - 250;
    UI::DrawRoundedRect(renderer, catBtnX, headerY, 230, 42, 8, Colors::BG_CARD);

    std::string curGroup = (groupIndex < (int)pl.groups.size()) ? pl.groups[groupIndex] : "Todos";
    std::string groupLabel = "L < " + StringUtils::Truncate(curGroup, 12) + " > R";
    UI::DrawTextCentered(renderer, app->GetFontSmall(), groupLabel,
                         {catBtnX, headerY, 230, 42}, Colors::DANGO_PINK);

    UI::DrawDivider(renderer, headerX, 74, Layout::SCREEN_W - headerX - 20);

    // ------------------------------------------------------------------------
    // Horizontal Poster Shelf (Virtualization + Lazy Texture Loading)
    // ------------------------------------------------------------------------
    int total = (int)displayedChannels.size();
    int shelfStartX = headerX;
    int shelfY = SHELF_Y;

    if (total == 0) {
        UI::DrawText(renderer, app->GetFont(), "No se encontraron elementos en esta seccion",
                     Layout::SCREEN_W / 2 + 30, 300, Colors::TEXT_DISABLED, true, false);
    } else {
        // Strictly clip horizontal shelf strictly to the right of the sidebar rail
        SDL_Rect shelfClip = { SIDEBAR_W + 2, 76, Layout::SCREEN_W - (SIDEBAR_W + 2), 404 };
        SDL_RenderSetClipRect(renderer, &shelfClip);

        for (int i = 0; i < total; i++) {
            int cardX = (int)(shelfStartX + i * (POSTER_W + POSTER_GAP) - currentScrollX);
            bool isFocused = (focusZone == FocusZone::Shelf && i == channelIndex);
            int cardW = isFocused ? POSTER_FOCUSED_W : POSTER_W;
            int cardH = isFocused ? POSTER_FOCUSED_H : POSTER_H;
            int cardY = isFocused ? (shelfY - (POSTER_FOCUSED_H - POSTER_H) / 2) : shelfY;

            // Virtualization culling: skip posters strictly outside the clipped viewport
            if (cardX + cardW + 8 < SIDEBAR_W || cardX > Layout::SCREEN_W + 40) {
                continue;
            }

            RenderPosterCard(renderer, displayedChannels[i], i, cardX, cardY, cardW, cardH, isFocused);
        }

        SDL_RenderSetClipRect(renderer, nullptr);
    }

    // ------------------------------------------------------------------------
    // Active Item Detail & Footer Area
    // ------------------------------------------------------------------------
    int detailY = 482;
    UI::DrawDivider(renderer, headerX, detailY, Layout::SCREEN_W - headerX - 20);

    if (total > 0 && channelIndex >= 0 && channelIndex < total) {
        const auto& ch = displayedChannels[channelIndex];
        bool isContainer = (ch.type == PlaylistItemType::FolderContainer);

        // Big Item Title with vector icon
        if (isContainer) {
            UI::DrawIconFolder(renderer, headerX + 16, detailY + 30, 26, Colors::DANGO_ORANGE);
            UI::DrawText(renderer, app->GetFontLarge(), StringUtils::Truncate(ch.name, 48),
                         headerX + 38, detailY + 12, Colors::TEXT_PRIMARY, false, false);
        } else {
            UI::DrawIconFilm(renderer, headerX + 16, detailY + 30, 26, Colors::DANGO_PINK);
            UI::DrawText(renderer, app->GetFontLarge(), StringUtils::Truncate(ch.name, 48),
                         headerX + 38, detailY + 12, Colors::TEXT_PRIMARY, false, false);
        }

        // Item Subtitle / Category
        std::string subInfo = isContainer ? ("[Carpeta / Categoria] " + ch.groupTitle) : ("[Video] " + ch.groupTitle);
        UI::DrawText(renderer, app->GetFontSmall(), subInfo,
                     headerX, detailY + 46, Colors::DANGO_PINK, false, false);

        // URL / Source Preview
        UI::DrawText(renderer, app->GetFontTiny(), StringUtils::Truncate(ch.url, 80),
                     headerX, detailY + 70, Colors::TEXT_DISABLED, false, false);

        // Controller Action Buttons
        int hintY = detailY + 98;

        // [A] Button
        UI::DrawRoundedRect(renderer, headerX, hintY, 26, 26, 6, Colors::DANGO_PINK);
        UI::DrawText(renderer, app->GetFontTiny(), "A", headerX + 13, hintY + 5, Colors::TEXT_PRIMARY, true, false);

        const char* aAction = isContainer ? "Abrir carpeta" : "Reproducir";
        UI::DrawText(renderer, app->GetFontSmall(), aAction, headerX + 32, hintY + 2, Colors::TEXT_PRIMARY, false, false);

        // [X] Favorite
        int xFavX = headerX + 170;
        UI::DrawRoundedRect(renderer, xFavX, hintY, 26, 26, 6, Colors::DANGO_ORANGE);
        UI::DrawText(renderer, app->GetFontTiny(), "X", xFavX + 13, hintY + 5, Colors::TEXT_PRIMARY, true, false);
        UI::DrawText(renderer, app->GetFontSmall(), "Favorito", xFavX + 32, hintY + 2, Colors::TEXT_SECONDARY, false, false);

        // [Y] Search
        int ySearchX = headerX + 290;
        UI::DrawRoundedRect(renderer, ySearchX, hintY, 26, 26, 6, Colors::DANGO_CREAM);
        UI::DrawText(renderer, app->GetFontTiny(), "Y", ySearchX + 13, hintY + 5, Colors::BG_DARK, true, false);
        UI::DrawTextCentered(renderer, app->GetFontSmall(), "Buscar", {ySearchX + 26, hintY, 70, 26}, Colors::TEXT_SECONDARY);

        // Item Counter on the right
        char countBuf[64];
        snprintf(countBuf, sizeof(countBuf), "%d / %d", channelIndex + 1, total);
        UI::DrawText(renderer, app->GetFontSmall(), countBuf,
                     Layout::SCREEN_W - 40, hintY + 2, Colors::TEXT_DISABLED, true, false);
    }
}

// ============================================================================
// Render: Poster Card (Proportional vertical poster + distinct title bar)
// ============================================================================

void IPTVBrowser::RenderPosterCard(SDL_Renderer* renderer, const ChannelItem& ch, int index,
                                   int x, int y, int w, int h, bool isFocused) {
    (void)index;
    bool isContainer = (ch.type == PlaylistItemType::FolderContainer);

    // Active Highlight: glowing outer border and elevation
    if (isFocused) {
        SDL_Color glowCol = isContainer ? Colors::DANGO_ORANGE : Colors::DANGO_PINK;
        UI::DrawRoundedRect(renderer, x - 4, y - 4, w + 8, h + 8, 14, glowCol);
    }

    // Card background
    SDL_Color cardBg = isFocused ? Colors::BG_SELECTED : Colors::BG_CARD;
    UI::DrawRoundedRect(renderer, x, y, w, h, 10, cardBg);

    // Title bar at the bottom of the card
    int titleH = isFocused ? 50 : 46;
    int posterAreaH = h - titleH - 4; // Area reserved purely for the poster image
    int posterAreaW = w - 4;

    // Try loading / retrieving poster image from TextureCache
    SDL_Texture* posterTex = nullptr;
    if (!ch.logoUrl.empty()) {
        posterTex = TextureCache::Instance().GetTexture(renderer, ch.logoUrl);
    }

    if (posterTex) {
        int texW = 0, texH = 0;
        SDL_QueryTexture(posterTex, NULL, NULL, &texW, &texH);

        int drawW = posterAreaW;
        int drawH = posterAreaH;
        int drawX = x + 2;
        int drawY = y + 2;

        if (texW > 0 && texH > 0) {
            float imgRatio = (float)texH / (float)texW;
            float boxRatio = (float)posterAreaH / (float)posterAreaW;

            // Preserve natural aspect ratio so posters are never squished or compressed
            if (imgRatio > boxRatio) {
                // Image is taller than poster box -> fit height, center horizontally
                drawH = posterAreaH;
                drawW = std::min(posterAreaW, (int)(posterAreaH / imgRatio));
                drawX = x + 2 + (posterAreaW - drawW) / 2;
            } else {
                // Image is wider/squarer than poster box -> fit width, center vertically
                drawW = posterAreaW;
                drawH = std::min(posterAreaH, (int)(posterAreaW * imgRatio));
                drawY = y + 2 + (posterAreaH - drawH) / 2;
            }
        }

        SDL_Rect dst{drawX, drawY, drawW, drawH};
        SDL_RenderCopy(renderer, posterTex, NULL, &dst);
    } else {
        // High-end placeholder card inside poster area
        UI::DrawRoundedRect(renderer, x + 4, y + 4, w - 8, posterAreaH - 4, 8, Colors::BG_DARK);

        // Centered vector icon (vector folder for folders, film frame for videos)
        if (isContainer) {
            UI::DrawIconFolder(renderer, x + w / 2, y + posterAreaH / 3, 56, Colors::DANGO_ORANGE);
        } else {
            UI::DrawIconFilm(renderer, x + w / 2, y + posterAreaH / 3, 56, Colors::DANGO_PINK);
        }

        // Poster title inside placeholder
        UI::DrawTextCentered(renderer, app->GetFontSmall(), StringUtils::Truncate(ch.name, 20),
                             {x + 8, y + posterAreaH / 2 + 10, w - 16, 36}, Colors::TEXT_PRIMARY);
    }

    // Badge on Top Right
    int badgeW = 46;
    int badgeH = 24;
    int badgeX = x + w - badgeW - 8;
    int badgeY = y + 8;

    if (isContainer) {
        UI::DrawRoundedRect(renderer, badgeX, badgeY, badgeW, badgeH, 6, Colors::DANGO_ORANGE);
        UI::DrawTextCentered(renderer, app->GetFontTiny(), "DIR", {badgeX, badgeY, badgeW, badgeH}, Colors::TEXT_PRIMARY);
    } else {
        std::string extLower = StringUtils::ToLower(StringUtils::GetExtension(ch.url));
        bool isVod = (extLower == ".mp4" || extLower == ".mkv" || extLower == ".avi");
        SDL_Color bColor = isVod ? Colors::DANGO_PINK : Colors::DANGO_GREEN;
        const char* bText = isVod ? "VOD" : "LIVE";

        UI::DrawRoundedRect(renderer, badgeX, badgeY, badgeW, badgeH, 6, bColor);
        UI::DrawTextCentered(renderer, app->GetFontTiny(), bText, {badgeX, badgeY, badgeW, badgeH}, Colors::BG_DARK);
    }

    // Bottom title bar under poster image (placed strictly underneath the artwork, never crops)
    UI::DrawRoundedRect(renderer, x, y + h - titleH, w, titleH, 8, isFocused ? Colors::BG_SELECTED : Colors::BG_CARD);
    UI::DrawTextCentered(renderer, app->GetFontSmall(), StringUtils::Truncate(ch.name, 18),
                         {x + 6, y + h - titleH + 3, w - 12, 22}, isFocused ? Colors::TEXT_PRIMARY : Colors::TEXT_SECONDARY);
    UI::DrawTextCentered(renderer, app->GetFontTiny(), StringUtils::Truncate(ch.groupTitle, 16),
                         {x + 6, y + h - titleH + 25, w - 12, 18}, Colors::TEXT_DISABLED);
}

// ============================================================================
// Render: Playlists View (Root View)
// ============================================================================

void IPTVBrowser::RenderPlaylistsView(SDL_Renderer* renderer) {
    int headerY = 28;

    // Back Button [B]
    UI::DrawRoundedRect(renderer, Layout::MARGIN, headerY, 80, 44, 8, Colors::BG_CARD);
    UI::DrawRoundedRect(renderer, Layout::MARGIN + 8, headerY + 10, 24, 24, 6, Colors::DANGO_PINK);
    UI::DrawText(renderer, app->GetFontTiny(), "B",
                 Layout::MARGIN + 20, headerY + 13, Colors::TEXT_PRIMARY, true, false);
    UI::DrawText(renderer, app->GetFontSmall(), "Menu",
                 Layout::MARGIN + 38, headerY + 12, Colors::TEXT_SECONDARY, false, false);

    UI::DrawText(renderer, app->GetFontLarge(), "Listas IPTV & Streaming",
                 Layout::MARGIN + 100, headerY + 4, Colors::DANGO_CREAM, false, false);

    UI::DrawDivider(renderer, Layout::MARGIN, 84, Layout::SCREEN_W - Layout::MARGIN * 2);

    int startY = 114;
    int rowH = 74;
    int rowGap = 10;
    int listW = Layout::SCREEN_W - Layout::MARGIN * 2;
    const auto& saved = PlaylistManager::Instance().GetSaved();
    int total = 2 + (int)saved.size();

    for (int i = 0; i < total; i++) {
        int y = startY + i * (rowH + rowGap);
        bool isSel = (i == playlistIndex);

        SDL_Color cardBg = isSel ? Colors::BG_SELECTED : Colors::BG_CARD;
        UI::DrawRoundedRect(renderer, Layout::MARGIN, y, listW, rowH, Layout::CORNER_RADIUS, cardBg);

        if (isSel) {
            UI::DrawRoundedRect(renderer, Layout::MARGIN, y, 4, rowH, 2, Colors::DANGO_PINK);
        }

        if (i == 0) {
            UI::DrawRoundedRect(renderer, Layout::MARGIN + 16, y + 18, 40, 38, 6, Colors::DANGO_PINK);
            UI::DrawText(renderer, app->GetFontTiny(), "URL", Layout::MARGIN + 36, y + 29, Colors::TEXT_PRIMARY, true, false);
            UI::DrawText(renderer, app->GetFont(), "+ Anadir Lista por URL (Teclado)", Layout::MARGIN + 68, y + 14, Colors::TEXT_PRIMARY, false, false);
            UI::DrawText(renderer, app->GetFontTiny(), "Descargar y sincronizar lista M3U remota mediante HTTP/HTTPS", Layout::MARGIN + 68, y + 44, Colors::TEXT_DISABLED, false, false);
        } else if (i == 1) {
            UI::DrawRoundedRect(renderer, Layout::MARGIN + 16, y + 18, 40, 38, 6, Colors::DANGO_ORANGE);
            UI::DrawText(renderer, app->GetFontTiny(), "SD", Layout::MARGIN + 36, y + 29, Colors::TEXT_PRIMARY, true, false);
            UI::DrawText(renderer, app->GetFont(), "Cargar lista desde la tarjeta SD", Layout::MARGIN + 68, y + 14, Colors::TEXT_PRIMARY, false, false);
            UI::DrawText(renderer, app->GetFontTiny(), "Explorar archivos .m3u, .m3u8 o .pls locales", Layout::MARGIN + 68, y + 44, Colors::TEXT_DISABLED, false, false);
        } else {
            const auto& entry = saved[i - 2];
            UI::DrawRoundedRect(renderer, Layout::MARGIN + 16, y + 18, 40, 38, 6, Colors::DANGO_GREEN);
            UI::DrawText(renderer, app->GetFontTiny(), entry.isRemote ? "NET" : "M3U", Layout::MARGIN + 36, y + 29, Colors::BG_DARK, true, false);
            UI::DrawText(renderer, app->GetFont(), entry.name, Layout::MARGIN + 68, y + 14, Colors::TEXT_PRIMARY, false, false);
            std::string sub = StringUtils::Truncate(entry.pathOrUrl, 60);
            UI::DrawText(renderer, app->GetFontTiny(), sub, Layout::MARGIN + 68, y + 44, Colors::TEXT_DISABLED, false, false);
        }
    }

    // Footer hints
    int footerY = Layout::SCREEN_H - Layout::FOOTER_H - 10;
    UI::DrawDivider(renderer, Layout::MARGIN, footerY, Layout::SCREEN_W - Layout::MARGIN * 2);

    int hintY = footerY + 14;
    UI::DrawRoundedRect(renderer, Layout::MARGIN, hintY, 26, 26, 6, Colors::DANGO_PINK);
    UI::DrawText(renderer, app->GetFontTiny(), "A", Layout::MARGIN + 13, hintY + 5, Colors::TEXT_PRIMARY, true, false);
    UI::DrawText(renderer, app->GetFontSmall(), "Abrir", Layout::MARGIN + 32, hintY + 2, Colors::TEXT_SECONDARY, false, false);

    int xX = Layout::MARGIN + 120;
    UI::DrawRoundedRect(renderer, xX, hintY, 26, 26, 6, Colors::DANGO_ORANGE);
    UI::DrawText(renderer, app->GetFontTiny(), "X", xX + 13, hintY + 5, Colors::TEXT_PRIMARY, true, false);
    UI::DrawText(renderer, app->GetFontSmall(), "Eliminar guardada", xX + 32, hintY + 2, Colors::TEXT_SECONDARY, false, false);
}
