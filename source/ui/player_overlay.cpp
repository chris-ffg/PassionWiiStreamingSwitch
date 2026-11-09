// ============================================================================
// DangoPlayer Switch — Player Overlay Implementation
// ============================================================================

#include "ui/player_overlay.h"
#include "app.h"
#include "ui/ui_common.h"
#include "utils/string_utils.h"
#include "utils/logger.h"
#include <algorithm>
#include <cstdio>

PlayerOverlay::PlayerOverlay(App* app) : app(app) {
    // Configure seek bar
    int seekY = Layout::SCREEN_H - Layout::FOOTER_H - 36;
    int seekW = Layout::SCREEN_W - Layout::MARGIN * 2;
    seekBar.SetBounds(Layout::MARGIN, seekY, seekW, 10);
    seekBar.SetColors({70, 70, 90, 220}, Colors::DANGO_PINK, Colors::DANGO_CREAM);
}

void PlayerOverlay::Show() {
    visible = true;
    autoHideTimer = AUTO_HIDE_DELAY;
}

void PlayerOverlay::Hide() {
    if (!modalOpen) {
        visible = false;
    }
}

void PlayerOverlay::Toggle() {
    if (visible) {
        Hide();
    } else {
        Show();
    }
}

void PlayerOverlay::ShowOSD(const std::string& message) {
    osdText = message;
    osdTimer = 1.4f;
}

void PlayerOverlay::Update(float dt) {
    toast.Update(dt);

    if (osdTimer > 0.0f) {
        osdTimer -= dt;
    }

    // Auto-hide handling
    if (visible && !modalOpen && MediaPlayer::Instance().IsPlaying()) {
        autoHideTimer -= dt;
        if (autoHideTimer <= 0.0f) {
            visible = false;
        }
    }

    // Alpha fade animation
    float targetAlpha = visible ? 1.0f : 0.0f;
    alpha += (targetAlpha - alpha) * dt * 8.0f;

    // Keep seek bar progress synchronized
    if (!modalOpen) {
        seekBar.SetProgress(MediaPlayer::Instance().GetProgress());
    }
}

