// ============================================================================
// DangoPlayer Switch — Settings Screen Implementation
// ============================================================================

#include "ui/settings_screen.h"
#include "app.h"
#include "ui/ui_common.h"
#include "config.h"
#include "utils/logger.h"
#include <algorithm>

SettingsScreen::SettingsScreen(App* app) : Screen(app) {
    RefreshOptions();
}

void SettingsScreen::OnEnter() {
    Logger::Info("SettingsScreen: OnEnter");
    RefreshOptions();
}

void SettingsScreen::RefreshOptions() {
    options.clear();
    const auto& cfg = Config::Instance().Get();

    // 1. Default Volume
    options.push_back({
        "Volumen por defecto",
        "Nivel de audio al iniciar la reproduccion de videos",
        std::to_string(cfg.defaultVolume) + "%"
    });

    // 2. Resume Playback
    options.push_back({
        "Reanudar reproduccion",
        "Guardar y restaurar la posicion exacta del video al reabrir",
        cfg.resumePlayback ? "Activado" : "Desactivado"
    });

    // 3. Theme
    options.push_back({
        "Tema visual",
        "Esquema de colores de la interfaz de usuario",
        cfg.themeIndex == 0 ? "Dango Oscuro" : "Medianoche"
    });

    // 4. System Info (Read-only)
    options.push_back({
        "Informacion del sistema",
        "Switch V1  •  Atmosphere 23.0.0  •  Title Override (3.2 GB)",
        "OK"
    });
}

void SettingsScreen::ChangeSetting(int index, int direction) {
    auto& cfg = Config::Instance().Get();

    switch (index) {
        case 0: // Volume (+/- 5%)
            cfg.defaultVolume = std::clamp(cfg.defaultVolume + direction * 5, 0, 100);
            toast.Show("Volumen: " + std::to_string(cfg.defaultVolume) + "%", 1.2f);
            break;

        case 1: // Resume playback
            cfg.resumePlayback = !cfg.resumePlayback;
            toast.Show(cfg.resumePlayback ? "Reanudar: Activado" : "Reanudar: Desactivado", 1.2f);
            break;

        case 2: // Theme
            cfg.themeIndex = (cfg.themeIndex + 1) % 2;
            toast.Show(cfg.themeIndex == 0 ? "Tema: Dango Oscuro" : "Tema: Medianoche", 1.2f);
            break;

        case 3:
            toast.Show("Atmosphere 23.0.0 en Nintendo Switch V1", 2.0f);
            break;
    }

    Config::Instance().Save();
    RefreshOptions();
}

void SettingsScreen::HandleInput(u64 kDown, u64 kHeld) {
    int total = (int)options.size();

    if (kDown & HidNpadButton_AnyDown) {
        selectedIndex = (selectedIndex + 1) % total;
    }

    if (kDown & HidNpadButton_AnyUp) {
        selectedIndex = (selectedIndex - 1 + total) % total;
    }

    // Left / Right arrows change setting
    if (kDown & (HidNpadButton_AnyLeft | HidNpadButton_L)) {
        ChangeSetting(selectedIndex, -1);
    }

    if (kDown & (HidNpadButton_AnyRight | HidNpadButton_R)) {
        ChangeSetting(selectedIndex, +1);
    }

    // A: Toggle or advance setting
    if (kDown & HidNpadButton_A) {
        ChangeSetting(selectedIndex, +1);
    }

    // B: Return to previous screen
    if (kDown & HidNpadButton_B) {
        Config::Instance().Save();
        app->PopScreen();
    }
}

void SettingsScreen::HandleTouch(const TouchEvent& touch) {
    if (touch.type != TouchEvent::Down) return;

    // Header Back button touch
    if (touch.x >= Layout::MARGIN && touch.x <= Layout::MARGIN + 120 &&
        touch.y >= 30 && touch.y <= 85) {
        Config::Instance().Save();
        app->PopScreen();
        return;
    }

    // Check setting row clicks
    int startY = 120;
    int rowH = 74;
    int rowGap = 8;
    int listW = Layout::SCREEN_W - Layout::MARGIN * 2;

    for (int i = 0; i < (int)options.size(); i++) {
        int itemY = startY + i * (rowH + rowGap);
        if (touch.x >= Layout::MARGIN && touch.x <= Layout::MARGIN + listW &&
            touch.y >= itemY && touch.y <= itemY + rowH) {
            selectedIndex = i;
            ChangeSetting(i, +1);
            return;
        }
    }
}

void SettingsScreen::Update(float dt) {
    toast.Update(dt);
}

