// realcore 6.24.00: el lector de CD/DVD de PS2 (source/file/ps2/cddvd_device.cpp en el .mdebug de la A124). Tipos,
// locales y globales de sus stabs; el orden de las sentencias, de su mapa de lineas. Las etiquetas de las reservas
// (nombre, fichero y linea) son las del original.
#include "cddvd_device.h"

#include "Speed/Indep/Libs/realcore/6.24.00/include/common/realcore/file.h"
#include "Speed/Indep/Libs/realcore/6.24.00/include/common/realcore/std.h"
#include "Speed/Indep/Libs/realcore/6.24.00/include/common/realcore/std/list.h"
#include "Speed/Indep/Libs/realcore/6.24.00/include/common/realcore/system.h"
#include "Speed/Indep/Libs/realcore/6.24.00/include/common/realcore/system/systask.h"
#include "Allocator/iallocator.h"

#include <ctype.h>
#include <eekernel.h>
#include <libcdvd.h>
#include <stdlib.h>
#include <string.h>

extern FILESYSOPTS gFileSysOpts;

#define SECTOR_SIZE 2048

// Una entrada de directorio de ISO 9660, empaquetada: los enteros caen en direcciones impares.
struct ISO9660DIR {
    char DIR_entry_size;         // offset 0x0
    char DIR_extend_size;        // offset 0x1
    unsigned int intelsector;    // offset 0x2
    unsigned int motorolasector; // offset 0x6
    unsigned int intellength;    // offset 0xA
    unsigned int motorolalength; // offset 0xE
    char DIR_extent_date[7];     // offset 0x12
    char DIR_flags;              // offset 0x19
    char DIR_unit_size;          // offset 0x1A
    char DIR_interleave;         // offset 0x1B
    unsigned short DIR_L_sequence; // offset 0x1C
    unsigned short DIR_M_sequence; // offset 0x1E
    char DIR_name_length;        // offset 0x20
    char DIR_name[0];            // offset 0x21
} __attribute__((packed));

// El descriptor de volumen primario (sector 16).
struct ISO9660VOLUME {
    char VD_type;                     // offset 0x0
    char VD_standard_id[5];           // offset 0x1
    char VD_vol_version;              // offset 0x6
    char VD_unused1[1];               // offset 0x7
    char VD_system_id[32];            // offset 0x8
    char VD_volume_id[32];            // offset 0x28
    char VD_unused2[8];               // offset 0x48
    unsigned int VD_L_volume_size;    // offset 0x50
    unsigned int VD_M_volume_size;    // offset 0x54
    char VD_unused3[32];              // offset 0x58
    unsigned short VD_L_set_size;     // offset 0x78
    unsigned short VD_M_set_size;     // offset 0x7A
    unsigned short VD_L_sequence;     // offset 0x7C
    unsigned short VD_M_sequence;     // offset 0x7E
    unsigned short VD_L_block_size;   // offset 0x80
    unsigned short VD_M_block_size;   // offset 0x82
    unsigned int VD_L_path_size;      // offset 0x84
    unsigned int VD_M_path_size;      // offset 0x88
    unsigned int VD_L_path;           // offset 0x8C
    unsigned int VD_opt_L_path;       // offset 0x90
    unsigned int VD_M_path;           // offset 0x94
    unsigned int VD_opt_M_path;       // offset 0x98
    ISO9660DIR VD_root_dir;           // offset 0x9C
    char VD_root_dir_pad;             // offset 0xBD
    char VD_set_id[128];              // offset 0xBE
    char VD_publisher[128];           // offset 0x13E
    char VD_preparer[128];            // offset 0x1BE
    char VD_application[128];         // offset 0x23E
    char VD_copyright[37];            // offset 0x2BE
    char VD_abstract[37];             // offset 0x2E3
    char VD_bibliograph[37];          // offset 0x308
    char VD_create_date[17];          // offset 0x32D
    char VD_modify_date[17];          // offset 0x33E
    char VD_expire_date[17];          // offset 0x34F
    char VD_use_date[17];             // offset 0x360
    char VD_path_version;             // offset 0x371
    char VD_unused4[1];               // offset 0x372
    char VD_user[512];                // offset 0x373
    char VD_unused5[653];             // offset 0x573
} __attribute__((packed));

struct NAMEDDIRFILE {
    char name[256]; // offset 0x0
    int sector;     // offset 0x100
    int size;       // offset 0x104
};

