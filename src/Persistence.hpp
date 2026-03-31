#pragma once
#include <cstdint>

struct HiScoreEntry {
    uint32_t score;
    char     initials[4];
};

class Persistence {
public:
    static void Save(const HiScoreEntry* entries);
    static void Load(HiScoreEntry* entries);
};
