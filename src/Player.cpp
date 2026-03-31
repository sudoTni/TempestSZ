#include "Player.hpp"
#include "Well.hpp"
#include "Bullet.hpp"
#include "SFX.hpp"
#include "Input.hpp"
#include "Enemy.hpp"
#include "Particles.hpp"

void Player::Init() {
    lane_ = 0;
    alive_ = true;
    shootCooldown_ = 0;
    ResetSuperZapper();
}

void Player::Update(float dt) {
    if (!alive_) return;

    // Shared lane movement path for both real and IRP virtual input.
    static float laneStepCooldown = 0.0f;
    laneStepCooldown -= dt;
    const float laneDelta = Input::GetLaneDelta();
    if (laneDelta != 0.0f && laneStepCooldown <= 0.0f) {
        lane_ += (laneDelta > 0.0f) ? 1.0f : -1.0f;
        if (lane_ < 0.0f) lane_ = Well::Instance().IsClosed() ? NUM_LANES - 1.0f : 0.0f;
        if (lane_ >= (float)NUM_LANES) lane_ = Well::Instance().IsClosed() ? 0.0f : NUM_LANES - 1.0f;
        laneStepCooldown = 0.1f;
    }

    if (shootCooldown_ > 0) shootCooldown_ -= dt;
    if (Input::IsFireDown() && shootCooldown_ <= 0) {
        BulletManager::Instance().SpawnPlayerBullet(lane_);
        shootCooldown_ = PLAYER_SHOOT_COOLDOWN;
        SFX::Instance().Play(SFX_PLAYER_FIRE); // Fire
    }

    if (Input::IsSuperZapPressed() && HasSuperZapper()) {
        UseSuperZapper();
        SFX::Instance().Play(SFX_SUPERZAPPER);
        Particles::Instance().EmitSuperZapperBurst();

        auto& enemies = EnemyManager::Instance().GetEnemies();
        for (auto& e : enemies) {
            if (!e.alive) continue;
            e.alive = false;
            Particles::Instance().EmitExplosion(Well::Instance().Project({e.lane, e.depth}), COL_PULSAR);
        }
    }
}
