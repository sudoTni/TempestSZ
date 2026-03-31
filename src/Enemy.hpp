#pragma once
#include <raylib.h>
#include <vector>
#include <cstdint>
#include "Constants.hpp"

enum class EnemyType : uint8_t {
    FLIPPER, TANKER, SPIKER, FUSEBALL, PULSAR
};

struct Enemy {
    EnemyType type;
    float lane;
    float depth;
    bool alive;
    bool onRim;
    float flipTimer;
    float moveDir;
    float auxTimer;
    float spikeLen;
};

class EnemyManager {
public:
    static EnemyManager& Instance() {
        static EnemyManager instance;
        return instance;
    }

    void Init();
    void Update(float dt);
    void SpawnWave(int wave);
    std::vector<Enemy>& GetEnemies() { return enemies_; }
    const std::vector<Enemy>& GetEnemies() const { return enemies_; }

private:
    EnemyManager() = default;
    std::vector<Enemy> enemies_;
};
