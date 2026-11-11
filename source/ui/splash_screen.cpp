// ============================================================================
// DangoPlayer Switch — Splash Screen Implementation
// ============================================================================

#include "ui/splash_screen.h"
#include "ui/main_menu.h"
#include "app.h"
#include "ui/ui_common.h"
#include "utils/logger.h"
#include <algorithm>
#include <cmath>

SplashScreen::SplashScreen(App* app) : Screen(app) {}

void SplashScreen::OnEnter() {
    Logger::Info("SplashScreen: OnEnter");
    timer = 0.0f;
    alpha = 0.0f;
    scaleAnim = 0.85f;
    transitioning = false;
}

void SplashScreen::HandleInput(u64 kDown, u64 kHeld) {
    // Any button skips splash screen immediately
    if (kDown & (HidNpadButton_A | HidNpadButton_B | HidNpadButton_Plus)) {
        if (!transitioning) {
            transitioning = true;
            app->ReplaceScreen(std::make_unique<MainMenu>(app));
        }
    }
}

void SplashScreen::HandleTouch(const TouchEvent& touch) {
    if (touch.type == TouchEvent::Down && !transitioning) {
        transitioning = true;
        app->ReplaceScreen(std::make_unique<MainMenu>(app));
    }
}

void SplashScreen::Update(float dt) {
    if (transitioning) return;

    timer += dt;

    // Fade in during first 0.6s, hold, then fade out during last 0.3s
    if (timer < 0.6f) {
        alpha = timer / 0.6f;
        scaleAnim = 0.85f + (timer / 0.6f) * 0.15f;
    } else if (timer > SPLASH_DURATION - 0.3f) {
        alpha = (SPLASH_DURATION - timer) / 0.3f;
    } else {
        alpha = 1.0f;
        scaleAnim = 1.0f;
    }
    alpha = std::clamp(alpha, 0.0f, 1.0f);

    if (timer >= SPLASH_DURATION) {
        transitioning = true;
        app->ReplaceScreen(std::make_unique<MainMenu>(app));
    }
}

void SplashScreen::Render(SDL_Renderer* renderer) {
    SDL_Texture* bg = app->GetBackgroundTexture();
    if (bg) {
        // Show the pure cover artwork at full brightness and clarity
        SDL_SetTextureColorMod(bg, 255, 255, 255);
        SDL_SetTextureAlphaMod(bg, 255);
        SDL_RenderCopy(renderer, bg, nullptr, nullptr);

        // Smooth fade-in and fade-out transition: 0 overlay opacity at peak (alpha == 1.0)
        float clampedAlpha = std::clamp(alpha, 0.0f, 1.0f);
        Uint8 overlayAlpha = (Uint8)((1.0f - clampedAlpha) * 255.0f);
        if (overlayAlpha > 0) {
            UI::DrawRect(renderer, 0, 0, Layout::SCREEN_W, Layout::SCREEN_H, {0, 0, 0, overlayAlpha});
        }
    } else {
        UI::DrawRect(renderer, 0, 0, Layout::SCREEN_W, Layout::SCREEN_H, Colors::BG_DARK);
    }
}
