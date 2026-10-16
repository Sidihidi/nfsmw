#ifndef PC_ECSTASY_ELIGHT_PLAT_H
#define PC_ECSTASY_ELIGHT_PLAT_H

struct eLightMaterialPlatInterface {
    // w12c6: elInit hace rep stosd x42 (0xA8 B); sin esto son 0xA4. GC y PS2 tambien llevan PlatInfo.
    void *PlatInfo; // offset 0x0, size 0x4

    void CreatePlatInfo();
    void UpdatePlatInfo();
};

void elInitPlat();

// eLight.hpp (common code) calls it, as on the other platforms.
unsigned int PlatConvertColor(unsigned int colour);

#endif
