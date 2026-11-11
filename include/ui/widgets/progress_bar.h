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
    void SetDragging(bool dragging) { isDragging = dragging; }
    bool IsDragging() const { return isDragging; }

    /// Check if point falls inside interactive bar/knob hit target.
    bool ContainsPoint(int touchX, int touchY, int padX = 20, int padY = 24) const;

    /// Update progress from horizontal coordinate, clamped cleanly to [0, 1].
    void UpdateFromX(int touchX);

    /// Returns true if a given screen point (e.g. touch/click) is within bounds and updates progress.
    bool HandleTouch(int touchX, int touchY);

    void Render(SDL_Renderer* renderer);

    int GetX() const { return x; }
    int GetY() const { return y; }
    int GetW() const { return w; }
    int GetH() const { return h; }

private:
    int x = 0, y = 0, w = 100, h = 10;
    float progress = 0.0f;
    bool showKnob = true;
    bool isDragging = false;
    SDL_Color bgColor;
    SDL_Color fillColor;
    SDL_Color knobColor;
};
