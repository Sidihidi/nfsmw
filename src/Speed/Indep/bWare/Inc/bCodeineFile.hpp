#ifndef BWARE_BCODEINEFILE_H
#define BWARE_BCODEINEFILE_H

#ifdef EA_PRAGMA_ONCE_SUPPORTED
#pragma once
#endif

// indep/bware/inc/bCodeineFile.hpp de la A124: un fichero del PC de desarrollo servido por Codeine a traves de bFunk.
// El layout (88 B), los enum y el orden de los metodos salen de los stabs. Los inline en linea (Write, GetStatus y
// los accesores) son las lineas 66-74 del original: Write se define antes que GetStatus, asi que no la puede
// expandir y la llama, y por eso el original emite GetStatus fuera de linea.
class bCodeineFile {
  public:
    enum eStatus {
        READY = 0,
        OPENING = 1,
        READING = 2,
        WRITING = 3,
        MAP_TO_CD = 4,
        OPEN_ERROR = 5,
    };
    enum eSeekMode {
        SET = 0,
        CUR = 1,
        END = 2,
    };

    void Open(const char *filename, int open_flags);
    void Close();
    void Seek(int position, eSeekMode mode);
    int ReadAsync(void *buf, int num_bytes);
    int Read(void *buf, int num_bytes);
    int WriteAsync(const void *buf, int num_bytes);
    int Write(const void *buf, int num_bytes) { int n = WriteAsync(buf, num_bytes); while (GetStatus() == WRITING) Service(); return n; }
    eStatus GetStatus() {
        Service();
        return Status;
    }
    int GetFileSize() { return FileSize; }
    int GetPosition() { return Position; }
    const char *GetFilename() { return Filename; }
    int GetID() { return ID; }
    void SetID(int id) { ID = id; }
    static bCodeineFile *GetFile(int id);
    static const char *GetStatusName(eStatus status);
    static void ClearCache();
    static void InvalidateCache(const char *filename);

  private:
    void Service();

    int OpenFlags;      // offset 0x0, size 0x4
    eStatus Status;     // offset 0x4, size 0x4
    int ID;             // offset 0x8, size 0x4
    int FileSize;       // offset 0xC, size 0x4
    int Position;       // offset 0x10, size 0x4
    int ReadPosition;   // offset 0x14, size 0x4
    char Filename[64];  // offset 0x18, size 0x40

    static char CachedFilename[4][128];
    static int CachedFileSize[4];
    static eStatus CachedStatus[4];
    static int NextCachePosition;
};

void bCacheCodeineDirs(char *dir_names, int dir_names_size, int max_files);
void bRefreshCachedCodeineDirs();
void bClearCodeineFileCache();
void bInvalidateCodeineFileCache(const char *filename);
void AddCodeineDeviceDriver();

#endif
