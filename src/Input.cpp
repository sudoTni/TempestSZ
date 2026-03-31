#include "Input.hpp"

namespace {
struct VirtualInputState {
    bool active = false;
    float laneDelta = 0.0f;
    bool firePressed = false;
    bool fireDown = false;
    bool coinPressed = false;
    bool startPressed = false;
    bool superZapPressed = false;
};

VirtualInputState g_virtual;
} // namespace

namespace Input {

void BeginFrame() {
    if (!g_virtual.active) {
        return;
    }

    // One-shot presses are consumed each frame while held states persist.
    g_virtual.firePressed = false;
    g_virtual.coinPressed = false;
    g_virtual.startPressed = false;
    g_virtual.superZapPressed = false;
}

void SetVirtualActive(bool active) {
    g_virtual.active = active;
    if (!active) {
        g_virtual = VirtualInputState{};
    }
}

bool IsVirtualActive() {
    return g_virtual.active;
}

void SetVirtualLane(float delta) {
    g_virtual.laneDelta = delta;
}

void SetVirtualFire(bool pressed) {
    g_virtual.firePressed = pressed;
    g_virtual.fireDown = pressed;
}

void SetVirtualCoin(bool pressed) {
    g_virtual.coinPressed = pressed;
}

void SetVirtualStart(bool pressed) {
    g_virtual.startPressed = pressed;
}

void SetVirtualSuperZap(bool pressed) {
    g_virtual.superZapPressed = pressed;
}

float GetLaneDelta() {
    if (g_virtual.active) {
        return g_virtual.laneDelta;
    }

    float delta = 0.0f;
    if (IsKeyDown(KEY_LEFT)) {
        delta -= 1.0f;
    }
    if (IsKeyDown(KEY_RIGHT)) {
        delta += 1.0f;
    }
    if (IsGamepadAvailable(0)) {
        const float axis = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_X);
        if (axis > 0.3f) {
            delta += 1.0f;
        } else if (axis < -0.3f) {
            delta -= 1.0f;
        }
    }
    if (delta > 1.0f) {
        delta = 1.0f;
    }
    if (delta < -1.0f) {
        delta = -1.0f;
    }
    return delta;
}

bool IsFirePressed() {
    if (g_virtual.active) {
        const bool v = g_virtual.firePressed;
        g_virtual.firePressed = false;
        return v;
    }
    return IsKeyPressed(KEY_SPACE) || IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN);
}

bool IsFireDown() {
    if (g_virtual.active) {
        return g_virtual.fireDown;
    }
    return IsKeyDown(KEY_SPACE) || IsGamepadButtonDown(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN);
}

bool IsCoinPressed() {
    if (g_virtual.active) {
        const bool v = g_virtual.coinPressed;
        g_virtual.coinPressed = false;
        return v;
    }
    return IsKeyPressed(KEY_FIVE);
}

bool IsStartPressed() {
    if (g_virtual.active) {
        const bool v = g_virtual.startPressed;
        g_virtual.startPressed = false;
        return v;
    }
    return IsKeyPressed(KEY_ONE);
}

bool IsSuperZapPressed() {
    if (g_virtual.active) {
        const bool v = g_virtual.superZapPressed;
        g_virtual.superZapPressed = false;
        return v;
    }
    return IsKeyPressed(KEY_X) || IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT);
}

} // namespace Input
