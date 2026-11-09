// ============================================================================
// DangoPlayer Switch — Media Information Types
// ============================================================================

#pragma once

#include <string>
#include <vector>

enum class TrackType {
    Audio,
    Subtitle,
    Video
};

struct TrackInfo {
    int id = 0;
    TrackType type = TrackType::Audio;
    std::string title;
    std::string language;
    std::string codec;
    bool selected = false;
};

struct MediaMetadata {
    std::string title;
    std::string path;
    double duration = 0.0;
    int videoWidth = 0;
    int videoHeight = 0;
    std::string videoCodec;
    std::string audioCodec;
    bool hwDecoded = false;
    std::vector<TrackInfo> audioTracks;
    std::vector<TrackInfo> subtitleTracks;
};

enum class PlayerStatus {
    Idle,
    Loading,
    Playing,
    Paused,
    Ended,
    Error
};
