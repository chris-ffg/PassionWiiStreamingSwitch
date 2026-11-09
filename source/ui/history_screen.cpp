// ============================================================================
// DangoPlayer Switch — History & Favorites Screen Implementation
// ============================================================================

#include "ui/history_screen.h"
#include "ui/player_screen.h"
#include "app.h"
#include "ui/ui_common.h"
#include "utils/string_utils.h"
#include "utils/logger.h"
#include <algorithm>
#include <cstdio>

HistoryScreen::HistoryScreen(App* app) : Screen(app) {}

void HistoryScreen::OnEnter() {
    Logger::Info("HistoryScreen: OnEnter");
    RefreshLists();
}

void HistoryScreen::RefreshLists() {
    selectedIndex = 0;
    scrollOffset = 0;
}

void HistoryScreen::HandleInput(u64 kDown, u64 kHeld) {
    int total = (activeTab == 0)
                ? (int)HistoryManager::Instance().GetCount()
                : (int)FavoritesManager::Instance().GetCount();

    // Tab Switch [L / R]
    if (kDown & (HidNpadButton_L | HidNpadButton_R)) {
        activeTab = (activeTab == 0) ? 1 : 0;
        RefreshLists();
        toast.Show(activeTab == 0 ? "Pestana: Historial" : "Pestana: Favoritos", 1.0f);
        return;
    }

    // D-Pad / Stick navigation
    if (kDown & HidNpadButton_AnyDown) {
        if (total > 0 && selectedIndex < total - 1) {
            selectedIndex++;
            if (selectedIndex >= scrollOffset + ITEMS_PER_PAGE) {
                scrollOffset = selectedIndex - ITEMS_PER_PAGE + 1;
            }
        }
    }
    if (kDown & HidNpadButton_AnyUp) {
        if (total > 0 && selectedIndex > 0) {
            selectedIndex--;
            if (selectedIndex < scrollOffset) {
                scrollOffset = selectedIndex;
            }
        }
    }

    // Play item [A]
    if (kDown & HidNpadButton_A) {
        if (total > 0 && selectedIndex >= 0 && selectedIndex < total) {
            if (activeTab == 0) {
                const auto& h = HistoryManager::Instance().GetEntries()[selectedIndex];
                Logger::Info("HistoryScreen: Resuming from history -> %s", h.pathOrUrl.c_str());
                app->PushScreen(std::make_unique<PlayerScreen>(app, h.pathOrUrl, h.title));
            } else {
                const auto& f = FavoritesManager::Instance().GetFavorites()[selectedIndex];
                Logger::Info("HistoryScreen: Playing favorite -> %s", f.pathOrUrl.c_str());
                app->PushScreen(std::make_unique<PlayerScreen>(app, f.pathOrUrl, f.title));
            }
        }
    }

    // Delete / Clear [X]
    if (kDown & HidNpadButton_X) {
        if (activeTab == 0) {
            HistoryManager::Instance().Clear();
            RefreshLists();
            toast.Show("Historial borrado", 1.5f);
        } else {
            if (total > 0 && selectedIndex >= 0 && selectedIndex < total) {
                const auto& f = FavoritesManager::Instance().GetFavorites()[selectedIndex];
                FavoritesManager::Instance().RemoveFavorite(f.pathOrUrl);
                selectedIndex = std::max(0, selectedIndex - 1);
                toast.Show("Favorito eliminado", 1.5f);
            }
        }
    }

    // Back to Main Menu [B]
    if (kDown & HidNpadButton_B) {
        app->PopScreen();
    }
}

