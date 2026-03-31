#pragma once
#include <cstdint>
#include <functional>
#include <map>
#include <vector>

enum class QState : uint8_t {
    NEWGAM  = 0,   // attract / coin insert
    NEWLIF  = 1,   // spawn player on new life
    PLAY    = 2,   // main gameplay
    ENDLIF  = 3,   // end of life sequence
    ENDGAM  = 4,   // game over
    PAUSE   = 5,   // pause
    ENDWAV  = 7,   // wave clear zoom
    HISCHK  = 8,   // high score check
    GETINI  = 9,   // initials entry
};

class GameState {
public:
    static GameState& Instance() {
        static GameState instance;
        return instance;
    }

    void Init();
    void Update(float dt);
    
    void SetState(QState newState);
    QState GetState() const { return currentState_; }
    
    int GetWave() const { return wave_; }
    void NextWave();
    
    int GetScore() const { return score_; }
    void AddScore(int pts);
    
    int GetHighScore() const { return hiScore_; }
    bool IsNewHighScore() const { return newHiScore_; }

    int GetLives() const { return lives_; }
    void DecLives() { if (lives_ > 0) lives_--; }

    float GetWaveZoom() const { return waveZoom_; }
    float GetStateTimer() const { return stateTimer_; }

private:
    GameState() = default;
    
    QState currentState_ = QState::NEWGAM;
    int wave_ = 1;
    int score_ = 0;
    int hiScore_ = 0;
    bool newHiScore_ = false;
    int lives_ = 3;
    float waveZoom_ = 0.0f;
    float stateTimer_ = 0.0f;
};
