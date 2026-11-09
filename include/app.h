// ============================================================================
// DangoPlayer Switch — App Class
// ============================================================================
// Central application class. Manages:
// - SDL2 lifecycle (window, renderer)
// - Font loading from romfs or SD card
// - Screen stack (push/pop navigation)
// - Main loop (input → update → render at 60fps via vsync)
// - Controller input via libnx PadState
// ============================================================================

#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <switch.h>
#include <vector>
#include <memory>
#include <string>

// Forward declaration
class Screen;

class App {
public:
    App();
    ~App();

    // Lifecycle
    bool Initialize();
    void Run();
    void Shutdown();

    // Screen management (stack-based navigation)
    void PushScreen(std::unique_ptr<Screen> screen);
    void PopScreen();
    void ReplaceScreen(std::unique_ptr<Screen> screen);

    // Accessors
    SDL_Renderer* GetRenderer() const { return renderer; }
    TTF_Font*     GetFontLarge() const { return fontLarge; }
    TTF_Font*     GetFont() const { return fontMedium; }
    TTF_Font*     GetFontSmall() const { return fontSmall; }
    TTF_Font*     GetFontTiny() const { return fontTiny; }
    bool          IsRunning() const { return running; }
    bool          IsDocked() const { return isDocked; }
    void          Quit() { running = false; }
    SDL_Texture*  GetBackgroundTexture() const { return bgTexture; }

    // App metadata
    static constexpr const char* APP_NAME    = "PassionWiiStreaming Switch Edition";
    static constexpr const char* APP_VERSION = "1.0.0";
    static constexpr int SCREEN_W = 1280;
    static constexpr int SCREEN_H = 720;

private:
    bool InitSDL();
    bool LoadFonts();

    SDL_Window*   window    = nullptr;
    SDL_Renderer* renderer  = nullptr;
    SDL_Texture*  bgTexture = nullptr;

    // Fonts at different sizes (loaded from the same TTF file)
    TTF_Font* fontLarge  = nullptr;  // 36pt — titles
    TTF_Font* fontMedium = nullptr;  // 24pt — body text, menu items
    TTF_Font* fontSmall  = nullptr;  // 18pt — descriptions, hints
    TTF_Font* fontTiny   = nullptr;  // 14pt — version, subtle labels

    // Navigation: stack of screens (back() = active screen)
    std::vector<std::unique_ptr<Screen>> screenStack;

    // Input
    PadState pad;
    float stickDebounce = 0.0f;

    bool isDocked = false;
    bool running = false;
};
