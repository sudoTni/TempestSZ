#pragma once
#include <raylib.h>
#include <libopenmpt/libopenmpt.hpp>
#include <vector>
#include <string>
#include <memory>
#include <atomic>
#include "Constants.hpp"

class Jukebox {
public:
    static Jukebox& Instance() {
        static Jukebox instance;
        return instance;
    }

    void Init();
    void Update(float dt);
    void DrawWidget();
    void SetIRPWidgetProfile(bool enabled);
    void Shutdown();

private:
    Jukebox() = default;
    
    void FillAudio_(float* out, int frames);
    void UpdateVU_(const float* buf, int frames);
    void LoadNextTrack_();

    AudioStream stream_;
    std::unique_ptr<openmpt::module> modCurrent_;
    std::unique_ptr<openmpt::module> modNext_;
    
    std::vector<std::string> playlist_;
    std::vector<int> playOrder_;
    int playIdx_ = 0;
    
    bool isCrossfading_ = false;
    float crossfadeTimer_ = 0.0f;
    std::atomic<bool> trackEnded_{ false };
    
    float vuLevels_[8] = {0};
    float vuPeak_[8] = {0};
    float barPos_[8] = {0};
    float barVel_[8] = {0};
    float scanLinePhase_ = 0.0f;
    float rainbowHue_     = 0.0f;
    bool irpWidgetProfile_ = false;
    std::string currentTitle_;
};
