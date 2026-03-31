#include "Collision.hpp"
#include <cmath>
#include "GameState.hpp"
#include "Well.hpp"
#include "SFX.hpp"
#include "Particles.hpp"

void Collision::Update() {
    auto& bullets = BulletManager::Instance().GetBullets();
    auto& enemies = EnemyManager::Instance().GetEnemies();
    
    for (auto& b : bullets) {
        if (!b.alive) continue;
        for (auto& e : enemies) {
            if (!e.alive) continue;
            if (CheckHit(b, e)) {
                b.alive = false;
                e.alive = false;
                GameState::Instance().AddScore(150);
                SFX::Instance().Play(SFX_ENEMY_DESTROYED);
                Color enemyCol = COL_FLIPPER;
                switch (e.type) {
                    case EnemyType::FLIPPER: enemyCol = COL_FLIPPER; break;
                    case EnemyType::TANKER: enemyCol = COL_TANKER; break;
                    case EnemyType::SPIKER: enemyCol = COL_SPIKER; break;
                    case EnemyType::FUSEBALL: enemyCol = COL_FUSEBALL; break;
                    case EnemyType::PULSAR: enemyCol = COL_PULSAR; break;
                }
                Particles::Instance().EmitExplosion(Well::Instance().Project({e.lane, e.depth}), enemyCol);
                Particles::Instance().EmitImpact(Well::Instance().Project({e.lane, e.depth}));
            }
        }
    }

    if (GameState::Instance().GetState() != QState::PLAY || !Player::Instance().IsAlive()) {
        return;
    }

    const float playerLane = Player::Instance().GetLane();
    for (auto& e : enemies) {
        if (!e.alive || !e.onRim) continue;
        const float laneDist = std::fabs(e.lane - playerLane);
        if (laneDist <= 0.45f) {
            e.alive = false;
            Player::Instance().SetAlive(false);
            GameState::Instance().DecLives();
            GameState::Instance().SetState(QState::ENDLIF);
            SFX::Instance().Play(SFX_PLAYER_DEATH);
            Particles::Instance().EmitPlayerDeath(Well::Instance().Project({playerLane, 0.02f}));
            break;
        }
    }
}

bool Collision::CheckHit(const Bullet& b, const Enemy& e) {
    if (std::abs(b.lane - e.lane) > 0.5f) return false;
    if (std::abs(b.depth - e.depth) > BULLET_HIT_RADIUS) return false;
    return true;
}
