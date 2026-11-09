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
    if (app->GetBackgroundTexture()) {
        SDL_RenderCopy(renderer, app->GetBackgroundTexture(), nullptr, nullptr);
        // Dim smoothly so branding text is clear
        Uint8 overlayAlpha = (Uint8)(100 + (1.0f - alpha) * 155);
        UI::DrawRect(renderer, 0, 0, Layout::SCREEN_W, Layout::SCREEN_H, {0, 0, 0, overlayAlpha});
    } else {
        UI::DrawRect(renderer, 0, 0, Layout::SCREEN_W, Layout::SCREEN_H, Colors::BG_DARK);
    }

    if (alpha <= 0.01f) return;

    int centerX = Layout::SCREEN_W / 2;
    int centerY = Layout::SCREEN_H / 2 - 20;

    // Title
    Uint8 a = (Uint8)(alpha * 255.0f);
    SDL_Color titleColor = {Colors::TEXT_PRIMARY.r, Colors::TEXT_PRIMARY.g, Colors::TEXT_PRIMARY.b, a};
    UI::DrawText(renderer, app->GetFontLarge(), "PassionWiiStreaming",
                 centerX, centerY + 20, titleColor, true, false);

    // Subtitle
    SDL_Color subColor = {230, 0, 40, a}; // Switch Red
    UI::DrawText(renderer, app->GetFont(), "Nintendo Switch Edition",
                 centerX, centerY + 70, subColor, true, false);

    // Categories
    SDL_Color catColor = {Colors::DANGO_CREAM.r, Colors::DANGO_CREAM.g, Colors::DANGO_CREAM.b, (Uint8)(a * 0.9f)};
    UI::DrawText(renderer, app->GetFontSmall(), "Peliculas  •  Series  •  Animes  •  Doramas",
                 centerX, centerY + 115, catColor, true, false);

    // Subtext
    SDL_Color copyColor = {Colors::TEXT_DISABLED.r, Colors::TEXT_DISABLED.g, Colors::TEXT_DISABLED.b, (Uint8)(a * 0.65f)};
    UI::DrawText(renderer, app->GetFontTiny(), "Horizon OS Homebrew  •  Atmosphere",
                 centerX, centerY + 155, copyColor, true, false);
}
