// ============================================================================
// DangoPlayer Switch — Main Menu Implementation
// ============================================================================

#include "ui/main_menu.h"
#include "ui/file_browser.h"
#include "ui/iptv_browser.h"
#include "ui/history_screen.h"
#include "ui/settings_screen.h"
#include "app.h"
#include "ui/ui_common.h"
#include "utils/logger.h"
#include <SDL2/SDL2_gfxPrimitives.h>
#include <cmath>
#include <cstdio>

// ============================================================================
// Constructor — Define menu items
// ============================================================================

MainMenu::MainMenu(App* app) : Screen(app) {
    items = {
        {"PassionWii Streaming",   "Catalogo de Peliculas, Series, Animes y Doramas"},
        {"Archivos Locales",       "Reproducir videos desde la tarjeta SD o USB"},
        {"Historial y Favoritos",  "Contenido reciente y canales favoritos"},
        {"Configuracion",          "Ajustes de la aplicacion y reproductor"},
    };
}

// ============================================================================
// Lifecycle
// ============================================================================

void MainMenu::OnEnter() {
    Logger::Info("MainMenu: OnEnter");
    fadeIn = 0.0f;
}

// ============================================================================
// Input — D-pad navigation, A to select
// ============================================================================

void MainMenu::HandleInput(u64 kDown, u64 kHeld) {
    int count = (int)items.size();

    if (kDown & HidNpadButton_AnyDown) {
        selectedIndex = (selectedIndex + 1) % count;
    }

    if (kDown & HidNpadButton_AnyUp) {
        selectedIndex = (selectedIndex - 1 + count) % count;
    }

    if (kDown & HidNpadButton_A) {
        Logger::Info("MainMenu: Selected [%d] %s", selectedIndex, items[selectedIndex].label.c_str());

        switch (selectedIndex) {
            case 0:
                app->PushScreen(std::make_unique<IPTVBrowser>(app));
                break;
            case 1:
                app->PushScreen(std::make_unique<FileBrowser>(app));
                break;
            case 2:
                app->PushScreen(std::make_unique<HistoryScreen>(app));
                break;
            case 3:
                app->PushScreen(std::make_unique<SettingsScreen>(app));
                break;
        }
    }
}

// ============================================================================
// Touch Input — Direct tapping on menu items & footer buttons
// ============================================================================

void MainMenu::HandleTouch(const TouchEvent& touch) {
    if (touch.type != TouchEvent::Down) return;

    int menuStartY = 220;
    int itemWidth  = 620;
    int itemX      = (Layout::SCREEN_W - itemWidth) / 2;

    // Check menu item clicks
    for (int i = 0; i < (int)items.size(); i++) {
        int itemY = menuStartY + i * (Layout::ITEM_HEIGHT + Layout::ITEM_SPACING);
        if (touch.x >= itemX && touch.x <= itemX + itemWidth &&
            touch.y >= itemY && touch.y <= itemY + Layout::ITEM_HEIGHT) {
            
            if (selectedIndex == i) {
                // Second tap confirms selection
                switch (i) {
                    case 0:
                        app->PushScreen(std::make_unique<IPTVBrowser>(app));
                        break;
                    case 1:
                        app->PushScreen(std::make_unique<FileBrowser>(app));
                        break;
                    case 2:
                        app->PushScreen(std::make_unique<HistoryScreen>(app));
                        break;
                    case 3:
                        app->PushScreen(std::make_unique<SettingsScreen>(app));
                        break;
                }
            } else {
                selectedIndex = i;
            }
            return;
        }
    }

    // Check footer "+ Salir" button tap
    int footerY = Layout::SCREEN_H - Layout::FOOTER_H - 12;
    int exitBtnX = Layout::MARGIN + 210;
    int exitBtnY = footerY + 16;
    if (touch.x >= exitBtnX && touch.x <= exitBtnX + 100 &&
        touch.y >= exitBtnY - 6 && touch.y <= exitBtnY + 36) {
        Logger::Info("MainMenu: Tap on Exit button");
        app->Quit();
        return;
    }
}

// ============================================================================
// Update — Animations
// ============================================================================

