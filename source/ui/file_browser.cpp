// ============================================================================
// DangoPlayer Switch — File Browser Implementation
// ============================================================================

#include "ui/file_browser.h"
#include "ui/player_screen.h"
#include "ui/iptv_browser.h"
#include "utils/usb_helper.h"
#include "app.h"
#include "ui/ui_common.h"
#include "utils/string_utils.h"
#include "utils/logger.h"
#include <SDL2/SDL2_gfxPrimitives.h>
#include <algorithm>
#include <cmath>

// ============================================================================
// Constructor
// ============================================================================

FileBrowser::FileBrowser(App* app, const std::string& startPath)
    : Screen(app), currentPath(startPath) {}

// ============================================================================
// Lifecycle
// ============================================================================

void FileBrowser::OnEnter() {
    Logger::Info("FileBrowser: OnEnter (path: %s)", currentPath.c_str());
    if (items.empty()) {
        OpenDirectory(currentPath);
    }
}

// ============================================================================
// Directory Loading
// ============================================================================

void FileBrowser::OpenDirectory(const std::string& path) {
    currentPath = path;
    items = FileUtils::ScanDirectory(currentPath, currentFilter);
    selectedIndex = 0;
    scrollOffset = 0;
    scrollAnimY = 0.0f;

    Logger::Info("FileBrowser: Loaded %zu items from %s", items.size(), currentPath.c_str());
    if (items.empty()) {
        toast.Show("Carpeta vacia", 1.8f);
    }
}

void FileBrowser::CycleFilter() {
    switch (currentFilter) {
        case FileFilter::MediaOnly:
            currentFilter = FileFilter::VideoOnly;
            toast.Show("Filtro: Solo Videos", 1.5f);
            break;
        case FileFilter::VideoOnly:
            currentFilter = FileFilter::PlaylistOnly;
            toast.Show("Filtro: Solo Playlists", 1.5f);
            break;
        case FileFilter::PlaylistOnly:
            currentFilter = FileFilter::All;
            toast.Show("Filtro: Todos los archivos", 1.5f);
            break;
        case FileFilter::All:
            currentFilter = FileFilter::MediaOnly;
            toast.Show("Filtro: Multimedia", 1.5f);
            break;
    }
    // Reload current directory with new filter
    items = FileUtils::ScanDirectory(currentPath, currentFilter);
    selectedIndex = 0;
    scrollOffset = 0;
    scrollAnimY = 0.0f;
}

// ============================================================================
// Input Handling
// ============================================================================

void FileBrowser::HandleInput(u64 kDown, u64 kHeld) {
    int total = (int)items.size();

    // D-Pad / Stick navigation
    if (kDown & HidNpadButton_AnyDown) {
        if (total > 0 && selectedIndex < total - 1) {
            selectedIndex++;
            if (selectedIndex >= scrollOffset + ITEMS_PER_PAGE) {
                scrollOffset = selectedIndex - ITEMS_PER_PAGE + 1;
            }
        }
    }

    if (kDown & HidNpadButton_AnyUp) {
        if (total > 0 && selectedIndex > 0) {
            selectedIndex--;
            if (selectedIndex < scrollOffset) {
                scrollOffset = selectedIndex;
            }
        }
    }

    // Page Up / Down (L / R shoulder)
    if (kDown & HidNpadButton_R) {
        if (total > 0) {
            selectedIndex = std::min(total - 1, selectedIndex + ITEMS_PER_PAGE);
            if (selectedIndex >= scrollOffset + ITEMS_PER_PAGE) {
                scrollOffset = std::min(std::max(0, total - ITEMS_PER_PAGE), selectedIndex - ITEMS_PER_PAGE + 1);
            }
        }
    }

    if (kDown & HidNpadButton_L) {
        if (total > 0) {
            selectedIndex = std::max(0, selectedIndex - ITEMS_PER_PAGE);
            if (selectedIndex < scrollOffset) {
                scrollOffset = selectedIndex;
            }
        }
    }

    // A: Select / Open
    if (kDown & HidNpadButton_A) {
        if (total > 0 && selectedIndex >= 0 && selectedIndex < total) {
            HandleItemSelection(selectedIndex);
        }
    }

    // B: Parent directory / Back to main menu
    if (kDown & HidNpadButton_B) {
        if (currentPath == "sdmc:/" || currentPath == "sdmc:" || currentPath.empty()) {
            app->PopScreen();
        } else {
            OpenDirectory(StringUtils::GetParentDirectory(currentPath));
        }
    }

    // X: Cycle Filter
    if (kDown & HidNpadButton_X) {
        CycleFilter();
    }

    // Y: Cycle Storage Drives (SD card <-> USB drives)
    if (kDown & HidNpadButton_Y) {
        auto devices = UsbHelper::GetAvailableDevices();
        if (devices.size() <= 1) {
            OpenDirectory("sdmc:/");
            toast.Show("Raiz SD (sdmc:) — Sin USB conectado", 1.8f);
        } else {
            size_t curIdx = 0;
            for (size_t i = 0; i < devices.size(); i++) {
                if (currentPath.rfind(devices[i].mountPoint, 0) == 0) {
                    curIdx = i;
                    break;
                }
            }
            size_t nextIdx = (curIdx + 1) % devices.size();
            OpenDirectory(devices[nextIdx].mountPoint);
            toast.Show(devices[nextIdx].label, 2.0f);
        }
    }
}

