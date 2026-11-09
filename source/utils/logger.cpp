// ============================================================================
// DangoPlayer Switch — Logger Implementation
// ============================================================================

#include "utils/logger.h"
#include <cstdio>
#include <cstdarg>

namespace Logger {

static bool sInitialized = false;

void Init() {
    sInitialized = true;
    Info("Logger initialized");
}

void Info(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    printf("[PASSIONWII INFO]  ");
    vprintf(fmt, args);
    printf("\n");
    fflush(stdout);
    va_end(args);
}

void Warn(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    printf("[PASSIONWII WARN]  ");
    vprintf(fmt, args);
    printf("\n");
    fflush(stdout);
    va_end(args);
}

void Error(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "[PASSIONWII ERROR] ");
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    fflush(stderr);
    va_end(args);
}

void Shutdown() {
    if (sInitialized) {
        Info("Logger shutting down");
        sInitialized = false;
    }
}

}  // namespace Logger
