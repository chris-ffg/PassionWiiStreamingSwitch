// ============================================================================
// DangoPlayer Switch — Progress Bar Widget
// ============================================================================

#pragma once

#include <SDL2/SDL.h>

class ProgressBar {
public:
    ProgressBar();

    void SetBounds(int x, int y, int w, int h);
    void SetProgress(float progress);  // 0.0f to 1.0f
    float GetProgress() const { return progress; }

    void SetColors(SDL_Color bg, SDL_Color fill, SDL_Color knob);
    void SetKnobVisible(bool visible) { showKnob = visible; }

    /// Returns true if a given screen point (e.g. touch/click) is within bounds and updates progress.
    bool HandleTouch(int touchX, int touchY);

    void Render(SDL_Renderer* renderer);

private:
    int x = 0, y = 0, w = 100, h = 10;
    float progress = 0.0f;
    bool showKnob = true;
    SDL_Color bgColor;
    SDL_Color fillColor;
    SDL_Color knobColor;
};
