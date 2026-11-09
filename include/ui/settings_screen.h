// ============================================================================
// DangoPlayer Switch — Settings Screen
// ============================================================================

#pragma once

#include "ui/screen.h"
#include "ui/widgets/toast.h"
#include <string>
#include <vector>

class SettingsScreen : public Screen {
public:
    explicit SettingsScreen(App* app);

    void OnEnter() override;
    void HandleInput(u64 kDown, u64 kHeld) override;
    void HandleTouch(const TouchEvent& touch) override;
    void Update(float dt) override;
    void Render(SDL_Renderer* renderer) override;

private:
    struct SettingOption {
        std::string title;
        std::string description;
        std::string currentValue;
    };

    void RefreshOptions();
    void ChangeSetting(int index, int direction);

    std::vector<SettingOption> options;
    int selectedIndex = 0;
    Toast toast;
};
