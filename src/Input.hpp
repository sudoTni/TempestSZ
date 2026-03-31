#pragma once
#include <raylib.h>

// Input abstraction layer.
// Wraps Raylib input queries so the IRP can inject virtual inputs
// without touching any game logic.
//
// When IRP is not active, all queries pass through to real Raylib calls.
// When IRP is active (SetVirtualActive(true)), virtual values are used
// instead of hardware input.

namespace Input {

    // Called once per frame by main loop BEFORE game update.
    // Clears one-shot virtual "pressed" flags after they've been read once.
    void BeginFrame();

    // ── Virtual override control ────────────────────────────────────────────
    void SetVirtualActive(bool active);
    bool IsVirtualActive();

    // Write virtual state (IRP calls these to drive the game)
    void SetVirtualLane(float delta);           // -1 = left, +1 = right, 0 = none
    void SetVirtualFire(bool pressed);
    void SetVirtualCoin(bool pressed);
    void SetVirtualStart(bool pressed);
    void SetVirtualSuperZap(bool pressed);

    // ── Game query API (replaces direct Raylib calls in gameplay code) ───────
    // Lane movement  — returns -1.0f (left), +1.0f (right), or 0.0f
    float GetLaneDelta();

    // Returns true if fire button was pressed THIS frame
    bool IsFirePressed();

    // Returns true while fire is held (auto-fire)
    bool IsFireDown();

    // Returns true if the coin-insert key was pressed THIS frame
    bool IsCoinPressed();

    // Returns true if the start key was pressed THIS frame
    bool IsStartPressed();

    // Returns true if SuperZapper was activated THIS frame
    bool IsSuperZapPressed();

} // namespace Input
