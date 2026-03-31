#pragma once
#include <raylib.h>

class HUD {
public:
    static HUD& Instance() {
        static HUD instance;
        return instance;
    }

    void Init();
    void Draw();
private:
    HUD() = default;
    void DrawLifeIcon(float x, float y);
    void DrawInitialsEntry();
};
