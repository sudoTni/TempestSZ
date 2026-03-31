#include "IRP.hpp"

#include "Bullet.hpp"
#include "Enemy.hpp"
#include "Input.hpp"
#include "Jukebox.hpp"
#include "Particles.hpp"
#include "Player.hpp"
#include "Well.hpp"
#include "Constants.hpp"

#include <raylib.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>

bool IRP::enabled_ = false;
bool IRP::running_ = false;
int IRP::scriptStep_ = 0;
float IRP::stepTimer_ = 0.0f;
std::string IRP::outputDir_;
std::vector<IRPShot> IRP::shots_;
std::vector<std::string> IRP::shotStateAtCapture_;

float IRP::virtualLane_ = 0.0f;
bool IRP::virtualFire_ = false;
bool IRP::virtualSuperZap_ = false;
bool IRP::virtualCoin_ = false;
bool IRP::virtualStart_ = false;

int IRP::pendingShot_ = -1;
bool IRP::stepEntered_ = false;
std::string IRP::runTimestampIso_;
bool IRP::showHiscoreOverlay_ = false;
bool IRP::showLevelSelectOverlay_ = false;
bool IRP::showCoinFlashOverlay_ = false;
bool IRP::showSpikeOverlay_ = false;
float IRP::superZapFlashAlpha_ = 0.0f;
bool IRP::manifestWritten_ = false;
bool IRP::geometryOnlyMode_ = false;
bool IRP::hudOnlyMode_ = false;

namespace {
std::string BuildIsoTimestamp() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%S");
    return oss.str();
}

std::string BuildFolderTimestamp() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y%m%d_%H%M%S");
    return oss.str();
}
} // namespace

void IRP::Init(bool enabled) {
    enabled_ = enabled;
    running_ = enabled;
    scriptStep_ = 0;
    stepTimer_ = 0.0f;
    pendingShot_ = -1;
    stepEntered_ = false;
    showHiscoreOverlay_ = false;
    showLevelSelectOverlay_ = false;
    showCoinFlashOverlay_ = false;
    showSpikeOverlay_ = false;
    superZapFlashAlpha_ = 0.0f;
    manifestWritten_ = false;
    geometryOnlyMode_ = false;
    hudOnlyMode_ = false;

    shots_.clear();
    shots_ = {
        {1,  "attract_title",               "Attract mode title screen, stable", false},
        {2,  "attract_hiscore",             "High score table in attract", false},
        {3,  "attract_demo_play",           "Attract demo gameplay in NEWGAM", false},
        {4,  "coin_insert_flash",           "Coin inserted and credit flash visible", false},
        {5,  "level_select",                "Level select screen with well preview", false},
        {6,  "wave_start_zoom",             "NEWLIF wave-start zoom materialisation", false},
        {7,  "gameplay_wave01",             "Wave 1 gameplay with active enemies", false},
        {8,  "gameplay_wave01_bloom",       "Wave 1 gameplay emphasizing bloom", false},
        {9,  "gameplay_particles_explosion", "Peak enemy explosion particles", false},
        {10, "gameplay_pulsar_arc",         "Pulsar firing arc frame", false},
        {11, "gameplay_fuseball_trail",     "Fuseball trail frame", false},
        {12, "gameplay_spiker_spike",       "Spiker spike beyond 50 percent lane", false},
        {13, "gameplay_superzapper",        "SuperZapper peak burst frame", false},
        {14, "player_death_explosion",      "Peak player death burst", false},
        {15, "endlife_sequence",            "ENDLIF sequence mid-frame", false},
        {16, "wave_clear_zoom",             "ENDWAV zoom and trail effects", false},
        {17, "gameplay_wave02",             "Wave 2 square well gameplay", false},
        {18, "gameplay_wave05",             "Wave 5 4KEY well gameplay", false},
        {19, "gameplay_wave09",             "Wave 9 staircase well gameplay", false},
        {20, "game_over_screen",            "ENDGAM game over display", false},
        {21, "hiscore_entry",               "GETINI initials entry mid-state", false},
        {22, "hud_fullscreen",              "Gameplay HUD with all key elements", false},
        {23, "jukebox_widget_active",       "Jukebox widget with active EQ bars", false},
        {24, "well_closed_circle",          "Closed circle well geometry only", false},
        {25, "well_open_plane",             "Open plane well geometry with visible gap", false},
        // Parity shots 26-30: directly comparable against original source tables
        {26, "parity_well_circle_wireframe",  "Wave 1 CIRCLE — geometry only for vertex parity check", false},
        {27, "parity_well_square_wireframe",  "Wave 2 SQUARE — geometry only for vertex parity check", false},
        {28, "parity_well_open_v_wireframe",  "Wave 11 PLANE — open well, geometry only, open edges visible", false},
        {29, "parity_enemy_flipper_closeup",  "Single Flipper stationary at rim — all colour channels visible", false},
        {30, "parity_hud_score_display",      "HUD fully visible: score, hi-score, lives, wave — no well/enemies", false},
    };

    if (!enabled_) {
        Input::SetVirtualActive(false);
        SetWindowTitle("SZ - Tempest");
        return;
    }

    Input::SetVirtualActive(true);
    SetRandomSeed(1337U);
    shotStateAtCapture_.assign(shots_.size(), "UNKNOWN");

    runTimestampIso_ = BuildIsoTimestamp();
    outputDir_ = MakeOutputDir_();
    TraceLog(LOG_INFO, "[IRP] Output directory: %s", outputDir_.c_str());
}