struct DISCFILE {
    const char *name; // offset 0x0
    unsigned int pos; // offset 0x4
    unsigned int len; // offset 0x8
    const char *dir;  // offset 0xC
};

struct FILEINFO {
    unsigned int hashvalue; // offset 0x0
    int sector;             // offset 0x4
    int size;               // offset 0x8
};

struct CDFILEHANDLE : public ListNodeS {
    FILEINFO *fi; // offset 0x4
    int offset;   // offset 0x8
};

struct READINFO {
    int bytes;          // offset 0x0
    int bytesremaining; // offset 0x4
    int byteoffset;     // offset 0x8
    int sectorstart;    // offset 0xC
    int sectorcount;    // offset 0x10
    unsigned char *buf; // offset 0x14
};

struct CDINFO {
    uint32_t status;                        // offset 0x0
    int numfiles;                           // offset 0x4
    uint32_t hashseed;                      // offset 0x8
    int sectbufsectnum;                     // offset 0xC
    sceCdRMode readmode;                    // offset 0x10
    READINFO ri;                            // offset 0x14
    ListSingle<CDFILEHANDLE> FreeHandleQueue; // offset 0x2C
    FILEINFO *dirinfo;                      // offset 0x38
    CDFILEHANDLE *filehandlemem;            // offset 0x3C
    unsigned char *sectbuf;                 // offset 0x40
    int resetsignal;                        // offset 0x44
};

// La lectura adelantada: un anillo de sectores que se llena con sceCdReadChain (dos tramos y el terminador).
struct PrefetchInfo {
    int mBufferSize;            // offset 0x0
    unsigned char *mBuffer;     // offset 0x4
    int mStartSector;           // offset 0x8
    int mReadStartSector;       // offset 0xC
    int mStartIndex;            // offset 0x10
    int mIsReading;             // offset 0x14
    uint32_t mLSN1;             // offset 0x18
    uint32_t mSector1;          // offset 0x1C
    uint32_t mBuf1;             // offset 0x20
    uint32_t mLSN2;             // offset 0x24
    uint32_t mSector2;          // offset 0x28
    uint32_t mBuf2;             // offset 0x2C
    uint32_t mLSNTerminator;    // offset 0x30
    uint32_t mSectorTerminator; // offset 0x34
    uint32_t mBufTerminator;    // offset 0x38
};

static NAMEDDIRFILE *gNamedDirectory = 0;
static unsigned int gNumDirectoryEntries = 0;
CDINFO Cdinfo;
Ps2DvdFileDeviceDriver Ps2Dvd_fdd("cdrom0:");
PrefetchInfo DvdPrefetchInfo;
unsigned char cdbuffer[2048] __attribute__((aligned(64)));
char directorypath[256];
static int hashcollision;

void ReadSectorsSafe(int sector, int numsectors, void *buffer) {
    int ok;
    int retrycount = 10;
    int rs_flg;

    do {
        if (retrycount == 0) {
            sceCdInit(SCECdINIT);
            retrycount = 10;
            sceCdMmode(gFileSysOpts.DiscType);
        }
        sceCdDiskReady(0);
        while (!sceCdRead(sector, numsectors, buffer, &Cdinfo.readmode)) {
            DelayThread(2000);
        }
        do {
            rs_flg = sceCdSync(1);
            DelayThread(2000);
        } while (rs_flg);
        ok = sceCdGetError() == SCECdErNO;
        if (!ok) {
            retrycount--;
        }
    } while (!ok);
}

ISO9660DIR *GetDirectorySector(int sect, int rootSect, void *buf) {
    return reinterpret_cast<ISO9660DIR *>(static_cast<char *>(buf) + (sect - rootSect) * SECTOR_SIZE);
}

