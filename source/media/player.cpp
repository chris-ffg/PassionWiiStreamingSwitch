// ============================================================================
// DangoPlayer Switch — Media Player Implementation
// ============================================================================

#include "media/player.h"
#include "config.h"
#include "network/http_client.h"
#include "utils/string_utils.h"
#include "utils/logger.h"
#ifdef __SWITCH__
#include <switch.h>
#endif
#include <mpv/client.h>
#include <mpv/render_gl.h>
#include <SDL2/SDL.h>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <clocale>
#include <algorithm>
#include <map>
#include <fstream>
#include <sstream>
#include <sys/stat.h>

static const char* RESUME_FILE = "sdmc:/switch/PassionWiiStreaming/resume_cache.txt";
static const char* MPV_CONFIG_DIR = "sdmc:/switch/PassionWiiStreaming";
static const char* MPV_FONTS_DIR = "sdmc:/switch/PassionWiiStreaming/fonts";

static bool CopyAssetFileIfNeeded(const char* srcPath, const char* dstPath) {
    struct stat srcSt{};
    if (stat(srcPath, &srcSt) != 0 || srcSt.st_size <= 0) {
        return false;
    }

    struct stat dstSt{};
    if (stat(dstPath, &dstSt) == 0 && dstSt.st_size == srcSt.st_size) {
        return true;
    }

    FILE* in = fopen(srcPath, "rb");
    if (!in) return false;

    FILE* out = fopen(dstPath, "wb");
    if (!out) {
        fclose(in);
        return false;
    }

    char buf[16384];
    size_t n = 0;
    while ((n = fread(buf, 1, sizeof(buf), in)) > 0) {
        if (fwrite(buf, 1, n, out) != n) {
            fclose(out);
            fclose(in);
            return false;
        }
    }

    fclose(out);
    fclose(in);
    return true;
}

static void EnsureSubtitleFontSetup() {
    mkdir("sdmc:/switch", 0777);
    mkdir(MPV_CONFIG_DIR, 0777);
    mkdir(MPV_FONTS_DIR, 0777);

    const char* srcFont = "romfs:/fonts/font.ttf";
    struct stat st{};
    if (stat(srcFont, &st) != 0) {
        srcFont = "romfs:/fonts/NotoSans-Regular.ttf";
    }

    bool okSub = CopyAssetFileIfNeeded(srcFont, "sdmc:/switch/PassionWiiStreaming/subfont.ttf");
    bool okDir = CopyAssetFileIfNeeded(srcFont, "sdmc:/switch/PassionWiiStreaming/fonts/NotoSans-Regular.ttf");
    Logger::Info("MediaPlayer: Subtitle font provisioning (%s): subfont=%d fontsDir=%d",
                 srcFont, okSub ? 1 : 0, okDir ? 1 : 0);
}

static void* get_proc_address_mpv(void* ctx, const char* name) {
    (void)ctx;
    return SDL_GL_GetProcAddress(name);
}

MediaPlayer& MediaPlayer::Instance() {
    static MediaPlayer instance;
    return instance;
}

MediaPlayer::MediaPlayer() {
    volume = Config::Instance().Get().defaultVolume;
}

MediaPlayer::~MediaPlayer() {
    Shutdown();
}