// ============================================================================
// Item Selection
// ============================================================================

void FileBrowser::HandleItemSelection(int index) {
    if (index < 0 || index >= (int)items.size()) return;
    const auto& item = items[index];

    if (item.isDirectory) {
        OpenDirectory(item.fullPath);
    } else if (item.type == FileType::Video || item.type == FileType::Audio) {
        Logger::Info("FileBrowser: Launching player for %s", item.fullPath.c_str());
        app->PushScreen(std::make_unique<PlayerScreen>(app, item.fullPath, item.name));
    } else if (item.type == FileType::Playlist) {
        Logger::Info("FileBrowser: Open playlist -> %s", item.fullPath.c_str());
        app->PushScreen(std::make_unique<IPTVBrowser>(app, item.fullPath));
    } else {
        toast.Show("Formato no soportado", 1.5f);
    }
}

// ============================================================================
// Touch Handling
// ============================================================================

void FileBrowser::HandleTouch(const TouchEvent& touch) {
    int listStartY = 110;
    int listEndY = Layout::SCREEN_H - Layout::FOOTER_H - 10;

    // Header Back button touch
    if (touch.type == TouchEvent::Down) {
        if (touch.x >= Layout::MARGIN && touch.x <= Layout::MARGIN + 120 &&
            touch.y >= 30 && touch.y <= 85) {
            if (currentPath == "sdmc:/" || currentPath == "sdmc:") {
                app->PopScreen();
            } else {
                OpenDirectory(StringUtils::GetParentDirectory(currentPath));
            }
            return;
        }

        // Header Filter button touch
        int filterBtnX = Layout::SCREEN_W - Layout::MARGIN - 170;
        if (touch.x >= filterBtnX && touch.x <= filterBtnX + 170 &&
            touch.y >= 30 && touch.y <= 85) {
            CycleFilter();
            return;
        }

        // Start drag or tap in list area
        if (touch.y >= listStartY && touch.y <= listEndY) {
            isDragging = true;
            touchStartY = touch.y;
            lastTouchY = touch.y;
        }
    } else if (touch.type == TouchEvent::Motion && isDragging) {
        int deltaY = touch.y - lastTouchY;
        lastTouchY = touch.y;

        // Drag scrolling
        if (std::abs(deltaY) > 8) {
            if (deltaY < 0 && scrollOffset + ITEMS_PER_PAGE < (int)items.size()) {
                scrollOffset++;
            } else if (deltaY > 0 && scrollOffset > 0) {
                scrollOffset--;
            }
        }
    } else if (touch.type == TouchEvent::Up && isDragging) {
        isDragging = false;
        // If movement was small, treat as a tap
        if (std::abs(touch.y - touchStartY) < 14) {
            int clickedRow = (touch.y - listStartY) / (ITEM_H + ITEM_GAP);
            int clickedIndex = scrollOffset + clickedRow;
            if (clickedIndex >= 0 && clickedIndex < (int)items.size()) {
                selectedIndex = clickedIndex;
                HandleItemSelection(clickedIndex);
            }
        }
    }
}

// ============================================================================
// Update — Animations
// ============================================================================

void FileBrowser::Update(float dt) {
    toast.Update(dt);

    // Smooth scroll interpolation
    float targetY = (float)scrollOffset;
    float diff = targetY - scrollAnimY;
    scrollAnimY += diff * dt * 15.0f;
}

// ============================================================================
// Render
// ============================================================================

