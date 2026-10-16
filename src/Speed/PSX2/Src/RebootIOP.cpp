#include "Speed/Indep/Src/Misc/bFile.hpp"
#include "Speed/Indep/bWare/Inc/Strings.hpp"
#include "Speed/Indep/bWare/Inc/bChunk.hpp"
#include "Speed/Indep/bWare/Inc/bFunk.hpp"
#include "Speed/Indep/bWare/Inc/bPrintf.hpp"
#include "Speed/Indep/bWare/Inc/bWare.hpp"

#include <eekernel.h>
#include <libcdvd.h>
#include <sif.h>
#include <sifdev.h>
#include <sifrpc.h>

// LoadIRXModulesFromEEMemory, PowerOffThread, PowerOffHandler y PreparePowerOff tambien son de este fichero, pero el
// enlazador las quito de las dos PS2 y no queda codigo del que sacarlas.

// total size: 0x80
struct IRXBundleHeader {
    int IRXSize;       // offset 0x0, size 0x4
    int ArgSize;       // offset 0x4, size 0x4
    char Filename[24]; // offset 0x8, size 0x18
    char Args[96];     // offset 0x20, size 0x60
};

// total size: 0xC
struct IRXModuleLoadInfo {
    char *Name;  // offset 0x0, size 0x4
    int ArgSize; // offset 0x4, size 0x4
    char *Args;  // offset 0x8, size 0x4
};

bool TempBool1;
bool *pTempBool2 = &TempBool1;
bool BoolArray[3] = {true, false, true};
int ThereIsADiscInTheDVD;

// 1 si hay un CD y 2 si hay un DVD, dejando la unidad en el modo que toca; 0 si no hay disco de juego.
// Cada tipo de CD lleva su propio cuerpo: el original compara los cuatro uno a uno, y con las cuatro etiquetas
// seguidas GCC los habria juntado en un rango.
int bCheckIfDiskInDrive() {
    int diskType = sceCdGetDiskType();

    switch (diskType) {
    case SCECdNODISC:
    case SCECdDVDV:
        return 0;

    case SCECdDETCT:
    case SCECdUNKNOWN:
    case SCECdCDDA:
    case SCECdIllgalMedia:
        break;

    case SCECdPS2DVD:
        sceCdMmode(SCECdDVD);
        return 2;

    case SCECdPSCD:
        sceCdMmode(SCECdCD);
        return 1;

    case SCECdPSCDDA:
        sceCdMmode(SCECdCD);
        return 1;

    case SCECdPS2CD:
        sceCdMmode(SCECdCD);
        return 1;

    case SCECdPS2CDDA:
        sceCdMmode(SCECdCD);
        return 1;

    default:
        return 0;
    }

    return 0;
}

int bIsThereACDInTheDrive() {
    return ThereIsADiscInTheDVD;
}

void InitializeDVD() {
    sceSifInitRpc(0);

    sceCdInit(SCECdINIT);

    ThereIsADiscInTheDVD = bCheckIfDiskInDrive();

    sceFsReset();
}

char *bGetIOPFileSystemPrefix() {
    if (ThereIsADiscInTheDVD) {
        return "cdrom:\\";
    }

    return "host:";
}

// Con la herramienta de Sony (DTL-T10000) conectada avisa a CODEINE antes y despues del reinicio: las funciones 53 y
// 54, cuyo nombre no sale en ningun volcado.
void MyRebootIOP(const char *image_filename) {
    char sImageName[256];

    InitializeDVD();

    if (bIsPlatDTL_T10000() && bIsBFunkAvailable()) {
        bFunkCallSync("CODEINE", 53, 0, 0, 0, 0);
    }

    bSPrintf(sImageName, "%s%s", bGetIOPFileSystemPrefix(), image_filename);

    while (!sceSifRebootIop(sImageName)) {
    }

    while (!sceSifSyncIop()) {
    }

    if (bIsPlatDTL_T10000() && bIsBFunkAvailable()) {
        bFunkCallSync("CODEINE", 54, 0, 0, 0, 0);
    }

    InitializeDVD();
}

int LoadIRXModules(const char *path, const IRXModuleLoadInfo *info, int num_infos) {
    int i;
    int ret = 0;

    for (i = 0; i < num_infos; i++) {
        char filename[256];

        bSPrintf(filename, "%s%s%s", bGetIOPFileSystemPrefix(), path, info[i].Name);

        if (sceSifLoadModule(filename, info[i].ArgSize, info[i].Args) < 0) {
            ret |= 1;
        }
    }

    return ret;
}

