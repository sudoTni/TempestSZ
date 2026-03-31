#include "Enemy.hpp"
#include "Well.hpp"
#include "GameState.hpp"
#include "Particles.hpp"
#include "Player.hpp"
#include "SFX.hpp"
#include <cmath>

void EnemyManager::Init() {
    enemies_.clear();
}

void EnemyManager::Update(float dt) {
    for (auto& e : enemies_) {
        if (!e.alive) continue;

        if (!e.onRim) {
            float speed = FLIPPER_APPROACH_SPEED;
            switch (e.type) {
                case EnemyType::TANKER:   speed = TANKER_APPROACH_SPEED; break;
                case EnemyType::SPIKER:   speed = SPIKER_APPROACH_SPEED; break;
                case EnemyType::FUSEBALL: speed = FUSEBALL_SPEED; break;
                case EnemyType::PULSAR:   speed = FLIPPER_APPROACH_SPEED * 0.9f; break;
                case EnemyType::FLIPPER:  speed = FLIPPER_APPROACH_SPEED; break;
            }
            e.depth -= speed * dt;
            if (e.depth <= 0) {
                e.depth = 0;
                e.onRim = true;
            }
        } else {
            e.flipTimer += dt;
            e.auxTimer += dt;

            if (e.type == EnemyType::FLIPPER || e.type == EnemyType::TANKER) {
                const float moveCadence = (e.type == EnemyType::TANKER) ? 0.36f : 0.25f;
                if (e.flipTimer >= moveCadence) {
                    e.flipTimer = 0.0f;
                    e.lane += e.moveDir;
                    if (Well::Instance().IsClosed()) {
                        if (e.lane < 0.0f) e.lane = NUM_LANES - 1.0f;
                        if (e.lane >= (float)NUM_LANES) e.lane = 0.0f;
                    } else {
                        if (e.lane <= 0.0f || e.lane >= (float)NUM_LANES - 1.0f) {
                            e.moveDir *= -1.0f;
                        }
                        e.lane = std::min(std::max(e.lane, 0.0f), (float)NUM_LANES - 1.0f);
                    }
                    SFX::Instance().Play(SFX_FLIPPER_FLIP);
                }
            } else if (e.type == EnemyType::FUSEBALL) {
                e.lane += e.moveDir * dt * 8.0f;
                if (Well::Instance().IsClosed()) {
                    if (e.lane < 0.0f) e.lane += (float)NUM_LANES;
                    if (e.lane >= (float)NUM_LANES) e.lane -= (float)NUM_LANES;
                } else {
                    if (e.lane <= 0.0f || e.lane >= (float)NUM_LANES - 1.0f) {
                        e.moveDir *= -1.0f;
                        SFX::Instance().Play(SFX_FUSEBALL_BOUNCE);
                    }
                    e.lane = std::min(std::max(e.lane, 0.0f), (float)NUM_LANES - 1.0f);
                }

                Vector2 pos = Well::Instance().Project({e.lane, 0.0f});
                Vector2 vel = {e.moveDir * 80.0f, 0.0f};
                Particles::Instance().EmitTrail(pos, vel, COL_FUSEBALL);
#ifdef SZ_ENHANCED_VFX
                const Color pulse = ((int)(GetTime() * 8.0) % 2 == 0) ? COL_FUSEBALL : WHITE;
                Particles::Instance().EmitTrail(pos, {e.moveDir * 110.0f, 0.0f}, pulse);
#endif
            } else if (e.type == EnemyType::SPIKER) {
                e.spikeLen = std::min(1.0f, e.spikeLen + dt * SPIKER_SPIKE_RATE);
#ifdef SZ_ENHANCED_VFX
                // Dim lane-biased green drift around active spikes.
                const float laneAngle = Well::Instance().GetRimAngle(e.lane);
                for (int n = 0; n < 2; ++n) {
                    const float a = laneAngle + ((float)GetRandomValue(-15, 15) * DEG2RAD);
                    const float speed = (float)GetRandomValue(20, 60);
                    Vector2 v = {cosf(a) * speed, sinf(a) * speed};
                    Vector2 p = Well::Instance().Project({e.lane, std::min(0.9f, e.spikeLen)});
                    Particles::Instance().EmitTrail(p, v, Color{0x22, 0xAA, 0x22, 0xFF});
                }
#endif
            } else if (e.type == EnemyType::PULSAR) {
                if (e.auxTimer >= 0.9f) {
                    e.auxTimer = 0.0f;
                    SFX::Instance().Play(SFX_PULSAR_FIRE);
                    Vector2 p1 = Well::Instance().Project({e.lane, 0.0f});
                    Vector2 p2 = Well::Instance().Project({Player::Instance().GetLane(), 0.25f});
                    Particles::Instance().EmitPulsarArc(p1, p2, COL_PULSAR);
                }
            }
        }
    }
}

void EnemyManager::SpawnWave(int wave) {
    enemies_.clear();
    int count = 8 + wave;
    count = std::min(count, 22);

    for (int i = 0; i < count; ++i) {
        EnemyType type = EnemyType::FLIPPER;
        const int roll = GetRandomValue(0, 99);

        if (wave >= 5 && roll > 93) type = EnemyType::PULSAR;
        else if (wave >= 4 && roll > 85) type = EnemyType::FUSEBALL;
        else if (wave >= 3 && roll > 75) type = EnemyType::SPIKER;
        else if (wave >= 2 && roll > 60) type = EnemyType::TANKER;

        const float lane = (float)GetRandomValue(0, NUM_LANES - 1);
        const float depth = (float)GetRandomValue(55, 100) / 100.0f;
        const float dir = (GetRandomValue(0, 1) == 0) ? -1.0f : 1.0f;

        enemies_.push_back({
            type,
            lane,
            depth,
            true,
            false,
            0.0f,
            dir,
            0.0f,
            0.0f
        });
    }
}