void ParseDirectory(int startsector, int directorylength, int rootStartSector, void *isoDirectoryBuffer) {
    ISO9660DIR *d;
    NAMEDDIRFILE *ndf;
    char filename[16];
    char olddirectorypath[256];

    d = GetDirectorySector(startsector, rootStartSector, isoDirectoryBuffer);
    directorylength--;
    d = reinterpret_cast<ISO9660DIR *>(reinterpret_cast<char *>(d) + d->DIR_entry_size);
    d = reinterpret_cast<ISO9660DIR *>(reinterpret_cast<char *>(d) + d->DIR_entry_size);
    for (;;) {
        while (d->DIR_entry_size != 0) {
            if (d->DIR_flags & 2) {
                MEM_copy(filename, d->DIR_name, d->DIR_name_length);
                filename[d->DIR_name_length] = '/';
                filename[d->DIR_name_length + 1] = 0;
                strcpy(olddirectorypath, directorypath);
                strcat(directorypath, filename);
                ParseDirectory(d->intelsector, (d->intellength + SECTOR_SIZE - 1) / SECTOR_SIZE, rootStartSector,
                               isoDirectoryBuffer);
                strcpy(directorypath, olddirectorypath);
            } else {
                MEM_copy(filename, d->DIR_name, d->DIR_name_length - 2);
                filename[d->DIR_name_length - 2] = 0;
                if (filename[d->DIR_name_length - 3] == '.') {
                    filename[d->DIR_name_length - 3] = 0;
                }
                ndf = &gNamedDirectory[gNumDirectoryEntries];
                strcpy(ndf->name, directorypath);
                strcat(ndf->name, filename);
                ndf->sector = d->intelsector;
                ndf->size = d->intellength;
                gNumDirectoryEntries++;
            }
            d = reinterpret_cast<ISO9660DIR *>(reinterpret_cast<char *>(d) + d->DIR_entry_size);
        }
        if (directorylength == 0) {
            break;
        }
        d = GetDirectorySector(++startsector, rootStartSector, isoDirectoryBuffer);
        directorylength--;
    }
}

static bool StartReadWithRetry() {
    int i;
    int stat;
    READINFO *r = &Cdinfo.ri;
    unsigned char *buf;

    if (Cdinfo.status & 1) {
        buf = Cdinfo.sectbuf;
    } else {
        buf = r->buf;
    }
    i = 0;
    stat = 0;
    while (i < gFileSysOpts.mErrorRetryCount) {
        stat = sceCdRead(r->sectorstart, r->sectorcount, buf, &Cdinfo.readmode);
        DelayThread(2000);
        if (stat == 1) {
            sceCdSync(0);
            if (sceCdGetError() == SCECdErNO) {
                break;
            }
        }
        i++;
    }
    return stat == 1;
}

static void DvdRead() {
    READINFO *r = &Cdinfo.ri;

    while (r->bytesremaining > 0) {
        if (!StartReadWithRetry()) {
            Cdinfo.status |= 2;
            WaitSema(Cdinfo.resetsignal);
        }
        if (Cdinfo.status & 1) {
            Cdinfo.status &= ~1;
            MEM_copy(r->buf, Cdinfo.sectbuf + r->byteoffset, r->bytes);
            Cdinfo.sectbufsectnum = r->sectorstart;
        }
        r->buf += r->bytes;
        r->bytesremaining -= r->bytes;
        r->sectorstart += r->sectorcount;
        r->byteoffset = 0;
        if (r->bytesremaining < SECTOR_SIZE) {
            r->bytes = r->bytesremaining;
            r->sectorcount = 1;
            Cdinfo.status |= 1;
        } else if (reinterpret_cast<unsigned int>(r->buf) & 0x3F) {
            r->sectorcount = 1;
            Cdinfo.status |= 1;
            r->bytes = SECTOR_SIZE;
        } else {
            r->bytes = r->bytesremaining & ~(SECTOR_SIZE - 1);
            r->sectorcount = r->bytes >> 11;
            Cdinfo.status &= ~1;
        }
    }
}

bool CDVD_ResetNeeded() {
    return (Cdinfo.status & 2) != 0;
}

int CDVD_resettask() {
    int rs_flg;
    int wait0 = 0;
    READINFO ri;

    MEM_copy(&ri, &Cdinfo.ri, sizeof(ri));
    do {
        wait0++;
        while (!sceCdInit(SCECdINIT)) {
            DelayThread(30000);
            while (!sceCdInit(SCECdEXIT)) {
                DelayThread(30000);
            }
        }
        sceCdMmode(gFileSysOpts.DiscType);
        DelayThread(wait0 * 2000);
        while (!sceCdRead(16, 1, Cdinfo.sectbuf, &Cdinfo.readmode)) {
            DelayThread(wait0 * 2000);
        }
        do {
            rs_flg = sceCdSync(1);
            DelayThread(wait0 * 2000);
        } while (rs_flg);
        if (sceCdGetError() == SCECdErNO) {
            while (ri.sectorcount > 0) {
                sceCdDiskReady(0);
                while (!sceCdRead(ri.sectorstart, 1, (Cdinfo.status & 1) ? Cdinfo.sectbuf : ri.buf,
                                  &Cdinfo.readmode)) {
                    DelayThread(wait0 * 2000);
                }
                do {
                    rs_flg = sceCdSync(1);
                    DelayThread(wait0 * 2000);
                } while (rs_flg);
                if (sceCdGetError() != SCECdErNO) {
                    DelayThread(30000);
                    break;
                }
                ri.sectorstart++;
                ri.sectorcount--;
                ri.buf += SECTOR_SIZE;
                ri.bytes -= SECTOR_SIZE;
            }
        }
    } while (sceCdGetError() != SCECdErNO);
    Cdinfo.status &= ~2;
    SignalSema(Cdinfo.resetsignal);
    return 0;
}