void PlayerOverlay::HandleInput(u64 kDown, u64 kHeld) {
    if (!kDown) return; // Do not reset auto-hide on empty frames

    auto& player = MediaPlayer::Instance();

    // If overlay is hidden, any button press wakes it up
    if (!visible) {
        Show();
        if (kDown & HidNpadButton_B) {
            player.Stop();
            app->PopScreen();
        }
        return;
    }

    // Reset auto-hide on any active button press
    autoHideTimer = AUTO_HIDE_DELAY;

    // [B] Exit Player
    if (kDown & HidNpadButton_B) {
        player.Stop();
        app->PopScreen();
        return;
    }

    // [R] Cycle Video Rotation (180° -> 0° -> 90° -> 270°)
    if (kDown & HidNpadButton_R) {
        player.CycleRotation();
        ShowOSD("Rotacion: " + std::to_string(player.GetRotation()) + "°");
        return;
    }

    // [Minus] or [Stick click] can also hide controls immediately
    if (kDown & (HidNpadButton_Minus | HidNpadButton_StickL | HidNpadButton_StickR)) {
        Hide();
        return;
    }

    // =====================================================
    // Track Modal Navigation
    // =====================================================
    if (modalOpen) {
        const auto& meta = player.GetMetadata();
        const auto& trackList = (modalTrackType == TrackType::Audio)
                                ? meta.audioTracks
                                : meta.subtitleTracks;
        int count = (int)trackList.size() + 1; // +1 for "Off" option in subtitles

        if (kDown & HidNpadButton_AnyDown) {
            modalSelectedIndex = (modalSelectedIndex + 1) % count;
        }
        if (kDown & HidNpadButton_AnyUp) {
            modalSelectedIndex = (modalSelectedIndex - 1 + count) % count;
        }

        if (kDown & HidNpadButton_A) {
            if (modalTrackType == TrackType::Audio) {
                if (modalSelectedIndex >= 0 && modalSelectedIndex < (int)meta.audioTracks.size()) {
                    player.SetAudioTrack(meta.audioTracks[modalSelectedIndex].id);
                    ShowOSD("Audio: " + meta.audioTracks[modalSelectedIndex].language);
                }
            } else {
                if (modalSelectedIndex == 0) {
                    player.SetSubtitleTrack(0); // Subtitle off
                    ShowOSD("Subtitulos: Desactivados");
                } else {
                    int subIdx = modalSelectedIndex - 1;
                    if (subIdx >= 0 && subIdx < (int)meta.subtitleTracks.size()) {
                        player.SetSubtitleTrack(meta.subtitleTracks[subIdx].id);
                        ShowOSD("Subtitulos: " + meta.subtitleTracks[subIdx].language);
                    }
                }
            }
            modalOpen = false;
        }

        if (kDown & HidNpadButton_B) {
            modalOpen = false;
        }
        return;
    }

    // =====================================================
    // Player Controls
    // =====================================================

    // Play / Pause [A]
    if (kDown & HidNpadButton_A) {
        player.TogglePause();
        ShowOSD(player.IsPaused() ? "Pausa" : "Reproduciendo");
    }

    // Seek ±10s [Left / Right D-Pad]
    if (kDown & HidNpadButton_AnyLeft) {
        player.Seek(-10.0, true);
        ShowOSD("<< -10s");
    }
    if (kDown & HidNpadButton_AnyRight) {
        player.Seek(+10.0, true);
        ShowOSD("+10s >>");
    }

    // Seek ±60s [ZL / ZR]
    if (kDown & HidNpadButton_ZL) {
        player.Seek(-60.0, true);
        ShowOSD("<< -1 min");
    }
    if (kDown & HidNpadButton_ZR) {
        player.Seek(+60.0, true);
        ShowOSD("+1 min >>");
    }

    // Volume [Up / Down D-Pad]
    if (kDown & HidNpadButton_AnyUp) {
        player.SetVolume(player.GetVolume() + 5);
        ShowOSD("Volumen: " + std::to_string(player.GetVolume()) + "%");
    }
    if (kDown & HidNpadButton_AnyDown) {
        player.SetVolume(player.GetVolume() - 5);
        ShowOSD("Volumen: " + std::to_string(player.GetVolume()) + "%");
    }

    // Audio Track Modal [X]
    if (kDown & HidNpadButton_X) {
        modalTrackType = TrackType::Audio;
        modalSelectedIndex = 0;
        modalOpen = true;
    }

    // Subtitle Track Modal [Y]
    if (kDown & HidNpadButton_Y) {
        modalTrackType = TrackType::Subtitle;
        modalSelectedIndex = 0;
        modalOpen = true;
    }

    // Exit Player [B]
    if (kDown & HidNpadButton_B) {
        player.Stop();
        app->PopScreen();
    }
}

bool PlayerOverlay::HandleTouch(const TouchEvent& touch) {
    if (touch.type != TouchEvent::Down) {
        if (seekBar.HandleTouch(touch.x, touch.y)) {
            if (touch.type == TouchEvent::Up) {
                MediaPlayer::Instance().SeekPercent(seekBar.GetProgress());
            }
            return true;
        }
        return false;
    }

    // Touch down event
    if (modalOpen) {
        modalOpen = false;
        return true;
    }

    if (!visible) {
        // Tapping screen while hidden reveals controls
        Show();
        return true;
    }

    // Interactive Zone: Back button [B] (Top-left)
    if (touch.x >= Layout::MARGIN && touch.x <= Layout::MARGIN + 120 &&
        touch.y >= 10 && touch.y <= 75) {
        MediaPlayer::Instance().Stop();
        app->PopScreen();
        return true;
    }

    // Interactive Zone: Top-right Rotation button
    int badgeX = Layout::SCREEN_W - Layout::MARGIN - 170;
    int rotBtnW = 150;
    int rotBtnX = badgeX - rotBtnW - 14;
    if (touch.x >= rotBtnX && touch.x <= rotBtnX + rotBtnW &&
        touch.y >= 15 && touch.y <= 65) {
        MediaPlayer::Instance().CycleRotation();
        ShowOSD("Rotacion: " + std::to_string(MediaPlayer::Instance().GetRotation()) + "°");
        autoHideTimer = AUTO_HIDE_DELAY;
        return true;
    }

    // Interactive Zone: Seek bar
    int seekY = Layout::SCREEN_H - Layout::FOOTER_H - 36;
    if (touch.y >= seekY - 18 && touch.y <= seekY + 28) {
        if (seekBar.HandleTouch(touch.x, touch.y)) {
            MediaPlayer::Instance().SeekPercent(seekBar.GetProgress());
            autoHideTimer = AUTO_HIDE_DELAY;
            return true;
        }
    }

    // Interactive Zone: Bottom action bar buttons
    if (touch.y >= Layout::SCREEN_H - 55 && touch.y <= Layout::SCREEN_H - 5) {
        // Play / Pause
        if (touch.x >= Layout::MARGIN && touch.x <= Layout::MARGIN + 140) {
            MediaPlayer::Instance().TogglePause();
            ShowOSD(MediaPlayer::Instance().IsPaused() ? "Pausa" : "Reproduciendo");
            autoHideTimer = AUTO_HIDE_DELAY;
            return true;
        }
        // Audio modal
        if (touch.x >= Layout::MARGIN + 150 && touch.x <= Layout::MARGIN + 255) {
            modalTrackType = TrackType::Audio;
            modalSelectedIndex = 0;
            modalOpen = true;
            return true;
        }
        // Subtitles modal
        if (touch.x >= Layout::MARGIN + 260 && touch.x <= Layout::MARGIN + 380) {
            modalTrackType = TrackType::Subtitle;
            modalSelectedIndex = 0;
            modalOpen = true;
            return true;
        }
        // Rotation button
        if (touch.x >= Layout::MARGIN + 385 && touch.x <= Layout::MARGIN + 540) {
            MediaPlayer::Instance().CycleRotation();
            ShowOSD("Rotacion: " + std::to_string(MediaPlayer::Instance().GetRotation()) + "°");
            autoHideTimer = AUTO_HIDE_DELAY;
            return true;
        }
    }

    // Tapping anywhere else on the screen hides the overlay immediately!
    Hide();
    return true;
}

