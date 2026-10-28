#include "QueuedFile.hpp"
#include "Joylog.hpp"
#include "Speed/Indep/Src/Misc/Profiler.hpp"
#include "Speed/Indep/bWare/Inc/Strings.hpp"
#include "Speed/Indep/bWare/Inc/bSlotPool.hpp"
#include "Speed/Indep/bWare/Inc/bWare.hpp"
#include "Timer.hpp"
#include "bFile.hpp"
#include "Platform.h"

// static const int PrintQueuedFiles;
// static const int PrintQueuedFileChecksum;
int QueuedFileDefaultPriority = 5; // size: 0x4
int QueuedFileMinPriority = 0;
int QueuedFileMinPriorityTimeoutCounter = 0;
bool QueuedFileJoylogEnabled = true;
float QueuedFileDebugTimeStart = 0.0f;
SlotPool *QueuedFileSlotPool = nullptr;

// c34ord3 m6: los cuatro cuerpos que el original tiene en ESTE fichero
// (debug_lines: :123, :131, :339 y :349), en el orden en que los declara la
// clase. Es POSICION, no optimizacion: sin ellos aqui, las cuatro salen en el
// bloque diferido en el sitio de Main.cpp (fichero 3) y no en el 16.
// El DWARF de GameCube expande GetPriority dos veces aqui.
inline int QueuedFile::SortByPriority(QueuedFile *before, QueuedFile *after) {
    return before->GetPriority() >= after->GetPriority();
}

inline void QueuedFile::ReadDoneCallback(
#ifdef EA_BUILD_A124
    int param
#else
    void *param
#endif
) {
#ifdef EA_BUILD_A124
    reinterpret_cast<QueuedFile *>(param)->Status = QDONE;
#else
    reinterpret_cast<QueuedFile *>(param)->ReadDoneCallback();
#endif
}

inline void QueuedFileBundle::operator delete(void *ptr) {
    bFree(QueuedFileSlotPool, ptr);
}

