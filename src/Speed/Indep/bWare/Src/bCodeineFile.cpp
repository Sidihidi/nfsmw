#include "Speed/Indep/bWare/Inc/bCodeineFile.hpp"
#include "Speed/Indep/bWare/Inc/bDebug.hpp"
#include "Speed/Indep/bWare/Inc/bFunk.hpp"
#include "Speed/Indep/bWare/Inc/bWare.hpp"
#include "Speed/Indep/Libs/realcore/6.24.00/include/common/realcore/file/driver.h"
#include "Speed/Indep/Libs/realcore/6.24.00/include/common/realcore/system.h"

// indep/bware/src/bCodeineFile.cpp de la A124 (zPlatform): los ficheros "sb:" que Codeine sirve desde el PC, con la
// cache de directorios que el PC rellena por DMA y un driver de RealFile encima. Las cadenas y los numeros de funcion
// de bFunk son los del original.

bCodeineDirEntry *pCodeineDirEntryTable;
int MaxCachedCodeineFiles;
int SizeofCachedCodeineDirNames;
char CachedCodeineDirNames[400];

void bCacheCodeineDirs(char *dir_names, int dir_names_size, int max_files) {
    if (pCodeineDirEntryTable) {
        delete[] pCodeineDirEntryTable;
        pCodeineDirEntryTable = 0;
    }

    MaxCachedCodeineFiles = max_files;
    SizeofCachedCodeineDirNames = dir_names_size;

    bMemCpy(CachedCodeineDirNames, dir_names, dir_names_size);

    if (max_files) {
        pCodeineDirEntryTable = new ("bCodeineFileCache", 0) bCodeineDirEntry[max_files];
    }

    bRefreshCachedCodeineDirs();
}

void bRefreshCachedCodeineDirs() {
    if (pCodeineDirEntryTable) {
        bMemSet(pCodeineDirEntryTable, 0, MaxCachedCodeineFiles * sizeof(bCodeineDirEntry));

        if (bCodeineVersion >= 2.71f) {
            bFileCacheDirsPacket packet;

            packet.DirEntryAddress = reinterpret_cast<int>(pCodeineDirEntryTable);
            packet.MaxEntries = MaxCachedCodeineFiles;
            packet.DirNamesSize = SizeofCachedCodeineDirNames;
            bMemCpy(packet.DirNames, CachedCodeineDirNames, SizeofCachedCodeineDirNames);

            bFunkCallASync("CODEINE", 13, &packet, sizeof(packet));
        }
    }
}

static unsigned int bGetDirNameHash(const char *filename) {
    char mangled_name[160];
    int len = bStrLen(filename);
    int n;

    for (n = 0; n < len; n++) {
        char c = bToUpper(filename[n]);

        if (c == '/') {
            c = '\\';
        }
        mangled_name[n] = c;
    }
    mangled_name[n] = 0;
    return bStringHash(mangled_name);
}

static const char *eStatusNames[6] = {"READY", "OPENING", "READING", "WRITING", "MAP_TO_CD", "OPEN_ERROR"};

const char *bCodeineFile::GetStatusName(eStatus status) {
    return eStatusNames[status];
}

char bCodeineFile::CachedFilename[4][128];
int bCodeineFile::CachedFileSize[4];
bCodeineFile::eStatus bCodeineFile::CachedStatus[4];
int bCodeineFile::NextCachePosition;