bool MediaPlayer::Initialize() {
    if (initialized) return true;

    Logger::Info("MediaPlayer: Initializing libmpv with OpenGL render context...");
    std::setlocale(LC_NUMERIC, "C");

    // Provision bundled TrueType subtitle font to SD so libass on Horizon OS always has a font
    EnsureSubtitleFontSetup();

    mpv = mpv_create();
    if (!mpv) {
        Logger::Error("MediaPlayer: Failed to create mpv instance!");
        return false;
    }

    // Config & font paths (required for libass/mpv subtitle rendering without system fontconfig)
    mpv_set_option_string(mpv, "config", "yes");
    mpv_set_option_string(mpv, "config-dir", MPV_CONFIG_DIR);
    mpv_set_option_string(mpv, "sub-fonts-dir", MPV_FONTS_DIR);
    mpv_set_option_string(mpv, "osd-fonts-dir", MPV_FONTS_DIR);
    mpv_set_option_string(mpv, "osd-level", "0");

    // Audio output: use native Nintendo Switch Horizon OS audio driver (ao_hos)
    mpv_set_option_string(mpv, "ao", "hos,null,");

    // Hardware decoding configuration (Tegra X1 NVDEC)
    mpv_set_option_string(mpv, "hwdec", "auto-safe");
    mpv_set_option_string(mpv, "tls-verify", "no");

    // Network & streaming reliability options
    mpv_set_option_string(mpv, "user-agent",
        "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36");
    mpv_set_option_string(mpv, "network-timeout", "30");
    mpv_set_option_string(mpv, "demuxer-lavf-o", "reconnect=1,reconnect_streamed=1,reconnect_delay_max=5");

    // Buffer and cache tuned for Switch RAM
    mpv_set_option_string(mpv, "cache", "yes");
    mpv_set_option_string(mpv, "demuxer-max-bytes", "64MiB");
    mpv_set_option_string(mpv, "demuxer-max-back-bytes", "32MiB");

    // Video orientation: Nintendo Switch OpenGL FBO 0 has flip_y=1, so default rotation is 0
    mpv_set_option_string(mpv, "video-rotate", "0");

    // Fast decoding optimizations
    mpv_set_option_string(mpv, "vd-lavc-fast", "yes");
    mpv_set_option_string(mpv, "vd-lavc-skiploopfilter", "all");
    mpv_set_option_string(mpv, "vd-lavc-threads", "4");

    // Audio & Subtitle configuration (libass needs explicit fonts + visibility on Switch)
    mpv_set_option_string(mpv, "sub-auto", "fuzzy");
    mpv_set_option_string(mpv, "audio-file-auto", "fuzzy");
    mpv_set_option_string(mpv, "sub-font", "Noto Sans");
    mpv_set_option_string(mpv, "sub-font-size", "48");
    mpv_set_option_string(mpv, "sub-color", "#FFFFFFFF");
    mpv_set_option_string(mpv, "sub-border-size", "2.5");
    mpv_set_option_string(mpv, "sub-border-color", "#FF000000");
    mpv_set_option_string(mpv, "sub-shadow-offset", "1");
    mpv_set_option_string(mpv, "sub-shadow-color", "0.0/0.0/0.0/0.5");
    mpv_set_option_string(mpv, "sub-visibility", "yes");
    mpv_set_option_string(mpv, "embeddedfonts", "yes");

    // Set initial volume
    int64_t initVol = volume;
    mpv_set_property(mpv, "volume", MPV_FORMAT_INT64, &initVol);

    // Request async events
    mpv_request_log_messages(mpv, "info");

    int res = mpv_initialize(mpv);
    if (res < 0) {
        Logger::Error("MediaPlayer: mpv_initialize failed: %s", mpv_error_string(res));
        mpv_destroy(mpv);
        mpv = nullptr;
        return false;
    }

    mpv_observe_property(mpv, 1, "track-list/count", MPV_FORMAT_INT64);

    // Initialize libmpv OpenGL render context without advanced control to avoid deadlock
    mpv_opengl_init_params gl_init = { get_proc_address_mpv, nullptr };
    mpv_render_param params[] = {
        { MPV_RENDER_PARAM_API_TYPE, (void*)MPV_RENDER_API_TYPE_OPENGL },
        { MPV_RENDER_PARAM_OPENGL_INIT_PARAMS, &gl_init },
        { MPV_RENDER_PARAM_INVALID, nullptr }
    };

    int gl_res = mpv_render_context_create(&mpv_gl, mpv, params);
    if (gl_res < 0) {
        Logger::Warn("MediaPlayer: mpv_render_context_create warning (%d): %s",
                     gl_res, mpv_error_string(gl_res));
    } else {
        Logger::Info("MediaPlayer: mpv_render_context created successfully for OpenGL");
    }

    initialized = true;
    status = PlayerStatus::Idle;
    Logger::Info("MediaPlayer: Initialized successfully");
    return true;
}

