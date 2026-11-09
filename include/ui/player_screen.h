// ============================================================================
// DangoPlayer Switch — Player Screen
// ============================================================================

#pragma once

#include "ui/screen.h"
#include "ui/player_overlay.h"
#include "media/player.h"
#include <string>

class PlayerScreen : public Screen {
public:
    PlayerScreen(App* app, const std::string& filePath, const std::string& title = "");
    ~PlayerScreen() override;

    void OnEnter() override;
    void OnExit() override;

    void HandleInput(u64 kDown, u64 kHeld) override;
    void HandleTouch(const TouchEvent& touch) override;
    void Update(float dt) override;
    void Render(SDL_Renderer* renderer) override;

private:
    std::string mediaPath;
    std::string mediaTitle;
    PlayerOverlay overlay;
    bool hasShownError = false;
    float errorTimer = 2.5f;
};
