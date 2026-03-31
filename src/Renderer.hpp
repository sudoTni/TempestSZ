#pragma once
#include <raylib.h>
#include "Constants.hpp"

class Renderer {
public:
    static Renderer& Instance() {
        static Renderer instance;
        return instance;
    }

    void Init();
    void Shutdown();

    void BeginFrame();
    void BeginGameRender();
    void EndGameRender();
    void ApplyBloom();
    void DrawFinalComposite();

    // Scene drawing
    void DrawWell();
    void DrawEnemies();
    void DrawPlayer();
    void DrawBullets();

    // Glow line utility
    void GlowLine(Vector2 a, Vector2 b, Color baseColor, float baseThick = 1.5f);

private:
    Renderer() = default;

    RenderTexture2D gameRT_;
    RenderTexture2D blurRT_[2];
    RenderTexture2D compositeRT_;

    Shader bloomDownsample_;
    Shader bloomBlur_;
    Shader bloomComposite_;
};
