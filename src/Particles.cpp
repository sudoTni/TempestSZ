#include "Particles.hpp"
#include "Well.hpp"
#include <raymath.h>
#include <algorithm>
#include <cmath>

void Particles::Init() {
    for (int i = 0; i < MAX_PARTICLES; ++i) pool_[i].active = false;
}

void Particles::Update(float dt) {
#ifdef SZ_ENHANCED_VFX
    superZapFlash_ = std::max(0.0f, superZapFlash_ - dt / 0.3f);
#endif

    for (int i = 0; i < MAX_PARTICLES; ++i) {
        if (!pool_[i].active) continue;

        if (pool_[i].spawnDelay > 0.0f) {
            pool_[i].spawnDelay -= dt;
            if (pool_[i].spawnDelay > 0.0f) {
                continue;
            }
        }
        
        pool_[i].pos.x += pool_[i].vel.x * dt;
        pool_[i].pos.y += pool_[i].vel.y * dt;
        pool_[i].life -= dt;
        pool_[i].size -= pool_[i].sizeDecay * dt;
        
        if (pool_[i].life <= 0 || pool_[i].size <= 0) pool_[i].active = false;
    }
}

void Particles::Draw() {
    for (int i = 0; i < MAX_PARTICLES; ++i) {
        if (!pool_[i].active) continue;
        if (pool_[i].spawnDelay > 0.0f) continue;
        
        const float alpha = pool_[i].life / pool_[i].maxLife;
        const float progress = 1.0f - alpha;
        Color c = ColorAlpha(pool_[i].col, alpha);
        
        switch (pool_[i].type) {
            case RING: {
                float r = pool_[i].size * (1.0f - alpha) * 100.0f;
                DrawCircleLinesV(pool_[i].pos, r, c);
                break;
            }
            case TRAIL: {
#ifdef SZ_ENHANCED_VFX
                if (pool_[i].col.r == WHITE.r && pool_[i].col.g == WHITE.g && pool_[i].col.b == WHITE.b) {
                    // White -> cyan -> blue for wave-clear depth streaks.
                    if (progress < 0.5f) {
                        const float t = progress * 2.0f;
                        c = {
                            (unsigned char)(255.0f + (0.0f - 255.0f) * t),
                            (unsigned char)(255.0f + (255.0f - 255.0f) * t),
                            (unsigned char)(255.0f + (255.0f - 255.0f) * t),
                            (unsigned char)(alpha * 255.0f)
                        };
                    } else {
                        const float t = (progress - 0.5f) * 2.0f;
                        c = {
                            0,
                            (unsigned char)(255.0f + (20.0f - 255.0f) * t),
                            255,
                            (unsigned char)(alpha * 255.0f)
                        };
                    }
                }
                Vector2 end = Vector2Subtract(pool_[i].pos, Vector2Scale(pool_[i].vel, 0.06f));
                const float trailW = std::max(1.0f, pool_[i].size);
                DrawLineEx(pool_[i].pos, end, trailW, c);
#else
                Vector2 end = Vector2Subtract(pool_[i].pos, Vector2Scale(pool_[i].vel, 0.05f));
                DrawLineEx(pool_[i].pos, end, 1.5f, c);
#endif
                break;
            }
            case SHRAPNEL: {
                Vector2 end = Vector2Add(pool_[i].pos, Vector2Scale(pool_[i].vel, 0.08f));
                DrawLineEx(pool_[i].pos, end, 2.0f, c);
                break;
            }
            case PULSAR_ARC: {
                Color arcColor = c;
#ifdef SZ_ENHANCED_VFX
                // RED -> WHITE -> CYAN -> WHITE -> RED cycle over arc lifetime.
                if (progress < 0.25f) {
                    const float t = progress / 0.25f;
                    arcColor = {
                        255,
                        (unsigned char)(34.0f + (255.0f - 34.0f) * t),
                        (unsigned char)(34.0f + (255.0f - 34.0f) * t),
                        (unsigned char)(alpha * 255.0f)
                    };
                } else if (progress < 0.5f) {
                    const float t = (progress - 0.25f) / 0.25f;
                    arcColor = {
                        (unsigned char)(255.0f + (0.0f - 255.0f) * t),
                        255,
                        255,
                        (unsigned char)(alpha * 255.0f)
                    };
                } else if (progress < 0.75f) {
                    const float t = (progress - 0.5f) / 0.25f;
                    arcColor = {
                        (unsigned char)(0.0f + (255.0f - 0.0f) * t),
                        255,
                        255,
                        (unsigned char)(alpha * 255.0f)
                    };
                } else {
                    const float t = (progress - 0.75f) / 0.25f;
                    arcColor = {
                        255,
                        (unsigned char)(255.0f + (34.0f - 255.0f) * t),
                        (unsigned char)(255.0f + (34.0f - 255.0f) * t),
                        (unsigned char)(alpha * 255.0f)
                    };
                }
#endif
                Vector2 end = Vector2Add(pool_[i].pos, Vector2Scale(pool_[i].vel, 0.05f));
                Vector2 mid = Vector2Lerp(pool_[i].pos, end, 0.5f);
#ifdef SZ_ENHANCED_VFX
                mid.x += pool_[i].vel.x;
                mid.y += pool_[i].vel.y;
#else
                mid.x += (float)GetRandomValue(-5, 5);
                mid.y += (float)GetRandomValue(-5, 5);
#endif
                DrawLineV(pool_[i].pos, mid, arcColor);
                DrawLineV(mid, end, arcColor);
                break;
            }
            default: // SPARK
                DrawCircleV(pool_[i].pos, pool_[i].size, c);
                break;
        }
    }

#ifdef SZ_ENHANCED_VFX
    if (superZapFlash_ > 0.0f) {
        DrawRectangle(0, 0, SCREEN_W, SCREEN_H, ColorAlpha(WHITE, superZapFlash_));
    }
#endif
}

