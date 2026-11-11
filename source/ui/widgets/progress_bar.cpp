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

bool ProgressBar::ContainsPoint(int touchX, int touchY, int padX, int padY) const {
    return (touchX >= x - padX && touchX <= x + w + padX &&
            touchY >= y - padY && touchY <= y + h + padY);
}

void ProgressBar::UpdateFromX(int touchX) {
    if (w <= 0) return;
    float newProg = (float)(touchX - x) / (float)w;
    SetProgress(newProg);
}

bool ProgressBar::HandleTouch(int touchX, int touchY) {
    if (ContainsPoint(touchX, touchY, 16, 22)) {
        UpdateFromX(touchX);
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
        if (isDragging) {
            filledCircleRGBA(renderer, (Sint16)knobX, (Sint16)knobY, (Sint16)(h + 9),
                             fillColor.r, fillColor.g, fillColor.b, 110);
        }
        int knobRadius = isDragging ? (h + 4) : (h + 1);
        filledCircleRGBA(renderer, (Sint16)knobX, (Sint16)knobY, (Sint16)knobRadius,
                         knobColor.r, knobColor.g, knobColor.b, knobColor.a);
    }
}
