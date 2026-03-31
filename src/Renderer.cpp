#include "Renderer.hpp"
#include "Well.hpp"
#include "Player.hpp"
#include "Enemy.hpp"
#include "Bullet.hpp"
#include "GameState.hpp"
#include "embedded_assets.hpp"
#include <rlgl.h>
#include <cmath>

void Renderer::Init() {
    gameRT_ = LoadRenderTexture(SCREEN_W, SCREEN_H);
    blurRT_[0] = LoadRenderTexture(SCREEN_W / 2, SCREEN_H / 2);
    blurRT_[1] = LoadRenderTexture(SCREEN_W / 2, SCREEN_H / 2);
    compositeRT_ = LoadRenderTexture(SCREEN_W, SCREEN_H);

    // Clamp bloom sampling at texture edges to prevent wrap-around light leaks.
    SetTextureWrap(gameRT_.texture, TEXTURE_WRAP_CLAMP);
    SetTextureWrap(blurRT_[0].texture, TEXTURE_WRAP_CLAMP);
    SetTextureWrap(blurRT_[1].texture, TEXTURE_WRAP_CLAMP);
    SetTextureWrap(compositeRT_.texture, TEXTURE_WRAP_CLAMP);

    // Load shaders from embedded data (no external files needed).
    bloomDownsample_ = LoadShaderFromMemory(0, g_shader_bloom_downsample_fs);
    bloomBlur_        = LoadShaderFromMemory(0, g_shader_bloom_blur_fs);
    bloomComposite_   = LoadShaderFromMemory(0, g_shader_bloom_composite_fs);
}

void Renderer::Shutdown() {
    UnloadRenderTexture(gameRT_);
    UnloadRenderTexture(blurRT_[0]);
    UnloadRenderTexture(blurRT_[1]);
    UnloadRenderTexture(compositeRT_);
    UnloadShader(bloomDownsample_);
    UnloadShader(bloomBlur_);
    UnloadShader(bloomComposite_);
}

void Renderer::BeginFrame() {
    // Preparations
}

void Renderer::BeginGameRender() {
    BeginTextureMode(gameRT_);
    ClearBackground(BLACK);
}

void Renderer::EndGameRender() {
    EndTextureMode();
}

void Renderer::ApplyBloom() {
    static float offsets[] = {
        0.5f,
        1.5f,
        2.5f,
#ifdef SZ_ENHANCED_VFX
        3.5f
#endif
    };
    int current = 0;

    // 1. Downsample + Threshold
    BeginTextureMode(blurRT_[0]);
        ClearBackground(BLACK);
        BeginShaderMode(bloomDownsample_);
            float threshold = BLOOM_THRESHOLD;
            SetShaderValue(bloomDownsample_, GetShaderLocation(bloomDownsample_, "threshold"), &threshold, SHADER_UNIFORM_FLOAT);
            DrawTexturePro(
                gameRT_.texture,
                {0, 0, (float)gameRT_.texture.width, -(float)gameRT_.texture.height},
                {0, 0, (float)blurRT_[0].texture.width, (float)blurRT_[0].texture.height},
                {0, 0},
                0.0f,
                WHITE);
        EndShaderMode();
    EndTextureMode();

    // 2. Kawase Blur (Ping-pong)
    BeginShaderMode(bloomBlur_);
    const int passCount = (int)(sizeof(offsets) / sizeof(offsets[0]));
    for (int i = 0; i < passCount; ++i) {
        int next = 1 - current;
        SetShaderValue(bloomBlur_, GetShaderLocation(bloomBlur_, "offset"), &offsets[i], SHADER_UNIFORM_FLOAT);
        Vector2 ts = { 1.0f / blurRT_[current].texture.width, 1.0f / blurRT_[current].texture.height };
        SetShaderValue(bloomBlur_, GetShaderLocation(bloomBlur_, "texelSize"), &ts, SHADER_UNIFORM_VEC2);
        
        BeginTextureMode(blurRT_[next]);
            ClearBackground(BLACK);
            DrawTextureRec(blurRT_[current].texture, {0, 0, (float)blurRT_[current].texture.width, -(float)blurRT_[current].texture.height}, {0, 0}, WHITE);
        EndTextureMode();
        current = next;
    }
    EndShaderMode();

    // 3. Composite
    BeginTextureMode(compositeRT_);
        ClearBackground(BLACK);
        BeginShaderMode(bloomComposite_);
            SetShaderValueTexture(bloomComposite_, GetShaderLocation(bloomComposite_, "bloomTex"), blurRT_[current].texture);
            float strength = BLOOM_STRENGTH;
            SetShaderValue(bloomComposite_, GetShaderLocation(bloomComposite_, "bloomStrength"), &strength, SHADER_UNIFORM_FLOAT);
            DrawTextureRec(gameRT_.texture, {0, 0, (float)gameRT_.texture.width, -(float)gameRT_.texture.height}, {0, 0}, WHITE);
        EndShaderMode();
    EndTextureMode();
}

