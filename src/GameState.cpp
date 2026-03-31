#include "GameState.hpp"
#include "Well.hpp"
#include "Enemy.hpp"
#include "Player.hpp"
#include "Bullet.hpp"
#include "Particles.hpp"
#include "SFX.hpp"
#include "Input.hpp"
#include <algorithm>
#include <cmath>

void GameState::Init() {
    currentState_ = QState::NEWGAM;
    wave_ = 1;
    score_ = 0;
    newHiScore_ = false;
    lives_ = 3;
    waveZoom_ = 0.0f;
    stateTimer_ = 0.0f;
    Well::Instance().Init(wave_);
    EnemyManager::Instance().Init();
    BulletManager::Instance().Init();
}

void GameState::Update(float dt) {
    stateTimer_ += dt;

    switch (currentState_) {
        case QState::NEWGAM:
            if (Input::IsStartPressed() || Input::IsCoinPressed()) {
                SetState(QState::NEWLIF);
            }
            break;

        case QState::NEWLIF:
            if (stateTimer_ >= 2.0f) {
                SetState(QState::PLAY);
            }
            break;

        case QState::PLAY:
            Player::Instance().Update(dt);
            EnemyManager::Instance().Update(dt);
            BulletManager::Instance().Update(dt);
            
            {
                auto& ev = EnemyManager::Instance().GetEnemies();
                bool allDead = std::all_of(ev.begin(), ev.end(), [](const Enemy& e){ return !e.alive; });
                if (allDead && !ev.empty()) {
                    SetState(QState::ENDWAV);
                }
            }
            break;

        case QState::ENDWAV:
            waveZoom_ += dt * 0.5f;
            if (std::fmod(stateTimer_, 0.1f) < dt) {
                Particles::Instance().EmitWaveClear(Well::Instance());
            }
            if (stateTimer_ >= 3.0f) {
                NextWave();
                SetState(QState::NEWLIF);
            }
            break;

        case QState::ENDLIF:
            if (stateTimer_ >= 3.0f) {
                if (lives_ > 0) SetState(QState::NEWLIF);
                else SetState(QState::ENDGAM);
            }
            break;

        case QState::ENDGAM:
            if (stateTimer_ >= 2.0f) {
                SetState(QState::HISCHK);
            }
            break;

        case QState::HISCHK:
            // Check if current score qualifies for high-score table.
            // After a short display pause, proceed to initials entry or attract.
            if (stateTimer_ >= 1.5f) {
                if (score_ > hiScore_) {
                    SetState(QState::GETINI);
                } else {
                    SetState(QState::NEWGAM);
                }
            }
            break;

        case QState::GETINI:
            if (stateTimer_ >= 2.0f) {
                // Commit hi-score on initials confirmation
                if (score_ > hiScore_) {
                    hiScore_ = score_;
                }
                SetState(QState::NEWGAM);
            }
            break;

        default:
            break;
    }
}

void GameState::SetState(QState newState) {
    currentState_ = newState;
    stateTimer_ = 0.0f;

    if (newState == QState::NEWLIF) {
        Player::Instance().Init();
        EnemyManager::Instance().SpawnWave(wave_);
    }
    if (newState == QState::ENDWAV) {
        waveZoom_ = 0.0f;
        SFX::Instance().Play(SFX_WAVE_CLEAR);
    }
}

void GameState::NextWave() {
    wave_++;
    Well::Instance().Init(wave_);
}

void GameState::AddScore(int pts) {
    const int prevScore = score_;
    score_ += pts;
    // Flag the first time score crosses the current hi-score this session
    if (!newHiScore_ && score_ > hiScore_ && prevScore <= hiScore_) {
        newHiScore_ = true;
    }
}
