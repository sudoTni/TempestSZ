#pragma once
#include <raylib.h>
#include <vector>
#include <cstdint>
#include "Constants.hpp"

enum ParticleType : uint8_t {
    SPARK, RING, TRAIL, SHRAPNEL, PULSAR_ARC
};

struct Particle {
    Vector2 pos;
    Vector2 vel;
    Color   col;
    float   life;
    float   maxLife;
    float   size;
    float   sizeDecay;
    float   spawnDelay;
    ParticleType type;
    bool    active;
};

class Particles {
public:
    static Particles& Instance() {
        static Particles instance;
        return instance;
    }

    void Init();
    void Update(float dt);
    void Draw();

    void EmitExplosion(Vector2 pos, Color col);
    void EmitPlayerDeath(Vector2 pos);
    void EmitImpact(Vector2 pos);
    void EmitWaveClear(const class Well& well);
    void EmitPulsarArc(Vector2 p1, Vector2 p2, Color col);
    void EmitSuperZapperBurst();
    void EmitTrail(Vector2 pos, Vector2 vel, Color col);

    void AddParticle(Vector2 pos, Vector2 vel, Color col, float life, float size, float sizeDecay, ParticleType type, float spawnDelay = 0.0f);

private:
    Particles() = default;
    Particle pool_[MAX_PARTICLES];
    int nextIdx_ = 0;
    float superZapFlash_ = 0.0f;
};