void Renderer::DrawFinalComposite() {
    DrawTextureRec(compositeRT_.texture, {0, 0, (float)compositeRT_.texture.width, -(float)compositeRT_.texture.height}, {0, 0}, WHITE);
}

void Renderer::DrawWell() {
    const auto& rim = Well::Instance().GetRimVertices();
    if (rim.empty()) return;
    Vector2 vp = Well::Instance().GetVanishingPoint();
    const float t = (float)GetTime();
    
    // Draw rails
    for (size_t i = 0; i < rim.size(); ++i) {
        Color railCol = COL_WELL;

#ifdef SZ_ENHANCED_VFX
        // Rail shimmer with lane phase offsets at approximately 2 Hz.
        const float shimmer = 0.5f + 0.5f * sinf(t * 2.0f * 2.0f * PI + (float)i * 0.6f);
        railCol.r = (unsigned char)(0x08 + (0x42 - 0x08) * shimmer);
        railCol.g = (unsigned char)(0x08 + (0x42 - 0x08) * shimmer);
        railCol.b = (unsigned char)(0xCC + (0xFF - 0xCC) * shimmer);

        // Depth fade: near half bright, far half fades toward 60% alpha.
        const Vector2 split = {
            vp.x + (rim[i].x - vp.x) * 0.40f,
            vp.y + (rim[i].y - vp.y) * 0.40f
        };
        GlowLine(rim[i], split, railCol, 1.5f);
        GlowLine(split, vp, ColorAlpha(railCol, 0.60f), 1.3f);
#else
        GlowLine(rim[i], vp, railCol, 1.5f);
#endif

        // Rim edges
        size_t next = (i + 1) % rim.size();
        if (Well::Instance().IsClosed() || i < rim.size() - 1) {
            Color rimCol = railCol;
#ifdef SZ_ENHANCED_VFX
            if (GameState::Instance().GetState() == QState::NEWLIF && GameState::Instance().GetStateTimer() < 1.5f) {
                // Traveling cyan pulse along lane index during wave spawn.
                const float sweep = (GameState::Instance().GetStateTimer() / 1.5f) * (float)NUM_LANES;
                const float d = fabsf((float)i - sweep);
                if (d < 1.5f) {
                    const float k = 1.0f - (d / 1.5f);
                    rimCol = {
                        (unsigned char)(rimCol.r + (0x66 - rimCol.r) * k),
                        (unsigned char)(rimCol.g + (0xFF - rimCol.g) * k),
                        (unsigned char)(rimCol.b + (0xFF - rimCol.b) * k),
                        255
                    };
                }
            }
#endif
            GlowLine(rim[i], rim[next], rimCol, 2.0f);
        }
    }

#ifdef SZ_ENHANCED_VFX
    // Vanishing-point glow: small radial gradient disc at the well centre,
    // bloom-eligible, mimics original vector bright-up at depth extremes.
    {
        const Vector2 vp2 = Well::Instance().GetVanishingPoint();
        DrawCircleV(vp2, 12.0f, ColorAlpha(WHITE, 0.18f));
        DrawCircleV(vp2,  7.0f, ColorAlpha(WHITE, 0.40f));
        DrawCircleV(vp2,  3.0f, ColorAlpha(WHITE, 0.80f));
    }
#endif
}