void MediaPlayer::Shutdown() {
#ifdef __SWITCH__
    appletSetMediaPlaybackState(false);
#endif
    if (!initialized) return;

    if (!metadata.path.empty() && position > 10.0) {
        SaveResumePosition(metadata.path, position);
    }

    Stop();

    if (mpv_gl) {
        mpv_render_context_free(mpv_gl);
        mpv_gl = nullptr;
    }

    if (mpv) {
        mpv_terminate_destroy(mpv);
        mpv = nullptr;
    }

    initialized = false;
    status = PlayerStatus::Idle;
    Logger::Info("MediaPlayer: Shutdown complete");
}

void MediaPlayer::RenderVideo() {
    if (!mpv_gl) return;

    // Process render updates
    mpv_render_context_update(mpv_gl);

    int w = 1280;
    int h = 720;
    mpv_opengl_fbo fbo = { 0, w, h, 0 };
    int flip_y = 1;
    mpv_render_param params[] = {
        { MPV_RENDER_PARAM_OPENGL_FBO, &fbo },
        { MPV_RENDER_PARAM_FLIP_Y, &flip_y },
        { MPV_RENDER_PARAM_INVALID, nullptr }
    };
    mpv_render_context_render(mpv_gl, params);
}

bool MediaPlayer::Open(const std::string& path, const std::string& explicitTitle) {
    if (!initialized) {
        if (!Initialize()) return false;
    }

    Stop();

    // Drain any remaining events before opening the new file
    if (mpv) {
        while (true) {
            mpv_event* event = mpv_wait_event(mpv, 0);
            if (!event || event->event_id == MPV_EVENT_NONE) break;
        }
    }

    std::string safePath = HttpClient::SanitizeUrl(path);

    metadata = MediaMetadata();
    metadata.path = safePath;

    // If explicitTitle was passed from the playlist item, use it (decoded)
    if (!explicitTitle.empty()) {
        metadata.title = StringUtils::UrlDecode(explicitTitle);
    } else {
        std::string rawName = StringUtils::GetFilename(path);
        metadata.title = StringUtils::UrlDecode(rawName);
    }

    position = 0.0;
    duration = 0.0;
    status = PlayerStatus::Loading;

    Logger::Info("MediaPlayer: Loading stream: %s (Title: %s, Rotation: %d)", safePath.c_str(), metadata.title.c_str(), currentRotation);

    // Apply current rotation setting and reset track selection cleanly for new file
    std::string rotStr = std::to_string(currentRotation);
    mpv_set_property_string(mpv, "video-rotate", rotStr.c_str());
    mpv_set_property_string(mpv, "sid", "auto");
    mpv_set_property_string(mpv, "aid", "auto");
    int subVis = 1;
    mpv_set_property(mpv, "sub-visibility", MPV_FORMAT_FLAG, &subVis);

    const char* cmd[] = {"loadfile", safePath.c_str(), "replace", nullptr};
    int res = mpv_command(mpv, cmd);
    if (res < 0) {
        Logger::Error("MediaPlayer: loadfile failed: %s", mpv_error_string(res));
        status = PlayerStatus::Error;
        return false;
    }

    // Force playback to start unpaused and unmuted
    int pauseVal = 0;
    mpv_set_property(mpv, "pause", MPV_FORMAT_FLAG, &pauseVal);
    int muteVal = 0;
    mpv_set_property(mpv, "mute", MPV_FORMAT_FLAG, &muteVal);
    int64_t curVol = volume > 0 ? volume : 100;
    mpv_set_property(mpv, "volume", MPV_FORMAT_INT64, &curVol);

#ifdef __SWITCH__
    // Inhibit auto-sleep & screen dimming during playback
    appletSetMediaPlaybackState(true);
#endif

    // Check if resume position exists
    if (Config::Instance().Get().resumePlayback) {
        double resumePos = GetResumePosition(path);
        if (resumePos > 15.0) {
            Logger::Info("MediaPlayer: Resuming from %.1f seconds", resumePos);
            Seek(resumePos, false);
        }
    }

    return true;
}

void MediaPlayer::Play() {
    if (!mpv) return;
    int pauseVal = 0;
    mpv_set_property(mpv, "pause", MPV_FORMAT_FLAG, &pauseVal);
    status = PlayerStatus::Playing;
#ifdef __SWITCH__
    // Inhibit auto-sleep while actively playing
    appletSetMediaPlaybackState(true);
#endif
}

