// ============================================================================
// DangoPlayer Switch — Toast Notification Widget
// ============================================================================

#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <string>

class Toast {
public:
    Toast();

    /// Show a message for a given duration in seconds.
    void Show(const std::string& message, float duration = 2.5f);

    /// Update fade-in/out timers.
    void Update(float dt);

    /// Render overlay if active.
    void Render(SDL_Renderer* renderer, TTF_Font* font);

    bool IsActive() const { return active; }

private:
    std::string text;
    float durationRemaining = 0.0f;
    float maxDuration = 2.5f;
    float alpha = 0.0f;
    bool active = false;
};
