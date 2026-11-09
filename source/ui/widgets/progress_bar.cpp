// ============================================================================
// DangoPlayer Switch — Progress Bar Widget Implementation
// ============================================================================

#include "ui/widgets/progress_bar.h"
#include "ui/ui_common.h"
#include <SDL2/SDL2_gfxPrimitives.h>
#include <algorithm>

ProgressBar::ProgressBar() {
    bgColor   = {50, 50, 70, 200};
    fillColor = Colors::DANGO_PINK;
    knobColor = Colors::DANGO_CREAM;
}

void ProgressBar::SetBounds(int newX, int newY, int newW, int newH) {
    x = newX; y = newY; w = newW; h = newH;
}

void ProgressBar::SetProgress(float newProgress) {
    progress = std::clamp(newProgress, 0.0f, 1.0f);
}

void ProgressBar::SetColors(SDL_Color bg, SDL_Color fill, SDL_Color knob) {
    bgColor = bg;
    fillColor = fill;
    knobColor = knob;
}

bool ProgressBar::HandleTouch(int touchX, int touchY) {
    // Expand touch target area slightly vertically for comfort
    int hitPadY = 16;
    if (touchX >= x && touchX <= x + w &&
        touchY >= y - hitPadY && touchY <= y + h + hitPadY) {
        float newProg = (float)(touchX - x) / (float)w;
        SetProgress(newProg);
        return true;
    }
    return false;
}

void ProgressBar::Render(SDL_Renderer* renderer) {
    int radius = h / 2;

    // Background track
    UI::DrawRoundedRect(renderer, x, y, w, h, radius, bgColor);

    // Filled track
    int fillW = (int)(w * progress);
    if (fillW > 0) {
        if (fillW < radius * 2) fillW = radius * 2;
        if (fillW > w) fillW = w;
        UI::DrawRoundedRect(renderer, x, y, fillW, h, radius, fillColor);
    }

    // Knob
    if (showKnob) {
        int knobX = x + (int)(w * progress);
        int knobY = y + h / 2;
        int knobRadius = h + 2;
        filledCircleRGBA(renderer, (Sint16)knobX, (Sint16)knobY, (Sint16)knobRadius,
                         knobColor.r, knobColor.g, knobColor.b, knobColor.a);
    }
}