void Particles::EmitSuperZapperBurst() {
    Vector2 center = { SCREEN_W * 0.5f, SCREEN_H * 0.5f };
#ifdef SZ_ENHANCED_VFX
    for (int i = 0; i < 256; ++i) {
#else
    for (int i = 0; i < 128; ++i) {
#endif
        float angle = GetRandomValue(0, 360) * DEG2RAD;
        float speed = (float)GetRandomValue(160, 850);
        Vector2 vel = { cosf(angle) * speed, sinf(angle) * speed };
        AddParticle(center, vel, WHITE, 1.0f, 4.0f, 4.0f, SPARK);
    }
#ifdef SZ_ENHANCED_VFX
    superZapFlash_ = 0.30f;  // Reduced from 0.55 — 256 sparks provide enough bloom
#endif
}

void Particles::EmitTrail(Vector2 pos, Vector2 vel, Color col) {
    AddParticle(pos, vel, col, 0.5f, 2.0f, 2.0f, TRAIL);
}

void Particles::AddParticle(Vector2 pos, Vector2 vel, Color col, float life, float size, float sizeDecay, ParticleType type, float spawnDelay) {
    int idx = nextIdx_;
    pool_[idx].active = true;
    pool_[idx].pos = pos;
    pool_[idx].vel = vel;
    pool_[idx].col = col;
    pool_[idx].life = life;
    pool_[idx].maxLife = life;
    pool_[idx].size = size;
    pool_[idx].sizeDecay = sizeDecay;
    pool_[idx].spawnDelay = spawnDelay;
    pool_[idx].type = type;
    nextIdx_ = (nextIdx_ + 1) % MAX_PARTICLES;
}

void Particles::EmitExplosion(Vector2 pos, Color col) {
    int sparkCount = GetRandomValue(24, 60);
#ifdef SZ_ENHANCED_VFX
    sparkCount = GetRandomValue(80, 120);
#endif
    for (int i = 0; i < sparkCount; ++i) {
        float angle = (float)GetRandomValue(0, 360) * DEG2RAD;
        float speed = (float)GetRandomValue(50, 300);
        Vector2 vel = { cosf(angle) * speed, sinf(angle) * speed };
        AddParticle(pos, vel, col, (float)GetRandomValue(5, 15) * 0.1f, 2.0f, 1.0f, SPARK);
    }
#ifdef SZ_ENHANCED_VFX
    // 4 radial shrapnel lines, colour matches enemy, fade 0.4 s
    for (int i = 0; i < 4; ++i) {
        float angle = (float)GetRandomValue(0, 360) * DEG2RAD;
        float speed = (float)GetRandomValue(100, 400);
        Vector2 vel = { cosf(angle) * speed, sinf(angle) * speed };
        AddParticle(pos, vel, col, 0.4f, 2.0f, 0.5f, SHRAPNEL);
    }
    // Primary ring at t=0
    AddParticle(pos, {0, 0}, col, 0.7f, 1.8f, 0.0f, RING);
    // Secondary ring wave at t=0.08 s
    AddParticle(pos, {0, 0}, ColorAlpha(col, 0.8f), 1.0f, 2.4f, 0.0f, RING, 0.08f);
#endif
}

void Particles::EmitPlayerDeath(Vector2 pos) {
    for (int i = 0; i < 80; ++i) {
        float angle = (float)GetRandomValue(0, 360) * DEG2RAD;
        float speed = (float)GetRandomValue(100, 500);
        Vector2 vel = { cosf(angle) * speed, sinf(angle) * speed };
        Color col = (i % 2 == 0) ? YELLOW : WHITE;
        AddParticle(pos, vel, col, 2.0f, 3.0f, 1.0f, SPARK);
    }
    for (int i = 0; i < 16; ++i) {
        float angle = (float)GetRandomValue(0, 360) * DEG2RAD;
        float speed = (float)GetRandomValue(200, 600);
        Vector2 vel = { cosf(angle) * speed, sinf(angle) * speed };
        AddParticle(pos, vel, RED, 1.5f, 2.0f, 0.5f, SHRAPNEL);
    }
    for (int i = 0; i < 4; ++i) {
        AddParticle(pos, {0,0}, WHITE, 1.0f + i * 0.2f, 1.0f + i, 0, RING);
    }
#ifdef SZ_ENHANCED_VFX
    // Delayed second expansion layer starts around 0.15s after initial burst.
    AddParticle(pos, {0, 0}, WHITE, 1.2f, 2.2f, 0.0f, RING, 0.15f);
    AddParticle(pos, {0, 0}, SKYBLUE, 1.05f, 2.7f, 0.0f, RING, 0.15f);
    for (int i = 0; i < 8; ++i) {
        const float angle = ((float)i / 8.0f) * 2.0f * PI;
        Vector2 dir = { cosf(angle) * 30.0f, sinf(angle) * 30.0f };
        AddParticle(pos, dir, COL_PULSAR, 1.2f, 2.0f, 0.5f, PULSAR_ARC, 0.15f);
    }
#endif
}

void Particles::EmitImpact(Vector2 pos) {
    for (int i = 0; i < 6; ++i) {
        float angle = (float)GetRandomValue(0, 360) * DEG2RAD;
        float speed = (float)GetRandomValue(50, 150);
        Vector2 vel = { cosf(angle) * speed, sinf(angle) * speed };
        AddParticle(pos, vel, WHITE, 0.3f, 1.5f, 2.0f, SPARK);
    }
}

void Particles::EmitWaveClear(const Well& well) {
#ifdef SZ_ENHANCED_VFX
    for (int i = 0; i < 64; ++i) {
#else
    for (int i = 0; i < 32; ++i) {
#endif
        float lane = (float)GetRandomValue(0, NUM_LANES - 1);
        float depth = (float)GetRandomValue(0, 100) / 100.0f;
        Vector2 p1 = well.Project({lane, depth});
        Vector2 p2 = well.Project({lane, depth + 0.05f});
        Vector2 vel = Vector2Scale(Vector2Subtract(p2, p1), 60.0f); // Rush down
        AddParticle(p1, vel, WHITE, 0.5f, 2.0f, 1.0f, TRAIL);
    }
}

void Particles::EmitPulsarArc(Vector2 p1, Vector2 p2, Color col) {
    int arcCount = 6;
#ifdef SZ_ENHANCED_VFX
    arcCount = 10;
#endif
    for (int i = 0; i < arcCount; ++i) {
        Vector2 mid = Vector2Lerp(p1, p2, (float)i / (float)arcCount);
        Vector2 vel = { (float)GetRandomValue(-45, 45), (float)GetRandomValue(-45, 45) };
        AddParticle(mid, vel, col, 0.2f, 2.0f, 5.0f, PULSAR_ARC);
    }
}
