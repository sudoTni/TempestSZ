#include "ContourSystem.hpp"
#include <algorithm>

void ContourSystem::Init() {
    // Implement WTABLE logic from tech spec
    // Example: WCHARFR (Enemy fire timer)
    records_[0] = { { CT_T1, 1, 100, { 120 } } }; // Constant 120 frames at start
    records_[0].push_back({ CT_TA, 2, 32, { 120, -2 } }); // Decreases by 2 each wave
    records_[0].push_back({ CT_T1, 33, 99, { 60 } }); // Hard cap at 60
}

int ContourSystem::Evaluate(int wave, int tableID) const {
    auto it = records_.find(tableID);
    if (it == records_.end()) return 0;

    for (const auto& rec : it->second) {
        if (wave >= rec.waveMin && wave <= rec.waveMax) {
            switch (rec.type) {
                case CT_T1: return rec.params[0];
                case CT_TA: return rec.params[0] + rec.params[1] * (wave - rec.waveMin);
                case CT_TZ: {
                    int idx = wave - rec.waveMin;
                    if (idx < (int)rec.params.size()) return rec.params[idx];
                    return rec.params.back();
                }
                default: return rec.params[0];
            }
        }
    }
    return 0;
}
