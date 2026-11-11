// ============================================================================
// DangoPlayer Switch — Player Overlay (HUD)
// ============================================================================

#pragma once

#include "ui/widgets/progress_bar.h"
#include "ui/widgets/toast.h"
#include "media/player.h"
#include "ui/screen.h"
#include <SDL2/SDL.h>
#include <string>

class PlayerOverlay {
public:
    PlayerOverlay(App* app);

    void Show();
    void Hide();
    void Toggle();
    bool IsVisible() const { return visible || modalOpen; }

    void HandleInput(u64 kDown, u64 kHeld);
    bool HandleTouch(const TouchEvent& touch);

    void Update(float dt);
    void Render(SDL_Renderer* renderer);

    /// Show feedback on screen (e.g. "+10s", "Volumen 80%")
    void ShowOSD(const std::string& message);

private:
    void RenderTopBar(SDL_Renderer* renderer);
    void RenderBottomBar(SDL_Renderer* renderer);
    void RenderOSD(SDL_Renderer* renderer);
    void RenderTrackModal(SDL_Renderer* renderer);
    void ApplyModalSelection();

    App* app;
    ProgressBar seekBar;
    Toast toast;

    bool visible = true;
    bool isDraggingSeek = false;
    float autoHideTimer = 3.5f;
    float alpha = 1.0f;

    // OSD message (seek, volume, pause)
    std::string osdText;
    float osdTimer = 0.0f;

    // Modal track selector (Audio or Subtitle)
    bool modalOpen = false;
    TrackType modalTrackType = TrackType::Audio;
    int modalSelectedIndex = 0;

    static constexpr float AUTO_HIDE_DELAY = 3.5f;
    static constexpr int BOTTOM_BAR_H = 120;
    static constexpr int SEEK_BAR_Y   = Layout::SCREEN_H - BOTTOM_BAR_H + 46; // 646: cleanly below timecode, above hints
};
