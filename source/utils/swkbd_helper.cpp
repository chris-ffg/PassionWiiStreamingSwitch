// ============================================================================
// DangoPlayer Switch — Software Keyboard Helper Implementation
// ============================================================================

#include "utils/swkbd_helper.h"
#include "utils/logger.h"
#include <switch.h>
#include <vector>

namespace SwkbdHelper {

std::string Show(const std::string& guideText, const std::string& initialText, int maxLen) {
    SwkbdConfig kbd;
    Result rc = swkbdCreate(&kbd, 0);
    if (R_FAILED(rc)) {
        Logger::Warn("SwkbdHelper: swkbdCreate failed (rc=0x%x)", rc);
        return "";
    }

    swkbdConfigMakePresetDefault(&kbd);
    swkbdConfigSetGuideText(&kbd, guideText.c_str());
    if (!initialText.empty()) {
        swkbdConfigSetInitialText(&kbd, initialText.c_str());
    }

    std::vector<char> buffer(maxLen + 1, 0);
    rc = swkbdShow(&kbd, buffer.data(), buffer.size());
    swkbdClose(&kbd);

    if (R_SUCCEEDED(rc)) {
        return std::string(buffer.data());
    }
    return "";
}

}  // namespace SwkbdHelper