void bCodeineFile::Open(const char *filename, int open_flags) {
    OpenFlags = open_flags;
    Status = OPENING;
    FileSize = -1;
    Position = 0;
    ReadPosition = 0;

    bMemSet(Filename, 0, sizeof(Filename));
    bStrNCpy(Filename, filename, sizeof(Filename) - 1);

    bFileOpenPacket packet;
    bMemSet(&packet, 0, sizeof(packet));
    packet.OpenFlags = OpenFlags;
    packet.FileID = reinterpret_cast<int>(this);
    packet.FilePositionAddress = reinterpret_cast<int>(&ReadPosition);
    bStrCpy(packet.Filename, filename);

    bool found_cached_value = false;

    for (int i = 0; i < 4; i++) {
        if (CachedFilename[i][0] && (open_flags & 1) && bStrCmp(CachedFilename[i], filename) == 0) {
            Status = CachedStatus[i];
            FileSize = CachedFileSize[i];
            found_cached_value = true;
            break;
        }
    }

    bool found_dir_entry_value = false;
    if (!found_cached_value) {
        unsigned int name_hash = bGetDirNameHash(filename);

        for (int i = 0; i < MaxCachedCodeineFiles; i++) {
            bCodeineDirEntry *dir_entry = &pCodeineDirEntryTable[i];

            if (dir_entry->NameHash == 0) {
                break;
            }
            if (dir_entry->NameHash == name_hash) {
                Status = READY;
                FileSize = dir_entry->FileSize;
                found_dir_entry_value = true;
                break;
            }
        }
    }

    if (found_cached_value || found_dir_entry_value) {
        packet.CachedFileSize = FileSize;
        bFunkCallASync("CODEINE", 10, &packet, sizeof(packet));
    } else {
        int result;
        int return_code;
        unsigned int ret = bGetTicker();

        return_code = bFunkCallSync("CODEINE", 10, &packet, sizeof(packet), &result, sizeof(bFunkPacket) - sizeof(bFunkPacketHeader));
        if (return_code == 4) {
            if (result >= 0) {
                FileSize = result;
                Status = READY;
            } else if (result == -2) {
                Status = MAP_TO_CD;
            } else {
                Status = OPEN_ERROR;
            }
        } else {
            Status = OPEN_ERROR;
        }

        if (open_flags & 1) {
            CachedFileSize[NextCachePosition] = FileSize;
            CachedStatus[NextCachePosition] = Status;

            bMemSet(CachedFilename[NextCachePosition], 0, sizeof(CachedFilename[0]));
            bStrNCpy(CachedFilename[NextCachePosition], filename, sizeof(CachedFilename[0]) - 1);

            NextCachePosition = (NextCachePosition + 1) & 3;
        }
    }
}

void bCodeineFile::Close() {
    while (Status == OPENING || Status == READING || Status == WRITING) Service();

    if (Status == READY) {
        bFileClosePacket packet;

        packet.FileID = reinterpret_cast<int>(this);
        bFunkCallASync("CODEINE", 11, &packet, sizeof(packet));
    }
}

void bCodeineFile::Service() {
    bMonitorService();

    switch (Status) {
    case OPENING:
        break;
    case READING:
        if (ReadPosition == Position) {
            Status = READY;
        }
        break;
    }
}

void bCodeineFile::Seek(int position, eSeekMode mode) {
    if (Status != READY) {
        return;
    }

    switch (mode) {
    case SET:
        Position = position;
        break;
    case END:
        Position = FileSize - position;
        break;
    case CUR:
        Position += position;
        break;
    }

    if (Position > FileSize) {
        Position = FileSize;
    } else if (Position < 0) {
        Position = 0;
    }
}

int bCodeineFile::ReadAsync(void *buf, int num_bytes) {
    if (Status != READY) {
        return 0;
    }

    if (Position + num_bytes > FileSize) {
        num_bytes = FileSize - Position;
    }
    ReadPosition = Position;

    bFileReadPacket packet = {reinterpret_cast<int>(this), Position, reinterpret_cast<int>(buf), num_bytes};
    bFunkCallASync("CODEINE", 12, &packet, sizeof(packet));

    Position += num_bytes;
    Status = READING;
    return num_bytes;
}

