#include "Jukebox.hpp"
#include "embedded_assets.hpp"
#include <algorithm>
#include <cmath>
#include <exception>
#include <iostream>
#include <numeric>
#include <random>
#include <cstring>

void Jukebox::Init() {
    // Populate playlist from embedded BGM assets
    for (int i = 0; i < g_bgm_mod_count; ++i)
        playlist_.push_back(g_bgm_mods[i].name);

    if (playlist_.empty()) return;

    // Random shuffle on each launch; IRP overrides bar visuals independently.
    playOrder_.resize(playlist_.size());
    std::iota(playOrder_.begin(), playOrder_.end(), 0);
    std::mt19937 rng(std::random_device{}());
    std::shuffle(playOrder_.begin(), playOrder_.end(), rng);

    stream_ = LoadAudioStream(BGM_SAMPLE_RATE, 32, BGM_CHANNELS);
    SetAudioStreamCallback(stream_, [](void* buffer, unsigned int frames) {
        Jukebox::Instance().FillAudio_((float*)buffer, (int)frames);
    });
    
    LoadNextTrack_(); // Load into modCurrent
    modCurrent_ = std::move(modNext_);
    if (modCurrent_) {
        currentTitle_ = modCurrent_->get_metadata("title");
        if (currentTitle_.empty()) currentTitle_ = playlist_[playOrder_[(playIdx_ - 1 + playOrder_.size()) % playOrder_.size()]];
        // Strip path separators to get bare filename
        auto sep = currentTitle_.find_last_of("/\\");
        if (sep != std::string::npos) currentTitle_ = currentTitle_.substr(sep + 1);
        // Strip .mod/.xm extension
        auto dot = currentTitle_.rfind('.');
        if (dot != std::string::npos) currentTitle_ = currentTitle_.substr(0, dot);
    }
    LoadNextTrack_(); // Preload into modNext
    
    PlayAudioStream(stream_);

    for (int i = 0; i < 8; ++i) {
        vuLevels_[i] = 0.0f;
        vuPeak_[i] = 0.0f;
        barPos_[i] = 0.0f;
        barVel_[i] = 0.0f;
    }
    scanLinePhase_ = 0.0f;
}

void Jukebox::Update(float dt) {
    // Track-end detection (flag set from audio thread)
    if (!isCrossfading_ && trackEnded_.load(std::memory_order_relaxed)) {
        trackEnded_.store(false, std::memory_order_relaxed);
        isCrossfading_ = true;
        crossfadeTimer_ = CROSSFADE_SECS; // advance immediately on next Update
    }

    if (isCrossfading_) {
        crossfadeTimer_ += dt;
        if (crossfadeTimer_ >= CROSSFADE_SECS) {
            isCrossfading_ = false;
            modCurrent_ = std::move(modNext_);
            crossfadeTimer_ = 0;
            if (modCurrent_) {
                currentTitle_ = modCurrent_->get_metadata("title");
                if (currentTitle_.empty()) currentTitle_ = playlist_[playOrder_[(playIdx_ - 1 + playOrder_.size()) % playOrder_.size()]];
                auto sep = currentTitle_.find_last_of("/\\");
                if (sep != std::string::npos) currentTitle_ = currentTitle_.substr(sep + 1);
                auto dot = currentTitle_.rfind('.');
                if (dot != std::string::npos) currentTitle_ = currentTitle_.substr(0, dot);
            }
            LoadNextTrack_();
        }
    }

    scanLinePhase_ += dt * 0.5f;
    if (scanLinePhase_ > 1.0f) scanLinePhase_ -= 1.0f;

    rainbowHue_ += dt * 72.0f;   // full HSV cycle every 5 s
    if (rainbowHue_ >= 360.0f) rainbowHue_ -= 360.0f;

    for (int i = 0; i < 8; ++i) {
        float target = std::clamp(vuLevels_[i], 0.0f, 1.2f);
        if (irpWidgetProfile_) {
            // Keep all bars lit with deterministic phase offsets during IRP shot capture.
            const float phase = scanLinePhase_ * 2.0f * PI * 2.0f + (float)i * 0.85f;
            target = 0.24f + 0.72f * (0.5f + 0.5f * sinf(phase));
        }
#ifdef SZ_ENHANCED_VFX
        const float k = 25.0f;
        const float damping = 10.5f;
        barVel_[i] += (target - barPos_[i]) * k * dt - barVel_[i] * damping * dt;
        barPos_[i] += barVel_[i] * dt;
        // Soft overshoot cap at +8% on high peaks.
        const float peakCap = target * 1.08f;
        if (barPos_[i] > peakCap) barPos_[i] = peakCap;
#else
        barPos_[i] = target;
#endif

        if (barPos_[i] > vuPeak_[i]) {
            vuPeak_[i] = barPos_[i];
        } else {
            vuPeak_[i] = std::max(0.0f, vuPeak_[i] - dt * 0.8f);
        }
    }
}

void Jukebox::SetIRPWidgetProfile(bool enabled) {
    irpWidgetProfile_ = enabled;
}

