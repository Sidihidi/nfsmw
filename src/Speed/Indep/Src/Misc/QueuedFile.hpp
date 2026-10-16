#ifndef MISC_QUEUED_FILE_H
#define MISC_QUEUED_FILE_H

#ifdef EA_PRAGMA_ONCE_SUPPORTED
#pragma once
#endif

#include "Speed/Indep/bWare/Inc/bList.hpp"
#include "Speed/Indep/bWare/Inc/bSlotPool.hpp"
#include "Speed/Indep/bWare/Inc/bWare.hpp"

extern SlotPool *QueuedFileSlotPool;
extern int QueuedFileDefaultPriority;

// total size: 0x10
struct QueuedFileParams {
    QueuedFileParams()
        : Priority(QueuedFileDefaultPriority), //
          BlockSize(0x7ffffff)
#ifndef EA_BUILD_A124
          ,
          Compressed(false), //
          UncompressedSize(0)
#endif
    {}

    int Priority;  // offset 0x0, size 0x4
    int BlockSize; // offset 0x4, size 0x4
#ifndef EA_BUILD_A124
    // The A124 prototype has no compressed reads (no DecompressionTable either): 0x8 bytes, Priority and BlockSize.
    bool Compressed;      // offset 0x8, size 0x1
    int UncompressedSize; // offset 0xC, size 0x4
#endif
};

enum QueuedFileStatus {
    QWAITING = 0,
    QREADING = 1,
    QDONE = 2,
    QERROR = 3,
};

// total size: 0x48 (0x40 en la A124)
class QueuedFile : public bTNode<QueuedFile> {
  public:
    // La A124 pasa el parametro de las llamadas de vuelta como int (sus stabs: CallbackParam y CallbackParam2 son int,
    // y __10QueuedFilePvPCciiT1iP16QueuedFileParams); GameCube y las versiones de venta, como void *.
    QueuedFile(void *buf, const char *filename, int file_pos, int num_bytes, void *callback_function,
#ifdef EA_BUILD_A124
               int callback_param,
#else
               void *callback_param,
#endif
               QueuedFileParams *params);

    ~QueuedFile();

    char *GetDebugPrintHeader();

    void BeginRead();

    static void ServiceDecompression();

#ifndef EA_BUILD_A124 // la A124 no tiene este miembro (ni descompresion): su ReadDoneCallback estatica da el fichero por leido
    void ReadDoneCallback();
#endif

    static void *operator new(std::size_t size, void *ptr) {
        return ptr;
    }

    void operator delete(void *ptr) {
        bFree(QueuedFileSlotPool, ptr);
    }

    // static int SortBySeekPosition(QueuedFile *before, QueuedFile *after) {}

    static int SortByPriority(QueuedFile *before, QueuedFile *after);

    static int GetNumFilesDecompressing() {
        return DecompressionTableTop - DecompressionTableBot;
    }

#ifdef EA_BUILD_A124
    static void ReadDoneCallback(int param);
#else
    static void ReadDoneCallback(void *param);
#endif

    int GetHandle() {
        return Handle;
    }

    const char *GetFilename() {
        return Filename;
    }

    int GetFilePos() {
        return FilePos;
    }

    int GetNumBytes() {
        return NumBytes;
    }

    int GetNumRead() {
        return NumRead;
    }

    void *GetBuffer() {
        return pBuf;
    }

    // float GetStartReadTime() {}

    int GetPriority() {
#ifdef EA_BUILD_A124
        return Priority;
#else
        return Params.Priority;
#endif
    }

    QueuedFileStatus GetStatus() {
        return Status;
    }

    void SetStatus(QueuedFileStatus status) {
        Status = status;
    }

    int IsFinishedAllReading() {
        return NumRead == NumBytes;
    }

    unsigned int CalculateChecksum() {
        return bCalculateCrc32(pBuf, NumBytes, 0xFFFFFFFF);
    }

    void *GetCallback() {
        return CallbackFunction;
    }

    void SetCallbackParam2(
#ifdef EA_BUILD_A124
        int param
#else
        void *param
#endif
    ) {
        CallbackParam2 = param;
        CallbackModeUseParam2 = true;
    }

    void CallDoneCallback(int error_status) {
        if (CallbackFunction) {
            if (CallbackModeUseParam2) {
#ifdef EA_BUILD_A124
                ((void (*)(int, int, int))CallbackFunction)(CallbackParam, error_status, CallbackParam2);
#else
                ((void (*)(void *, int, void *))CallbackFunction)(CallbackParam, error_status, CallbackParam2);
#endif
            } else {
#ifdef EA_BUILD_A124
                ((void (*)(int, int))CallbackFunction)(CallbackParam, error_status);
#else
                ((void (*)(void *, int))CallbackFunction)(CallbackParam, error_status);
#endif
            }
        }
    }

    friend class QueuedFileBundle;

  private:
#ifdef EA_BUILD_A124
    void SetParams(QueuedFileParams *params) {
        BlockSize = params->BlockSize;
        Priority = params->Priority;
    }
#endif