void MediaPlayer::Pause() {
    if (!mpv) return;
    int pauseVal = 1;
    mpv_set_property(mpv, "pause", MPV_FORMAT_FLAG, &pauseVal);
    status = PlayerStatus::Paused;
#ifdef __SWITCH__
    // Allow auto-sleep and screen dimming when paused
    appletSetMediaPlaybackState(false);
#endif
}

void MediaPlayer::TogglePause() {
    if (status == PlayerStatus::Playing) {
        Pause();
    } else {
        Play();
    }
}

void MediaPlayer::Stop() {
#ifdef __SWITCH__
    // Allow auto-sleep when playback is stopped
    appletSetMediaPlaybackState(false);
#endif
    if (!mpv) return;

    if (!metadata.path.empty() && position > 10.0) {
        SaveResumePosition(metadata.path, position);
    }

    const char* cmd[] = {"stop", nullptr};
    mpv_command(mpv, cmd);

    // Drain all pending events immediately so no leftover events linger in the queue
    while (true) {
        mpv_event* event = mpv_wait_event(mpv, 0);
        if (!event || event->event_id == MPV_EVENT_NONE) break;
    }

    status = PlayerStatus::Idle;
    position = 0.0;
    duration = 0.0;
}

void MediaPlayer::CycleRotation() {
    if (currentRotation == 0) currentRotation = 90;
    else if (currentRotation == 90) currentRotation = 180;
    else if (currentRotation == 180) currentRotation = 270;
    else currentRotation = 0;

    SetRotation(currentRotation);
}

void MediaPlayer::SetRotation(int degrees) {
    currentRotation = degrees;
    if (mpv) {
        std::string rotStr = std::to_string(currentRotation);
        mpv_set_property_string(mpv, "video-rotate", rotStr.c_str());
        Logger::Info("MediaPlayer: Video rotation set to %d degrees", currentRotation);
    }
}

void MediaPlayer::Seek(double seconds, bool relative) {
    if (!mpv || status == PlayerStatus::Idle || status == PlayerStatus::Error) return;

    char secBuf[64];
    snprintf(secBuf, sizeof(secBuf), "%.2f", seconds);

    const char* mode = relative ? "relative" : "absolute";
    const char* cmd[] = {"seek", secBuf, mode, nullptr};
    mpv_command(mpv, cmd);

    if (relative) {
        position = std::max(0.0, std::min(duration, position + seconds));
    } else {
        position = std::max(0.0, std::min(duration, seconds));
    }
}

void MediaPlayer::SeekPercent(float percent) {
    if (duration > 0.0) {
        double target = duration * std::clamp(percent, 0.0f, 1.0f);
        Seek(target, false);
    }
}

void MediaPlayer::SetVolume(int vol) {
    volume = std::clamp(vol, 0, 100);
    if (!mpv) return;

    int64_t v = volume;
    mpv_set_property(mpv, "volume", MPV_FORMAT_INT64, &v);
}

void MediaPlayer::RefreshTracks() {
    ParseTracks();
}

void MediaPlayer::CycleAudioTrack() {
    if (!mpv) return;
    const char* cmd[] = {"cycle", "aid", nullptr};
    mpv_command(mpv, cmd);
    ParseTracks();
}

void MediaPlayer::CycleSubtitleTrack() {
    if (!mpv) return;
    const char* cmd[] = {"cycle", "sid", nullptr};
    mpv_command(mpv, cmd);
    ParseTracks();
}

void MediaPlayer::SetAudioTrack(int id) {
    if (!mpv) return;
    int64_t trackId = id;
    mpv_set_property(mpv, "aid", MPV_FORMAT_INT64, &trackId);
    ParseTracks();
}

void MediaPlayer::SetSubtitleTrack(int id) {
    if (!mpv) return;
    if (id <= 0) {
        mpv_set_property_string(mpv, "sid", "no");
        Logger::Info("MediaPlayer: Subtitles disabled");
    } else {
        int vis = 1;
        mpv_set_property(mpv, "sub-visibility", MPV_FORMAT_FLAG, &vis);
        int64_t trackId = id;
        int err = mpv_set_property(mpv, "sid", MPV_FORMAT_INT64, &trackId);
        if (err < 0) {
            std::string idStr = std::to_string(id);
            err = mpv_set_property_string(mpv, "sid", idStr.c_str());
        }
        Logger::Info("MediaPlayer: Subtitle track set to sid=%d (res=%d)", id, err);
    }
    ParseTracks();
}

