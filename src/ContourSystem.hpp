#pragma once
#include <vector>
#include <map>
#include <cstdint>

enum ContourType : uint8_t {
    CT_T1 = 2, CT_TZ = 4, CT_TE = 0, CT_TZANDF = 6, CT_TA = 8, CT_TB = 0xA, CT_TR = 0xC
};

struct ContourRecord {
    ContourType type;
    int waveMin, waveMax;
    std::vector<int> params;
};
class ContourSystem {
public:
    static ContourSystem& Instance() {
        static ContourSystem instance;
        return instance;
    }
    void Init();
    int Evaluate(int wave, int tableID) const;

private:
    ContourSystem() = default;
    std::map<int, std::vector<ContourRecord>> records_;
};