    static int CurrentHandle;                  // size: 0x4, address: 0x8041EA64
    static int DecompressionTableBot;          // size: 0x4, address: 0x8041EA68
    static int DecompressionTableTop;          // size: 0x4, address: 0x8041EA6C
    static QueuedFile *DecompressionTable[32]; // size: 0x80, address: 0x804FE9CC

    void *pBuf;                // offset 0x8, size 0x4
    const char *Filename;      // offset 0xC, size 0x4
    int FilePos;               // offset 0x10, size 0x4
    int NumBytes;              // offset 0x14, size 0x4
    int NumRead;               // offset 0x18, size 0x4
#ifdef EA_BUILD_A124
    // La A124 guarda los parametros sueltos y con BlockSize delante (stabs: 0x1C BlockSize, 0x20 Priority); los
    // copia SetParams.
    int BlockSize;             // offset 0x1C, size 0x4
    int Priority;              // offset 0x20, size 0x4
#else
    QueuedFileParams Params;   // offset 0x1C, size 0x10
#endif
    void *CallbackFunction;    // offset 0x2C, size 0x4
#ifdef EA_BUILD_A124
    int CallbackParam;         // offset 0x28, size 0x4
    int CallbackParam2;        // offset 0x2C, size 0x4
#else
    void *CallbackParam;       // offset 0x30, size 0x4
    void *CallbackParam2;      // offset 0x34, size 0x4
#endif
    int CallbackModeUseParam2; // offset 0x38, size 0x4
    int Handle;                // offset 0x3C, size 0x4
    QueuedFileStatus Status;   // offset 0x40, size 0x4
    float StartReadTime;       // offset 0x44, size 0x4
};

void InitQueuedFiles();
void ServiceQueuedFiles();
// La A124 solo tiene la version con enteros (AddQueuedFile__FPvPCciiPFii_viP16QueuedFileParams y
// AddQueuedFile2__FPvPCciiPFiii_viiP16QueuedFileParams); las de venta la dejan en linea sobre la de punteros (el
// DWARF de GameCube la expande) y en SLUS sale fuera de linea en zEAXSound como un salto a la de punteros.
void AddQueuedFile(void *buf, const char *filename, int file_pos, int num_bytes,
#ifdef EA_BUILD_A124
                   void (*callback)(int, int), int callback_param,
#else
                   void (*callback)(void *, int), void *callback_param,
#endif
                   QueuedFileParams *params);
void AddQueuedFile2(void *buf, const char *filename, int file_pos, int num_bytes,
#ifdef EA_BUILD_A124
                    void (*callback)(int, int, int), int callback_param, int callback_param2,
#else
                    void (*callback)(void *, int, void *), void *callback_param, void *callback_param2,
#endif
                    QueuedFileParams *params);
int GetQueuedFileSize(const char *filename);
bool IsQueuedFileBusy();
void BlockWhileQueuedFileBusy();
void SetQueuedFileMinPriority(int priority);

#ifndef EA_BUILD_A124
inline void AddQueuedFile(void *buf, const char *filename, int file_pos, int num_bytes, void (*callback)(intptr_t, int), intptr_t callback_param,
                          QueuedFileParams *params) {
    AddQueuedFile(buf, filename, file_pos, num_bytes, reinterpret_cast<void (*)(void *, int)>(callback), reinterpret_cast<void *>(callback_param),
                  params);
}
#endif

// total size: 0x3C
class QueuedFileBundle {
  public:
    QueuedFileBundle() {
        ReadBuffer = 0;
        ReadBufferBot = 0;
        ReadBufferTop = 0;
        NumBytesQueued = 0;
        NumQueuedFiles = 0;
        MemoryPoolNumber = 0;
    }

    // El destructor libera el bufer de lectura: GameCube lo expande en ReadCallbackBridge y StartQueuedFileReading, y
    // las dos PS2 lo llaman fuera de linea (_$_16QueuedFileBundle).
    ~QueuedFileBundle() {
        if (ReadBuffer) {
            bFree(ReadBuffer);
        }
    }

    void *operator new(std::size_t size) {
        return bOMalloc(QueuedFileSlotPool);
    }

    void operator delete(void *ptr);

    const char *GetFilename() {
        return QueuedFiles[0]->GetFilename();
    }

    // La A124 lo declara con el parametro entero (sus stabs: ReadCallbackBridge__16QueuedFileBundleii), como el resto de
    // su API de llamadas de vuelta; las de venta, con el puntero (GameCube y PC, y SLUS se lo pasa a la AddQueuedFile de
    // punteros).
#ifdef EA_BUILD_A124
    static void ReadCallbackBridge(intptr_t param, int error_status);
#else
    static void ReadCallbackBridge(void *param, int error_status);
#endif

    bool TestAddQueuedFile(QueuedFile *q);
    void BeginRead();
    void ReadCallback(int error_status);

    int8 *ReadBuffer;            // offset 0x0, size 0x4
    int ReadBufferBot;           // offset 0x4, size 0x4
    int ReadBufferTop;           // offset 0x8, size 0x4
    int NumBytesQueued;          // offset 0xC, size 0x4
    int16 MemoryPoolNumber;      // offset 0x10, size 0x2
    int16 NumQueuedFiles;        // offset 0x12, size 0x2
    QueuedFile *QueuedFiles[10]; // offset 0x14, size 0x28
};

#endif