void IRP::ResetVirtualInputs_() {
    virtualLane_ = 0.0f;
    virtualFire_ = false;
    virtualSuperZap_ = false;
    virtualCoin_ = false;
    virtualStart_ = false;

    Input::SetVirtualLane(0.0f);
    Input::SetVirtualFire(false);
    Input::SetVirtualSuperZap(false);
    Input::SetVirtualCoin(false);
    Input::SetVirtualStart(false);
}

void IRP::PrepareWave_(int waveNum, bool spawnEnemies) {
    GameState::Instance().Init();
    for (int w = 1; w < waveNum; ++w) {
        GameState::Instance().NextWave();
    }
    GameState::Instance().SetState(QState::NEWLIF);
    Player::Instance().SetLane(0.0f);
    Player::Instance().SetAlive(true);
    if (!spawnEnemies) {
        EnemyManager::Instance().GetEnemies().clear();
    }
}

void IRP::AdvanceScript_() {
    scriptStep_++;
    stepTimer_ = 0.0f;
    stepEntered_ = false;
    showHiscoreOverlay_ = false;
    showLevelSelectOverlay_ = false;
    showCoinFlashOverlay_ = false;
    showSpikeOverlay_ = false;
    geometryOnlyMode_ = false;
    hudOnlyMode_ = false;
}

