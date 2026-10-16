// Prototipos locales (linkage C++ manglado, como la TU original).
void MEM_clear(void *ptr, int size);
void MEM_copy(void *dst, const void *src, int size);

struct FILESYSOPTS {
    int size;                              // +0x00
    struct IAllocator *allocator;          // +0x04
    int MaxOpenFiles;                      // +0x08
    int MaxFileOps;                        // +0x0C
    int nSearchLocs;                       // +0x10
    int nSearchPathLength;                 // +0x14
    int MaxDevices;                        // +0x18
    int ThreadStackSize;                   // +0x1C
    int (*decompresssize)(const void *);   // +0x20
    int (*decompress)(const void *, void *); // +0x24
    unsigned int LargeReadSliceSize;       // +0x28
    unsigned int AllocAlignBoundary;       // +0x2C
    int DiscType;                          // +0x30
#ifdef EA_PLATFORM_PLAYSTATION2
    // En PS2 la estructura mide 0x50 (stabs de la A124): los datos del disco van entre DiscType y mErrorRetryCount.
    bool IsDiscBootable;                     // +0x34
    const char *DemoCdRoot;                  // +0x38
    unsigned int MaxNumDiscFiles;            // +0x3C
    unsigned int hashseed;                   // +0x40
    struct DISCFILE *dfTable;                // +0x44
    bool bUseTMFSRootDirAsAppRootDir;        // +0x48
    int mErrorRetryCount;                    // +0x4C
#else
    int mErrorRetryCount;                    // +0x34
#endif
};

struct TVPALIGN {
    int mAlign;
    int mTvp;
    int mFlag;

    TVPALIGN() {
        mAlign = 2;
        mTvp = 0x80;
        mFlag = 0;
    }
};

extern FILESYSOPTS gFileSysOpts;
extern int bIsFileSysInitialized;
extern TVPALIGN FileAlignTvp;

bool FILE_getopts(FILESYSOPTS *pfso) {
    int ssize = pfso->size;

    if (ssize > static_cast<int>(sizeof(FILESYSOPTS))) {
        MEM_clear(pfso, ssize);
    }
    MEM_copy(pfso, &gFileSysOpts, ssize);
    return true;
}

bool FILE_setopts(FILESYSOPTS *pfso) {
    if (pfso->size < static_cast<int>(sizeof(FILESYSOPTS))) {
        MEM_clear(&gFileSysOpts, sizeof(FILESYSOPTS));
    }
    MEM_copy(&gFileSysOpts, pfso, pfso->size);
    FileAlignTvp.mTvp = pfso->AllocAlignBoundary;
    return true;
}

// Los valores de PS2 son los del .data de la A124 y de SLUS (iguales en las dos).
FILESYSOPTS gFileSysOpts = {
#ifdef EA_PLATFORM_PLAYSTATION2
    sizeof(FILESYSOPTS), 0, 8, 0x80, 2, 0x400, 4, 0x2000, 0, 0, 0x8000, 0x80, 0, false, "", 0x400, 0x1505, 0, false, 3,
#else
    sizeof(FILESYSOPTS), 0, 8, 0x80, 2, 0x400, 4, 0x1000, 0, 0, 0x8000, 0x80, 1, 0,
#endif
};
int bIsFileSysInitialized = 0;
TVPALIGN FileAlignTvp;