#ifdef EA_BUILD_A124
inline void QueuedFileBundle::ReadCallbackBridge(intptr_t param, int error_status) {
    QueuedFileBundle *bundle = reinterpret_cast<QueuedFileBundle *>(param);
#else
inline void QueuedFileBundle::ReadCallbackBridge(void *param, int error_status) {
    QueuedFileBundle *bundle = static_cast<QueuedFileBundle *>(param);
#endif
    bundle->ReadCallback(error_status);
    delete bundle;
}

int QueuedFile::CurrentHandle = 1;
int QueuedFile::DecompressionTableBot = 0;
int QueuedFile::DecompressionTableTop = 0;
QueuedFile *QueuedFile::DecompressionTable[32];

// static const int QueuedFileBundleMaxReadGap;
// static const int QueuedFileBundleMaxReadSize;
int EnableQueuedFileBundle = true;
int QueuedFileNumReadsInProgress = 0;
// static const int QueuedFileMaxReadsInProgress;
// static const int QueuedFileDontBlock;

bTList<QueuedFile> WaitingQueuedFileList;
bTList<QueuedFile> ReadingQueuedFileList;
#ifdef EA_BUILD_A124
char LastQueuedFilename[60]; // 0x3C en el mapa de la A124
#else
char LastQueuedFilename[100];
#endif

float GetQueuedFileDebugTime() {
    return GetDebugRealTime() - QueuedFileDebugTimeStart;
}

bool IsQueuedFileJoyloggable(const char *filename) {
#ifdef EA_BUILD_A124 // la A124 compara el nombre entero, sin comodin
    return bStrICmp(filename, "Sound\\Speech\\CopSpeech.big") != 0;
#else
    return bMatchNameWithWildcard("Sound\\Speech\\CopSpeech*.big", filename) == 0;
#endif
}

void SetQueuedFileMinPriority(int priority) {
    QueuedFileMinPriority = priority;
}

QueuedFile::QueuedFile(void *buf, const char *filename, int file_pos, int num_bytes, void *callback_function,
#ifdef EA_BUILD_A124
                       int callback_param,
#else
                       void *callback_param,
#endif
                       QueuedFileParams *params) {
    this->pBuf = buf;
    this->FilePos = file_pos;
    this->NumBytes = num_bytes;
    this->NumRead = 0;

#ifdef EA_BUILD_A124
    // La A124 copia los parametros que le pasan, o los de por defecto, con SetParams (stabs: default_params en 0(sp);
    // su mapa de lineas la declara despues de NumRead).
    QueuedFileParams default_params;
    if (!params) {
        params = &default_params;
    }
    SetParams(params);
#else
    if (params) {
        Params = *params;
    }
#endif
    this->Filename = bAllocateSharedString(filename);
    this->CallbackFunction = callback_function;
    this->CallbackParam = callback_param;
    this->CallbackParam2 = nullptr;
    this->CallbackModeUseParam2 = 0;
    this->Handle = QueuedFile::CurrentHandle;
    this->Status = QWAITING;
    this->StartReadTime = 0.0f; // scaf-data r71: bloque +0x16E8
    if (++QueuedFile::CurrentHandle > 100000) {
        QueuedFile::CurrentHandle = 1;
    }
#ifdef EA_PLATFORM_PLAYSTATION2
    // Las dos PS2 solo comprueban el hash del nombre (VerifyData de 32 bits en el canal de estado).
    Joylog::VerifyData(bStringHash(filename), 32, JOYLOG_CHANNEL_QUEUEDFILE_STATUS);
#else
    if (Joylog::IsCapturing()) {
        int len = bStrLen(filename);
        Joylog::AddData(len + 1, 8, JOYLOG_CHANNEL_QUEUEDFILENAME);
        Joylog::AddData(filename, len + 1, JOYLOG_CHANNEL_QUEUEDFILENAME);
    } else {
        if (Joylog::IsReplaying()) {
            char temp_filename[128];
            int len = Joylog::GetData(8, JOYLOG_CHANNEL_QUEUEDFILENAME);
            Joylog::GetData(temp_filename, len, JOYLOG_CHANNEL_QUEUEDFILENAME);
            if (!bStrEqual(temp_filename, filename)) {
                bBreak();
            }
        }
    }
#endif
}

QueuedFile::~QueuedFile() {
    bFreeSharedString(Filename);
}

void QueuedFile::BeginRead() {
    ProfileNode profile_node;
    StartReadTime = GetQueuedFileDebugTime();
    if (NumBytes == 0) {
        Status = QDONE;
        return;
    }

    bFile *f = bOpen(Filename, 1, 1);
    if (f == nullptr) {
        Status = QERROR;
    } else {
        int num_to_read = NumBytes - NumRead;
#ifdef EA_BUILD_A124
        if (num_to_read > BlockSize) {
            num_to_read = BlockSize;
        }
#else
        if (num_to_read > Params.BlockSize) {
            num_to_read = Params.BlockSize;
        }
#endif
        int file_size = bFileSize(f);
        if (FilePos + NumRead + num_to_read > file_size) {
            Status = QERROR;
        } else {
            Status = QREADING;
            bSeek(f, FilePos + NumRead, 0);
#ifdef EA_BUILD_A124
            bReadAsync(f, static_cast<char *>(pBuf) + NumRead, num_to_read, &QueuedFile::ReadDoneCallback, reinterpret_cast<int>(this));
#else
            bReadAsync(f, static_cast<char *>(pBuf) + NumRead, num_to_read, &QueuedFile::ReadDoneCallback, this);
#endif
            NumRead = NumRead + num_to_read;
        }
        bClose(f);
    }
}

#ifndef EA_BUILD_A124
void QueuedFile::ReadDoneCallback() {
    if (Params.Compressed) {
        int table_pos = DecompressionTableTop % 32;
        DecompressionTable[table_pos] = this;
        DecompressionTableTop++;
    } else {
        Status = QDONE;
    }
}
#endif

// TODO move
extern int CarLoaderMemoryPoolNumber;

bool QueuedFileBundle::TestAddQueuedFile(QueuedFile *q) {
    if (NumQueuedFiles >= 10) {
        return false;
    }
    if (q->GetNumRead() != 0) {
        return false;
    }
#ifndef EA_BUILD_A124
    if (q->Params.Compressed != 0) {
        return false;
    }
#endif
    if (NumQueuedFiles > 0) {
        if (bStrCmp(q->GetFilename(), QueuedFiles[0]->GetFilename()) != 0) {
            return false;
        }
    }
    int read_buffer_bot = q->GetFilePos() & ~0x7FF;
    int read_buffer_top = q->GetFilePos() + q->GetNumBytes();
    int num_bytes_queued = q->GetNumBytes() + NumBytesQueued;
    if (NumQueuedFiles > 0) {
        if (read_buffer_bot > ReadBufferBot) {
            read_buffer_bot = ReadBufferBot;
        }
        if (read_buffer_top < ReadBufferTop) {
            read_buffer_top = ReadBufferTop;
        }
    }
    int read_size = read_buffer_top - read_buffer_bot;
    if (read_size > 0x40000) {
        return false;
    }
    int prev_read_size = ReadBufferTop - ReadBufferBot;
    if (read_size - prev_read_size - q->GetNumBytes() > 0x8000) {
        return false;
    }
    // TODO dwarf (gap uses the wrong register)
    int gap = bLargestMalloc(0) - 0x10000;
    bool not_enough_memory = read_size * 2 > gap;
    int memory_pool = 0;
    if (not_enough_memory) {
        if (bStrNICmp(q->GetFilename(), "CARS\\", 5) == 0) {
            not_enough_memory = read_size > bLargestMalloc(CarLoaderMemoryPoolNumber);
            memory_pool = CarLoaderMemoryPoolNumber;
        }
    }
    bool joylog_not_enough_memory = Joylog::AddOrGetData(not_enough_memory, 1, JOYLOG_CHANNEL_QUEUEDFILE_STATUS) != 0;
    if (joylog_not_enough_memory != not_enough_memory) {
        not_enough_memory = joylog_not_enough_memory;
    }
    if (not_enough_memory) {
        return false;
    }
    MemoryPoolNumber = static_cast<short>(memory_pool);
    ReadBufferBot = read_buffer_bot;
    ReadBufferTop = read_buffer_top;
    NumBytesQueued = num_bytes_queued;
    QueuedFiles[NumQueuedFiles++] = q;
    return true;
}

static const int PrintQueuedFiles = ENABLE_IN_DEBUG;

void QueuedFileBundle::BeginRead() {
    int read_size = ReadBufferTop - ReadBufferBot;
    if (PrintQueuedFiles) {
        for (int n = 0; n < NumQueuedFiles; n++) {
            // TODO what to use instead of bPrintf?
            // bPrintf(" %d", QueuedFiles[n]->Handle);
        }
    }
    ReadBuffer = static_cast<signed char *>(bMalloc(read_size, "QueuedFileBundle", 0, (MemoryPoolNumber & 0xF) | 0x1040));
#ifdef EA_BUILD_A124
    AddQueuedFile(ReadBuffer, GetFilename(), ReadBufferBot, read_size, &ReadCallbackBridge, reinterpret_cast<intptr_t>(this), nullptr);
#else
    AddQueuedFile(ReadBuffer, GetFilename(), ReadBufferBot, read_size, &ReadCallbackBridge, this, nullptr);
#endif
    QueuedFile *q = WaitingQueuedFileList.RemoveTail();
    WaitingQueuedFileList.AddHead(q);
}

void QueuedFileBundle::ReadCallback(int error_status) {
    int num_bytes_copied;
    for (int n = 0; n < NumQueuedFiles; n++) {
        QueuedFile *q = QueuedFiles[n];
        bMemCpy(q->GetBuffer(), ReadBuffer + (q->GetFilePos() - ReadBufferBot), q->GetNumBytes());
        num_bytes_copied += q->GetNumBytes();
        q->CallDoneCallback(error_status);
        delete q;
    }
}

void CheckQueuedFileCallbacks() {
    bServiceFileSystem();
    if (QueuedFileNumReadsInProgress) {
        QueuedFile *q = ReadingQueuedFileList.GetHead();
        QueuedFileStatus status = q->GetStatus();
        if (QueuedFileJoylogEnabled) {
            if (Joylog::IsReplaying()) {
                status = static_cast<QueuedFileStatus>(Joylog::GetData(4, JOYLOG_CHANNEL_QUEUEDFILE_STATUS));
                if (status != QREADING) {
                    ProfileNode profile_node;
                    while (q->GetStatus() == QREADING) {
                        bThreadYield(8);
                        bServiceFileSystem();
                    }
                    q->GetStatus();
                }
            } else {
                Joylog::AddData(status, 4, JOYLOG_CHANNEL_QUEUEDFILE_STATUS);
            }
        }
        if (status == QDONE) {
            ReadingQueuedFileList.Remove(q);
            QueuedFileNumReadsInProgress--;
            if (q->IsFinishedAllReading()) {
                ProfileNode profile_node;
                q->CallDoneCallback(0);
            } else {
                q->SetStatus(QWAITING);
                WaitingQueuedFileList.AddTail(q);
                return;
            }
        } else {
            if (status == QERROR) {
                ReadingQueuedFileList.Remove(q);
                QueuedFileNumReadsInProgress--;
                q->CallDoneCallback(1);
            } else {
                // hack
                do {
                } while (false);
                return;
            }
        }
        delete q;
        q = nullptr;
    }
}

void StartQueuedFileReading() {
    // Las dos PS2 comprueban las lecturas en curso en la cabecera del bucle (lo rota: la vuelve a mirar al final), y la
    // A124 todavia no descomprime; GameCube sale del bucle con return.
#ifdef EA_BUILD_A124
    while (QueuedFileNumReadsInProgress <= 2 && !WaitingQueuedFileList.IsEmpty())
#elif defined(EA_PLATFORM_PLAYSTATION2)
    while (QueuedFileNumReadsInProgress - QueuedFile::GetNumFilesDecompressing() <= 2 && !WaitingQueuedFileList.IsEmpty())
#else
    while (true)
#endif
    {
#ifndef EA_PLATFORM_PLAYSTATION2
        if ((QueuedFileNumReadsInProgress - QueuedFile::GetNumFilesDecompressing() >= 3) || WaitingQueuedFileList.IsEmpty()) {
            return;
        }
#endif
        WaitingQueuedFileList.Sort(&QueuedFile::SortByPriority);
        {
            QueuedFile *q = WaitingQueuedFileList.GetHead();
            if (QueuedFileNumReadsInProgress > 0) {
                if (bStrCmp(q->GetFilename(), LastQueuedFilename) != 0) {
                    return;
                }
            }
        }
        if (EnableQueuedFileBundle) {
            QueuedFileBundle *q_bundle = new QueuedFileBundle();

            for (QueuedFile *q = WaitingQueuedFileList.GetHead(); q != WaitingQueuedFileList.EndOfList(); q = q->GetNext()) {
                if (!q_bundle->TestAddQueuedFile(q)) {
                    break;
                }
            }
            if (q_bundle->NumQueuedFiles < 2) {
                delete q_bundle;
            } else {
                q_bundle->BeginRead();
                for (int n = 0; n < q_bundle->NumQueuedFiles; n++) {
                    WaitingQueuedFileList.Remove(q_bundle->QueuedFiles[n]);
                }
            }
        }
        QueuedFile *q = WaitingQueuedFileList.GetHead();
        if (q->GetPriority() < QueuedFileMinPriority) {
            if (QueuedFileNumReadsInProgress == 0) {
                if (QueuedFileMinPriorityTimeoutCounter++ > 10) {
                    QueuedFileMinPriority = 0;
                }
            }
            return;
        }
        QueuedFileMinPriorityTimeoutCounter = 0;
        bSafeStrCpy(LastQueuedFilename, q->GetFilename(), sizeof(LastQueuedFilename));
        q->BeginRead();
        WaitingQueuedFileList.Remove(q);
        ReadingQueuedFileList.AddTail(q);
        QueuedFileNumReadsInProgress++;
    }
}

void ServiceQueuedFiles() {
    ProfileNode profile_node;
    CheckQueuedFileCallbacks();
    StartQueuedFileReading();
}

bool IsQueuedFileBusy() {
    return QueuedFileNumReadsInProgress != 0 || !WaitingQueuedFileList.IsEmpty();
}

void BlockWhileQueuedFileBusy() {
    ServiceQueuedFiles();
    while (IsQueuedFileBusy()) {
#ifndef EA_PLATFORM_PLAYSTATION2
        DVDErrorTask(nullptr, 0);
#endif
        bThreadYield(8);
        ServiceQueuedFiles();
    }
}

void InitQueuedFiles() {
    QueuedFileSlotPool = bNewSlotPool(sizeof(QueuedFile), 200, "QueuedFileSlotPool", 0);
}

int GetQueuedFileSize(const char *filename) {
    return bFileSize(filename);
}

void AddQueuedFile(void *buf, const char *filename, int file_pos, int num_bytes,
#ifdef EA_BUILD_A124
                   void (*callback)(int, int), int callback_param,
#else
                   void (*callback)(void *, int), void *callback_param,
#endif
                   QueuedFileParams *params) {
    QueuedFile *q;
    if (!IsQueuedFileJoyloggable(filename)) {
        bFile *f = bOpen(filename, 1, 1);
        bSeek(f, file_pos, 0);
#ifdef EA_BUILD_A124
        bReadAsync(f, buf, num_bytes, ((void (*)(int))callback), callback_param);
#else
        bReadAsync(f, buf, num_bytes, ((void (*)(void *))callback), callback_param);
#endif
        bClose(f);
    } else {
        q = new (bOMalloc(QueuedFileSlotPool)) QueuedFile(buf, filename, file_pos, num_bytes, ((void *)callback), callback_param, params);
        WaitingQueuedFileList.AddTail(q);
    }
}

void AddQueuedFile2(void *buf, const char *filename, int file_pos, int num_bytes,
#ifdef EA_BUILD_A124
                    void (*callback)(int, int, int), int callback_param, int callback_param2,
#else
                    void (*callback)(void *, int, void *), void *callback_param, void *callback_param2,
#endif
                    QueuedFileParams *params) {
    QueuedFile *q = new (bOMalloc(QueuedFileSlotPool)) QueuedFile(buf, filename, file_pos, num_bytes, ((void *)callback), callback_param, params);
    q->SetCallbackParam2(callback_param2);
    WaitingQueuedFileList.AddTail(q);
}

#if defined(EA_PLATFORM_XENON) && defined(MILESTONE_BUILD)
// EGM 0x823EDF48 (ProStreet: QueuedFileSetJoylogEnabled, 12 B). Es la unica escritura de QueuedFileJoylogEnabled: sin
// ella LTCG pliega la lectura de CheckQueuedFileCallbacks.
void QueuedFileSetJoylogEnabled(bool enabled) {
    QueuedFileJoylogEnabled = enabled;
}
#endif