void IRP::UpdatePre(float dt) {
    if (!enabled_) {
        return;
    }

    if (!running_) {
        SetWindowTitle("SZ - Tempest [IRP COMPLETE - shots in irp_shots/]");
        return;
    }

    const int totalSteps = (int)shots_.size();
    SetWindowTitle(TextFormat("SZ - Tempest [IRP RUNNING - step %d/%d]", scriptStep_ + 1, totalSteps));

    stepTimer_ += dt;
    superZapFlashAlpha_ = std::max(0.0f, superZapFlashAlpha_ - dt / 0.3f);

    ResetVirtualInputs_();
    Jukebox::Instance().SetIRPWidgetProfile(false);

    auto queueShot = [&](int idx) {
        if (pendingShot_ < 0) {
            pendingShot_ = idx;
        }
    };

    auto keepEnemiesAwayFromRim = [&]() {
        auto& enemies = EnemyManager::Instance().GetEnemies();
        for (size_t i = 0; i < enemies.size(); ++i) {
            Enemy& e = enemies[i];
            if (!e.alive) {
                continue;
            }
            e.onRim = false;
            const float targetDepth = 0.72f + 0.02f * (float)(i % 4);
            if (e.depth < targetDepth) {
                e.depth = targetDepth;
            }
        }
    };

    switch (scriptStep_) {
        case 0: {
            if (!stepEntered_) {
                stepEntered_ = true;
                GameState::Instance().Init();
                GameState::Instance().SetState(QState::NEWGAM);
            }
            if (stepTimer_ >= 1.5f) {
                queueShot(0);
                AdvanceScript_();
            }
            break;
        }

        case 1: {
            if (!stepEntered_) {
                stepEntered_ = true;
                GameState::Instance().SetState(QState::NEWGAM);
                showHiscoreOverlay_ = true;
            }
            showHiscoreOverlay_ = true;
            if (stepTimer_ >= 1.2f) {
                queueShot(1);
                AdvanceScript_();
            }
            break;
        }

        case 2: {
            if (!stepEntered_) {
                stepEntered_ = true;
                PrepareWave_(1, true);
                GameState::Instance().SetState(QState::PLAY);
            }
            virtualLane_ = (sinf(stepTimer_ * 2.0f) > 0.0f) ? 1.0f : -1.0f;
            virtualFire_ = true;
            if (stepTimer_ >= 2.0f) {
                queueShot(2);
                AdvanceScript_();
            }
            break;
        }

        case 3: {
            if (!stepEntered_) {
                stepEntered_ = true;
                GameState::Instance().SetState(QState::NEWGAM);
                showCoinFlashOverlay_ = true;
                virtualCoin_ = true;
            }
            GameState::Instance().SetState(QState::NEWGAM);
            if (stepTimer_ < (1.0f / 60.0f)) {
                virtualCoin_ = true;
            }
            showCoinFlashOverlay_ = true;
            if (stepTimer_ >= (2.0f / 60.0f)) {
                queueShot(3);
                AdvanceScript_();
            }
            break;
        }

        case 4: {
            if (!stepEntered_) {
                stepEntered_ = true;
                GameState::Instance().SetState(QState::NEWGAM);
                showLevelSelectOverlay_ = true;
            }
            showLevelSelectOverlay_ = true;
            if (stepTimer_ >= 0.8f) {
                queueShot(4);
                AdvanceScript_();
            }
            break;
        }

        case 5: {
            if (!stepEntered_) {
                stepEntered_ = true;
                PrepareWave_(1, true);
                GameState::Instance().SetState(QState::NEWLIF);
            }
            if (stepTimer_ >= 0.9f) {
                queueShot(5);
                AdvanceScript_();
            }
            break;
        }

        case 6: {
            if (!stepEntered_) {
                stepEntered_ = true;
                PrepareWave_(1, true);
                GameState::Instance().SetState(QState::PLAY);
            }
            virtualFire_ = true;
            virtualLane_ = (sinf(stepTimer_ * 2.5f) > 0.0f) ? 1.0f : -1.0f;
            if (stepTimer_ >= 3.0f) {
                queueShot(6);
                AdvanceScript_();
            }
            break;
        }

        case 7: {
            if (!stepEntered_) {
                stepEntered_ = true;
                PrepareWave_(1, true);
                GameState::Instance().SetState(QState::PLAY);
                // Emit three bright explosions spread around the well so that
                // bloom glow is clearly visible as soft halos in this shot.
                Particles::Instance().EmitExplosion(Well::Instance().Project({3.0f,  0.08f}), COL_FLIPPER);
                Particles::Instance().EmitExplosion(Well::Instance().Project({8.0f,  0.14f}), COL_TANKER);
                Particles::Instance().EmitExplosion(Well::Instance().Project({13.0f, 0.08f}), COL_FLIPPER);
            }
            virtualFire_ = true;
            virtualLane_ = (sinf(stepTimer_ * 2.5f) > 0.0f) ? 1.0f : -1.0f;
            if (stepTimer_ >= 0.12f) {
                queueShot(7);
                AdvanceScript_();
            }
            break;
        }

        case 8: {
            if (!stepEntered_) {
                stepEntered_ = true;
                Vector2 p = Well::Instance().Project({6.0f, 0.35f});
                Particles::Instance().EmitExplosion(p, COL_FLIPPER);
            }
            if (stepTimer_ >= 0.08f) {
                queueShot(8);
                AdvanceScript_();
            }
            break;
        }

        case 9: {
            if (!stepEntered_) {
                stepEntered_ = true;
                Vector2 p1 = Well::Instance().Project({2.0f, 0.0f});
                Vector2 p2 = Well::Instance().Project({2.0f, 0.5f});
                Particles::Instance().EmitPulsarArc(p1, p2, COL_PULSAR);
            }
            if (stepTimer_ >= 0.06f) {
                queueShot(9);
                AdvanceScript_();
            }
            break;
        }

        case 10: {
            if (!stepEntered_) {
                stepEntered_ = true;
                PrepareWave_(1, false);
                GameState::Instance().SetState(QState::PLAY);
                Player::Instance().SetAlive(false);
                // Spawn a live Fuseball on the rim so EnemyManager emits
                // the double-trail naturally each frame.
                Enemy fb;
                fb.type     = EnemyType::FUSEBALL;
                fb.lane     = 6.0f;
                fb.depth    = 0.0f;
                fb.alive    = true;
                fb.onRim    = true;
                fb.flipTimer = 0.0f;
                fb.auxTimer  = 0.0f;
                fb.moveDir   = 1.0f;
                fb.spikeLen  = 0.0f;
                EnemyManager::Instance().GetEnemies().push_back(fb);
            }
            if (stepTimer_ >= 0.8f) {
                queueShot(10);
                AdvanceScript_();
            }
            break;
        }

        case 11: {
            if (!stepEntered_) {
                stepEntered_ = true;
                showSpikeOverlay_ = true;
            }
            showSpikeOverlay_ = true;
            if (stepTimer_ >= 0.8f) {
                queueShot(11);
                AdvanceScript_();
            }
            break;
        }

        case 12: {
            if (!stepEntered_) {
                stepEntered_ = true;
                PrepareWave_(1, true);
                GameState::Instance().SetState(QState::PLAY);
                Particles::Instance().EmitSuperZapperBurst();
            }

            // Take the shot at ~0.07s so the screen-fill white flash still
            // contributes (~8% alpha) without causing full-screen over-bloom.
            if (stepTimer_ >= 0.07f) {
                queueShot(12);
                AdvanceScript_();
            }
            break;
        }

        case 13: {
            if (!stepEntered_) {
                stepEntered_ = true;
                PrepareWave_(1, true);
                GameState::Instance().SetState(QState::PLAY);
                Vector2 p = Well::Instance().Project({Player::Instance().GetLane(), 0.02f});
                Particles::Instance().EmitPlayerDeath(p);
                GameState::Instance().DecLives();
                GameState::Instance().SetState(QState::ENDLIF);
            }
            if (stepTimer_ >= 0.20f) {
                queueShot(13);
                AdvanceScript_();
            }
            break;
        }

        case 14: {
            if (!stepEntered_) {
                stepEntered_ = true;
                GameState::Instance().SetState(QState::ENDLIF);
            }
            if (stepTimer_ >= 1.4f) {
                queueShot(14);
                AdvanceScript_();
            }
            break;
        }

        case 15: {
            if (!stepEntered_) {
                stepEntered_ = true;
                GameState::Instance().SetState(QState::ENDWAV);
            }
            if (stepTimer_ >= 1.4f) {
                queueShot(15);
                AdvanceScript_();
            }
            break;
        }

        case 16: {
            if (!stepEntered_) {
                stepEntered_ = true;
                PrepareWave_(2, true);
                GameState::Instance().SetState(QState::PLAY);
                Player::Instance().SetAlive(true);
                Player::Instance().SetLane(8.0f);
            }
            keepEnemiesAwayFromRim();
            if (stepTimer_ >= 3.0f) {
                queueShot(16);
                AdvanceScript_();
            }
            break;
        }

        case 17: {
            if (!stepEntered_) {
                stepEntered_ = true;
                PrepareWave_(5, true);
                GameState::Instance().SetState(QState::PLAY);
                Player::Instance().SetAlive(true);
                Player::Instance().SetLane(8.0f);
            }
            keepEnemiesAwayFromRim();
            if (stepTimer_ >= 3.0f) {
                queueShot(17);
                AdvanceScript_();
            }
            break;
        }

        case 18: {
            if (!stepEntered_) {
                stepEntered_ = true;
                PrepareWave_(9, true);
                GameState::Instance().SetState(QState::PLAY);
                Player::Instance().SetAlive(true);
                Player::Instance().SetLane(8.0f);
            }
            keepEnemiesAwayFromRim();
            if (stepTimer_ >= 3.0f) {
                queueShot(18);
                AdvanceScript_();
            }
            break;
        }

        case 19: {
            if (!stepEntered_) {
                stepEntered_ = true;
                GameState::Instance().SetState(QState::ENDGAM);
            }
            if (stepTimer_ >= 0.9f) {
                queueShot(19);
                AdvanceScript_();
            }
            break;
        }

        case 20: {
            if (!stepEntered_) {
                stepEntered_ = true;
                GameState::Instance().SetState(QState::GETINI);
            }
            if (stepTimer_ >= 0.9f) {
                queueShot(20);
                AdvanceScript_();
            }
            break;
        }

        case 21: {
            if (!stepEntered_) {
                stepEntered_ = true;
                PrepareWave_(3, true);
                GameState::Instance().SetState(QState::PLAY);
                GameState::Instance().AddScore(123456);
            }
            if (stepTimer_ >= 1.2f) {
                queueShot(21);
                AdvanceScript_();
            }
            break;
        }

        case 22: {
            if (!stepEntered_) {
                stepEntered_ = true;
            }
            Jukebox::Instance().SetIRPWidgetProfile(true);
            if (stepTimer_ >= 1.8f) {
                queueShot(22);
                AdvanceScript_();
            }
            break;
        }

        case 23: {
            if (!stepEntered_) {
                stepEntered_ = true;
                PrepareWave_(1, true);
                GameState::Instance().SetState(QState::PLAY);
                Player::Instance().SetAlive(false);
                auto& enemies = EnemyManager::Instance().GetEnemies();
                enemies.clear();
                // Keep PLAY active without rendering enemies.
                enemies.push_back({EnemyType::FLIPPER, 0.0f, 1.0f, false, false, 0.0f, 1.0f, 0.0f, 0.0f});
            }
            if (stepTimer_ >= 0.45f) {
                queueShot(23);
                AdvanceScript_();
            }
            break;
        }

        case 24: {
            if (!stepEntered_) {
                stepEntered_ = true;
                PrepareWave_(11, true);
                GameState::Instance().SetState(QState::PLAY);
                Player::Instance().SetAlive(false);
                auto& enemies = EnemyManager::Instance().GetEnemies();
                enemies.clear();
                // Keep PLAY active without rendering enemies.
                enemies.push_back({EnemyType::FLIPPER, 0.0f, 1.0f, false, false, 0.0f, 1.0f, 0.0f, 0.0f});
            }
            if (stepTimer_ >= 0.45f) {
                queueShot(24);
                AdvanceScript_();
            }
            break;
        }

        // ------ Parity shots 26-30 (script indices 25-29) ------

        case 25: {
            // 26: CIRCLE well wireframe — geometry only
            if (!stepEntered_) {
                stepEntered_ = true;
                PrepareWave_(1, false);
                GameState::Instance().SetState(QState::PLAY);
                Player::Instance().SetAlive(false);
                EnemyManager::Instance().GetEnemies().clear();
                // Sentinel to suppress ENDWAV transition
                EnemyManager::Instance().GetEnemies().push_back(
                    {EnemyType::FLIPPER, 0.0f, 1.0f, false, false, 0.0f, 1.0f, 0.0f, 0.0f});
                geometryOnlyMode_ = true;
            }
            geometryOnlyMode_ = true;
            if (stepTimer_ >= 0.5f) {
                queueShot(25);
                AdvanceScript_();
            }
            break;
        }

        case 26: {
            // 27: SQUARE well wireframe — geometry only
            if (!stepEntered_) {
                stepEntered_ = true;
                PrepareWave_(2, false);
                GameState::Instance().SetState(QState::PLAY);
                Player::Instance().SetAlive(false);
                EnemyManager::Instance().GetEnemies().clear();
                EnemyManager::Instance().GetEnemies().push_back(
                    {EnemyType::FLIPPER, 0.0f, 1.0f, false, false, 0.0f, 1.0f, 0.0f, 0.0f});
                geometryOnlyMode_ = true;
            }
            geometryOnlyMode_ = true;
            if (stepTimer_ >= 0.5f) {
                queueShot(26);
                AdvanceScript_();
            }
            break;
        }

        case 27: {
            // 28: PLANE (wave 11) well wireframe — open, geometry only
            if (!stepEntered_) {
                stepEntered_ = true;
                PrepareWave_(11, false);
                GameState::Instance().SetState(QState::PLAY);
                Player::Instance().SetAlive(false);
                EnemyManager::Instance().GetEnemies().clear();
                EnemyManager::Instance().GetEnemies().push_back(
                    {EnemyType::FLIPPER, 0.0f, 1.0f, false, false, 0.0f, 1.0f, 0.0f, 0.0f});
                geometryOnlyMode_ = true;
            }
            geometryOnlyMode_ = true;
            if (stepTimer_ >= 0.5f) {
                queueShot(27);
                AdvanceScript_();
            }
            break;
        }

        case 28: {
            // 29: Single Flipper stationary at rim — colour parity check
            if (!stepEntered_) {
                stepEntered_ = true;
                PrepareWave_(1, false);
                GameState::Instance().SetState(QState::PLAY);
                Player::Instance().SetAlive(false);
                EnemyManager::Instance().GetEnemies().clear();
                // Place a single live Flipper at lane 8 (top-center), right on rim
                Enemy fe{};
                fe.type      = EnemyType::FLIPPER;
                fe.lane      = 8.0f;
                fe.depth     = 0.0f;
                fe.alive     = true;
                fe.onRim     = true;
                fe.flipTimer = 0.0f;
                fe.auxTimer  = 0.0f;
                fe.moveDir   = 0.0f; // stationary
                fe.spikeLen  = 0.0f;
                EnemyManager::Instance().GetEnemies().push_back(fe);
            }
            if (stepTimer_ >= 0.5f) {
                queueShot(28);
                AdvanceScript_();
            }
            break;
        }

        case 29: {
            // 30: HUD only — score/lives/wave visible, no well or enemies
            if (!stepEntered_) {
                stepEntered_ = true;
                PrepareWave_(1, false);
                GameState::Instance().SetState(QState::PLAY);
                GameState::Instance().AddScore(87250);
                Player::Instance().SetAlive(false);
                EnemyManager::Instance().GetEnemies().clear();
                hudOnlyMode_ = true;
            }
            hudOnlyMode_ = true;
            if (stepTimer_ >= 0.5f) {
                queueShot(29);
                running_ = false;
                Input::SetVirtualLane(0.0f);
                Input::SetVirtualFire(false);
                Input::SetVirtualSuperZap(false);
                Input::SetVirtualCoin(false);
                Input::SetVirtualStart(false);
                geometryOnlyMode_ = false;
                hudOnlyMode_ = false;
            }
            break;
        }

        default:
            running_ = false;
            break;
    }

    Input::SetVirtualLane(virtualLane_);
    Input::SetVirtualFire(virtualFire_);
    Input::SetVirtualSuperZap(virtualSuperZap_);
    Input::SetVirtualCoin(virtualCoin_);
    Input::SetVirtualStart(virtualStart_);
}

