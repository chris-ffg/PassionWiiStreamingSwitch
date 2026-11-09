// ============================================================================
// PassionWiiStreaming Switch — App Implementation
// ============================================================================

#include "app.h"
#include "ui/screen.h"
#include "ui/ui_common.h"
#include "ui/main_menu.h"
#include "ui/splash_screen.h"
#include "utils/logger.h"
#include "config.h"
#include <SDL2/SDL_image.h>
#include <sys/stat.h>

// ============================================================================
// Construction / Destruction
// ============================================================================

App::App() {}
App::~App() {}

// ============================================================================
// Initialize — SDL2, fonts, input, initial screen
// ============================================================================

bool App::Initialize() {
    Logger::Init();
    Logger::Info("Initializing %s v%s", APP_NAME, APP_VERSION);

    if (!InitSDL()) {
        return false;
    }

    if (!LoadFonts()) {
        return false;
    }

    // Ensure essential directories exist on SD card
    mkdir("sdmc:/switch", 0777);
    mkdir("sdmc:/switch/PassionWiiStreaming", 0777);
    mkdir("sdmc:/switch/PassionWiiStreaming/fonts", 0777);
    mkdir("sdmc:/switch/PassionWiiStreaming/cache", 0777);

    // Load user configuration from SD card
    Config::Instance().Load();

    // Initialize controller input (Joy-Con, Pro Controller, Handheld)
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    padInitializeDefault(&pad);

    // Push splash screen as initial screen (transitions to MainMenu)
    PushScreen(std::make_unique<SplashScreen>(this));

    running = true;
    Logger::Info("Initialization complete — entering main loop");
    return true;
}

// ============================================================================
// InitSDL — Window, renderer, subsystems
// ============================================================================

bool App::InitSDL() {
    // Note: SDL_INIT_AUDIO is intentionally omitted so libmpv's native Nintendo Switch
    // audren driver has exclusive, low-latency access to the console audio hardware.
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) < 0) {
        Logger::Error("SDL_Init failed: %s", SDL_GetError());
        return false;
    }

    if (TTF_Init() < 0) {
        Logger::Error("TTF_Init failed: %s", TTF_GetError());
        return false;
    }

    window = SDL_CreateWindow(
        APP_NAME,
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        SCREEN_W, SCREEN_H,
        SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL
    );
    if (!window) {
        Logger::Error("SDL_CreateWindow failed: %s", SDL_GetError());
        return false;
    }

    renderer = SDL_CreateRenderer(
        window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );
    if (!renderer) {
        Logger::Error("SDL_CreateRenderer failed: %s", SDL_GetError());
        return false;
    }

    // Enable alpha blending for transparent overlays
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    // Open the first joystick (Joy-Con / Pro Controller)
    if (SDL_NumJoysticks() > 0) {
        SDL_JoystickOpen(0);
    }

    // Load custom PassionWii galaxy background from romfs
    bgTexture = IMG_LoadTexture(renderer, "romfs:/background.jpg");
    if (bgTexture) {
        Logger::Info("Loaded background texture from romfs:/background.jpg");
    } else {
        Logger::Warn("Could not load romfs:/background.jpg: %s", IMG_GetError());
    }

    Logger::Info("SDL2 initialized: %dx%d, accelerated + vsync", SCREEN_W, SCREEN_H);
    return true;
}

// ============================================================================
// LoadFonts — Try romfs first, then SD card fallback
// ============================================================================