void CDVD_watchdogtimeout(void *, int) {
    if (Cdinfo.status & 2) {
        CDVD_resettask();
    }
}

static int fi_cmp(const void *arg1, const void *arg2) {
    long retval = static_cast<long>(static_cast<const FILEINFO *>(arg1)->hashvalue) -
                  static_cast<long>(static_cast<const FILEINFO *>(arg2)->hashvalue);

    if (retval < 0) {
        return -1;
    }
    if (retval > 0) {
        return 1;
    }
    hashcollision++;
    return 0;
}

static unsigned int CDVD_hashname(const char *szKey, unsigned int seed) {
    unsigned int nHash = seed;
    unsigned int c;
    const char *s = szKey + strlen(szKey) - 1;

    do {
        c = *s--;
        nHash = nHash * 33 + c;
    } while (s >= szKey);
    return nHash;
}

static unsigned int ComputeHashNames(NAMEDDIRFILE *ndf, FILEINFO *fi, int nEntries, unsigned int seed) {
    int i;
    bool seedok = false;
    FILEINFO *lf;
    NAMEDDIRFILE *ln;

    do {
        lf = fi;
        ln = ndf;
        for (i = 0; i < nEntries; i++) {
            lf->hashvalue = CDVD_hashname(ln->name, seed);
            lf->sector = ln->sector;
            lf->size = ln->size;
            lf++;
            ln++;
        }
        hashcollision = 0;
        qsort(fi, nEntries, sizeof(FILEINFO), fi_cmp);
        if (hashcollision) {
            seed = seed * 101 + 7;
        } else {
            seedok = true;
        }
    } while (!seedok);
    return seed;
}

static CDFILEHANDLE *iAllocateCdFileHandle() {
    return Cdinfo.FreeHandleQueue.Pop();
}

static void iFreeCdFileHandle(CDFILEHANDLE *cd_h) {
    MEM_clear(cd_h, sizeof(CDFILEHANDLE));
    Cdinfo.FreeHandleQueue.Push(cd_h);
}