void SettingsScreen::Render(SDL_Renderer* renderer) {
    // =====================================================
    // HEADER
    // =====================================================
    int headerY = 28;

    // Back Button [B]
    UI::DrawRoundedRect(renderer, Layout::MARGIN, headerY, 80, 44, 8, Colors::BG_CARD);
    UI::DrawRoundedRect(renderer, Layout::MARGIN + 8, headerY + 10, 24, 24, 6, Colors::DANGO_PINK);
    UI::DrawText(renderer, app->GetFontTiny(), "B",
                 Layout::MARGIN + 20, headerY + 13, Colors::TEXT_PRIMARY, true, false);
    UI::DrawText(renderer, app->GetFontSmall(), "Atras",
                 Layout::MARGIN + 38, headerY + 12, Colors::TEXT_SECONDARY, false, false);

    // Title
    UI::DrawText(renderer, app->GetFontLarge(), "Configuracion",
                 Layout::MARGIN + 100, headerY + 4, Colors::DANGO_CREAM, false, false);

    UI::DrawDivider(renderer, Layout::MARGIN, 84, Layout::SCREEN_W - Layout::MARGIN * 2);

    // =====================================================
    // SETTINGS OPTIONS
    // =====================================================
    int startY = 114;
    int rowH = 76;
    int rowGap = 10;
    int listW = Layout::SCREEN_W - Layout::MARGIN * 2;

    for (int i = 0; i < (int)options.size(); i++) {
        int itemY = startY + i * (rowH + rowGap);
        bool isSelected = (i == selectedIndex);

        SDL_Color cardBg = isSelected ? Colors::BG_SELECTED : Colors::BG_CARD;
        UI::DrawRoundedRect(renderer, Layout::MARGIN, itemY, listW, rowH, Layout::CORNER_RADIUS, cardBg);

        if (isSelected) {
            UI::DrawRoundedRect(renderer, Layout::MARGIN, itemY, 4, rowH, 2, Colors::DANGO_PINK);
        }

        // Title
        SDL_Color titleColor = isSelected ? Colors::TEXT_PRIMARY : Colors::TEXT_SECONDARY;
        UI::DrawText(renderer, app->GetFont(), options[i].title,
                     Layout::MARGIN + 24, itemY + 14, titleColor, false, false);

        // Description
        UI::DrawText(renderer, app->GetFontTiny(), options[i].description,
                     Layout::MARGIN + 24, itemY + 44, Colors::TEXT_DISABLED, false, false);

        // Value pill on the right
        int pillW = 160;
        int pillH = 38;
        int pillX = Layout::MARGIN + listW - pillW - 20;
        int pillY = itemY + (rowH - pillH) / 2;

        SDL_Color pillBg = isSelected ? Colors::BG_HOVER : Colors::BG_DARK;
        UI::DrawRoundedRect(renderer, pillX, pillY, pillW, pillH, 8, pillBg);

        SDL_Color valColor = isSelected ? Colors::DANGO_PINK : Colors::DANGO_CREAM;
        UI::DrawTextCentered(renderer, app->GetFontSmall(), options[i].currentValue,
                             {pillX, pillY, pillW, pillH}, valColor);
    }

    // =====================================================
    // FOOTER (Hints)
    // =====================================================
    int footerY = Layout::SCREEN_H - Layout::FOOTER_H - 10;
    UI::DrawDivider(renderer, Layout::MARGIN, footerY, Layout::SCREEN_W - Layout::MARGIN * 2);

    int hintY = footerY + 14;

    // (A / D-Pad) Cambiar valor
    UI::DrawRoundedRect(renderer, Layout::MARGIN, hintY, 26, 26, 6, Colors::DANGO_PINK);
    UI::DrawText(renderer, app->GetFontTiny(), "A", Layout::MARGIN + 13, hintY + 5, Colors::TEXT_PRIMARY, true, false);
    UI::DrawText(renderer, app->GetFontSmall(), "Cambiar", Layout::MARGIN + 32, hintY + 2, Colors::TEXT_SECONDARY, false, false);

    int lrX = Layout::MARGIN + 140;
    UI::DrawRoundedRect(renderer, lrX, hintY, 44, 26, 6, Colors::DANGO_GREEN);
    UI::DrawText(renderer, app->GetFontTiny(), "< >", lrX + 22, hintY + 5, Colors::TEXT_PRIMARY, true, false);
    UI::DrawText(renderer, app->GetFontSmall(), "Ajustar", lrX + 50, hintY + 2, Colors::TEXT_SECONDARY, false, false);

    int bX = Layout::MARGIN + 280;
    UI::DrawRoundedRect(renderer, bX, hintY, 26, 26, 6, Colors::DANGO_ORANGE);
    UI::DrawText(renderer, app->GetFontTiny(), "B", bX + 13, hintY + 5, Colors::TEXT_PRIMARY, true, false);
    UI::DrawText(renderer, app->GetFontSmall(), "Guardar y Salir", bX + 32, hintY + 2, Colors::TEXT_SECONDARY, false, false);

    toast.Render(renderer, app->GetFontSmall());
}