void FileBrowser::Render(SDL_Renderer* renderer) {
    // =====================================================
    // HEADER
    // =====================================================
    int headerY = 28;

    // Back Button [B]
    UI::DrawRoundedRect(renderer, Layout::MARGIN, headerY, 80, 44, 8, Colors::BG_CARD);
    UI::DrawRoundedRect(renderer, Layout::MARGIN + 8, headerY + 10, 24, 24, 6, Colors::DANGO_PINK);
    UI::DrawText(renderer, app->GetFontTiny(), "B",
                 Layout::MARGIN + 20, headerY + 13, Colors::TEXT_PRIMARY, true, false);
    UI::DrawText(renderer, app->GetFontSmall(), "Atras",
                 Layout::MARGIN + 38, headerY + 12, Colors::TEXT_SECONDARY, false, false);

    // Path title (truncated if too long)
    std::string displayPath = StringUtils::Truncate(currentPath, 48);
    UI::DrawText(renderer, app->GetFont(), displayPath,
                 Layout::MARGIN + 100, headerY + 10, Colors::DANGO_CREAM, false, false);

    // Filter pill on the right
    int filterBtnX = Layout::SCREEN_W - Layout::MARGIN - 180;
    UI::DrawRoundedRect(renderer, filterBtnX, headerY, 180, 44, 8, Colors::BG_CARD);
    UI::DrawRoundedRect(renderer, filterBtnX + 10, headerY + 10, 24, 24, 6, Colors::DANGO_GREEN);
    UI::DrawText(renderer, app->GetFontTiny(), "X",
                 filterBtnX + 22, headerY + 13, Colors::TEXT_PRIMARY, true, false);

    std::string filterLabel = "Multimedia";
    if (currentFilter == FileFilter::VideoOnly) filterLabel = "Solo Videos";
    else if (currentFilter == FileFilter::PlaylistOnly) filterLabel = "Playlists";
    else if (currentFilter == FileFilter::All) filterLabel = "Todos";

    UI::DrawText(renderer, app->GetFontSmall(), filterLabel,
                 filterBtnX + 42, headerY + 12, Colors::TEXT_PRIMARY, false, false);

    // Header divider
    UI::DrawDivider(renderer, Layout::MARGIN, 84, Layout::SCREEN_W - Layout::MARGIN * 2);

    // =====================================================
    // FILE LIST (Virtualized)
    // =====================================================
    int listStartY = 96;
    int listX = Layout::MARGIN;
    int listW = Layout::SCREEN_W - Layout::MARGIN * 2 - 20;

    int totalItems = (int)items.size();
    if (totalItems == 0) {
        UI::DrawText(renderer, app->GetFont(), "No se encontraron archivos en esta carpeta",
                     Layout::SCREEN_W / 2, 320, Colors::TEXT_DISABLED, true, false);
    } else {
        int firstIndex = std::max(0, scrollOffset);
        int lastIndex = std::min(totalItems - 1, scrollOffset + ITEMS_PER_PAGE - 1);

        for (int i = firstIndex; i <= lastIndex; i++) {
            int row = i - scrollOffset;
            int itemY = listStartY + row * (ITEM_H + ITEM_GAP);
            bool isSelected = (i == selectedIndex);

            // Card background
            SDL_Color cardBg = isSelected ? Colors::BG_SELECTED : Colors::BG_CARD;
            UI::DrawRoundedRect(renderer, listX, itemY, listW, ITEM_H, Layout::CORNER_RADIUS, cardBg);

            if (isSelected) {
                // Left accent bar
                UI::DrawRoundedRect(renderer, listX, itemY, 4, ITEM_H, 2, Colors::DANGO_PINK);
            }

            const auto& item = items[i];

            // Icon badge based on type
            SDL_Color badgeColor;
            const char* badgeText = "";
            switch (item.type) {
                case FileType::ParentDir:
                    badgeColor = Colors::DANGO_CREAM;
                    badgeText = "UP";
                    break;
                case FileType::Directory:
                    badgeColor = Colors::DANGO_ORANGE;
                    badgeText = "DIR";
                    break;
                case FileType::Video:
                    badgeColor = Colors::DANGO_PINK;
                    badgeText = "VID";
                    break;
                case FileType::Audio:
                    badgeColor = Colors::DANGO_GREEN;
                    badgeText = "AUD";
                    break;
                case FileType::Playlist:
                    badgeColor = {120, 180, 240, 255};
                    badgeText = "M3U";
                    break;
                default:
                    badgeColor = Colors::TEXT_DISABLED;
                    badgeText = "FILE";
                    break;
            }

            // Badge box
            UI::DrawRoundedRect(renderer, listX + 16, itemY + 16, 46, 36, 6, badgeColor);
            UI::DrawText(renderer, app->GetFontTiny(), badgeText,
                         listX + 39, itemY + 26, Colors::BG_DARK, true, false);

            // File / Folder Name
            SDL_Color textColor = isSelected ? Colors::TEXT_PRIMARY : Colors::TEXT_SECONDARY;
            std::string nameDisplay = StringUtils::Truncate(item.name, 48);
            UI::DrawText(renderer, app->GetFont(), nameDisplay,
                         listX + 76, itemY + 14, textColor, false, false);

            // Subtitle / Metadata (size or details)
            if (!item.isDirectory) {
                std::string metaStr = StringUtils::FormatBytes(item.size);
                std::string ext = StringUtils::GetExtension(item.name);
                if (!ext.empty()) {
                    metaStr += "  •  " + StringUtils::ToLower(ext.substr(1));
                }
                UI::DrawText(renderer, app->GetFontTiny(), metaStr,
                             listX + 76, itemY + 42, Colors::TEXT_DISABLED, false, false);
            } else if (item.type == FileType::Directory) {
                UI::DrawText(renderer, app->GetFontTiny(), "Carpeta",
                             listX + 76, itemY + 42, Colors::TEXT_DISABLED, false, false);
            }
        }

        // =====================================================
        // SCROLLBAR (Right side)
        // =====================================================
        if (totalItems > ITEMS_PER_PAGE) {
            int scrollbarX = Layout::SCREEN_W - Layout::MARGIN - 10;
            int scrollbarY = listStartY;
            int scrollbarH = ITEMS_PER_PAGE * (ITEM_H + ITEM_GAP);
            int trackW = 6;

            // Track background
            UI::DrawRoundedRect(renderer, scrollbarX, scrollbarY, trackW, scrollbarH, 3, Colors::BG_CARD);

            // Thumb
            float visibleRatio = (float)ITEMS_PER_PAGE / (float)totalItems;
            int thumbH = std::max(20, (int)(scrollbarH * visibleRatio));
            float scrollProgress = (float)scrollOffset / (float)(totalItems - ITEMS_PER_PAGE);
            int thumbY = scrollbarY + (int)((scrollbarH - thumbH) * scrollProgress);

            UI::DrawRoundedRect(renderer, scrollbarX, thumbY, trackW, thumbH, 3, Colors::DANGO_PINK);
        }
    }

    // =====================================================
    // FOOTER (Hints)
    // =====================================================
    int footerY = Layout::SCREEN_H - Layout::FOOTER_H - 10;
    UI::DrawDivider(renderer, Layout::MARGIN, footerY, Layout::SCREEN_W - Layout::MARGIN * 2);

    int hintY = footerY + 14;

    // (A) Abrir
    UI::DrawRoundedRect(renderer, Layout::MARGIN, hintY, 26, 26, 6, Colors::DANGO_PINK);
    UI::DrawText(renderer, app->GetFontTiny(), "A", Layout::MARGIN + 13, hintY + 5, Colors::TEXT_PRIMARY, true, false);
    UI::DrawText(renderer, app->GetFontSmall(), "Abrir", Layout::MARGIN + 32, hintY + 2, Colors::TEXT_SECONDARY, false, false);

    // (B) Atras
    int bX = Layout::MARGIN + 120;
    UI::DrawRoundedRect(renderer, bX, hintY, 26, 26, 6, Colors::DANGO_ORANGE);
    UI::DrawText(renderer, app->GetFontTiny(), "B", bX + 13, hintY + 5, Colors::TEXT_PRIMARY, true, false);
    UI::DrawText(renderer, app->GetFontSmall(), "Carpeta arriba", bX + 32, hintY + 2, Colors::TEXT_SECONDARY, false, false);

    // (X) Filtrar
    int xX = Layout::MARGIN + 290;
    UI::DrawRoundedRect(renderer, xX, hintY, 26, 26, 6, Colors::DANGO_GREEN);
    UI::DrawText(renderer, app->GetFontTiny(), "X", xX + 13, hintY + 5, Colors::TEXT_PRIMARY, true, false);
    UI::DrawText(renderer, app->GetFontSmall(), "Filtrar", xX + 32, hintY + 2, Colors::TEXT_SECONDARY, false, false);

    // (Y) Cambiar Unidad (SD/USB)
    int yX = Layout::MARGIN + 420;
    UI::DrawRoundedRect(renderer, yX, hintY, 26, 26, 6, Colors::DANGO_CREAM);
    UI::DrawText(renderer, app->GetFontTiny(), "Y", yX + 13, hintY + 5, Colors::BG_DARK, true, false);
    UI::DrawText(renderer, app->GetFontSmall(), "Unidad (SD/USB)", yX + 32, hintY + 2, Colors::TEXT_SECONDARY, false, false);

    // Item counter on bottom right
    char countBuf[48];
    snprintf(countBuf, sizeof(countBuf), "%d / %d",
             totalItems > 0 ? selectedIndex + 1 : 0, totalItems);
    UI::DrawText(renderer, app->GetFontSmall(), countBuf,
                 Layout::SCREEN_W - Layout::MARGIN - 10, hintY + 2, Colors::TEXT_DISABLED, false, false);

    // Toast overlay (if active)
    toast.Render(renderer, app->GetFontSmall());
}