bool Ps2DvdFileDeviceDriver::Init() {
    CDFILEHANDLE *f;
    SemaParam sp;
    NAMEDDIRFILE *ndf;
    DISCFILE *df;
    char path[256];
    char filename[16];
    char *c;
    int rootDirectorySector;
    int rootDirectorySize;
    void *IsoDirectoryBuf;
    ISO9660DIR *d;
    int i;

    MEM_fill(&Cdinfo, 0, sizeof(Cdinfo));
    Cdinfo.readmode.trycount = 0;
    Cdinfo.readmode.spindlctrl = SCECdSpinNom;
    Cdinfo.readmode.datapattern = SCECdSecS2048;
    Cdinfo.ri.sectorstart = 16;
    FlushCache(0);
    gNamedDirectory = static_cast<NAMEDDIRFILE *>(gFileSysOpts.allocator->Alloc(
        gFileSysOpts.MaxNumDiscFiles * sizeof(NAMEDDIRFILE),
        EA::TagValuePair(EA::Allocator::ATT_NAME, "Named Dir") +
            EA::TagValuePair(EA::Allocator::ATT_FILE, "d:/packages/realcore/6.24.00/source/file/ps2/cddvd_device.cpp") +
            EA::TagValuePair(EA::Allocator::ATT_LINE, 831)));
    MEM_clear(gNamedDirectory, gFileSysOpts.MaxNumDiscFiles * sizeof(NAMEDDIRFILE));
    if (gFileSysOpts.dfTable != 0) {
        ndf = gNamedDirectory;
        df = gFileSysOpts.dfTable;
        while (df->name[0] != 0) {
            strcpy(path, df->dir + 7);
            for (c = path; *c != 0; c++) {
                if (*c == '\\') {
                    *c = '/';
                }
            }
            strcpy(filename, df->name);
            filename[strlen(df->name) - 2] = 0;
            strcat(path, filename);
            strcpy(ndf->name, path);
            ndf->sector = df->pos;
            ndf->size = df->len;
            df++;
            gNumDirectoryEntries++;
            ndf++;
        }
    } else {
        ReadSectorsSafe(Cdinfo.ri.sectorstart, 1, cdbuffer);
        directorypath[0] = '/';
        directorypath[1] = 0;
        if (gFileSysOpts.DemoCdRoot[0] != 0) {
            Cdinfo.ri.sectorstart = reinterpret_cast<ISO9660VOLUME *>(cdbuffer)->VD_root_dir.intelsector;
            ReadSectorsSafe(Cdinfo.ri.sectorstart, 1, cdbuffer);
            d = reinterpret_cast<ISO9660DIR *>(cdbuffer);
            while (reinterpret_cast<unsigned char *>(d) < cdbuffer + sizeof(cdbuffer)) {
                if (strncmp(d->DIR_name, gFileSysOpts.DemoCdRoot, d->DIR_name_length) == 0) {
                    break;
                }
                d = reinterpret_cast<ISO9660DIR *>(reinterpret_cast<char *>(d) + d->DIR_entry_size);
            }
            strcat(directorypath, gFileSysOpts.DemoCdRoot);
            strcat(directorypath, "/");
            rootDirectorySector = d->intelsector;
            rootDirectorySize = (d->intellength + SECTOR_SIZE - 1) / SECTOR_SIZE;
        } else {
            rootDirectorySector = reinterpret_cast<ISO9660VOLUME *>(cdbuffer)->VD_root_dir.intelsector;
            rootDirectorySize =
                (reinterpret_cast<ISO9660VOLUME *>(cdbuffer)->VD_root_dir.intellength + SECTOR_SIZE - 1) / SECTOR_SIZE;
        }
        IsoDirectoryBuf =
            gFileSysOpts.allocator->Alloc(500 * SECTOR_SIZE, EA::TagValuePair(EA::Allocator::ATT_ALIGNMENT, 64));
        ReadSectorsSafe(rootDirectorySector, 500, IsoDirectoryBuf);
        ParseDirectory(rootDirectorySector, rootDirectorySize, rootDirectorySector, IsoDirectoryBuf);
        gFileSysOpts.allocator->Free(IsoDirectoryBuf, 0);
    }
    if (gFileSysOpts.MaxNumDiscFiles < gNumDirectoryEntries) {
        gFileSysOpts.allocator->Free(gNamedDirectory, 0);
        gNamedDirectory = 0;
        gNumDirectoryEntries = 0;
        return false;
    }
    Cdinfo.dirinfo = static_cast<FILEINFO *>(gFileSysOpts.allocator->Alloc(
        (gNumDirectoryEntries + gFileSysOpts.MaxOpenFiles) * sizeof(FILEINFO),
        EA::TagValuePair(EA::Allocator::ATT_ALLOC_HIGH, 1) +
            EA::TagValuePair(EA::Allocator::ATT_NAME, "Directory buffer") +
            EA::TagValuePair(EA::Allocator::ATT_FILE, "d:/packages/realcore/6.24.00/source/file/ps2/cddvd_device.cpp") +
            EA::TagValuePair(EA::Allocator::ATT_LINE, 949)));
    Cdinfo.numfiles = gNumDirectoryEntries;
    MEM_clear(Cdinfo.dirinfo, (gNumDirectoryEntries + gFileSysOpts.MaxOpenFiles) * sizeof(FILEINFO));
    Cdinfo.hashseed = ComputeHashNames(gNamedDirectory, Cdinfo.dirinfo, gNumDirectoryEntries, gFileSysOpts.hashseed);
    Cdinfo.filehandlemem = reinterpret_cast<CDFILEHANDLE *>(Cdinfo.dirinfo + gNumDirectoryEntries);
    f = Cdinfo.filehandlemem;
    for (i = 0; i < gFileSysOpts.MaxOpenFiles; i++) {
        Cdinfo.FreeHandleQueue.Push(f);
        f++;
    }
    gFileSysOpts.allocator->Free(gNamedDirectory, 0);
    gNamedDirectory = 0;
    gNumDirectoryEntries = 0;
    Cdinfo.sectbuf = cdbuffer;
    sp.maxCount = 1;
    sp.initCount = 0;
    Cdinfo.resetsignal = CreateSema(&sp);
    SYNCTASK_add(CDVD_watchdogtimeout, 0, 0, 0);
    MEM_clear(&DvdPrefetchInfo, sizeof(DvdPrefetchInfo));
    return true;
}

