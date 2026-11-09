// ============================================================================
// DangoPlayer Switch — UI Common Implementation
// ============================================================================

#include "ui/ui_common.h"
#include <SDL2/SDL2_gfxPrimitives.h>
#include <algorithm>

namespace UI {

// ============================================================================
// Basic Drawing
// ============================================================================

void SetColor(SDL_Renderer* renderer, SDL_Color color) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
}

void DrawRect(SDL_Renderer* renderer, int x, int y, int w, int h, SDL_Color color) {
    SetColor(renderer, color);
    SDL_Rect rect = {x, y, w, h};
    SDL_RenderFillRect(renderer, &rect);
}

void DrawRoundedRect(SDL_Renderer* renderer, int x, int y, int w, int h,
                     int radius, SDL_Color color) {
    if (radius <= 0) {
        DrawRect(renderer, x, y, w, h, color);
        return;
    }
    roundedBoxRGBA(renderer,
                   (Sint16)x, (Sint16)y,
                   (Sint16)(x + w), (Sint16)(y + h),
                   (Sint16)radius,
                   color.r, color.g, color.b, color.a);
}

void DrawRoundedRectOutline(SDL_Renderer* renderer, int x, int y, int w, int h,
                            int radius, SDL_Color color) {
    if (radius <= 0) {
        SetColor(renderer, color);
        SDL_Rect rect = {x, y, w, h};
        SDL_RenderDrawRect(renderer, &rect);
        return;
    }
    roundedRectangleRGBA(renderer,
                         (Sint16)x, (Sint16)y,
                         (Sint16)(x + w), (Sint16)(y + h),
                         (Sint16)radius,
                         color.r, color.g, color.b, color.a);
}

// ============================================================================
// Text Rendering
// ============================================================================

void DrawText(SDL_Renderer* renderer, TTF_Font* font, const std::string& text,
              int x, int y, SDL_Color color, bool centerX, bool centerY) {
    if (!font || text.empty()) return;

    SDL_Surface* surface = TTF_RenderUTF8_Blended(font, text.c_str(), color);
    if (!surface) return;

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (!texture) {
        SDL_FreeSurface(surface);
        return;
    }

    SDL_Rect dstRect;
    dstRect.w = surface->w;
    dstRect.h = surface->h;
    dstRect.x = centerX ? (x - surface->w / 2) : x;
    dstRect.y = centerY ? (y - surface->h / 2) : y;

    SDL_RenderCopy(renderer, texture, nullptr, &dstRect);

    SDL_DestroyTexture(texture);
    SDL_FreeSurface(surface);
}

void DrawTextCentered(SDL_Renderer* renderer, TTF_Font* font, const std::string& text,
                      SDL_Rect rect, SDL_Color color) {
    DrawText(renderer, font, text,
             rect.x + rect.w / 2, rect.y + rect.h / 2,
             color, true, true);
}

// ============================================================================
// Decorative Elements
// ============================================================================

void DrawDivider(SDL_Renderer* renderer, int x, int y, int width, SDL_Color color) {
    // Draw a subtle gradient-like divider (center brighter, edges fade)
    for (int i = 0; i < width; i++) {
        // Fade at edges (first/last 20%)
        float edgeFade = 1.0f;
        float relPos = (float)i / (float)width;
        if (relPos < 0.2f) {
            edgeFade = relPos / 0.2f;
        } else if (relPos > 0.8f) {
            edgeFade = (1.0f - relPos) / 0.2f;
        }

        Uint8 alpha = (Uint8)(color.a * edgeFade);
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, alpha);
        SDL_RenderDrawPoint(renderer, x + i, y);
    }
}

