// ============================================================================
// DangoPlayer Switch — Software Keyboard Helper (Horizon OS Native)
// ============================================================================

#pragma once

#include <string>

namespace SwkbdHelper {
    /// Show the native Nintendo Switch on-screen software keyboard.
    /// Returns the text entered by the user, or empty if cancelled.
    std::string Show(const std::string& guideText,
                     const std::string& initialText = "",
                     int maxLen = 512);
}
