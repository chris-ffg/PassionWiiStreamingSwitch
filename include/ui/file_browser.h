// ============================================================================
// DangoPlayer Switch — File Browser Screen
// ============================================================================
// Local file explorer for sdmc:/ and external USB drives.
// Features:
// - Directory traversal with parent directory ("..") support
// - Filter toggle: All Media, Video Only, Playlist Only
// - Virtualized smooth scrolling list
// - File size and format badges
// - Touch drag-scroll and tap selection
// ============================================================================

#pragma once

#include "ui/screen.h"
#include "utils/file_utils.h"
#include "ui/widgets/toast.h"
#include <vector>
#include <string>

class FileBrowser : public Screen {
public:
    explicit FileBrowser(App* app, const std::string& startPath = "sdmc:/");

    void OnEnter() override;
    void HandleInput(u64 kDown, u64 kHeld) override;
    void HandleTouch(const TouchEvent& touch) override;
    void Update(float dt) override;
    void Render(SDL_Renderer* renderer) override;

    /// Navigate to a specific directory
    void OpenDirectory(const std::string& path);

private:
    void CycleFilter();
    void HandleItemSelection(int index);

    std::string currentPath;
    FileFilter currentFilter = FileFilter::MediaOnly;
    std::vector<FileItem> items;

    int selectedIndex = 0;
    int scrollOffset = 0;         // First visible item index
    float scrollAnimY = 0.0f;     // Smooth visual scroll offset

    // Touch drag scrolling
    bool isDragging = false;
    int touchStartY = 0;
    int lastTouchY = 0;

    Toast toast;

    static constexpr int ITEMS_PER_PAGE = 7;
    static constexpr int ITEM_H = 68;
    static constexpr int ITEM_GAP = 6;
};
