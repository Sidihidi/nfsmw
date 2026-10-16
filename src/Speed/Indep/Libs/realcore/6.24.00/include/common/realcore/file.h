#ifndef REALCORE_COMMON_FILE_H
#define REALCORE_COMMON_FILE_H

// La API de ficheros de realcore que configura el juego al arrancar (psx2/src/platform.cpp, InitRealFile). Tipos y
// nombres de los stabs de la A124. Los .cpp de realcore aun declaran FILESYSOPTS cada uno por su cuenta y no incluyen
// este fichero.

namespace EA {
namespace Allocator {
class IAllocator;
}
} // namespace EA

struct DISCFILE;

// total size: 0x50 en PS2 (stabs de la A124)
struct FILESYSOPTS {
    int size;                                // offset 0x0, size 0x4
    EA::Allocator::IAllocator *allocator;    // offset 0x4, size 0x4
    int MaxOpenFiles;                        // offset 0x8, size 0x4
    int MaxFileOps;                          // offset 0xC, size 0x4
    int nSearchLocs;                         // offset 0x10, size 0x4
    int nSearchPathLength;                   // offset 0x14, size 0x4
    int MaxDevices;                          // offset 0x18, size 0x4
    int ThreadStackSize;                     // offset 0x1C, size 0x4
    int (*decompresssize)(const void *);     // offset 0x20, size 0x4
    int (*decompress)(const void *, void *); // offset 0x24, size 0x4
    unsigned int LargeReadSliceSize;         // offset 0x28, size 0x4
    unsigned int AllocAlignBoundary;         // offset 0x2C, size 0x4
    int DiscType;                            // offset 0x30, size 0x4
#ifdef EA_PLATFORM_PLAYSTATION2
    bool IsDiscBootable;              // offset 0x34, size 0x4
    const char *DemoCdRoot;           // offset 0x38, size 0x4
    unsigned int MaxNumDiscFiles;     // offset 0x3C, size 0x4
    unsigned int hashseed;            // offset 0x40, size 0x4
    DISCFILE *dfTable;                // offset 0x44, size 0x4
    bool bUseTMFSRootDirAsAppRootDir; // offset 0x48, size 0x4
#endif
    int mErrorRetryCount; // offset 0x4C, size 0x4
};

bool FILE_getopts(FILESYSOPTS *pfso);
bool FILE_setopts(FILESYSOPTS *pfso);
bool FILE_init(void *buf, int bufsize);

#ifdef EA_PLATFORM_PLAYSTATION2
// El dispositivo del lector de PS2: la tarea que vigila la bandeja y el reinicio del lector tras abrirla.
bool CDVD_ResetNeeded();
int CDVD_resettask();
void CDVD_watchdogtimeout(void *, int);
#endif

#endif
