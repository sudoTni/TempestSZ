#pragma once
#include <raylib.h>
#include "Constants.hpp"

class Player {
public:
    static Player& Instance() {
        static Player instance;
        return instance;
    }

    void Init();
    void Update(float dt);
    
    float GetLane() const { return lane_; }
    void SetLane(float l) { lane_ = l; }
    
    bool IsAlive() const { return alive_; }
    void SetAlive(bool a) { alive_ = a; }

    int GetLives() const { return lives_; }
    void SetLives(int l) { lives_ = l; }
    void LoseLife() { lives_--; }

    float GetDeathTimer() const { return deathTimer_; }
    void SetDeathTimer(float t) { deathTimer_ = t; }

    bool HasSuperZapper() const { return superZapper_; }
    void UseSuperZapper() { superZapper_ = false; superZapperUsed_ = true; }
    void ResetSuperZapper() { superZapper_ = true; superZapperUsed_ = false; }

private:
    Player() = default;
    
    float lane_ = 0;
    bool alive_ = true;
    int lives_ = 3;
    float shootCooldown_ = 0;
    float deathTimer_ = 0;
    bool superZapper_ = true;
    bool superZapperUsed_ = false;
};
