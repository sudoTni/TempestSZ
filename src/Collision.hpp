#pragma once
#include "Bullet.hpp"
#include "Enemy.hpp"
#include "Player.hpp"

class Collision {
public:
    static void Update();
private:
    static bool CheckHit(const Bullet& b, const Enemy& e);
};
