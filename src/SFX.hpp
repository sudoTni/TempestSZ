#pragma once
#include <raylib.h>
#include "Constants.hpp"

enum SFXType {
    SFX_PLAYER_FIRE,
    SFX_ENEMY_DESTROYED,
    SFX_PLAYER_DEATH,
    SFX_FLIPPER_FLIP,
    SFX_PULSAR_CHARGE,
    SFX_PULSAR_FIRE,
    SFX_FUSEBALL_BOUNCE,
    SFX_WAVE_CLEAR,
    SFX_SUPERZAPPER
};

struct SFXVoice {
    bool   active = false;
    float  phase = 0;
    float  freq = 0;
    float  freqTarget = 0;
    float  freqDecay = 0;
    float  amp = 0;
    float  ampDecay = 0;
    float  duration = 0;
    int    waveform = 0; // 0:SINE, 1:SAW, 2:SQUARE, 3:NOISE, 4:FM
    float  fmDepth = 0;
    float  fmFreq = 0;
    float  fmPhase = 0;
    float  filterLP = 1.0f;
    float  filterState = 0;
    float  distortion = 1.0f;
};

class SFX {
public:
    static SFX& Instance() {
        static SFX instance;
        return instance;
    }

    void Init();
    void Shutdown();
    void Play(SFXType type);

private:
    SFX() = default;
    static void AudioCallback(void* buffer, unsigned int frames);
    static float SynthSample(SFXVoice& v);

    AudioStream stream_;
    SFXVoice voices_[SFX_VOICES];
};