bool App::LoadFonts() {
    // Font search paths (tried in order)
    const char* fontPaths[] = {
        "romfs:/fonts/font.ttf",
        "sdmc:/switch/PassionWiiStreaming/fonts/font.ttf",
        "romfs:/fonts/NotoSans-Regular.ttf",
        "sdmc:/switch/PassionWiiStreaming/fonts/NotoSans-Regular.ttf",
        nullptr
    };

    const char* loadedPath = nullptr;
    for (int i = 0; fontPaths[i] != nullptr; i++) {
        fontLarge = TTF_OpenFont(fontPaths[i], 36);
        if (fontLarge) {
            loadedPath = fontPaths[i];
            break;
        }
    }

    if (!fontLarge) {
        Logger::Warn("No font file found! UI will render without text.");
        Logger::Warn("Place a .ttf font at romfs:/fonts/font.ttf or sdmc:/switch/DangoPlayer/fonts/font.ttf");
        // Continue without fonts — the UI will show shapes but no text
        return true;
    }

    // Open the same font at different sizes
    fontMedium = TTF_OpenFont(loadedPath, 24);
    fontSmall  = TTF_OpenFont(loadedPath, 18);
    fontTiny   = TTF_OpenFont(loadedPath, 14);

    Logger::Info("Fonts loaded from: %s", loadedPath);
    return true;
}

// ============================================================================
// Run — Main loop: input → update → render (60fps via vsync)
// ============================================================================

void App::Run() {
    Uint32 lastTick = SDL_GetTicks();

    while (appletMainLoop() && running) {
        // --- Delta time calculation ---
        Uint32 now = SDL_GetTicks();
        float dt = (now - lastTick) / 1000.0f;
        if (dt > 0.1f) dt = 0.1f;  // Clamp to avoid large jumps (e.g., on resume)
        lastTick = now;

        // --- Docked vs Handheld mode monitoring ---
        AppletOperationMode opMode = appletGetOperationMode();
        bool currentlyDocked = (opMode == AppletOperationMode_Console);
        if (currentlyDocked != isDocked) {
            isDocked = currentlyDocked;
            Logger::Info("App: Operation mode changed to %s", isDocked ? "Docked (TV)" : "Handheld (Portatil)");
        }

        // --- Pump SDL events (mouse/touch/quit) ---
        bool backTriggered = false;
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_KEYDOWN) {
                // Map ESC and Android Back button to Joy-Con [B] button for Stack POP
                if (event.key.keysym.sym == SDLK_ESCAPE || event.key.keysym.sym == SDLK_AC_BACK) {
                    backTriggered = true;
                }
            } else if (event.type == SDL_MOUSEBUTTONDOWN) {
                TouchEvent touch{TouchEvent::Down, event.button.x, event.button.y, 0};
                if (!screenStack.empty()) {
                    screenStack.back()->HandleTouch(touch);
                }
            } else if (event.type == SDL_MOUSEBUTTONUP) {
                TouchEvent touch{TouchEvent::Up, event.button.x, event.button.y, 0};
                if (!screenStack.empty()) {
                    screenStack.back()->HandleTouch(touch);
                }
            } else if (event.type == SDL_MOUSEMOTION) {
                TouchEvent touch{TouchEvent::Motion, event.motion.x, event.motion.y, 0};
                if (!screenStack.empty()) {
                    screenStack.back()->HandleTouch(touch);
                }
            } else if (event.type == SDL_FINGERDOWN) {
                TouchEvent touch{TouchEvent::Down, (int)(event.tfinger.x * SCREEN_W), (int)(event.tfinger.y * SCREEN_H), (int)event.tfinger.fingerId};
                if (!screenStack.empty()) {
                    screenStack.back()->HandleTouch(touch);
                }
            } else if (event.type == SDL_FINGERUP) {
                TouchEvent touch{TouchEvent::Up, (int)(event.tfinger.x * SCREEN_W), (int)(event.tfinger.y * SCREEN_H), (int)event.tfinger.fingerId};
                if (!screenStack.empty()) {
                    screenStack.back()->HandleTouch(touch);
                }
            }
        }

        // --- Update controller state via libnx ---
        padUpdate(&pad);
        u64 kDown = padGetButtonsDown(&pad);
        u64 kHeld = padGetButtons(&pad);
        if (backTriggered) {
            kDown |= HidNpadButton_B;
        }

        // --- Analog stick support (Left stick up/down navigation) ---
        HidAnalogStickState stickL = padGetStickPos(&pad, 0);
        if (stickDebounce > 0.0f) {
            stickDebounce -= dt;
            if (stickDebounce < 0.0f) stickDebounce = 0.0f;
        }

        if (std::abs(stickL.y) > 18000) {
            if (stickDebounce <= 0.0f) {
                if (stickL.y > 18000) {
                    kDown |= HidNpadButton_AnyUp;
                } else if (stickL.y < -18000) {
                    kDown |= HidNpadButton_AnyDown;
                }
                stickDebounce = 0.22f; // repeat delay for smooth scrolling
            }
        } else {
            // Reset debounce immediately when stick returns to deadzone
            stickDebounce = 0.0f;
        }

        // --- Global input: + button exits from root screen ---
        if (kDown & HidNpadButton_Plus) {
            if (screenStack.size() <= 1) {
                running = false;
                continue;
            } else {
                PopScreen();
                continue;
            }
        }

        // --- Delegate to current screen ---
        if (!screenStack.empty()) {
            screenStack.back()->HandleInput(kDown, kHeld);
            screenStack.back()->Update(dt);
        }

        // --- Render ---
        UI::SetColor(renderer, Colors::BG_DARK);
        SDL_RenderClear(renderer);

        if (bgTexture) {
            SDL_RenderCopy(renderer, bgTexture, nullptr, nullptr);
            // Soft dark overlay to keep UI elements, text, and posters readable and vibrant
            UI::DrawRect(renderer, 0, 0, SCREEN_W, SCREEN_H, {10, 10, 16, 175});
        }

        if (!screenStack.empty()) {
            screenStack.back()->Render(renderer);
        }

        SDL_RenderPresent(renderer);  // Vsync'd — blocks until next vblank
    }
}

