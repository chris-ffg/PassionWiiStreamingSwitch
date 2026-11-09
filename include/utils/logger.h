// ============================================================================
// DangoPlayer Switch — Logger
// ============================================================================
// Simple logging utility that writes to stdout/stderr. When running via
// nxlink, output is redirected to the PC console for debugging.
//
// Usage:
//   Logger::Info("Player initialized: %s", filename);
//   Logger::Warn("Stream buffer low: %d bytes", remaining);
//   Logger::Error("Failed to open: %s", SDL_GetError());
// ============================================================================

#pragma once

namespace Logger {
    /// Initialize the logger.
    void Init();

    /// Log an informational message.
    void Info(const char* fmt, ...);

    /// Log a warning message.
    void Warn(const char* fmt, ...);

    /// Log an error message (goes to stderr).
    void Error(const char* fmt, ...);

    /// Shutdown the logger.
    void Shutdown();
}