void Ps2DvdFileDeviceDriver::Restore() {
    DeleteSema(Cdinfo.resetsignal);
    Cdinfo.resetsignal = 0;
    SYNCTASK_del(CDVD_watchdogtimeout);
    if (Cdinfo.dirinfo != 0) {
        gFileSysOpts.allocator->Free(Cdinfo.dirinfo, 0);
    }
    MEM_clear(&Cdinfo, sizeof(Cdinfo));
    sceCdInit(SCECdEXIT);
}

EAFileHandle Ps2DvdFileDeviceDriver::Open(const char *name, int oflags, int *pParentFileHandle) {
    char disc_fname[256];
    char *pdiscname = disc_fname;
    CDFILEHANDLE *cd_fh;
    int hashval;

    cd_fh = iAllocateCdFileHandle();
    if (oflags & 1) {
        if (strncmp(name, "cdrom0:", 7) == 0) {
            name += 7;
        }
        while (*name != 0) {
            if (*name == '\\') {
                *pdiscname = '/';
            } else {
                *pdiscname = toupper(*name);
            }
            name++;
            pdiscname++;
        }
        *pdiscname = 0;
        hashval = CDVD_hashname(disc_fname, Cdinfo.hashseed);
        cd_fh->fi = static_cast<FILEINFO *>(bsearch(&hashval, Cdinfo.dirinfo, Cdinfo.numfiles, sizeof(FILEINFO), fi_cmp));
    }
    if (cd_fh->fi == 0) {
        iFreeCdFileHandle(cd_fh);
        return -1;
    }
    return reinterpret_cast<EAFileHandle>(cd_fh);
}

void Ps2DvdFileDeviceDriver::Close(EAFileHandle h) {
    iFreeCdFileHandle(reinterpret_cast<CDFILEHANDLE *>(h));
}

uint32_t Ps2DvdFileDeviceDriver::Read(EAFileHandle h, void *buf, unsigned int bufsize, DeviceDriver *ddParent,
                                      EAFileHandle ddFileHandle) {
    int bytes;
    bool doDvdRead = true;
    READINFO *r = &Cdinfo.ri;

    bytes = reinterpret_cast<CDFILEHANDLE *>(h)->fi->size - reinterpret_cast<CDFILEHANDLE *>(h)->offset;
    if (bytes >= static_cast<int>(bufsize)) {
        bytes = bufsize;
    }
    r->byteoffset = reinterpret_cast<CDFILEHANDLE *>(h)->offset % SECTOR_SIZE;
    r->buf = static_cast<unsigned char *>(buf);
    if (r->byteoffset != 0) {
        Cdinfo.status |= 1;
        r->sectorcount = 1;
        r->bytes = (r->byteoffset + bytes > SECTOR_SIZE) ? SECTOR_SIZE - r->byteoffset : bytes;
    } else if (bytes < SECTOR_SIZE) {
        Cdinfo.status |= 1;
        r->sectorcount = 1;
        r->bytes = bytes;
    } else if (reinterpret_cast<unsigned int>(buf) & 0x3F) {
        Cdinfo.status |= 1;
        r->sectorcount = 1;
        r->bytes = SECTOR_SIZE;
    } else {
        Cdinfo.status &= ~1;
        r->bytes = bytes & ~(SECTOR_SIZE - 1);
        r->sectorcount = r->bytes >> 11;
    }
    r->bytesremaining = bytes;
    r->sectorstart = reinterpret_cast<CDFILEHANDLE *>(h)->fi->sector + reinterpret_cast<CDFILEHANDLE *>(h)->offset / SECTOR_SIZE;
    if (r->sectorstart == Cdinfo.sectbufsectnum && r->bytes <= SECTOR_SIZE) {
        MEM_copy(r->buf, Cdinfo.sectbuf + r->byteoffset, r->bytes);
        r->bytesremaining -= r->bytes;
        r->byteoffset = 0;
        r->buf += r->bytes;
        r->sectorstart++;
        if (r->bytesremaining > 0) {
            if (r->bytesremaining < SECTOR_SIZE) {
                Cdinfo.status |= 1;
                r->sectorcount = 1;
                r->bytes = r->bytesremaining;
            } else if (reinterpret_cast<unsigned int>(r->buf) & 0x3F) {
                Cdinfo.status |= 1;
                r->sectorcount = 1;
                r->bytes = SECTOR_SIZE;
            } else {
                Cdinfo.status &= ~1;
                r->bytes = r->bytesremaining & ~(SECTOR_SIZE - 1);
                r->sectorcount = r->bytes >> 11;
            }
        } else {
            doDvdRead = false;
        }
    }
    if (doDvdRead) {
        if (this->CheckReadAheadBuffer()) {
            PRINT_string(PRINT_FILE, "DvdRead: Data is not prebuffered; doing a normal read\n");
            DvdRead();
        }
    }
    this->FillReadAheadBuffer();
    reinterpret_cast<CDFILEHANDLE *>(h)->offset += bytes;
    return bytes;
}