void IRP::UpdatePost() {
    if (!enabled_) {
        return;
    }

    if (pendingShot_ >= 0) {
        TakeShot_(pendingShot_);
        pendingShot_ = -1;
    }

    if (!running_ && !manifestWritten_) {
        const bool allTaken = std::all_of(shots_.begin(), shots_.end(), [](const IRPShot& s) { return s.taken; });
        if (allTaken) {
            WriteManifest_();
            manifestWritten_ = true;
        }
    }
}

void IRP::DrawOverlay() {
    if (!enabled_) {
        return;
    }

    if (showHiscoreOverlay_) {
        DrawText("HIGH SCORES", SCREEN_W / 2 - 130, SCREEN_H / 2 - 170, 36, SKYBLUE);
        DrawText("1. AAA  125000", SCREEN_W / 2 - 130, SCREEN_H / 2 - 120, 28, SKYBLUE);
        DrawText("2. ZED   98000", SCREEN_W / 2 - 130, SCREEN_H / 2 - 84, 26, ColorAlpha(SKYBLUE, 0.9f));
        DrawText("3. KAT   77000", SCREEN_W / 2 - 130, SCREEN_H / 2 - 52, 24, ColorAlpha(SKYBLUE, 0.8f));
    }

    if (showCoinFlashOverlay_) {
        DrawText("CREDIT 01", SCREEN_W / 2 - 70, SCREEN_H / 2 + 40, 28, YELLOW);
    }

    if (showLevelSelectOverlay_) {
        DrawRectangle(SCREEN_W / 2 - 210, SCREEN_H / 2 - 130, 420, 220, ColorAlpha(BLACK, 0.45f));
        DrawRectangleLines(SCREEN_W / 2 - 210, SCREEN_H / 2 - 130, 420, 220, SKYBLUE);
        DrawText("LEVEL SELECT", SCREEN_W / 2 - 120, SCREEN_H / 2 - 100, 32, SKYBLUE);
        DrawText("WAVE 05 - 4KEY", SCREEN_W / 2 - 150, SCREEN_H / 2 - 48, 30, WHITE);
        DrawText("PRESS FIRE TO CONFIRM", SCREEN_W / 2 - 165, SCREEN_H / 2 + 4, 22, YELLOW);
    }

    if (showSpikeOverlay_) {
        Vector2 a = Well::Instance().Project({8.0f, 0.0f});
        Vector2 b = Well::Instance().Project({8.0f, 0.62f});
        DrawLineEx(a, b, 3.0f, COL_SPIKE);
    }

    if (GameState::Instance().GetState() == QState::GETINI) {
        DrawRectangle(SCREEN_W / 2 - 220, SCREEN_H / 2 - 90, 440, 180, ColorAlpha(BLACK, 0.45f));
        DrawRectangleLines(SCREEN_W / 2 - 220, SCREEN_H / 2 - 90, 440, 180, WHITE);
        DrawText("ENTER INITIALS", SCREEN_W / 2 - 130, SCREEN_H / 2 - 60, 32, WHITE);
        DrawText("A  <M>  Z", SCREEN_W / 2 - 70, SCREEN_H / 2 - 10, 40, YELLOW);
    }

    // Note: superZapper flash is rendered inside gameRT via Particles::Draw().
    // No additional overlay here to avoid double-white bloom washout.
}