void Renderer::DrawEnemies() {
    auto& enemies = EnemyManager::Instance().GetEnemies();
    for (auto& e : enemies) {
        if (!e.alive) continue;

        Color col = COL_FLIPPER;
        float size = 0.20f;
        switch (e.type) {
            case EnemyType::FLIPPER:
                col = COL_FLIPPER;
                size = 0.20f;
                break;
            case EnemyType::TANKER:
                col = COL_TANKER;
                size = 0.27f;
                break;
            case EnemyType::SPIKER:
                col = COL_SPIKER;
                size = 0.22f;
                break;
            case EnemyType::FUSEBALL:
                col = COL_FUSEBALL;
                size = 0.16f;
                break;
            case EnemyType::PULSAR:
                col = COL_PULSAR;
                size = 0.24f;
                break;
        }

        Vector2 p1 = Well::Instance().Project({e.lane - 0.2f, e.depth});
        Vector2 p2 = Well::Instance().Project({e.lane + 0.2f, e.depth});
        Vector2 p3 = Well::Instance().Project({e.lane, e.depth - 0.05f * (size / 0.20f)});
        Vector2 p4 = Well::Instance().Project({e.lane, e.depth + 0.05f * (size / 0.20f)});
        GlowLine(p1, p3, col, 1.5f);
        GlowLine(p3, p2, col, 1.5f);
        GlowLine(p2, p4, col, 1.5f);
        GlowLine(p4, p1, col, 1.5f);

        if (e.type == EnemyType::SPIKER && e.onRim && e.spikeLen > 0.05f) {
            const float depth = std::min(0.95f, e.spikeLen);
            Vector2 s0 = Well::Instance().Project({e.lane, 0.0f});
            Vector2 s1 = Well::Instance().Project({e.lane, depth});
            GlowLine(s0, s1, COL_SPIKE, 2.0f);
        }
    }
}

void Renderer::DrawPlayer() {
    float lane = Player::Instance().GetLane();
    if (!Player::Instance().IsAlive()) return;
    
    Vector2 p1 = Well::Instance().Project({lane - 0.4f, 0.0f});
    Vector2 p2 = Well::Instance().Project({lane + 0.4f, 0.0f});
    Vector2 p3 = Well::Instance().Project({lane, 0.08f}); // Inward point
    
    GlowLine(p1, p3, COL_PLAYER, 2.0f);
    GlowLine(p2, p3, COL_PLAYER, 2.0f);
    GlowLine(p1, p2, COL_PLAYER, 1.0f);
}

void Renderer::DrawBullets() {
    auto& bullets = BulletManager::Instance().GetBullets();
    for (auto& b : bullets) {
        if (!b.alive) continue;
        Vector2 p1 = Well::Instance().Project({b.lane, b.depth});
        Vector2 p2 = Well::Instance().Project({b.lane, b.depth + 0.03f});
        GlowLine(p1, p2, b.fromPlayer ? COL_SHOT_PLR : COL_SHOT_ENEMY, 2.0f);
#ifdef SZ_ENHANCED_VFX
        // Micro-trail: render 3 ghost positions at t-1, t-2, t-3 with alpha 60%, 35%, 15%
        if (b.fromPlayer) {
            static const float kAlpha[3] = { 0.60f, 0.35f, 0.15f };
            for (int g = 0; g < 3; ++g) {
                if (b.prevDepth[g] < 0.0f) break;
                Vector2 gp1 = Well::Instance().Project({b.lane, b.prevDepth[g]});
                Vector2 gp2 = Well::Instance().Project({b.lane, b.prevDepth[g] + 0.03f});
                DrawLineEx(gp1, gp2, 1.5f, ColorAlpha(COL_SHOT_PLR, kAlpha[g]));
            }
        }
#endif
    }
}

void Renderer::GlowLine(Vector2 a, Vector2 b, Color baseColor, float baseThick) {
    DrawLineEx(a, b, baseThick * 7.0f, ColorAlpha(baseColor, 0.07f));  // wide soft halo
    DrawLineEx(a, b, baseThick * 3.5f, ColorAlpha(baseColor, 0.22f));  // mid glow
    DrawLineEx(a, b, baseThick * 1.8f, ColorAlpha(baseColor, 0.60f));  // inner bright
    DrawLineEx(a, b, baseThick,       baseColor);                       // core
}