void MainMenu::Update(float dt) {
    toast.Update(dt);

    // Fade in
    if (fadeIn < 1.0f) {
        fadeIn += dt * 2.5f;
        if (fadeIn > 1.0f) fadeIn = 1.0f;
    }

    // Title version pulse (slow sine wave)
    titlePulse += dt * 1.5f;
    if (titlePulse > 6.2831853f) titlePulse -= 6.2831853f;  // wrap at 2π

    // Smooth selection cursor interpolation
    float targetY = (float)selectedIndex;
    float diff = targetY - selectionAnimY;
    selectionAnimY += diff * dt * 14.0f;  // Smooth spring-like follow
}

// ============================================================================
// Render
// ============================================================================

void MainMenu::Render(SDL_Renderer* renderer) {
    int centerX = Layout::SCREEN_W / 2;

    // =====================================================
    // HEADER: Official PassionWii Typography Logo + Switch Badge
    // =====================================================
    if (app->GetLogoTexture()) {
        int logoW = 290;
        int logoH = 103;
        int logoX = centerX - logoW / 2;
        int logoY = 20;
        SDL_Rect logoDst = {logoX, logoY, logoW, logoH};
        SDL_RenderCopy(renderer, app->GetLogoTexture(), nullptr, &logoDst);

        // Switch Edition pill badge
        int badgeW = 230;
        int badgeH = 22;
        int badgeX = centerX - badgeW / 2;
        int badgeY = logoY + logoH + 4;
        UI::DrawRoundedRect(renderer, badgeX, badgeY, badgeW, badgeH, 10, {225, 20, 45, 240});
        UI::DrawTextCentered(renderer, app->GetFontTiny(), "Nintendo Switch Edition",
                             {badgeX, badgeY, badgeW, badgeH}, Colors::TEXT_PRIMARY);

        // Subtle divider below header
        UI::DrawDivider(renderer, Layout::MARGIN, badgeY + 28,
                        Layout::SCREEN_W - Layout::MARGIN * 2);
    } else {
        int headerY = 50;
        UI::DrawText(renderer, app->GetFontLarge(),
                     "PassionWiiStreaming",
                     centerX, headerY + 8, Colors::TEXT_PRIMARY, true, false);

        float pulse = 0.5f + 0.5f * sinf(titlePulse);
        SDL_Color versionColor = UI::LerpColor(Colors::TEXT_DISABLED, Colors::DANGO_CREAM, pulse * 0.3f);
        char versionStr[96];
        snprintf(versionStr, sizeof(versionStr), "Nintendo Switch Edition v%s  —  Atmosphere", App::APP_VERSION);
        UI::DrawText(renderer, app->GetFontTiny(),
                     versionStr,
                     centerX, headerY + 50, versionColor, true, false);

        UI::DrawDivider(renderer, Layout::MARGIN, headerY + 78,
                        Layout::SCREEN_W - Layout::MARGIN * 2);
    }

    // =====================================================
    // MENU ITEMS (Translucent Frosted Glass)
    // =====================================================
    int menuStartY = 180;
    int itemWidth  = 640;
    int itemX      = (Layout::SCREEN_W - itemWidth) / 2;

    // Animated selection highlight background
    {
        float animY = menuStartY + selectionAnimY * (Layout::ITEM_HEIGHT + Layout::ITEM_SPACING);
        UI::DrawRoundedRect(renderer, itemX - 10, (int)animY - 4,
                            itemWidth + 20, Layout::ITEM_HEIGHT + 8,
                            Layout::CORNER_RADIUS, {65, 65, 95, 210});

        // Left accent bar (Switch Red)
        UI::DrawRoundedRect(renderer, itemX - 10, (int)animY,
                            4, Layout::ITEM_HEIGHT,
                            2, {235, 30, 55, 255});
    }

    // Render each menu item
    for (int i = 0; i < (int)items.size(); i++) {
        int itemY = menuStartY + i * (Layout::ITEM_HEIGHT + Layout::ITEM_SPACING);
        bool isSelected = (i == selectedIndex);

        // Item card background (translucent for background glow)
        SDL_Color bgColor = isSelected ? SDL_Color{50, 50, 75, 200} : SDL_Color{30, 30, 48, 160};
        UI::DrawRoundedRect(renderer, itemX, itemY, itemWidth, Layout::ITEM_HEIGHT,
                            Layout::CORNER_RADIUS - 2, bgColor);

        // Item icon indicator (colored dot)
        SDL_Color dotColor;
        switch (i) {
            case 0: dotColor = Colors::DANGO_ORANGE; break;  // Streaming — orange
            case 1: dotColor = Colors::DANGO_BLUE;   break;  // Archivos — blue
            case 2: dotColor = Colors::DANGO_CREAM;  break;  // Historial & Favs — cream
            case 3: dotColor = Colors::DANGO_GREEN;  break;  // Config — green
            default: dotColor = Colors::TEXT_SECONDARY; break;
        }
        filledCircleRGBA(renderer,
                         (Sint16)(itemX + 24), (Sint16)(itemY + Layout::ITEM_HEIGHT / 2),
                         6,
                         dotColor.r, dotColor.g, dotColor.b, 255);

        // Item label
        SDL_Color textColor = isSelected ? Colors::TEXT_PRIMARY : Colors::TEXT_SECONDARY;
        UI::DrawText(renderer, app->GetFont(),
                     items[i].label,
                     itemX + 44, itemY + 10, textColor, false, false);

        // Description (only for selected item)
        if (isSelected) {
            UI::DrawText(renderer, app->GetFontTiny(),
                         items[i].description,
                         itemX + 44, itemY + 38, Colors::TEXT_DISABLED, false, false);
        }
    }

    // =====================================================
    // INFO CARD (below menu)
    // =====================================================
    int cardY = menuStartY + (int)items.size() * (Layout::ITEM_HEIGHT + Layout::ITEM_SPACING) + 20;
    int cardW = itemWidth;
    int cardH = 82;

    UI::DrawRoundedRect(renderer, itemX, cardY, cardW, cardH,
                        Layout::CORNER_RADIUS, {30, 30, 48, 160});

    UI::DrawText(renderer, app->GetFontSmall(),
                 "PassionWiiStreaming Switch Edition",
                 itemX + Layout::PADDING, cardY + 12, Colors::DANGO_CREAM, false, false);

    UI::DrawText(renderer, app->GetFontTiny(),
                 "Catalogo oficial de peliculas, series, animes y doramas.",
                 itemX + Layout::PADDING, cardY + 38, Colors::TEXT_SECONDARY, false, false);

    UI::DrawText(renderer, app->GetFontTiny(),
                 "Ejecutar en Title Override (R + juego) para maxima memoria y rendimiento.",
                 itemX + Layout::PADDING, cardY + 58, Colors::TEXT_DISABLED, false, false);

    // =====================================================
    // FOOTER: Controller hints
    // =====================================================
    int footerY = Layout::SCREEN_H - Layout::FOOTER_H - 12;

    UI::DrawDivider(renderer, Layout::MARGIN, footerY,
                    Layout::SCREEN_W - Layout::MARGIN * 2);

    int hintY = footerY + 16;

    // (A) Seleccionar
    UI::DrawRoundedRect(renderer, Layout::MARGIN + 10, hintY, 26, 26,
                        8, Colors::DANGO_PINK);
    UI::DrawText(renderer, app->GetFontTiny(), "A",
                 Layout::MARGIN + 23, hintY + 5, Colors::TEXT_PRIMARY, true, false);
    UI::DrawText(renderer, app->GetFontSmall(), "Seleccionar",
                 Layout::MARGIN + 44, hintY + 2, Colors::TEXT_SECONDARY, false, false);

    // (+) Salir
    UI::DrawRoundedRect(renderer, Layout::MARGIN + 210, hintY, 26, 26,
                        8, Colors::DANGO_GREEN);
    UI::DrawText(renderer, app->GetFontTiny(), "+",
                 Layout::MARGIN + 223, hintY + 5, Colors::TEXT_PRIMARY, true, false);
    UI::DrawText(renderer, app->GetFontSmall(), "Salir",
                 Layout::MARGIN + 244, hintY + 2, Colors::TEXT_SECONDARY, false, false);

    // (B) Atras (shown on right side)
    UI::DrawRoundedRect(renderer, Layout::SCREEN_W - Layout::MARGIN - 150, hintY, 26, 26,
                        8, Colors::DANGO_ORANGE);
    UI::DrawText(renderer, app->GetFontTiny(), "B",
                 Layout::SCREEN_W - Layout::MARGIN - 137, hintY + 5,
                 Colors::TEXT_PRIMARY, true, false);
    UI::DrawText(renderer, app->GetFontSmall(), "Atras",
                 Layout::SCREEN_W - Layout::MARGIN - 118, hintY + 2,
                 Colors::TEXT_SECONDARY, false, false);

    // Toast overlay
    toast.Render(renderer, app->GetFontSmall());
}
