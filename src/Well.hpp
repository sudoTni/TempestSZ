#pragma once
#include <raylib.h>
#include <vector>
#include <cstdint>
#include "Constants.hpp"

enum WellID {
    WELL_CIRCLE   = 0,
    WELL_SQUARE   = 1,
    WELL_CROSS    = 2,
    WELL_PEANUT   = 3,
    WELL_4KEY     = 4,
    WELL_TRIANGLE = 5,
    WELL_CLOVER   = 6,
    WELL_V        = 7,
    WELL_PLANE    = 8,
    WELL_U        = 9,
    WELL_JAGGED   = 10,
    WELL_LYING8   = 11,
    WELL_HEART    = 12,
    WELL_STAIRCASE= 13,
    WELL_STARX    = 14,
    WELL_WAVEX    = 15,
};

struct WellDef {
    bool   isClosed;
    float  eyeY;
    float  eyeZ;
    float  rimAngles[NUM_LANES];
    float  depAngles[NUM_LANES];
};

struct WellRawData {
    int     id;
    int     isClosed; // 0=closed, -1=planar
    uint8_t eyeY;
    uint8_t eyeZ;
    uint8_t xAngles[NUM_LANES];
    uint8_t zAngles[NUM_LANES];
};

extern const WellRawData WELL_DATA_TABLE[NUM_WELLS];

struct WellPoint {
    float lane;   // 0..NUM_LANES-1
    float depth;  // 0.0 (rim) .. 1.0 (far)
};

constexpr int WELL_SEQUENCE[16] = {
    0, 1, 2, 3, 4, 5, 6, 7, 13, 9, 8, 12, 14, 15, 10, 11
};

class Well {
public:
    static Well& Instance() {
        static Well instance;
        return instance;
    }

    void Init(int waveNum);
    Vector2 Project(WellPoint wp) const;
    float GetRimAngle(float lane) const;
    bool IsClosed() const { return def_.isClosed; }
    
    // For rendering
    const std::vector<Vector2>& GetRimVertices() const { return rimVertices_; }
    Vector2 GetVanishingPoint() const { return vanishingPoint_; }

private:
    Well() = default;
    void BuildRim_();

    WellDef def_;
    std::vector<Vector2> rimVertices_;
    Vector2 vanishingPoint_;
    int numLanes_ = NUM_LANES;
};