bool IRP::IsRunning() {
    return running_;
}

bool IRP::IsEnabled() {
    return enabled_;
}

bool IRP::GeometryOnlyMode() {
    return geometryOnlyMode_;
}

bool IRP::HudOnlyMode() {
    return hudOnlyMode_;
}

const char* IRP::QStateName_(QState s) {
    switch (s) {
        case QState::NEWGAM: return "NEWGAM";
        case QState::NEWLIF: return "NEWLIF";
        case QState::PLAY: return "PLAY";
        case QState::ENDLIF: return "ENDLIF";
        case QState::ENDGAM: return "ENDGAM";
        case QState::PAUSE: return "PAUSE";
        case QState::ENDWAV: return "ENDWAV";
        case QState::HISCHK: return "HISCHK";
        case QState::GETINI: return "GETINI";
        default: return "UNKNOWN";
    }
}

void IRP::TakeShot_(int shotIndex) {
    if (shotIndex < 0 || shotIndex >= (int)shots_.size()) {
        return;
    }

    IRPShot& shot = shots_[shotIndex];
    if (shot.taken) {
        return;
    }

    char fname[512];
    std::snprintf(
        fname,
        sizeof(fname),
        "%s/%02d_%s.png",
        outputDir_.c_str(),
        shot.index,
        shot.label.c_str());

    Image img = LoadImageFromScreen();
    ExportImage(img, fname);
    UnloadImage(img);

    shot.taken = true;
    shotStateAtCapture_[shotIndex] = QStateName_(GameState::Instance().GetState());
    TraceLog(LOG_INFO, "[IRP] Shot %02d taken: %s (%s)", shot.index, shot.label.c_str(), QStateName_(GameState::Instance().GetState()));
}

