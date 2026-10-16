#include "Speed/Indep/Src/Misc/VersionNumber.hpp"

extern int BuildVersionChangelistNumber;

// Estripada en GameCube (Range 0xFFFFFFFF); la A124 la tiene.
void GetGameVersionNumberString(char *buffer, int bufsize) {
    char temp[100] = "";

    bSPrintf(temp, "%d.%d", GAME_VERSION_NUMBER_MAJOR, GAME_VERSION_NUMBER_MINOR);
    bStrCpy(buffer, temp);
}

// Estripada en GameCube y en la A124 (Range 0xFFFFFFFF); PC la tiene (0x64BB20).
void GetLongGameVersionNumberString(char *buffer, int bufsize) {
    char temp[100] = "";

    bSPrintf(temp, "%d.%d.%d", GAME_VERSION_NUMBER_MAJOR, GAME_VERSION_NUMBER_MINOR, BuildVersionChangelistNumber);
    bStrCpy(buffer, temp);
}