void HistoryScreen::HandleTouch(const TouchEvent& touch) {
    if (touch.type != TouchEvent::Down) return;

    // Header Back button
    if (touch.x >= Layout::MARGIN && touch.x <= Layout::MARGIN + 120 &&
        touch.y >= 30 && touch.y <= 85) {
        app->PopScreen();
        return;
    }

    // Tab buttons touch
    int tabW = 160;
    int tab1X = Layout::SCREEN_W - Layout::MARGIN - tabW * 2 - 10;
    int tab2X = Layout::SCREEN_W - Layout::MARGIN - tabW;
    int tabY = 28;

    if (touch.y >= tabY && touch.y <= tabY + 44) {
        if (touch.x >= tab1X && touch.x < tab1X + tabW) {
            activeTab = 0;
            RefreshLists();
            return;
        } else if (touch.x >= tab2X && touch.x < tab2X + tabW) {
            activeTab = 1;
            RefreshLists();
            return;
        }
    }

    // List items touch
    int listStartY = 96;
    int total = (activeTab == 0)
                ? (int)HistoryManager::Instance().GetCount()
                : (int)FavoritesManager::Instance().GetCount();

    for (int row = 0; row < ITEMS_PER_PAGE; row++) {
        int idx = scrollOffset + row;
        if (idx >= total) break;

        int y = listStartY + row * (ITEM_H + ITEM_GAP);
        if (touch.x >= Layout::MARGIN && touch.x <= Layout::SCREEN_W - Layout::MARGIN &&
            touch.y >= y && touch.y <= y + ITEM_H) {
            selectedIndex = idx;
            if (activeTab == 0) {
                const auto& ent = HistoryManager::Instance().GetEntries()[idx];
                app->PushScreen(std::make_unique<PlayerScreen>(app, ent.pathOrUrl, ent.title));
            } else {
                const auto& fav = FavoritesManager::Instance().GetFavorites()[idx];
                app->PushScreen(std::make_unique<PlayerScreen>(app, fav.pathOrUrl, fav.title));
            }
            return;
        }
    }
}

void HistoryScreen::Update(float dt) {
    toast.Update(dt);
}

