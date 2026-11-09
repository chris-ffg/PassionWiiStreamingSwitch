// ============================================================================
// DangoPlayer Switch — Main Menu Screen
// ============================================================================
// The home screen of the application. Displays:
// - DangoPlayer logo (dango icon 🍡 + title text)
// - Navigation menu with three options:
//   1. Archivos Locales (local file browser)
//   2. IPTV (playlist/channel browser)
//   3. Configuración (settings)
// - Footer with controller button hints
//
// Navigation: D-pad/Stick up/down, A to select, + to exit
// ============================================================================

#pragma once

#include "ui/screen.h"
#include "ui/widgets/toast.h"
#include <string>
#include <vector>

class MainMenu : public Screen {
public:
    explicit MainMenu(App* app);

    void OnEnter() override;
    void HandleInput(u64 kDown, u64 kHeld) override;
    void HandleTouch(const TouchEvent& touch) override;
    void Update(float dt) override;
    void Render(SDL_Renderer* renderer) override;

private:
    struct MenuItem {
        std::string label;
        std::string description;
    };

    std::vector<MenuItem> items;
    int selectedIndex = 0;

    // Animations
    float selectionAnimY = 0.0f;    // Smooth vertical selection interpolation
    float titlePulse     = 0.0f;    // Subtle pulsing effect on version text
    float fadeIn         = 0.0f;    // Fade-in on screen entry

    Toast toast;
};
