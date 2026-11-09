// ============================================================================
// DangoPlayer Switch — UI Common: Colors, Layout, Drawing Helpers
// ============================================================================
// Defines the visual identity of DangoPlayer Switch:
// - Dango-inspired color palette (🍡 pink, cream, green on dark background)
// - Layout constants for consistent spacing
// - Helper functions for rendering text, rounded rectangles, and the dango icon
// ============================================================================

#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include <string>

// ============================================================================
// Color Palette — Dango Theme (warm tones on dark background)
// ============================================================================
namespace Colors {
    // Background layers (dark → light)
    constexpr SDL_Color BG_DARK      = { 26,  26,  46, 255};  // #1A1A2E — deepest background
    constexpr SDL_Color BG_CARD      = { 42,  42,  62, 255};  // #2A2A3E — card/panel surfaces
    constexpr SDL_Color BG_SELECTED  = { 58,  58,  82, 255};  // #3A3A52 — selected item
    constexpr SDL_Color BG_HOVER     = { 50,  50,  72, 255};  // #323048 — hover state

    // Dango accent colors (🍡 the three dango balls)
    constexpr SDL_Color DANGO_PINK   = {232, 130, 155, 255};  // #E8829B — primary accent
    constexpr SDL_Color DANGO_CREAM  = {245, 232, 211, 255};  // #F5E8D3 — warm secondary
    constexpr SDL_Color DANGO_GREEN  = {144, 190, 147, 255};  // #90BE93 — tertiary green
    constexpr SDL_Color DANGO_ORANGE = {240, 168, 104, 255};  // #F0A868 — warm highlight
    constexpr SDL_Color DANGO_BLUE   = {120, 160, 230, 255};  // #78A0E6 — blue accent

    // Text hierarchy
    constexpr SDL_Color TEXT_PRIMARY   = {255, 255, 255, 255};  // full white
    constexpr SDL_Color TEXT_SECONDARY = {176, 176, 192, 255};  // muted blue-gray
    constexpr SDL_Color TEXT_DISABLED  = {100, 100, 120, 255};  // dim gray

    // Semantic colors
    constexpr SDL_Color SUCCESS = {100, 200, 120, 255};
    constexpr SDL_Color WARNING = {240, 200,  80, 255};
    constexpr SDL_Color DANGER  = {220,  80,  80, 255};

    // Structural
    constexpr SDL_Color DIVIDER = {60, 60, 80, 200};
}

// ============================================================================
// Layout Constants
// ============================================================================
namespace Layout {
    constexpr int SCREEN_W = 1280;
    constexpr int SCREEN_H = 720;

    constexpr int MARGIN       = 40;
    constexpr int PADDING      = 16;
    constexpr int ITEM_HEIGHT  = 64;
    constexpr int ITEM_SPACING = 8;
    constexpr int CORNER_RADIUS = 12;

    constexpr int HEADER_H = 80;
    constexpr int FOOTER_H = 48;

    // Font sizes (in points)
    constexpr int FONT_LARGE  = 36;
    constexpr int FONT_MEDIUM = 24;
    constexpr int FONT_SMALL  = 18;
    constexpr int FONT_TINY   = 14;
}

// ============================================================================
// Drawing Helpers
// ============================================================================
namespace UI {
    /// Set the renderer's draw color from an SDL_Color.
    void SetColor(SDL_Renderer* renderer, SDL_Color color);

    /// Draw a filled rectangle.
    void DrawRect(SDL_Renderer* renderer, int x, int y, int w, int h, SDL_Color color);

    /// Draw a filled rounded rectangle (uses SDL2_gfx).
    void DrawRoundedRect(SDL_Renderer* renderer, int x, int y, int w, int h,
                         int radius, SDL_Color color);

    /// Draw an outlined rounded rectangle.
    void DrawRoundedRectOutline(SDL_Renderer* renderer, int x, int y, int w, int h,
                                int radius, SDL_Color color);

    /// Draw text at a position. If centerX/centerY, the position is the center axis.
    /// Safely does nothing if font is nullptr.
    void DrawText(SDL_Renderer* renderer, TTF_Font* font, const std::string& text,
                  int x, int y, SDL_Color color, bool centerX = false, bool centerY = false);

    /// Draw text centered within a given rectangle.
    void DrawTextCentered(SDL_Renderer* renderer, TTF_Font* font, const std::string& text,
                          SDL_Rect rect, SDL_Color color);

    /// Draw a horizontal divider line.
    void DrawDivider(SDL_Renderer* renderer, int x, int y, int width,
                     SDL_Color color = Colors::DIVIDER);

    /// Draw the DangoPlayer dango icon (three colored circles on a stick).
    /// The icon is drawn vertically centered at (centerX, centerY).
    void DrawDangoIcon(SDL_Renderer* renderer, int centerX, int centerY, int radius);

    /// Vector icon helpers (independent of font/emojis, rendered with SDL2_gfx)
    void DrawIconFolder(SDL_Renderer* renderer, int cx, int cy, int size, SDL_Color color);
    void DrawIconHome(SDL_Renderer* renderer, int cx, int cy, int size, SDL_Color color);
    void DrawIconTV(SDL_Renderer* renderer, int cx, int cy, int size, SDL_Color color);
    void DrawIconStar(SDL_Renderer* renderer, int cx, int cy, int size, SDL_Color color);
    void DrawIconSearch(SDL_Renderer* renderer, int cx, int cy, int size, SDL_Color color);
    void DrawIconSettings(SDL_Renderer* renderer, int cx, int cy, int size, SDL_Color color);
    void DrawIconFilm(SDL_Renderer* renderer, int cx, int cy, int size, SDL_Color color);

    /// Linearly interpolate between two colors.
    SDL_Color LerpColor(SDL_Color a, SDL_Color b, float t);
}
