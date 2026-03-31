#include "HUD.hpp"
#include "GameState.hpp"
#include "Player.hpp"
#include "Constants.hpp"
#include <algorithm>
#include <cstdio>
#include <string>

void HUD::Init() {}

void HUD::Draw() {
    static int prevScore = 0;
    static int prevWave = 1;
    static int prevLives = 3;
    static float scorePulse = 0.0f;
    static float wavePulse = 0.0f;
    static float lifeFlash = 0.0f;
    static float hiScoreFlash = 0.0f;  // flashes when new hi-score is set

    const int score = GameState::Instance().GetScore();
    const int wave = GameState::Instance().GetWave();
    const int lives = GameState::Instance().GetLives();
    const int hiScore = GameState::Instance().GetHighScore();
    const bool newHiScore = GameState::Instance().IsNewHighScore();

    if (score > prevScore) {
        scorePulse = 0.2f;
    }
    if (wave != prevWave) {
        wavePulse = 0.4f;
    }
    if (lives < prevLives) {
        lifeFlash = 0.1f;
    }
    if (newHiScore && hiScoreFlash <= 0.0f) {
        hiScoreFlash = 1.2f;  // trigger a ~1.2 s hi-score flash
    }

    const float dt = GetFrameTime();
    scorePulse   = std::max(0.0f, scorePulse   - dt);
    wavePulse    = std::max(0.0f, wavePulse    - dt);
    lifeFlash    = std::max(0.0f, lifeFlash    - dt);
    hiScoreFlash = std::max(0.0f, hiScoreFlash - dt);

    prevScore = score;
    prevWave = wave;
    prevLives = lives;

    // Score (6 digits, upper-center)
    char scoreBuf[32];
    std::snprintf(scoreBuf, sizeof(scoreBuf), "SCORE %06d", score);

    float scoreScale = 1.0f;
#ifdef SZ_ENHANCED_VFX
    if (scorePulse > 0.0f) {
        const float t = 1.0f - (scorePulse / 0.2f);
        if (t < 0.5f) {
            scoreScale = 1.0f + (1.35f - 1.0f) * (t / 0.5f);
        } else {
            scoreScale = 1.35f + (1.0f - 1.35f) * ((t - 0.5f) / 0.5f);
        }
    }
#endif

    Font font = GetFontDefault();
    const float scoreSize = 28.0f * scoreScale;
    const Vector2 scoreDim = MeasureTextEx(font, scoreBuf, scoreSize, 1.0f);
    DrawTextEx(font, scoreBuf, {(SCREEN_W - scoreDim.x) * 0.5f, 14.0f}, scoreSize, 1.0f, COL_SCORE_TXT);

    // Hi-score (cyan, upper-right) — flash yellow when new hi-score is first set
    char hiBuf[32];
    std::snprintf(hiBuf, sizeof(hiBuf), "HI %06d", std::max(hiScore, score));
#ifdef SZ_ENHANCED_VFX
    Color hiCol = SKYBLUE;
    if (hiScoreFlash > 0.0f) {
        // Rapidly blink between YELLOW and WHITE for ~1.2 s
        hiCol = (((int)(hiScoreFlash * 8.0f)) % 2 == 0) ? YELLOW : WHITE;
    }
    DrawText(hiBuf, SCREEN_W - 220, 20, 24, hiCol);
#else
    DrawText(hiBuf, SCREEN_W - 220, 20, 24, SKYBLUE);
#endif

    // Wave number (zoom on transition)
    float waveScale = 1.0f;
#ifdef SZ_ENHANCED_VFX
    if (wavePulse > 0.0f) {
        const float t = 1.0f - (wavePulse / 0.4f);
        waveScale = 3.0f + (1.0f - 3.0f) * t;
    }
#endif
    char waveBuf[32];
    std::snprintf(waveBuf, sizeof(waveBuf), "WAVE %02d", wave);
    DrawTextEx(font, waveBuf, {20.0f, 20.0f}, 24.0f * waveScale, 1.0f, BLUE);

    // Lives as blaster icons (upper-left)
    for (int i = 0; i < lives; ++i) {
        Color lifeCol = COL_PLAYER;
#ifdef SZ_ENHANCED_VFX
        if (lifeFlash > 0.0f) {
            lifeCol = WHITE;
        }
#endif
        DrawTriangle({50.0f + i * 26.0f, 68.0f}, {40.0f + i * 26.0f, 84.0f}, {60.0f + i * 26.0f, 84.0f}, lifeCol);
    }

    // SuperZapper indicator — shown below lives row
    // Bright white when available, dark gray (dimmed) when spent this wave
    {
        const bool szAvail = Player::Instance().HasSuperZapper();
        const Color szColor = szAvail ? Color{0xFF, 0xFF, 0xFF, 0xFF}
                                      : Color{0x44, 0x44, 0x44, 0xFF};
        DrawText("SZ", 42, 92, 18, szColor);
        // Small lightning bolt indicator strip to the right of text
        const float bx = 68.0f;
        const float by = 92.0f;
        if (szAvail) {
            // Filled indicator — bright yellow when ready
            DrawLineEx({bx, by + 2}, {bx + 4, by + 8}, 2.0f, YELLOW);
            DrawLineEx({bx + 4, by + 8}, {bx + 1, by + 8}, 2.0f, YELLOW);
            DrawLineEx({bx + 1, by + 8}, {bx + 6, by + 16}, 2.0f, YELLOW);
        }
    }

    // State messages
    if (GameState::Instance().GetState() == QState::NEWGAM) {
        DrawText("PRESS 1 TO START", SCREEN_W/2 - 100, SCREEN_H/2, 20, WHITE);

        // Port credit — centered near bottom
        const char* portCredit = "SOURCE PORT BY MIKEM";
        const int creditFontSize = 18;
        const int creditW = MeasureText(portCredit, creditFontSize);
#ifdef SZ_ENHANCED_VFX
        // Gentle sine fade: hover between 45% and 100% opacity at ~0.5 Hz.
        const float alpha = 0.45f + 0.55f * (0.5f + 0.5f * sinf((float)GetTime() * PI));
        const Color creditCol = ColorAlpha({0x66, 0xAA, 0xFF, 0xFF}, alpha);
#else
        const Color creditCol = {0x66, 0xAA, 0xFF, 0xFF};
#endif
        DrawText(portCredit, (SCREEN_W - creditW) / 2, SCREEN_H - 48, creditFontSize, creditCol);
    } else if (GameState::Instance().GetState() == QState::ENDGAM) {
        DrawText("GAME OVER", SCREEN_W/2 - 60, SCREEN_H/2, 30, RED);
    }
}