void IRP::WriteManifest_() {
    if (outputDir_.empty()) {
        return;
    }

    const std::filesystem::path manifestPath = std::filesystem::path(outputDir_) / "manifest.md";
    std::ofstream out(manifestPath, std::ios::out | std::ios::trunc);
    if (!out.is_open()) {
        TraceLog(LOG_WARNING, "[IRP] Failed to write manifest: %s", manifestPath.string().c_str());
        return;
    }

    out << "# SZ IRP Screenshot Manifest\n";
    out << "**Run timestamp:** " << runTimestampIso_ << "\n";
    out << "**Total shots:** " << shots_.size() << "\n";
    out << "**Build:** sz.exe (Release, Raylib 5.5)\n\n";
    out << "---\n\n";
    out << "## Shot Index\n\n";
    out << "| # | Filename | Description | QState at capture |\n";
    out << "|---|----------|-------------|-------------------|\n";

    for (const IRPShot& shot : shots_) {
        const std::string file = TextFormat("%02d_%s.png", shot.index, shot.label.c_str());
        out << "| " << std::setw(2) << std::setfill('0') << shot.index
            << " | " << file
            << " | " << shot.description
            << " | " << shotStateAtCapture_[shot.index - 1]
            << " |\n";
    }

    out.flush();
    TraceLog(LOG_INFO, "[IRP] Manifest written: %s", manifestPath.string().c_str());
}

std::string IRP::MakeOutputDir_() {
    const std::filesystem::path root("irp_shots");
    std::filesystem::create_directories(root);

    const std::filesystem::path runDir = root / BuildFolderTimestamp();
    std::filesystem::create_directories(runDir);

    return runDir.generic_string();
}