// ============================================================================
// Shutdown — Cleanup in reverse order
// ============================================================================

void App::Shutdown() {
    Logger::Info("Shutting down...");

    // Clear all screens
    while (!screenStack.empty()) {
        screenStack.back()->OnExit();
        screenStack.pop_back();
    }

    // Save user config
    Config::Instance().Save();

    // Destroy background texture
    if (bgTexture) {
        SDL_DestroyTexture(bgTexture);
        bgTexture = nullptr;
    }

    // Close fonts
    if (fontTiny)   { TTF_CloseFont(fontTiny);   fontTiny   = nullptr; }
    if (fontSmall)  { TTF_CloseFont(fontSmall);   fontSmall  = nullptr; }
    if (fontMedium) { TTF_CloseFont(fontMedium); fontMedium = nullptr; }
    if (fontLarge)  { TTF_CloseFont(fontLarge);   fontLarge  = nullptr; }

    TTF_Quit();

    if (renderer) { SDL_DestroyRenderer(renderer); renderer = nullptr; }
    if (window)   { SDL_DestroyWindow(window);     window   = nullptr; }
    SDL_Quit();

    Logger::Info("Shutdown complete");
    Logger::Shutdown();
}

// ============================================================================
// Screen Stack Management
// ============================================================================

void App::PushScreen(std::unique_ptr<Screen> screen) {
    if (!screenStack.empty()) {
        screenStack.back()->OnExit();
    }
    screenStack.push_back(std::move(screen));
    screenStack.back()->OnEnter();
    Logger::Info("Screen pushed (stack depth: %zu)", screenStack.size());
}

void App::PopScreen() {
    if (!screenStack.empty()) {
        screenStack.back()->OnExit();
        screenStack.pop_back();
        if (!screenStack.empty()) {
            screenStack.back()->OnEnter();
        }
        Logger::Info("Screen popped (stack depth: %zu)", screenStack.size());
    }
}

void App::ReplaceScreen(std::unique_ptr<Screen> screen) {
    if (!screenStack.empty()) {
        screenStack.back()->OnExit();
        screenStack.pop_back();
    }
    screenStack.push_back(std::move(screen));
    screenStack.back()->OnEnter();
    Logger::Info("Screen replaced (stack depth: %zu)", screenStack.size());
}