int bCodeineFile::WriteAsync(const void *buf, int num_bytes) {
    if ((OpenFlags & 1) || Status != READY) {
        return 0;
    }

    int num_written;
    for (num_written = 0; num_written < num_bytes;) {
        int max_size = sizeof(reinterpret_cast<bFileWritePacket *>(0)->Data);
        int num_to_write = num_bytes - num_written;

        if (num_to_write > max_size) {
            num_to_write = max_size;
        }
        bFileWritePacket packet = {reinterpret_cast<int>(this), Position + num_written};
        bMemCpy(packet.Data, static_cast<const char *>(buf) + num_written, num_to_write);
        num_written += num_to_write;

        bFunkCallASync("CODEINE", 14, &packet, num_to_write + 8);
    }

    Position += num_bytes;
    if (Position > FileSize) {
        FileSize = Position;
    }
    return num_bytes;
}

void bCodeineFile::ClearCache() {
    bMemSet(CachedFilename, 0, sizeof(CachedFilename));
    NextCachePosition = 0;
}

void bCodeineFile::InvalidateCache(const char *filename) {
    for (int i = 0; i < 4; i++) {
        if (CachedFilename[i][0] && bStrCmp(CachedFilename[i], filename) == 0) {
            CachedFilename[i][0] = 0;
            break;
        }
    }
}

void bClearCodeineFileCache() {
    bRefreshCachedCodeineDirs();
    bCodeineFile::ClearCache();
}

void bInvalidateCodeineFileCache(const char *filename) {
    unsigned int name_hash = bGetDirNameHash(filename);

    for (int i = 0; i < MaxCachedCodeineFiles; i++) {
        bCodeineDirEntry *dir_entry = &pCodeineDirEntryTable[i];

        if (dir_entry->NameHash == 0) {
            break;
        }
        if (dir_entry->NameHash == name_hash) {
            dir_entry->NameHash = 1;
        }
    }
    bCodeineFile::InvalidateCache(filename);
}

class CodeineDeviceDriver : public RealFile::DeviceDriver {
  public:
    CodeineDeviceDriver();
    virtual ~CodeineDeviceDriver();
    void *operator new(size_t size) { return bMalloc(size, "CodeineDeviceDriver", 0, 0); } void operator delete(void *ptr) { bFree(ptr); }
    virtual bool Init();
    virtual void Restore();
    virtual EAFileHandle Open(const char *name, int oflags, int *pParentFileHandle);
    virtual void Close(EAFileHandle h);
    virtual uint32 Read(EAFileHandle h, void *buf, unsigned int bufsize, RealFile::DeviceDriver *ddParent, EAFileHandle ddFileHandle);
    virtual uint32 Write(EAFileHandle h, const void *buf, unsigned int bufsize, RealFile::DeviceDriver *ddParent, EAFileHandle ddFileHandle);
    virtual uint64 Seek(EAFileHandle h, uint64 offset, int whence, RealFile::DeviceDriver *ddParent, EAFileHandle ddFileHandle);
    virtual uint64 Getsize(EAFileHandle h);
    virtual uint64 QueryLocation(EAFileHandle h);
    virtual bool Remove(const char *name);
    virtual uint64 Getspace();
    virtual const char *GetName();

  private:
    bCodeineFile *AllocFile();
    void FreeFile(bCodeineFile *file);
    bCodeineFile *GetFile(int id);

    int MaxCodeineFiles;           // offset 0x14, size 0x4
    bCodeineFile *pCodeineFiles;   // offset 0x18, size 0x4
};

CodeineDeviceDriver *pCodeineDeviceDriver;
int hCodeineDeviceID;

void AddCodeineDeviceDriver() {
    pCodeineDeviceDriver = new CodeineDeviceDriver;
    hCodeineDeviceID = RealFile::AddDevice(pCodeineDeviceDriver);

    RealFile::AddSearchLocation("sb:", true);
}

char WhatTheHell[80];

CodeineDeviceDriver::CodeineDeviceDriver() : RealFile::DeviceDriver(WhatTheHell) {
    MaxCodeineFiles = 0;
    pCodeineFiles = 0;
}

CodeineDeviceDriver::~CodeineDeviceDriver() {
    Restore();
}