void DrawDangoIcon(SDL_Renderer* renderer, int centerX, int centerY, int radius) {
    int spacing = radius * 2 + 4;

    // Stick (wooden skewer behind the balls)
    SDL_Color stickColor = {139, 119, 80, 255};
    DrawRect(renderer, centerX - 2, centerY - spacing - radius,
             4, spacing * 2 + radius * 2, stickColor);

    // Top ball — pink (🩷)
    filledCircleRGBA(renderer, (Sint16)centerX, (Sint16)(centerY - spacing),
                     (Sint16)radius,
                     Colors::DANGO_PINK.r, Colors::DANGO_PINK.g,
                     Colors::DANGO_PINK.b, 255);

    // Middle ball — cream (⚪)
    filledCircleRGBA(renderer, (Sint16)centerX, (Sint16)centerY,
                     (Sint16)radius,
                     Colors::DANGO_CREAM.r, Colors::DANGO_CREAM.g,
                     Colors::DANGO_CREAM.b, 255);

    // Bottom ball — green (🟢)
    filledCircleRGBA(renderer, (Sint16)centerX, (Sint16)(centerY + spacing),
                     (Sint16)radius,
                     Colors::DANGO_GREEN.r, Colors::DANGO_GREEN.g,
                     Colors::DANGO_GREEN.b, 255);
}

// ============================================================================
// Vector Icon Drawing Helpers (independent of font emojis)
// ============================================================================

void DrawIconFolder(SDL_Renderer* renderer, int cx, int cy, int size, SDL_Color color) {
    int w = size;
    int h = (int)(size * 0.75f);
    int x1 = cx - w / 2;
    int y1 = cy - h / 2;
    int x2 = cx + w / 2;
    int y2 = cy + h / 2;

    // Folder Tab (top-left)
    int tabW = (int)(w * 0.44f);
    int tabH = (int)(h * 0.28f);
    roundedBoxRGBA(renderer, x1, y1 - tabH + 2, x1 + tabW, y1 + 4, 3,
                   color.r, color.g, color.b, color.a);

    // Folder Main Body
    roundedBoxRGBA(renderer, x1, y1, x2, y2, 4,
                   color.r, color.g, color.b, color.a);

    // Slightly lighter front fold accent
    Uint8 flapAlpha = (Uint8)(color.a * 0.35f);
    boxRGBA(renderer, x1 + 2, y1 + (int)(h * 0.35f), x2 - 2, y2 - 2,
            255, 255, 255, flapAlpha);
}

void DrawIconHome(SDL_Renderer* renderer, int cx, int cy, int size, SDL_Color color) {
    int half = size / 2;
    // Triangular Roof
    filledTrigonRGBA(renderer,
                     (Sint16)cx, (Sint16)(cy - half),
                     (Sint16)(cx - half), (Sint16)(cy + 1),
                     (Sint16)(cx + half), (Sint16)(cy + 1),
                     color.r, color.g, color.b, color.a);

    // House Body
    int bodyHalfW = (int)(half * 0.72f);
    int bodyH = (int)(half * 0.9f);
    boxRGBA(renderer,
            cx - bodyHalfW, cy,
            cx + bodyHalfW, cy + bodyH,
            color.r, color.g, color.b, color.a);

    // Door (cutout)
    int doorW = (int)(bodyHalfW * 0.5f);
    int doorH = (int)(bodyH * 0.6f);
    boxRGBA(renderer,
            cx - doorW / 2, cy + bodyH - doorH,
            cx + doorW / 2, cy + bodyH,
            Colors::BG_DARK.r, Colors::BG_DARK.g, Colors::BG_DARK.b, 255);
}

void DrawIconTV(SDL_Renderer* renderer, int cx, int cy, int size, SDL_Color color) {
    int w = size;
    int h = (int)(size * 0.65f);
    int x1 = cx - w / 2;
    int y1 = cy - h / 2;
    int x2 = cx + w / 2;
    int y2 = cy + h / 2;

    // Antennas
    int antLen = (int)(size * 0.3f);
    thickLineRGBA(renderer, cx, y1, cx - antLen, y1 - antLen / 2, 2, color.r, color.g, color.b, color.a);
    thickLineRGBA(renderer, cx, y1, cx + antLen, y1 - antLen / 2, 2, color.r, color.g, color.b, color.a);

    // TV Outer Frame
    roundedBoxRGBA(renderer, x1, y1, x2, y2, 4, color.r, color.g, color.b, color.a);

    // Screen Inner Cutout
    int pad = std::max(2, (int)(size * 0.08f));
    roundedBoxRGBA(renderer, x1 + pad, y1 + pad, x2 - pad, y2 - pad, 2,
                   Colors::BG_DARK.r, Colors::BG_DARK.g, Colors::BG_DARK.b, 255);

    // Stand
    int standW = (int)(w * 0.45f);
    boxRGBA(renderer, cx - standW / 2, y2 + 1, cx + standW / 2, y2 + 3, color.r, color.g, color.b, color.a);
}

