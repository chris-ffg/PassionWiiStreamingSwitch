// ============================================================================
// DangoPlayer Switch — History & Favorites Screen
// ============================================================================

#pragma once

#include "ui/screen.h"
#include "media/history_manager.h"
#include "media/favorites_manager.h"
#include "ui/widgets/toast.h"
#include <vector>
#include <string>

class HistoryScreen : public Screen {
public:
    explicit HistoryScreen(App* app);

    void OnEnter() override;
    void HandleInput(u64 kDown, u64 kHeld) override;
    void HandleTouch(const TouchEvent& touch) override;
    void Update(float dt) override;
    void Render(SDL_Renderer* renderer) override;

private:
    void RefreshLists();

    int activeTab = 0; // 0 = Historial, 1 = Favoritos
    int selectedIndex = 0;
    int scrollOffset = 0;

    Toast toast;

    static constexpr int ITEMS_PER_PAGE = 7;
    static constexpr int ITEM_H = 68;
    static constexpr int ITEM_GAP = 6;
};