void Jukebox::LoadNextTrack_() {
    if (playlist_.empty()) return;
    std::string name = playlist_[playOrder_[playIdx_]];
    playIdx_ = (playIdx_ + 1) % playOrder_.size();

    // Locate the embedded mod by filename
    const EmbeddedMod* em = nullptr;
    for (int i = 0; i < g_bgm_mod_count; ++i) {
        if (name == g_bgm_mods[i].name) { em = &g_bgm_mods[i]; break; }
    }
    if (em) {
        try {
            modNext_ = std::make_unique<openmpt::module>(em->data, em->size);
        } catch (const std::exception& ex) {
            std::clog << "[Jukebox] Failed to load module " << name << ": " << ex.what() << "\n";
            modNext_.reset();
        }
    }
}

void Jukebox::FillAudio_(float* out, int frames) {
    if (!modCurrent_) {
        std::memset(out, 0, frames * 2 * sizeof(float));
        return;
    }
    
    try {
        size_t rendered = modCurrent_->read_interleaved_stereo(BGM_SAMPLE_RATE, frames, out);
        if ((int)rendered < frames) {
            // Module reached its end — signal main thread to crossfade
            trackEnded_.store(true, std::memory_order_relaxed);
            // Zero-fill any silence at the tail
            std::memset(out + rendered * 2, 0, (frames - (int)rendered) * 2 * sizeof(float));
        }
    } catch (const std::exception& ex) {
        std::clog << "[Jukebox] Audio callback decode error: " << ex.what() << "\n";
        std::memset(out, 0, frames * 2 * sizeof(float));
    }
    UpdateVU_(out, frames);
}

void Jukebox::UpdateVU_(const float* buf, int frames) {
    int slice = frames / 8;
    for (int b = 0; b < 8; ++b) {
        float rms = 0;
        for (int i = 0; i < slice; ++i) rms += buf[(b*slice + i)*2] * buf[(b*slice + i)*2];
        vuLevels_[b] = sqrtf(rms / slice) * 7.0f;
    }
}

void Jukebox::DrawWidget() {
    const int originX = 10;
    const int originY = SCREEN_H - 115;  // bar bottoms land at SCREEN_H - 115 + 90 = 695
    const int width = 8 * 22 + 10;
    const int barCeil = 90;  // max bar height in pixels — compact, no dead zone

    // Header area above the EQ bars
    const int headerH = 30;
    const int boxTop   = originY - 8 - headerH;
    const int boxH     = headerH + barCeil + 24;
    DrawRectangle(originX - 6, boxTop, width, boxH, ColorAlpha(BLACK, 0.45f));
    DrawRectangleLines(originX - 6, boxTop, width, boxH, ColorAlpha(WHITE, 0.45f));

    // Horizontal divider between header text and EQ bars
    DrawRectangle(originX - 6, originY - 8, width, 1, ColorAlpha(WHITE, 0.30f));

#ifdef SZ_ENHANCED_VFX
    // "MUSIC BY MAKTONE" — per-character HSV rainbow (demo-scene style)
    {
        const char* label = "MUSIC BY MAKTONE";
        const int fs = 10;
        const float hueStep = 360.0f / 16.0f;
        int cx = originX;
        for (int ci = 0; label[ci]; ++ci) {
            float hue = fmodf(rainbowHue_ + ci * hueStep, 360.0f);
            Color cc = ColorFromHSV(hue, 1.0f, 1.0f);
            char ch[2] = { label[ci], '\0' };
            DrawText(ch, cx, originY - 6 - headerH, fs, cc);
            cx += MeasureText(ch, fs);
        }
    }
#else
    DrawText("MUSIC BY MAKTONE", originX, originY - 6 - headerH, 10, WHITE);
#endif
    // Current track name — truncate to fit widget width
    {
        const std::string nowPlaying = currentTitle_.empty() ? "---" : currentTitle_;
        std::string display = "NOW PLAYING: " + nowPlaying;
        const int fs = 8;
        while (display.size() > 4 && MeasureText(display.c_str(), fs) > width - 8)
            display = display.substr(0, display.size() - 1);
        DrawText(display.c_str(), originX, originY - 6 - headerH + 12, fs, ColorAlpha(WHITE, 0.80f));
    }

    const Color palette[8] = {
        {0xFF, 0x44, 0x44, 0xFF},
        {0xFF, 0x88, 0x00, 0xFF},
        {0xFF, 0xCC, 0x00, 0xFF},
        {0x66, 0xFF, 0x55, 0xFF},
        {0x00, 0xEE, 0xAA, 0xFF},
        {0x00, 0xAA, 0xFF, 0xFF},
        {0x66, 0x66, 0xFF, 0xFF},
        {0xDD, 0x55, 0xFF, 0xFF},
    };

    for (int i = 0; i < 8; ++i) {
        const int x = originX + i * 22;
        const int h = (int)std::clamp(barPos_[i] * 78.0f, 0.0f, (float)barCeil);
        const int y = originY + barCeil - h;
        DrawRectangle(x, y, 16, h, palette[i]);

        const int peakY = originY + barCeil - (int)std::clamp(vuPeak_[i] * 78.0f, 0.0f, (float)barCeil);
        DrawRectangle(x, peakY, 16, 2, WHITE);
    }

#ifdef SZ_ENHANCED_VFX
    // Thin CRT-style scan line sweeping top-to-bottom.
    const int scanY = originY + (int)(scanLinePhase_ * (float)barCeil);
    DrawRectangle(originX, scanY, 8 * 22 - 6, 1, ColorAlpha(WHITE, 0.12f));
#endif
}

void Jukebox::Shutdown() {
    if (IsAudioStreamValid(stream_)) {
        UnloadAudioStream(stream_);
    }
}
