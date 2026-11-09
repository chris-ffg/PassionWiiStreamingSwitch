// ============================================================================
// DangoPlayer Switch — Toast Notification Widget Implementation
// ============================================================================

#include "ui/widgets/toast.h"
#include "ui/ui_common.h"
#include <algorithm>

Toast::Toast() {}

void Toast::Show(const std::string& message, float duration) {
    text = message;
    maxDuration = duration;
    durationRemaining = duration;
    active = true;
    alpha = 0.0f;
}

void Toast::Update(float dt) {
    if (!active) return;

    durationRemaining -= dt;
    if (durationRemaining <= 0.0f) {
        active = false;
        alpha = 0.0f;
        return;
    }

    // Fade in during first 0.3s, fade out during last 0.5s
    float elapsed = maxDuration - durationRemaining;
    if (elapsed < 0.3f) {
        alpha = elapsed / 0.3f;
    } else if (durationRemaining < 0.5f) {
        alpha = durationRemaining / 0.5f;
    } else {
        alpha = 1.0f;
    }
    alpha = std::clamp(alpha, 0.0f, 1.0f);
}

void Toast::Render(SDL_Renderer* renderer, TTF_Font* font) {
    if (!active || alpha <= 0.01f || !font || text.empty()) return;

    int textW = 0, textH = 0;
    TTF_SizeUTF8(font, text.c_str(), &textW, &textH);

    int padX = 24;
    int padY = 12;
    int boxW = textW + padX * 2;
    int boxH = textH + padY * 2;
    int boxX = (Layout::SCREEN_W - boxW) / 2;
    int boxY = Layout::SCREEN_H - Layout::FOOTER_H - boxH - 24;

    Uint8 a = (Uint8)(alpha * 240.0f);
    SDL_Color boxColor = {40, 40, 58, a};
    SDL_Color borderColor = {Colors::DANGO_PINK.r, Colors::DANGO_PINK.g, Colors::DANGO_PINK.b, a};
    SDL_Color textColor = {255, 255, 255, (Uint8)(alpha * 255.0f)};

    UI::DrawRoundedRect(renderer, boxX, boxY, boxW, boxH, 10, boxColor);
    UI::DrawRoundedRectOutline(renderer, boxX, boxY, boxW, boxH, 10, borderColor);
    UI::DrawText(renderer, font, text, boxX + padX, boxY + padY, textColor, false, false);
}