void MediaPlayer::ParseTracks() {
    if (!mpv) return;

    metadata.audioTracks.clear();
    metadata.subtitleTracks.clear();

    // Query track-list count
    int64_t count = 0;
    if (mpv_get_property(mpv, "track-list/count", MPV_FORMAT_INT64, &count) < 0) {
        return;
    }

    for (int64_t i = 0; i < count; i++) {
        char prop[64];

        snprintf(prop, sizeof(prop), "track-list/%lld/type", (long long)i);
        char* typeStr = mpv_get_property_string(mpv, prop);
        if (!typeStr) continue;

        snprintf(prop, sizeof(prop), "track-list/%lld/id", (long long)i);
        int64_t trackId = 0;
        mpv_get_property(mpv, prop, MPV_FORMAT_INT64, &trackId);

        snprintf(prop, sizeof(prop), "track-list/%lld/title", (long long)i);
        char* titleStr = mpv_get_property_string(mpv, prop);

        snprintf(prop, sizeof(prop), "track-list/%lld/lang", (long long)i);
        char* langStr = mpv_get_property_string(mpv, prop);

        snprintf(prop, sizeof(prop), "track-list/%lld/codec", (long long)i);
        char* codecStr = mpv_get_property_string(mpv, prop);

        snprintf(prop, sizeof(prop), "track-list/%lld/selected", (long long)i);
        int selected = 0;
        mpv_get_property(mpv, prop, MPV_FORMAT_FLAG, &selected);

        TrackInfo track;
        track.id = (int)trackId;
        track.title = titleStr ? titleStr : "";
        track.language = langStr ? langStr : "und";
        track.codec = codecStr ? codecStr : "";
        track.selected = (selected != 0);

        if (strcmp(typeStr, "audio") == 0) {
            track.type = TrackType::Audio;
            metadata.audioTracks.push_back(track);
        } else if (strcmp(typeStr, "sub") == 0) {
            track.type = TrackType::Subtitle;
            metadata.subtitleTracks.push_back(track);
        }

        if (typeStr) mpv_free(typeStr);
        if (titleStr) mpv_free(titleStr);
        if (langStr) mpv_free(langStr);
        if (codecStr) mpv_free(codecStr);
    }
}