void PlayerOverlay::Render(SDL_Renderer* renderer) {
    if (alpha <= 0.01f && !modalOpen) {
        if (osdTimer > 0.0f) {
            RenderOSD(renderer);
        }
        return;
    }

    RenderTopBar(renderer);
    RenderBottomBar(renderer);

    if (osdTimer > 0.0f) {
        RenderOSD(renderer);
    }

    if (modalOpen) {
        RenderTrackModal(renderer);
    }

    toast.Render(renderer, app->GetFontSmall());
}

void PlayerOverlay::RenderTopBar(SDL_Renderer* renderer) {
    int topH = 80;
    Uint8 a = (Uint8)(alpha * 220.0f);

    // Gradient / darkened top overlay
    UI::DrawRect(renderer, 0, 0, Layout::SCREEN_W, topH, {18, 18, 28, a});

    // Back Button [B]
    int btnY = 18;
    UI::DrawRoundedRect(renderer, Layout::MARGIN, btnY, 78, 44, 8, {42, 42, 62, a});
    UI::DrawRoundedRect(renderer, Layout::MARGIN + 8, btnY + 10, 24, 24, 6, Colors::DANGO_PINK);
    UI::DrawText(renderer, app->GetFontTiny(), "B",
                 Layout::MARGIN + 20, btnY + 13, Colors::TEXT_PRIMARY, true, false);
    UI::DrawText(renderer, app->GetFontSmall(), "Atras",
                 Layout::MARGIN + 38, btnY + 12, Colors::TEXT_SECONDARY, false, false);

    // Title
    const auto& meta = MediaPlayer::Instance().GetMetadata();
    std::string titleTrunc = StringUtils::Truncate(meta.title, 42);
    UI::DrawText(renderer, app->GetFont(), titleTrunc,
                 Layout::MARGIN + 100, btnY + 8, Colors::DANGO_CREAM, false, false);

    // Badges layout (right side)
    int badgeX = Layout::SCREEN_W - Layout::MARGIN - 170;

    // Rotation Button [R] (placed next to codec badge)
    int rotBtnW = 140;
    int rotBtnX = badgeX - rotBtnW - 12;
    UI::DrawRoundedRect(renderer, rotBtnX, btnY + 4, rotBtnW, 36, 6, {40, 40, 60, a});
    UI::DrawRoundedRect(renderer, rotBtnX + 8, btnY + 10, 24, 24, 4, Colors::DANGO_BLUE);
    UI::DrawText(renderer, app->GetFontTiny(), "R",
                 rotBtnX + 20, btnY + 14, Colors::TEXT_PRIMARY, true, false);
    std::string rotStr = "Rotar: " + std::to_string(MediaPlayer::Instance().GetRotation()) + "°";
    UI::DrawText(renderer, app->GetFontSmall(), rotStr,
                 rotBtnX + 38, btnY + 11, Colors::TEXT_PRIMARY, false, false);

    // Codec & Hardware Decoding Badge (right side)
    std::string badgeStr = meta.hwDecoded ? "NVDEC HW" : "CPU SW";
    SDL_Color badgeColor = meta.hwDecoded ? Colors::DANGO_GREEN : Colors::DANGO_ORANGE;

    UI::DrawRoundedRect(renderer, badgeX, btnY + 4, 170, 36, 6, {40, 40, 60, a});
    UI::DrawRoundedRect(renderer, badgeX + 8, btnY + 10, 24, 24, 4, badgeColor);
    UI::DrawText(renderer, app->GetFontTiny(), meta.hwDecoded ? "HW" : "SW",
                 badgeX + 20, btnY + 14, Colors::BG_DARK, true, false);

    std::string details = (!meta.videoCodec.empty()) ? meta.videoCodec : "Video";
    UI::DrawText(renderer, app->GetFontSmall(), details,
                 badgeX + 42, btnY + 11, Colors::TEXT_PRIMARY, false, false);
}

