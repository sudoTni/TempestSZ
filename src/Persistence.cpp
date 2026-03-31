#include "Persistence.hpp"
#include <raylib.h>

void Persistence::Save(const HiScoreEntry* entries) {
    SaveFileData("sz_hiscore.bin", (void*)entries, sizeof(HiScoreEntry) * 8);
}

void Persistence::Load(HiScoreEntry* entries) {
    if (FileExists("sz_hiscore.bin")) {
        int size = 0;
        unsigned char* data = LoadFileData("sz_hiscore.bin", &size);
        if (data) {
            for(int i=0; i<8; ++i) entries[i] = ((HiScoreEntry*)data)[i];
            UnloadFileData(data);
        }
    }
}
