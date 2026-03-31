#include "Bullet.hpp"

void BulletManager::Init() {
    bullets_.clear();
}

void BulletManager::Update(float dt) {
    for (auto& b : bullets_) {
        if (!b.alive) continue;
#ifdef SZ_ENHANCED_VFX
        // Shift ghost trail history before moving
        b.prevDepth[2] = b.prevDepth[1];
        b.prevDepth[1] = b.prevDepth[0];
        b.prevDepth[0] = b.depth;
#endif
        if (b.fromPlayer) {
            b.depth += BULLET_SPEED * dt;
            if (b.depth >= 1.0f) b.alive = false;
        } else {
            b.depth -= BULLET_SPEED * dt;
            if (b.depth <= 0) b.alive = false;
        }
    }
}

void BulletManager::SpawnPlayerBullet(float lane) {
    bullets_.push_back({lane, 0.0f, true, true});
}
