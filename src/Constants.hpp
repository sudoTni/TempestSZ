#pragma once
#include <raylib.h>
#include <cmath>

// Screen
constexpr int   SCREEN_W          = 1280;
constexpr int   SCREEN_H          = 720;
constexpr int   TARGET_FPS        = 60;

// Well
constexpr int   NUM_LANES         = 16;
constexpr int   NUM_WELLS         = 16;
constexpr float DEPTH_PERSPECTIVE = 0.82f;
constexpr float SCALE_EYE         = 0.04f;
constexpr float LANE_DEPTH        = 1.0f;

// Player
constexpr float PLAYER_SHOOT_COOLDOWN   = 0.13f;   // sec
constexpr float BULLET_SPEED            = 1.9f;    // depth-units/sec
constexpr float BULLET_HIT_RADIUS       = 0.05f;

// Enemies
constexpr float FLIPPER_APPROACH_SPEED  = 0.18f;
constexpr float FLIPPER_FLIP_RATE       = (2.0f * (float)PI) / 0.25f;
constexpr float TANKER_APPROACH_SPEED   = 0.08f;
constexpr float SPIKER_APPROACH_SPEED   = 0.12f;
constexpr float SPIKER_SPIKE_RATE       = 0.15f;
constexpr float FUSEBALL_SPEED          = 0.55f;

// Bloom
#ifdef SZ_ENHANCED_VFX
constexpr float BLOOM_THRESHOLD  = 0.45f;
constexpr float BLOOM_STRENGTH   = 1.60f;
#else
constexpr float BLOOM_THRESHOLD  = 0.50f;
constexpr float BLOOM_STRENGTH   = 1.35f;
#endif

// BGM
constexpr int   BGM_SAMPLE_RATE  = 48000;
constexpr int   BGM_CHANNELS     = 2;
constexpr float CROSSFADE_SECS   = 7.0f;

// SFX
constexpr int   SFX_SAMPLE_RATE  = 44100;
constexpr int   SFX_VOICES       = 8;
constexpr float INV_SAMPLE_RATE  = 1.0f / (float)SFX_SAMPLE_RATE;

// Particles
constexpr int   MAX_PARTICLES    = 4096;

// Colors (Tempest Original — from ALCOMN.MAC colour constants)
// WELCOL=BLUE(6), CURCOL=YELLOW(1), FLICOL=RED(3), TANCOL=PURPLE(2), TRACOL=GREEN(5)
constexpr Color COL_WELL       = { 0x14, 0x14, 0xFF, 0xFF };  // Vivid blue (WELCOL)
constexpr Color COL_PLAYER     = { 0xFF, 0xFF, 0x00, 0xFF };  // Yellow (CURCOL=YELLOW) PARITY-COL-01 fixed
constexpr Color COL_FLIPPER    = { 0xFF, 0x22, 0x22, 0xFF };  // Red (FLICOL=RED)       PARITY-COL-02 fixed
constexpr Color COL_TANKER     = { 0xCC, 0x00, 0xFF, 0xFF };  // Purple (TANCOL=PURPLE) PARITY-COL-03 fixed
constexpr Color COL_SPIKER     = { 0x00, 0xFF, 0x00, 0xFF };  // Green
constexpr Color COL_FUSEBALL   = { 0xFF, 0x44, 0xFF, 0xFF };  // Magenta
constexpr Color COL_PULSAR     = { 0xFF, 0x22, 0x22, 0xFF };  // Red
constexpr Color COL_SHOT_ENEMY = { 0xFF, 0x44, 0x00, 0xFF };  // Orange
constexpr Color COL_SHOT_PLR   = { 0xFF, 0xFF, 0xFF, 0xFF };  // White
constexpr Color COL_SPIKE      = { 0x00, 0xCC, 0x00, 0xFF };  // Green
constexpr Color COL_STAR       = { 0xAA, 0xAA, 0xFF, 0xFF };  // Lt blue
constexpr Color COL_SCORE_TXT  = { 0x00, 0xFF, 0x44, 0xFF };  // Green

// Utility
inline float angleFromByte(unsigned char b) {
    signed char signed_val = (signed char)(b - 0x80);
    return (signed_val / 112.0f) * (float)PI;
}
