// ============================================================================
// DangoPlayer Switch — Player Screen Implementation
// ============================================================================

#include "ui/player_screen.h"
#include "media/history_manager.h"
#include "app.h"
#include "ui/ui_common.h"
#include "utils/logger.h"
#ifdef __SWITCH__
#include <switch.h>
#endif

PlayerScreen::PlayerScreen(App* app, const std::string& filePath, const std::string& title)
    : Screen(app), mediaPath(filePath), mediaTitle(title), overlay(app) {}

PlayerScreen::~PlayerScreen() {}

void PlayerScreen::OnEnter() {
    Logger::Info("PlayerScreen: Starting playback for %s (Title: %s)", mediaPath.c_str(), mediaTitle.c_str());

    hasShownError = false;
    errorTimer = 0.0f;

    // Disarm any leftover callback before opening
    MediaPlayer::Instance().SetOnEndedCallback(nullptr);
    MediaPlayer::Instance().Open(mediaPath, mediaTitle);

    // When media finishes playing, automatically return to previous screen
    MediaPlayer::Instance().SetOnEndedCallback([this]() {
        Logger::Info("PlayerScreen: End of file reached, returning to browser");
        this->app->PopScreen();
    });

    overlay.Show();
}

void PlayerScreen::OnExit() {
    Logger::Info("PlayerScreen: Exiting player");

#ifdef __SWITCH__
    // Re-enable normal auto-sleep and screen dimming when leaving the player
    appletSetMediaPlaybackState(false);
#endif

    auto& player = MediaPlayer::Instance();
    // 1. Immediately disarm callback
    player.SetOnEndedCallback(nullptr);

    const auto& meta = player.GetMetadata();
    bool isStream = (mediaPath.rfind("http://", 0) == 0 ||
                     mediaPath.rfind("https://", 0) == 0 ||
                     mediaPath.rfind("rtsp://", 0) == 0);

    HistoryManager::Instance().AddEntry(
        meta.title.empty() ? mediaPath : meta.title,
        mediaPath,
        player.GetPosition(),
        player.GetDuration(),
        isStream
    );

    player.Stop();
}

void PlayerScreen::HandleInput(u64 kDown, u64 kHeld) {
    overlay.HandleInput(kDown, kHeld);
}

void PlayerScreen::HandleTouch(const TouchEvent& touch) {
    overlay.HandleTouch(touch);
}

void PlayerScreen::Update(float dt) {
    MediaPlayer::Instance().Update();
    overlay.Update(dt);

#ifdef __SWITCH__
    // Continuously inhibit auto-sleep only while actively playing; allow sleep when paused
    appletSetMediaPlaybackState(MediaPlayer::Instance().IsPlaying());
#endif

    if (MediaPlayer::Instance().GetStatus() == PlayerStatus::Error) {
        if (!hasShownError) {
            hasShownError = true;
            errorTimer = 2.5f;
        }
        errorTimer -= dt;
        if (errorTimer <= 0.0f) {
            app->PopScreen();
        }
    }
}

void PlayerScreen::Render(SDL_Renderer* renderer) {
    if (hasShownError) {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        UI::DrawRoundedRect(renderer, Layout::SCREEN_W / 2 - 250, Layout::SCREEN_H / 2 - 40, 500, 80, 12, Colors::BG_CARD);
        UI::DrawTextCentered(renderer, app->GetFont(), "Error al reproducir enlace multimedia",
                             {Layout::SCREEN_W / 2 - 250, Layout::SCREEN_H / 2 - 40, 500, 45}, Colors::DANGO_PINK);
        UI::DrawTextCentered(renderer, app->GetFontSmall(), "El servidor no responde o el formato no es compatible",
                             {Layout::SCREEN_W / 2 - 250, Layout::SCREEN_H / 2 + 5, 500, 35}, Colors::TEXT_SECONDARY);
        return;
    }

    // Flush any pending SDL2 draw calls
    SDL_RenderFlush(renderer);

    // 1. Render video frame directly to OpenGL framebuffer
    MediaPlayer::Instance().RenderVideo();

    // 2. Render HUD overlay on top
    overlay.Render(renderer);
}
