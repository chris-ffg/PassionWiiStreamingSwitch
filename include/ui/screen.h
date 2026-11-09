// ============================================================================
// DangoPlayer Switch — Screen Base Class
// ============================================================================
// Abstract base for all screens in the app. The App class manages a stack of
// Screens: back() is the active screen that receives input/update/render.
//
// Lifecycle:
//   OnEnter()  — called when this screen becomes the active (top) screen
//   OnExit()   — called when this screen is covered by another or popped
//
// Per-frame:
//   HandleInput(kDown, kHeld) — process controller buttons
//   Update(dt)                — update animations, state
//   Render(renderer)          — draw to screen
// ============================================================================

#pragma once

#include <SDL2/SDL.h>
#include <switch.h>

// Touch input event structure (translated from SDL2 or libnx)
struct TouchEvent {
    enum Type { Down, Up, Motion };
    Type type;
    int x;
    int y;
    int touchId;
};

// Forward declaration to avoid circular includes
class App;

class Screen {
public:
    explicit Screen(App* app) : app(app) {}
    virtual ~Screen() = default;

    // Lifecycle callbacks
    virtual void OnEnter() {}
    virtual void OnExit() {}

    // Per-frame input methods
    virtual void HandleInput(u64 kDown, u64 kHeld) = 0;
    virtual void HandleTouch(const TouchEvent& touch) {}

    // Per-frame logic and rendering
    virtual void Update(float dt) = 0;
    virtual void Render(SDL_Renderer* renderer) = 0;

protected:
    App* app;  // Non-owning pointer to the parent application
};