bool CodeineDeviceDriver::Init() {
    MaxCodeineFiles = 32;
    pCodeineFiles = static_cast<bCodeineFile *>(bMalloc(MaxCodeineFiles * sizeof(bCodeineFile), "CodeineFileTable", 0, 0));

    for (int id = 0; id < MaxCodeineFiles; id++) {
        bCodeineFile *file = &pCodeineFiles[id];

        bMemSet(file, 0, sizeof(bCodeineFile));
        file->SetID(-1);
    }
    return true;
}

void CodeineDeviceDriver::Restore() {
    if (MaxCodeineFiles) {
        MaxCodeineFiles = 0;
        bFree(pCodeineFiles);
        pCodeineFiles = 0;
    }
}

EAFileHandle CodeineDeviceDriver::Open(const char *name, int oflags, int *pParentFileHandle) {
    if (bStrNCmp(name, "sb:", 3) == 0) {
        name += 3;
        if (*name == '/') {
            name++;
        }
    }

    bCodeineFile *file = AllocFile();
    if (!file) {
        return -1;
    }

    int open_error = 1;
    file->Open(name, oflags);
    if (file->GetStatus() == bCodeineFile::READY) {
        open_error = 0;
    } else if (file->GetStatus() == bCodeineFile::OPEN_ERROR) {
        open_error = 1;
    }

    if (open_error) {
        FreeFile(file);
        return -1;
    }
    return file->GetID();
}

void CodeineDeviceDriver::Close(EAFileHandle h) {
    bCodeineFile *file = GetFile(h);
    file->Close();
    FreeFile(file);
}

uint32 CodeineDeviceDriver::Read(EAFileHandle h, void *buf, unsigned int bufsize, RealFile::DeviceDriver *ddParent, EAFileHandle ddFileHandle) {
    bCodeineFile *file = GetFile(h);
    int num_read = file->ReadAsync(buf, bufsize);

    while (file->GetStatus() == bCodeineFile::READING) {
        THREAD_yield(5);
        bMonitorService();
    }
    return num_read;
}

uint32 CodeineDeviceDriver::Write(EAFileHandle h, const void *buf, unsigned int bufsize, RealFile::DeviceDriver *ddParent, EAFileHandle ddFileHandle) {
    bCodeineFile *file = GetFile(h);
    return file->Write(buf, bufsize);
}

uint64 CodeineDeviceDriver::Seek(EAFileHandle h, uint64 offset, int whence, RealFile::DeviceDriver *ddParent, EAFileHandle ddFileHandle) {
    bCodeineFile *file = GetFile(h);
    file->Seek(offset, static_cast<bCodeineFile::eSeekMode>(whence));
    return file->GetPosition();
}

uint64 CodeineDeviceDriver::Getsize(EAFileHandle h) {
    return GetFile(h)->GetFileSize();
}

uint64 CodeineDeviceDriver::QueryLocation(EAFileHandle h) {
    return 0;
}

bool CodeineDeviceDriver::Remove(const char *name) {
    return true;
}

uint64 CodeineDeviceDriver::Getspace() {
    return 0xa00000;
}

const char *CodeineDeviceDriver::GetName() {
    return "sb:";
}

bCodeineFile *CodeineDeviceDriver::AllocFile() {
    for (int id = 0; id < MaxCodeineFiles; id++) {
        bCodeineFile *file = &pCodeineFiles[id];

        if (file->GetID() == -1) {
            file->SetID(id);
            return file;
        }
    }
    return 0;
}

void CodeineDeviceDriver::FreeFile(bCodeineFile *file) {
    bMemSet(file, 0, sizeof(bCodeineFile));
    file->SetID(-1);
}

bCodeineFile *CodeineDeviceDriver::GetFile(int id) {
    bCodeineFile *file = &pCodeineFiles[id];

    if (file->GetID() != id) {
        return 0;
    }
    return file;
}
