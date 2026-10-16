#ifndef _LIBMC_H
#define _LIBMC_H

// La libmc del SDK de Sony que usan el juego (el icon.sys de MemoryCardImp::ConstructSaveInfo) y realmemcard.
// Los layouts son los de los stabs de la A124 (sceMcIconSys mide 0x3C4, sceMcTblGetDir 0x40).

typedef int _iconVu0IVECTOR[4];
typedef float _iconVu0FVECTOR[4];

typedef struct {
    unsigned char Head[4];          // offset 0x0, size 0x4
    unsigned short Reserv1;         // offset 0x4, size 0x2
    unsigned short OffsLF;          // offset 0x6, size 0x2
    unsigned int Reserv2;           // offset 0x8, size 0x4
    unsigned int TransRate;         // offset 0xC, size 0x4
    _iconVu0IVECTOR BgColor[4];     // offset 0x10, size 0x40
    _iconVu0FVECTOR LightDir[3];    // offset 0x50, size 0x30
    _iconVu0FVECTOR LightColor[3];  // offset 0x80, size 0x30
    _iconVu0FVECTOR Ambient;        // offset 0xB0, size 0x10
    unsigned char TitleName[68];    // offset 0xC0, size 0x44
    unsigned char FnameView[64];    // offset 0x104, size 0x40
    unsigned char FnameCopy[64];    // offset 0x144, size 0x40
    unsigned char FnameDel[64];     // offset 0x184, size 0x40
    unsigned char Reserve3[512];    // offset 0x1C4, size 0x200
} sceMcIconSys;

typedef struct {
    unsigned char Resv2;
    unsigned char Sec;
    unsigned char Min;
    unsigned char Hour;
    unsigned char Day;
    unsigned char Month;
    unsigned short Year;
} sceMcStDateTime;

typedef struct {
    sceMcStDateTime _Create;
    sceMcStDateTime _Modify;
    unsigned int FileSizeByte;
    unsigned short AttrFile;
    unsigned short Reserve1;
    unsigned int Reserve2;
    unsigned int PdaAplNo;
    unsigned char EntryName[32];
} sceMcTblGetDir;

typedef enum {
    SCE_MC_FUNC_GETINFO = 1,
    SCE_MC_FUNC_OPEN = 2,
    SCE_MC_FUNC_CLOSE = 3,
    SCE_MC_FUNC_SEEK = 4,
    SCE_MC_FUNC_READ = 5,
    SCE_MC_FUNC_WRITE = 6,
    SCE_MC_FUNC_FLUSH = 10,
    SCE_MC_FUNC_MKDIR = 11,
    SCE_MC_FUNC_CHDIR = 12,
    SCE_MC_FUNC_GETDIR = 13,
    SCE_MC_FUNC_SETFILEINFO = 14,
    SCE_MC_FUNC_DELETE = 15,
    SCE_MC_FUNC_FORMAT = 16,
    SCE_MC_FUNC_UNFORMAT = 17,
    SCE_MC_FUNC_GETENTSPACE = 18,
    SCE_MC_FUNC_RENAME = 19,
    SCE_MC_FUNC_CHANGETHREADPRIORITY = 20,
    SCE_MC_FUNC_GETSLOTMAX = 21,
    SCE_MC_FUNC_SYNC = 30,
    SCE_MC_FUNC_INIT = 31,
    SCE_MC_FUNC_END = 32,
} SceMcFunc;

#define sceMcTypeNoCard 0
#define sceMcTypePS1 1
#define sceMcTypePS2 2
#define sceMcTypePDA 3

#define sceMcFileInfoCreate 0x01
#define sceMcFileInfoModify 0x02
#define sceMcFileInfoAttr 0x04

#define sceMcFileAttrReadable 0x0001
#define sceMcFileAttrWriteable 0x0002
#define sceMcFileAttrExecutable 0x0004
#define sceMcFileAttrDupProhibit 0x0008
#define sceMcFileAttrFile 0x0010
#define sceMcFileAttrSubdir 0x0020

#define sceMcExecRun 0
#define sceMcExecFinish 1
#define sceMcExecIdle (-1)

#ifdef __cplusplus
extern "C" {
#endif

int sceMcInit(void);
int sceMcEnd(void);
int sceMcGetInfo(int port, int slot, int *type, int *free, int *format);
int sceMcOpen(int port, int slot, const char *name, int mode);
int sceMcClose(int fd);
int sceMcSeek(int fd, int offset, int mode);
int sceMcRead(int fd, void *buff, int size);
int sceMcWrite(int fd, const void *buff, int size);
int sceMcFlush(int fd);
int sceMcMkdir(int port, int slot, const char *name);
int sceMcChdir(int port, int slot, const char *newDir, char *currentDir);
int sceMcGetDir(int port, int slot, const char *name, unsigned int mode, int maxent, sceMcTblGetDir *table);
int sceMcSetFileInfo(int port, int slot, const char *name, const char *info, unsigned int valid);
int sceMcDelete(int port, int slot, const char *name);
int sceMcFormat(int port, int slot);
int sceMcUnformat(int port, int slot);
int sceMcGetEntSpace(int port, int slot, const char *path);
int sceMcRename(int port, int slot, const char *oldName, const char *newName);
int sceMcChangeThreadPriority(int level);
int sceMcSync(int mode, int *cmd, int *result);

#ifdef __cplusplus
}
#endif

#endif