void PlayerOverlay::RenderBottomBar(SDL_Renderer* renderer) {
    int bottomH = 110;
    int bottomY = Layout::SCREEN_H - bottomH;
    Uint8 a = (Uint8)(alpha * 220.0f);

    // Darkened bottom overlay
    UI::DrawRect(renderer, 0, bottomY, Layout::SCREEN_W, bottomH, {18, 18, 28, a});

    // Time texts (Position & Duration)
    double pos = MediaPlayer::Instance().GetPosition();
    double dur = MediaPlayer::Instance().GetDuration();

    std::string timeStr = StringUtils::FormatTime(pos) + " / " + StringUtils::FormatTime(dur);
    UI::DrawText(renderer, app->GetFontSmall(), timeStr,
                 Layout::MARGIN, bottomY + 12, Colors::TEXT_SECONDARY, false, false);

    // Render interactive seek bar
    seekBar.Render(renderer);

    // Action button hints below seek bar
    int hintY = bottomY + 68;

    // (A) Play / Pause
    const char* playPauseText = MediaPlayer::Instance().IsPaused() ? "Reproducir" : "Pausar";
    UI::DrawRoundedRect(renderer, Layout::MARGIN, hintY, 24, 24, 6, Colors::DANGO_PINK);
    UI::DrawText(renderer, app->GetFontTiny(), "A", Layout::MARGIN + 12, hintY + 5, Colors::TEXT_PRIMARY, true, false);
    UI::DrawText(renderer, app->GetFontSmall(), playPauseText, Layout::MARGIN + 32, hintY + 2, Colors::TEXT_SECONDARY, false, false);

    // (X) Audio Track
    int xX = Layout::MARGIN + 160;
    UI::DrawRoundedRect(renderer, xX, hintY, 24, 24, 6, Colors::DANGO_GREEN);
    UI::DrawText(renderer, app->GetFontTiny(), "X", xX + 12, hintY + 5, Colors::TEXT_PRIMARY, true, false);
    UI::DrawText(renderer, app->GetFontSmall(), "Audio", xX + 32, hintY + 2, Colors::TEXT_SECONDARY, false, false);

    // (Y) Subtitles
    int yX = Layout::MARGIN + 270;
    UI::DrawRoundedRect(renderer, yX, hintY, 24, 24, 6, Colors::DANGO_CREAM);
    UI::DrawText(renderer, app->GetFontTiny(), "Y", yX + 12, hintY + 5, Colors::BG_DARK, true, false);
    UI::DrawText(renderer, app->GetFontSmall(), "Subtitulos", yX + 32, hintY + 2, Colors::TEXT_SECONDARY, false, false);

    // (R) Rotation
    int rX = Layout::MARGIN + 395;
    UI::DrawRoundedRect(renderer, rX, hintY, 24, 24, 6, Colors::DANGO_BLUE);
    UI::DrawText(renderer, app->GetFontTiny(), "R", rX + 12, hintY + 5, Colors::TEXT_PRIMARY, true, false);
    std::string rotLabel = "Rotar (" + std::to_string(MediaPlayer::Instance().GetRotation()) + "°)";
    UI::DrawText(renderer, app->GetFontSmall(), rotLabel, rX + 32, hintY + 2, Colors::TEXT_SECONDARY, false, false);

    // Volume level on right
    int vol = MediaPlayer::Instance().GetVolume();
    std::string volStr = "Vol: " + std::to_string(vol) + "%";
    UI::DrawText(renderer, app->GetFontSmall(), volStr,
                 Layout::SCREEN_W - Layout::MARGIN - 100, hintY + 2, Colors::TEXT_DISABLED, false, false);
}