void MediaPlayer::Update() {
    if (!mpv) return;

    // Process all pending MPV events
    while (true) {
        mpv_event* event = mpv_wait_event(mpv, 0);
        if (!event || event->event_id == MPV_EVENT_NONE) {
            break;
        }

        switch (event->event_id) {
            case MPV_EVENT_FILE_LOADED: {
                Logger::Info("MediaPlayer: Event File Loaded");
                status = PlayerStatus::Playing;
#ifdef __SWITCH__
                appletSetMediaPlaybackState(true);
#endif

                // Log active audio driver
                char* aoName = mpv_get_property_string(mpv, "current-ao");
                if (aoName) {
                    Logger::Info("MediaPlayer: Active audio driver: %s", aoName);
                    mpv_free(aoName);
                }

                // Query duration
                double dur = 0.0;
                if (mpv_get_property(mpv, "duration", MPV_FORMAT_DOUBLE, &dur) >= 0) {
                    duration = dur;
                    metadata.duration = dur;
                }

                // Query video dimensions & codec
                int64_t w = 0, h = 0;
                mpv_get_property(mpv, "width", MPV_FORMAT_INT64, &w);
                mpv_get_property(mpv, "height", MPV_FORMAT_INT64, &h);
                metadata.videoWidth = (int)w;
                metadata.videoHeight = (int)h;

                char* vcodec = mpv_get_property_string(mpv, "video-format");
                if (vcodec) {
                    metadata.videoCodec = vcodec;
                    mpv_free(vcodec);
                }

                char* acodec = mpv_get_property_string(mpv, "audio-codec-name");
                if (acodec) {
                    metadata.audioCodec = acodec;
                    mpv_free(acodec);
                }

                char* hwdec = mpv_get_property_string(mpv, "hwdec-current");
                if (hwdec) {
                    metadata.hwDecoded = (strcmp(hwdec, "no") != 0 && strcmp(hwdec, "") != 0);
                    mpv_free(hwdec);
                }

                ParseTracks();
                break;
            }

            case MPV_EVENT_END_FILE: {
#ifdef __SWITCH__
                appletSetMediaPlaybackState(false);
#endif
                mpv_event_end_file* end_file = (mpv_event_end_file*)event->data;
                if (!end_file) break;

                if (end_file->reason == MPV_END_FILE_REASON_ERROR) {
                    Logger::Error("MediaPlayer: Playback error (%d): %s",
                                  end_file->error, mpv_error_string(end_file->error));
                    status = PlayerStatus::Error;
                } else if (end_file->reason == MPV_END_FILE_REASON_EOF) {
                    Logger::Info("MediaPlayer: Natural End of File reached (duration: %.1f)", duration);
                    status = PlayerStatus::Ended;
                    if (onEndedCallback) {
                        onEndedCallback();
                    }
                } else {
                    Logger::Info("MediaPlayer: End of file event (reason=%d - stop/quit/redirect), ignoring callback", (int)end_file->reason);
                }
                break;
            }

            case MPV_EVENT_PLAYBACK_RESTART: {
                int paused = 0;
                mpv_get_property(mpv, "pause", MPV_FORMAT_FLAG, &paused);
                status = paused ? PlayerStatus::Paused : PlayerStatus::Playing;
#ifdef __SWITCH__
                appletSetMediaPlaybackState(!paused);
#endif
                break;
            }

            case MPV_EVENT_PROPERTY_CHANGE: {
                mpv_event_property* prop = (mpv_event_property*)event->data;
                if (prop && prop->name && strcmp(prop->name, "track-list/count") == 0) {
                    ParseTracks();
                }
                break;
            }

            case MPV_EVENT_LOG_MESSAGE: {
                mpv_event_log_message* msg = (mpv_event_log_message*)event->data;
                if (msg && msg->log_level >= MPV_LOG_LEVEL_WARN) {
                    Logger::Warn("mpv [%s] %s: %s", msg->prefix, msg->level, msg->text);
                }
                break;
            }

            default:
                break;
        }
    }

    // Poll current playback position and duration
    if (status == PlayerStatus::Playing || status == PlayerStatus::Paused) {
        double pos = 0.0;
        if (mpv_get_property(mpv, "time-pos", MPV_FORMAT_DOUBLE, &pos) >= 0) {
            position = pos;
        }

        if (duration <= 0.0) {
            double dur = 0.0;
            if (mpv_get_property(mpv, "duration", MPV_FORMAT_DOUBLE, &dur) >= 0) {
                duration = dur;
                metadata.duration = dur;
            }
        }
    }
}

// ============================================================================
// Resume Cache
// ============================================================================

double MediaPlayer::GetResumePosition(const std::string& path) {
    std::ifstream file(RESUME_FILE);
    if (!file.is_open()) return 0.0;

    std::string line;
    while (std::getline(file, line)) {
        size_t eq = line.find('=');
        if (eq != std::string::npos) {
            std::string p = line.substr(0, eq);
            if (p == path) {
                return strtod(line.substr(eq + 1).c_str(), nullptr);
            }
        }
    }
    return 0.0;
}

void MediaPlayer::SaveResumePosition(const std::string& path, double pos) {
    if (path.empty() || pos < 15.0) return;

    // Read all existing positions
    std::map<std::string, double> entries;
    std::ifstream inFile(RESUME_FILE);
    if (inFile.is_open()) {
        std::string line;
        while (std::getline(inFile, line)) {
            size_t eq = line.find('=');
            if (eq != std::string::npos) {
                std::string p = line.substr(0, eq);
                entries[p] = strtod(line.substr(eq + 1).c_str(), nullptr);
            }
        }
        inFile.close();
    }

    // If near end of video, clear the resume position so it starts from beginning next time
    if (duration > 0.0 && pos >= duration - 20.0) {
        entries.erase(path);
    } else {
        entries[path] = pos;
    }

    // Write back
    std::ofstream outFile(RESUME_FILE);
    if (outFile.is_open()) {
        for (const auto& kv : entries) {
            outFile << kv.first << "=" << kv.second << "\n";
        }
        outFile.close();
    }
}