void *LoadIOPFile(const char *filename, int *psize) {
    char full_filename[128];
    int rfd;
    int file_size;
    void *buf;

    bSPrintf(full_filename, "%s%s", bGetIOPFileSystemPrefix(), filename);

    rfd = sceOpen(full_filename, SCE_RDONLY);
    if (rfd < 0) {
        return 0;
    }

    file_size = sceLseek(rfd, 0, SCE_SEEK_END);
    sceLseek(rfd, 0, SCE_SEEK_SET);

    buf = bMalloc(file_size, full_filename, 0, 0x2000);

    sceRead(rfd, buf, file_size);
    sceClose(rfd);

    *psize = file_size;
    return buf;
}

bool NetworkFailedToLoad;

// El paquete es una lista de bChunk: en cada uno, una cabecera de 128 bytes alineada a 128 y detras el IRX, que se
// sube por DMA a la memoria del IOP y se carga desde alli.
bool LoadIRXModulesFromBundleFile(void *buf, int size) {
    FlushCache(0);

    sceSifInitIopHeap();

    bool got_error = false;

    bChunk *chunk = reinterpret_cast<bChunk *>(buf);
    bChunk *last_chunk = GetLastChunk(chunk, size);

    while (chunk < last_chunk) {
        IRXBundleHeader *header = reinterpret_cast<IRXBundleHeader *>(chunk->GetAlignedData(128));
        void *iop_addr = sceSifAllocSysMemory(1, header->IRXSize, 0);

        if (iop_addr == 0) {
            got_error = true;
        } else {
            sceSifDmaData sd;

            sd.data = reinterpret_cast<unsigned int>(header + 1);
            sd.addr = reinterpret_cast<unsigned int>(iop_addr);
            sd.size = header->IRXSize;
            sd.mode = 0;
            sceSifSetDma(&sd, 1);

            if (sceSifLoadModuleBuffer(iop_addr, header->ArgSize, header->Args) < 0) {
                if (bStrCmp(header->Filename, "SMAP.IRX") == 0) {
                    NetworkFailedToLoad = true;
                }
                got_error = true;
            }

            sceSifFreeSysMemory(iop_addr);
        }

        chunk = chunk->GetNext();
    }

    return !got_error;
}

// El prototipo arranca con la imagen del IOP de DNAS; la version final, con la normal.
void RebootIOP() {
#ifdef EA_BUILD_A124
    MyRebootIOP("NFS\\DNAS280.IMG");
#else
    MyRebootIOP("NFS\\IOPRP280.IMG");
#endif
}

// El original compara la misma cadena consigo misma (las dos son la misma direccion): seguramente una macro con el
// nombre de la imagen del IOP, cuyo nombre no sale en ningun volcado.
void LoadStartupIRXModules() {
    if (bStrICmp("IOPRP280.IMG", "IOPRP280.IMG") == 0) {
        int size;
        void *buf = LoadIOPFile("NFS\\IRX281A.BUN", &size);

        LoadIRXModulesFromBundleFile(buf, size);

        bFree(buf);
    }
}

void LoadNetworkIRXModules(bool connect_super_bender) {
    static bool firstTime = true;

    if (!firstTime) {
        return;
    }

    firstTime = false;

    IRXModuleLoadInfo info = {"DEV9.IRX", 0, 0};
    LoadIRXModules("NFS\\", &info, 1);

    int size;
    void *buf = LoadIOPFile("NFS\\IRX281N.BUN", &size);

    LoadIRXModulesFromBundleFile(buf, size);

    bFree(buf);

    if (connect_super_bender) {
        buf = LoadIOPFile("NFS\\IRX281N1.BUN", &size);

        LoadIRXModulesFromBundleFile(buf, size);

        bFree(buf);

        IRXModuleLoadInfo info = {"SBENDER.IRX", 0, 0};
        LoadIRXModules("NFS\\", &info, 1);
    }
}

static bool LoadNetworkIRXModulesPart2Done;

void LoadNetworkIRXModulesPart2() {
    int size;
    void *buf;

    if (!LoadNetworkIRXModulesPart2Done) {
        LoadNetworkIRXModulesPart2Done = true;

        buf = bGetFile("NFS\\IRX281N2.BUN", &size, 0);

        LoadIRXModulesFromBundleFile(buf, size);

        bFree(buf);
    }
}

void LoadRemainingIRXModules() {
    int size;
    void *buf = bGetFile("NFS\\IRX281B.BUN", &size, 64);

    LoadIRXModulesFromBundleFile(buf, size);

    bFree(buf);
}