uint64_t Ps2DvdFileDeviceDriver::Seek(EAFileHandle h, uint64_t offset, int whence, DeviceDriver *ddParent,
                                      EAFileHandle ddFileHandle) {
    CDFILEHANDLE *cd_fh = reinterpret_cast<CDFILEHANDLE *>(h);

    switch (whence) {
    case 0:
        cd_fh->offset = offset;
        break;
    case 1:
        cd_fh->offset += static_cast<int>(offset);
        break;
    case 2:
        cd_fh->offset = cd_fh->fi->size - static_cast<int>(offset);
        break;
    }
    return offset;
}

uint64_t Ps2DvdFileDeviceDriver::Getsize(EAFileHandle h) {
    return reinterpret_cast<CDFILEHANDLE *>(h)->fi->size;
}

uint64_t Ps2DvdFileDeviceDriver::QueryLocation(EAFileHandle h) {
    uint64_t file_location = reinterpret_cast<CDFILEHANDLE *>(h)->fi->sector;

    return file_location * SECTOR_SIZE;
}

bool Ps2DvdFileDeviceDriver::CheckReadAheadBuffer() {
    bool doDvdRead = true;
    READINFO *r = &Cdinfo.ri;
    int numRequiredSectors;
    int startIndex;
    unsigned char *srcbuf;
    int srcbytes;

    if (DvdPrefetchInfo.mBufferSize != 0) {
        numRequiredSectors = (r->byteoffset + r->bytes + r->bytesremaining + SECTOR_SIZE - 1) / SECTOR_SIZE;
        if (r->sectorstart >= DvdPrefetchInfo.mStartSector &&
            r->sectorstart + numRequiredSectors <= DvdPrefetchInfo.mStartSector + DvdPrefetchInfo.mBufferSize) {
            startIndex = DvdPrefetchInfo.mStartIndex + (r->sectorstart - DvdPrefetchInfo.mStartSector);
            if (startIndex >= DvdPrefetchInfo.mBufferSize) {
                startIndex -= DvdPrefetchInfo.mBufferSize;
            }
            srcbuf = DvdPrefetchInfo.mBuffer + startIndex * SECTOR_SIZE + r->byteoffset;
            srcbytes = r->bytes;
            while (r->bytesremaining != 0) {
                if (DvdPrefetchInfo.mIsReading) {
                    if (srcbuf < DvdPrefetchInfo.mBuffer + DvdPrefetchInfo.mStartIndex * SECTOR_SIZE ||
                        srcbuf + srcbytes >= DvdPrefetchInfo.mBuffer +
                                                 (DvdPrefetchInfo.mStartIndex + DvdPrefetchInfo.mReadStartSector -
                                                  DvdPrefetchInfo.mStartSector) *
                                                     SECTOR_SIZE) {
                        if (sceCdSync(1)) {
                            sceCdSync(0);
                        }
                        if (sceCdGetError() != SCECdErNO) {
                            break;
                        }
                        DvdPrefetchInfo.mIsReading = 0;
                        DvdPrefetchInfo.mReadStartSector = 0;
                    }
                }
                if (srcbuf == DvdPrefetchInfo.mBuffer + DvdPrefetchInfo.mBufferSize * SECTOR_SIZE) {
                    srcbuf = DvdPrefetchInfo.mBuffer;
                }
                if (DvdPrefetchInfo.mBuffer + DvdPrefetchInfo.mBufferSize * SECTOR_SIZE - srcbuf < srcbytes) {
                    srcbytes = DvdPrefetchInfo.mBuffer + DvdPrefetchInfo.mBufferSize * SECTOR_SIZE - srcbuf;
                }
                memcpy(r->buf, srcbuf, srcbytes);
                srcbuf += srcbytes;
                r->buf += srcbytes;
                r->bytesremaining -= srcbytes;
                r->sectorstart += (r->byteoffset + srcbytes) / SECTOR_SIZE;
                r->byteoffset = 0;
                srcbytes = r->bytesremaining;
            }
            if (r->bytesremaining == 0) {
                doDvdRead = false;
            }
        } else if (DvdPrefetchInfo.mIsReading) {
            sceCdBreak();
            sceCdSync(0);
            DvdPrefetchInfo.mIsReading = 0;
            DvdPrefetchInfo.mStartSector = 0;
        }
    }
    return doDvdRead;
}

