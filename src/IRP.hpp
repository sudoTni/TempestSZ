#pragma once

#include "GameState.hpp"
#include <string>
#include <vector>

struct IRPShot {
    int index;
    std::string label;
    std::string description;
    bool taken = false;
};

class IRP {
public:
    // Call once after all systems init. Pass true to activate IRP mode.
    static void Init(bool enabled);

    // Call every frame BEFORE BeginDrawing(). Drives game state for scripted sequence.
    static void UpdatePre(float dt);

    // Call every frame AFTER EndDrawing(). Takes screenshots at correct moments.
    static void UpdatePost();

    // Draws IRP-only overlays during rendering while the script is active.
    static void DrawOverlay();

    // Returns true while IRP is running; false = IRP complete (or disabled)
    static bool IsRunning();

    // Returns true if IRP mode is active (even if complete)
    static bool IsEnabled();

    // When true: skip enemies, player, bullets, particles — well only.
    static bool GeometryOnlyMode();

    // When true: skip well, enemies, player, bullets, particles — HUD only.
    static bool HudOnlyMode();

private:
    static void AdvanceScript_();
    static void TakeShot_(int shotIndex);
    static void WriteManifest_();
    static std::string MakeOutputDir_();
    static void PrepareWave_(int waveNum, bool spawnEnemies);
    static void ResetVirtualInputs_();
    static const char* QStateName_(QState s);

    static bool enabled_;
    static bool running_;
    static int scriptStep_;
    static float stepTimer_;
    static std::string outputDir_;
    static std::vector<IRPShot> shots_;
    static std::vector<std::string> shotStateAtCapture_;

    // Scripted input state - IRP overrides real input
    static float virtualLane_;
    static bool virtualFire_;
    static bool virtualSuperZap_;
    static bool virtualCoin_;
    static bool virtualStart_;

    static int pendingShot_;
    static bool stepEntered_;
    static std::string runTimestampIso_;
    static bool showHiscoreOverlay_;
    static bool showLevelSelectOverlay_;
    static bool showCoinFlashOverlay_;
    static bool showSpikeOverlay_;
    static float superZapFlashAlpha_;
    static bool manifestWritten_;
    static bool geometryOnlyMode_;
    static bool hudOnlyMode_;
};
