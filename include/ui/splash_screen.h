// ============================================================================
// DangoPlayer Switch — Splash Screen
// ============================================================================
// Introductory splash screen displayed on startup.
// Animates the Dango logo and transitions smoothly into the Main Menu.
// ============================================================================

#pragma once

#include "ui/screen.h"

class SplashScreen : public Screen {
public:
    explicit SplashScreen(App* app);

    void OnEnter() override;
    void HandleInput(u64 kDown, u64 kHeld) override;
    void HandleTouch(const TouchEvent& touch) override;
    void Update(float dt) override;
    void Render(SDL_Renderer* renderer) override;

private:
    float timer = 0.0f;
    float alpha = 0.0f;
    float scaleAnim = 0.8f;
    bool transitioning = false;

    static constexpr float SPLASH_DURATION = 1.8f;
};