void Ps2DvdFileDeviceDriver::FillReadAheadBuffer() {
    int currentSector;
    int numValidSectors;
    int lastStartIndex;
    int numReadSectors;
    int spaceToBufferEnd;
    int numInvalidSectors;

    if (DvdPrefetchInfo.mBufferSize != 0) {
        if (DvdPrefetchInfo.mIsReading) {
            DvdPrefetchInfo.mIsReading = sceCdSync(1);
        }
        currentSector = Cdinfo.ri.sectorstart;
        if (!DvdPrefetchInfo.mIsReading && currentSector != DvdPrefetchInfo.mStartSector) {
            if (currentSector < DvdPrefetchInfo.mStartSector ||
                currentSector > DvdPrefetchInfo.mStartSector + DvdPrefetchInfo.mBufferSize) {
                DvdPrefetchInfo.mStartIndex = 0;
                lastStartIndex = 0;
                DvdPrefetchInfo.mStartSector = currentSector;
                DvdPrefetchInfo.mReadStartSector = currentSector;
            } else {
                numInvalidSectors = currentSector - DvdPrefetchInfo.mStartSector;
                lastStartIndex = DvdPrefetchInfo.mStartIndex;
                DvdPrefetchInfo.mStartSector += numInvalidSectors;
                DvdPrefetchInfo.mStartIndex += numInvalidSectors;
                DvdPrefetchInfo.mReadStartSector =
                    DvdPrefetchInfo.mStartSector + DvdPrefetchInfo.mBufferSize - numInvalidSectors;
                if (DvdPrefetchInfo.mStartIndex >= DvdPrefetchInfo.mBufferSize) {
                    DvdPrefetchInfo.mStartIndex -= DvdPrefetchInfo.mBufferSize;
                }
            }
            numValidSectors = lastStartIndex - DvdPrefetchInfo.mStartIndex;
            if (numValidSectors < 0) {
                numValidSectors += DvdPrefetchInfo.mBufferSize;
            }
            numReadSectors = DvdPrefetchInfo.mBufferSize - numValidSectors;
            spaceToBufferEnd = DvdPrefetchInfo.mBufferSize - lastStartIndex;
            if (spaceToBufferEnd < numReadSectors) {
                DvdPrefetchInfo.mLSN1 = DvdPrefetchInfo.mReadStartSector;
                DvdPrefetchInfo.mSector1 = spaceToBufferEnd;
                DvdPrefetchInfo.mBuf1 =
                    reinterpret_cast<uint32_t>(DvdPrefetchInfo.mBuffer + lastStartIndex * SECTOR_SIZE);
                DvdPrefetchInfo.mLSN2 = DvdPrefetchInfo.mReadStartSector + spaceToBufferEnd;
                DvdPrefetchInfo.mSector2 = numReadSectors - spaceToBufferEnd;
                DvdPrefetchInfo.mBuf2 = reinterpret_cast<uint32_t>(DvdPrefetchInfo.mBuffer);
            } else {
                DvdPrefetchInfo.mLSN1 = DvdPrefetchInfo.mReadStartSector;
                DvdPrefetchInfo.mSector1 = numReadSectors;
                DvdPrefetchInfo.mBuf1 =
                    reinterpret_cast<uint32_t>(DvdPrefetchInfo.mBuffer + lastStartIndex * SECTOR_SIZE);
                DvdPrefetchInfo.mLSN2 = DvdPrefetchInfo.mSector2 = DvdPrefetchInfo.mBuf2 = 0xFFFFFFFF;
            }
            DvdPrefetchInfo.mIsReading =
                sceCdReadChain(reinterpret_cast<sceCdRChain *>(&DvdPrefetchInfo.mLSN1), &Cdinfo.readmode);
            if (!DvdPrefetchInfo.mIsReading) {
                DvdPrefetchInfo.mReadStartSector = 0;
                DvdPrefetchInfo.mStartSector = 0;
            }
        }
    }
}