void HistoryScreen::Render(SDL_Renderer* renderer) {
    int headerY = 28;

    // Back Button [B]
    UI::DrawRoundedRect(renderer, Layout::MARGIN, headerY, 80, 44, 8, Colors::BG_CARD);
    UI::DrawRoundedRect(renderer, Layout::MARGIN + 8, headerY + 10, 24, 24, 6, Colors::DANGO_PINK);
    UI::DrawText(renderer, app->GetFontTiny(), "B",
                 Layout::MARGIN + 20, headerY + 13, Colors::TEXT_PRIMARY, true, false);
    UI::DrawText(renderer, app->GetFontSmall(), "Menu",
                 Layout::MARGIN + 38, headerY + 12, Colors::TEXT_SECONDARY, false, false);

    // Title
    UI::DrawText(renderer, app->GetFontLarge(), (activeTab == 0 ? "Historial Reciente" : "Canales Favoritos"),
                 Layout::MARGIN + 100, headerY + 4, Colors::DANGO_CREAM, false, false);

    // Tab buttons on top right
    int tabW = 160;
    int tab1X = Layout::SCREEN_W - Layout::MARGIN - tabW * 2 - 10;
    int tab2X = Layout::SCREEN_W - Layout::MARGIN - tabW;

    SDL_Color tab1Bg = (activeTab == 0) ? Colors::BG_SELECTED : Colors::BG_CARD;
    SDL_Color tab2Bg = (activeTab == 1) ? Colors::BG_SELECTED : Colors::BG_CARD;

    UI::DrawRoundedRect(renderer, tab1X, headerY, tabW, 44, 8, tab1Bg);
    UI::DrawTextCentered(renderer, app->GetFontSmall(), "L  Recientes", {tab1X, headerY, tabW, 44},
                         activeTab == 0 ? Colors::DANGO_PINK : Colors::TEXT_SECONDARY);

    UI::DrawRoundedRect(renderer, tab2X, headerY, tabW, 44, 8, tab2Bg);
    UI::DrawTextCentered(renderer, app->GetFontSmall(), "Favoritos  R", {tab2X, headerY, tabW, 44},
                         activeTab == 1 ? Colors::DANGO_PINK : Colors::TEXT_SECONDARY);

    UI::DrawDivider(renderer, Layout::MARGIN, 84, Layout::SCREEN_W - Layout::MARGIN * 2);

    // List rendering
    int listStartY = 96;
    int listX = Layout::MARGIN;
    int listW = Layout::SCREEN_W - Layout::MARGIN * 2 - 20;

    int total = (activeTab == 0)
                ? (int)HistoryManager::Instance().GetCount()
                : (int)FavoritesManager::Instance().GetCount();

    if (total == 0) {
        const char* emptyMsg = (activeTab == 0)
                               ? "No hay elementos en el historial de reproduccion"
                               : "No hay canales o videos marcados como favoritos";
        UI::DrawText(renderer, app->GetFont(), emptyMsg,
                     Layout::SCREEN_W / 2, 320, Colors::TEXT_DISABLED, true, false);
    } else {
        int firstIndex = std::max(0, scrollOffset);
        int lastIndex = std::min(total - 1, scrollOffset + ITEMS_PER_PAGE - 1);

        for (int i = firstIndex; i <= lastIndex; i++) {
            int row = i - scrollOffset;
            int itemY = listStartY + row * (ITEM_H + ITEM_GAP);
            bool isSel = (i == selectedIndex);

            SDL_Color cardBg = isSel ? Colors::BG_SELECTED : Colors::BG_CARD;
            UI::DrawRoundedRect(renderer, listX, itemY, listW, ITEM_H, Layout::CORNER_RADIUS, cardBg);

            if (isSel) {
                UI::DrawRoundedRect(renderer, listX, itemY, 4, ITEM_H, 2, Colors::DANGO_PINK);
            }

            if (activeTab == 0) {
                const auto& item = HistoryManager::Instance().GetEntries()[i];

                // Badge
                UI::DrawRoundedRect(renderer, listX + 16, itemY + 16, 48, 36, 6,
                                    item.isStream ? Colors::DANGO_GREEN : Colors::DANGO_ORANGE);
                UI::DrawText(renderer, app->GetFontTiny(), item.isStream ? "NET" : "FILE",
                             listX + 40, itemY + 26, Colors::BG_DARK, true, false);

                // Title
                SDL_Color textCol = isSel ? Colors::TEXT_PRIMARY : Colors::TEXT_SECONDARY;
                UI::DrawText(renderer, app->GetFont(), StringUtils::Truncate(item.title, 48),
                             listX + 76, itemY + 12, textCol, false, false);

                // Progress / Position
                std::string sub = StringUtils::FormatTime(item.lastPosition);
                if (item.duration > 0.0) {
                    sub += " / " + StringUtils::FormatTime(item.duration);
                }
                UI::DrawText(renderer, app->GetFontTiny(), sub, listX + 76, itemY + 40, Colors::TEXT_DISABLED, false, false);
            } else {
                const auto& item = FavoritesManager::Instance().GetFavorites()[i];

                // Star badge
                UI::DrawRoundedRect(renderer, listX + 16, itemY + 16, 48, 36, 6, Colors::DANGO_CREAM);
                UI::DrawText(renderer, app->GetFontTiny(), "FAV", listX + 40, itemY + 26, Colors::BG_DARK, true, false);

                // Title
                SDL_Color textCol = isSel ? Colors::TEXT_PRIMARY : Colors::TEXT_SECONDARY;
                UI::DrawText(renderer, app->GetFont(), StringUtils::Truncate(item.title, 48),
                             listX + 76, itemY + 12, textCol, false, false);

                UI::DrawText(renderer, app->GetFontTiny(), item.group, listX + 76, itemY + 40, Colors::TEXT_DISABLED, false, false);
            }
        }
    }

    // Footer
    int footerY = Layout::SCREEN_H - Layout::FOOTER_H - 10;
    UI::DrawDivider(renderer, Layout::MARGIN, footerY, Layout::SCREEN_W - Layout::MARGIN * 2);

    int hintY = footerY + 14;
    UI::DrawRoundedRect(renderer, Layout::MARGIN, hintY, 26, 26, 6, Colors::DANGO_PINK);
    UI::DrawText(renderer, app->GetFontTiny(), "A", Layout::MARGIN + 13, hintY + 5, Colors::TEXT_PRIMARY, true, false);
    UI::DrawText(renderer, app->GetFontSmall(), "Reproducir", Layout::MARGIN + 32, hintY + 2, Colors::TEXT_SECONDARY, false, false);

    int xX = Layout::MARGIN + 160;
    UI::DrawRoundedRect(renderer, xX, hintY, 26, 26, 6, Colors::DANGO_ORANGE);
    UI::DrawText(renderer, app->GetFontTiny(), "X", xX + 13, hintY + 5, Colors::TEXT_PRIMARY, true, false);
    UI::DrawText(renderer, app->GetFontSmall(), (activeTab == 0 ? "Borrar Historial" : "Quitar Favorito"),
                 xX + 32, hintY + 2, Colors::TEXT_SECONDARY, false, false);

    toast.Render(renderer, app->GetFontSmall());
}
