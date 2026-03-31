#pragma once
#include <raylib.h>
#include <vector>
#include "Constants.hpp"

struct Bullet {
    float lane;
    float depth;
    bool alive;
    bool fromPlayer;
#ifdef SZ_ENHANCED_VFX
    // Ghost positions for micro-trail: prevDepth[0]=t-1, [1]=t-2, [2]=t-3
    float prevDepth[3] = {-1.0f, -1.0f, -1.0f};
#endif
};

class BulletManager {
public:
    static BulletManager& Instance() {
        static BulletManager instance;
        return instance;
    }

    void Init();
    void Update(float dt);
    void SpawnPlayerBullet(float lane);
    std::vector<Bullet>& GetBullets() { return bullets_; }
    const std::vector<Bullet>& GetBullets() const { return bullets_; }

private:
    BulletManager() = default;
    std::vector<Bullet> bullets_;
};
