#include "SFX.hpp"
#include <cmath>
#include <algorithm>

void SFX::Init() {
    stream_ = LoadAudioStream(SFX_SAMPLE_RATE, 32, 1); // Mono
    SetAudioStreamCallback(stream_, AudioCallback);
    PlayAudioStream(stream_);
}

void SFX::Shutdown() {
    UnloadAudioStream(stream_);
}

float SFX::SynthSample(SFXVoice& v) {
    float s = 0.0f;
    switch (v.waveform) {
        case 0: // SINE
            s = sinf(v.phase);
            break;
        case 1: // SAW
            s = (v.phase / PI) - 1.0f;
            break;
        case 2: // SQUARE
            s = (v.phase < PI) ? 1.0f : -1.0f;
            break;
        case 3: // NOISE
            s = (float)GetRandomValue(-100, 100) / 100.0f;
            break;
        case 4: // FM
            s = sinf(v.phase + v.fmDepth * sinf(v.fmPhase));
            break;
    }
    // Soft clip distortion
    if (v.distortion > 1.0f) {
        s *= v.distortion;
        if (s > 1.0f) s = 1.0f;
        if (s < -1.0f) s = -1.0f;
    }
    return s;
}

void SFX::AudioCallback(void* buffer, unsigned int frames) {
    float* out = (float*)buffer;
    auto& voices = Instance().voices_;
    
    for (unsigned int i = 0; i < frames; ++i) {
        float sample = 0;
        for (int j = 0; j < SFX_VOICES; ++j) {
            auto& v = voices[j];
            if (!v.active) continue;
            
            float s = SynthSample(v);
            v.filterState += v.filterLP * (s - v.filterState);
            sample += v.filterState * v.amp;
            
            v.amp -= v.ampDecay * INV_SAMPLE_RATE;
            v.freq += (v.freqTarget - v.freq) * v.freqDecay * INV_SAMPLE_RATE;
            v.phase += v.freq * 2.0f * PI * INV_SAMPLE_RATE;
            while (v.phase > 2.0f * PI) v.phase -= 2.0f * PI;
            
            v.fmPhase += v.fmFreq * 2.0f * PI * INV_SAMPLE_RATE;
            while (v.fmPhase > 2.0f * PI) v.fmPhase -= 2.0f * PI;
            
            v.duration -= INV_SAMPLE_RATE;
            if (v.duration <= 0 || v.amp <= 0) v.active = false;
        }
        out[i] = std::clamp(sample, -1.0f, 1.0f);
    }
}

void SFX::Play(SFXType type) {
    auto playVoice = [&](int wf, float f, float ft, float fd, float a, float ad, float dur, float flp = 1.0f, float dist = 1.0f, float fmd = 0, float fmf = 0) {
        for (int i = 0; i < SFX_VOICES; ++i) {
            if (!voices_[i].active) {
                auto& v = voices_[i];
                v.active = true;
                v.waveform = wf;
                v.freq = f;
                v.freqTarget = ft;
                v.freqDecay = fd;
                v.amp = a;
                v.ampDecay = ad;
                v.duration = dur;
                v.phase = 0;
                v.fmPhase = 0;
                v.filterLP = flp;
                v.filterState = 0;
                v.distortion = dist;
                v.fmDepth = fmd;
                v.fmFreq = fmf;
                return &v;
            }
        }
        return (SFXVoice*)nullptr;
    };

    switch (type) {
        case SFX_PLAYER_FIRE:
            playVoice(2, 900, 180, 15.0f, 0.4f, 2.0f, 0.15f); // SQUARE
            playVoice(3, 900, 180, 15.0f, 0.2f, 3.0f, 0.1f, 0.5f); // NOISE blend
            break;
        case SFX_ENEMY_DESTROYED:
            playVoice(1, 220, 80, 10.0f, 0.5f, 1.5f, 0.4f, 1.0f, 2.0f); // SAW crunchy
            playVoice(4, 220, 80, 10.0f, 0.3f, 1.5f, 0.4f, 1.0f, 1.0f, 3.0f, 440.0f); // FM
            break;
        case SFX_PLAYER_DEATH:
            playVoice(3, 60, 40, 2.0f, 0.6f, 0.5f, 1.5f, 0.1f); // NOISE LP (Low rumble)
            playVoice(0, 60, 20, 2.0f, 0.8f, 0.8f, 1.0f); // SINE thud
            break;
        case SFX_FLIPPER_FLIP:
            playVoice(2, 440, 440, 0, 0.3f, 10.0f, 0.05f);
            break;
        case SFX_PULSAR_CHARGE:
            playVoice(1, 80, 400, 10.0f, 0.4f, 0.5f, 0.5f);
            break;
        case SFX_PULSAR_FIRE:
            playVoice(2, 200, 200, 0, 0.5f, 5.0f, 0.2f, 1.0f, 2.0f);
            break;
        case SFX_FUSEBALL_BOUNCE:
            playVoice(4, 320, 400, 5.0f, 0.4f, 4.0f, 0.15f, 1.0f, 1.0f, 2.0f, 640.0f);
            break;
        case SFX_WAVE_CLEAR:
            // Just play a ascending sine arp (simulated)
            playVoice(0, 220, 440, 5.0f, 0.4f, 0.5f, 0.4f);
            playVoice(0, 330, 660, 5.0f, 0.3f, 0.5f, 0.4f);
            break;
        case SFX_SUPERZAPPER:
            playVoice(3, 1000, 100, 5.0f, 0.7f, 0.8f, 1.0f, 0.8f); // NOISE
            playVoice(2, 100, 50, 2.0f, 0.5f, 1.0f, 1.0f); // SQUARE
            break;
    }
}