void PlayerOverlay::RenderOSD(SDL_Renderer* renderer) {
    if (osdText.empty()) return;

    int boxW = 240;
    int boxH = 56;
    int boxX = (Layout::SCREEN_W - boxW) / 2;
    int boxY = Layout::SCREEN_H / 2 - 28;

    UI::DrawRoundedRect(renderer, boxX, boxY, boxW, boxH, 12, {20, 20, 32, 230});
    UI::DrawRoundedRectOutline(renderer, boxX, boxY, boxW, boxH, 12, Colors::DANGO_PINK);

    UI::DrawTextCentered(renderer, app->GetFont(), osdText,
                         {boxX, boxY, boxW, boxH}, Colors::TEXT_PRIMARY);
}

void PlayerOverlay::RenderTrackModal(SDL_Renderer* renderer) {
    // Dim background
    UI::DrawRect(renderer, 0, 0, Layout::SCREEN_W, Layout::SCREEN_H, {0, 0, 0, 180});

    int modalW = 500;
    int modalH = 340;
    int modalX = (Layout::SCREEN_W - modalW) / 2;
    int modalY = (Layout::SCREEN_H - modalH) / 2;

    UI::DrawRoundedRect(renderer, modalX, modalY, modalW, modalH, 16, Colors::BG_CARD);
    UI::DrawRoundedRectOutline(renderer, modalX, modalY, modalW, modalH, 16, Colors::DANGO_PINK);

    // Modal Title
    const char* title = (modalTrackType == TrackType::Audio) ? "Pistas de Audio" : "Subtitulos";
    UI::DrawText(renderer, app->GetFont(), title, modalX + 30, modalY + 24, Colors::DANGO_CREAM, false, false);
    UI::DrawDivider(renderer, modalX + 30, modalY + 64, modalW - 60);

    // Track Items
    const auto& meta = MediaPlayer::Instance().GetMetadata();
    int startY = modalY + 80;
    int itemH = 44;

    if (modalTrackType == TrackType::Audio) {
        for (int i = 0; i < (int)meta.audioTracks.size(); i++) {
            int y = startY + i * (itemH + 6);
            bool isSel = (i == modalSelectedIndex);

            if (isSel) {
                UI::DrawRoundedRect(renderer, modalX + 30, y, modalW - 60, itemH, 8, Colors::BG_SELECTED);
                UI::DrawRoundedRect(renderer, modalX + 30, y, 4, itemH, 2, Colors::DANGO_PINK);
            }

            std::string label = meta.audioTracks[i].language + "  •  " + meta.audioTracks[i].title;
            if (meta.audioTracks[i].selected) label += " (Activa)";
            UI::DrawText(renderer, app->GetFontSmall(), label, modalX + 48, y + 12,
                         isSel ? Colors::TEXT_PRIMARY : Colors::TEXT_SECONDARY, false, false);
        }
    } else {
        // Subtitles (Index 0 = Desactivado, followed by tracks)
        int totalRows = (int)meta.subtitleTracks.size() + 1;
        for (int i = 0; i < totalRows; i++) {
            int y = startY + i * (itemH + 6);
            bool isSel = (i == modalSelectedIndex);

            if (isSel) {
                UI::DrawRoundedRect(renderer, modalX + 30, y, modalW - 60, itemH, 8, Colors::BG_SELECTED);
                UI::DrawRoundedRect(renderer, modalX + 30, y, 4, itemH, 2, Colors::DANGO_PINK);
            }

            std::string label = (i == 0) ? "Desactivados" : (meta.subtitleTracks[i - 1].language + "  •  " + meta.subtitleTracks[i - 1].title);
            UI::DrawText(renderer, app->GetFontSmall(), label, modalX + 48, y + 12,
                         isSel ? Colors::TEXT_PRIMARY : Colors::TEXT_SECONDARY, false, false);
        }
    }

    // Modal Footer hint
    UI::DrawText(renderer, app->GetFontTiny(), "(A) Seleccionar    (B) Cancelar",
                 modalX + modalW / 2, modalY + modalH - 32, Colors::TEXT_DISABLED, true, false);
}