void DrawIconStar(SDL_Renderer* renderer, int cx, int cy, int size, SDL_Color color) {
    int outerR = size / 2;
    int innerR = (int)(outerR * 0.42f);
    Sint16 vx[10];
    Sint16 vy[10];
    for (int i = 0; i < 10; i++) {
        float angle = -3.14159265f / 2.0f + i * (3.14159265f / 5.0f);
        int r = (i % 2 == 0) ? outerR : innerR;
        vx[i] = (Sint16)(cx + r * cosf(angle));
        vy[i] = (Sint16)(cy + r * sinf(angle));
    }
    filledPolygonRGBA(renderer, vx, vy, 10, color.r, color.g, color.b, color.a);
}

void DrawIconSearch(SDL_Renderer* renderer, int cx, int cy, int size, SDL_Color color) {
    int rad = (int)(size * 0.32f);
    int glassCx = cx - (int)(size * 0.12f);
    int glassCy = cy - (int)(size * 0.12f);

    // Circle glass outline
    for (int r = rad - 1; r <= rad + 1; r++) {
        circleRGBA(renderer, glassCx, glassCy, r, color.r, color.g, color.b, color.a);
    }

    // Angled Handle
    int hStartX = glassCx + (int)(rad * 0.707f);
    int hStartY = glassCy + (int)(rad * 0.707f);
    int hEndX = cx + size / 2;
    int hEndY = cy + size / 2;
    thickLineRGBA(renderer, hStartX, hStartY, hEndX, hEndY, 3, color.r, color.g, color.b, color.a);
}

void DrawIconSettings(SDL_Renderer* renderer, int cx, int cy, int size, SDL_Color color) {
    int halfW = (int)(size * 0.45f);
    int spacing = (int)(size * 0.28f);

    // 3 slider tracks
    for (int i = -1; i <= 1; i++) {
        int trackY = cy + i * spacing;
        thickLineRGBA(renderer, cx - halfW, trackY, cx + halfW, trackY, 2,
                      color.r, color.g, color.b, (Uint8)(color.a * 0.6f));
    }

    // 3 slider knobs
    int knobOffsets[3] = {-halfW / 2, halfW / 3, -halfW / 4};
    for (int i = -1; i <= 1; i++) {
        int trackY = cy + i * spacing;
        int knobX = cx + knobOffsets[i + 1];
        filledCircleRGBA(renderer, knobX, trackY, (int)(size * 0.11f),
                         color.r, color.g, color.b, color.a);
    }
}

void DrawIconFilm(SDL_Renderer* renderer, int cx, int cy, int size, SDL_Color color) {
    int w = size;
    int h = (int)(size * 0.72f);
    int x1 = cx - w / 2;
    int y1 = cy - h / 2;
    int x2 = cx + w / 2;
    int y2 = cy + h / 2;

    // Card Frame
    roundedBoxRGBA(renderer, x1, y1, x2, y2, 4, color.r, color.g, color.b, color.a);

    // Inner cutout
    roundedBoxRGBA(renderer, x1 + 3, y1 + 3, x2 - 3, y2 - 3, 2,
                   Colors::BG_DARK.r, Colors::BG_DARK.g, Colors::BG_DARK.b, 255);

    // Play triangle
    int triSize = (int)(size * 0.24f);
    filledTrigonRGBA(renderer,
                     (Sint16)(cx - triSize / 2), (Sint16)(cy - triSize),
                     (Sint16)(cx - triSize / 2), (Sint16)(cy + triSize),
                     (Sint16)(cx + triSize), (Sint16)cy,
                     color.r, color.g, color.b, color.a);
}

// ============================================================================
// Utilities
// ============================================================================

SDL_Color LerpColor(SDL_Color a, SDL_Color b, float t) {
    t = std::max(0.0f, std::min(1.0f, t));
    return {
        (Uint8)(a.r + (b.r - a.r) * t),
        (Uint8)(a.g + (b.g - a.g) * t),
        (Uint8)(a.b + (b.b - a.b) * t),
        (Uint8)(a.a + (b.a - a.a) * t)
    };
}

}  // namespace UI
