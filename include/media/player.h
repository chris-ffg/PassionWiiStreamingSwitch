// ============================================================================
// DangoPlayer Switch — Media Player Core
// ============================================================================
// Core media playback engine built on top of libmpv and FFmpeg.
// Features:
// - Hardware video decoding via Tegra X1 NVDEC
// - Local file playback (MP4, MKV, AVI, TS, WebM, etc.)
// - Audio & subtitle track enumeration and cycling
// - Automatic resume playback position persistence
// - Safe fallback and property observer loop
// ============================================================================

#pragma once

#include "media/media_info.h"
#include <string>
#include <vector>
#include <functional>

// Forward declaration of mpv_handle and mpv_render_context
struct mpv_handle;
struct mpv_render_context;

class MediaPlayer {
public:
    static MediaPlayer& Instance();

    bool Initialize();
    void Shutdown();

    /// Open and start playing a media file or stream URL
    bool Open(const std::string& path, const std::string& explicitTitle = "");

    /// Render the current MPV video frame into the active OpenGL framebuffer
    void RenderVideo();

    /// Playback controls
    void Play();
    void Pause();
    void TogglePause();
    void Stop();

    /// Seek controls
    void Seek(double seconds, bool relative = true);
    void SeekPercent(float percent);

    /// Volume (0 to 100)
    void SetVolume(int vol);
    int GetVolume() const { return volume; }

    /// Tracks
    void CycleAudioTrack();
    void CycleSubtitleTrack();
    void SetAudioTrack(int id);
    void SetSubtitleTrack(int id);

    /// Video rotation controls (0, 90, 180, 270)
    void CycleRotation();
    void SetRotation(int degrees);
    int GetRotation() const { return currentRotation; }

    /// State queries
    PlayerStatus GetStatus() const { return status; }
    double GetPosition() const { return position; }
    double GetDuration() const { return duration; }
    float GetProgress() const {
        return (duration > 0.0) ? (float)(position / duration) : 0.0f;
    }
    bool IsPaused() const { return status == PlayerStatus::Paused; }
    bool IsPlaying() const { return status == PlayerStatus::Playing; }
    const MediaMetadata& GetMetadata() const { return metadata; }

    /// Per-frame polling of player events & position updates
    void Update();

    /// Resume cache (persisted in sdmc:/switch/DangoPlayer/resume_cache.txt)
    double GetResumePosition(const std::string& path);
    void SaveResumePosition(const std::string& path, double pos);

    /// Callback invoked when a video reaches EOF
    void SetOnEndedCallback(std::function<void()> cb) { onEndedCallback = cb; }

private:
    MediaPlayer();
    ~MediaPlayer();

    void ParseTracks();
    void CheckResume(const std::string& path);

    mpv_handle* mpv = nullptr;
    mpv_render_context* mpv_gl = nullptr;
    PlayerStatus status = PlayerStatus::Idle;

    double position = 0.0;
    double duration = 0.0;
    int volume = 100;
    int currentRotation = 0;

    MediaMetadata metadata;
    std::function<void()> onEndedCallback = nullptr;

    bool initialized = false;
};
