#include "Speed/Indep/Libs/realcore/6.24.00/include/common/realcore/file/driver.h"
#include "Speed/Indep/Libs/realcore/6.24.00/include/common/realcore/std/list.h"
#include "Speed/Indep/Libs/realcore/6.24.00/include/common/realcore/system.h"
#include "Speed/Indep/Libs/realcore/6.24.00/include/common/realcore/std.h"
#include <string.h>
struct SIGNAL { // opaco y de plataforma, como MUTEX (system.h): 0xC en PS2 (stabs de la A124)
#ifdef EA_PLATFORM_PLAYSTATION2
    int reserved[3]; // offset 0x0, size 0xC
#elif defined(EA_PLATFORM_WIN32)
    int reserved[2]; // 0x8 en PC: SIGNAL_create(dev + 0x44) y el atomic en 0x4C (speed.exe 0x7ED751)
#else
    int reserved[10]; // offset 0x0, size 0x28
#endif
};
struct THREAD { // 0x8 en PS2: THREAD_create(dev + 0x54) y pThreadStack en 0x5C (SLES 0x4FEE84, SLUS 0x48C564)
#ifdef EA_PLATFORM_PLAYSTATION2
    int reserved[2]; // offset 0x0, size 0x8
#elif defined(EA_PLATFORM_WIN32)
    int reserved[3]; // 0xC en PC: THREAD_create(dev + 0x68) guarda el HANDLE de _beginthreadex en +4 (0x7EC44F)
#else
    int reserved[198]; // offset 0x0, size 0x318
#endif
};
bool SIGNAL_create(SIGNAL *s);
bool THREAD_create(THREAD *t, int (*func)(void *), void *param, void *stack, int stacksize,
                   int priority);
void SIGNAL_set(SIGNAL *s);
void SIGNAL_wait(SIGNAL *s);
void SIGNAL_destroy(SIGNAL *s);
#ifdef EA_PLATFORM_WIN32
// PC: la atencion del dispositivo es un evento de Win32 (speed.exe 0x7ED751 y 0x7ED66A llaman por la tabla de
// importaciones a CreateEventA, WaitForSingleObject, ResetEvent, SetEvent y CloseHandle) y el hilo se destruye.
extern "C" {
__declspec(dllimport) void *__stdcall CreateEventA(void *lpEventAttributes, int bManualReset, int bInitialState,
                                                   const char *lpName);
__declspec(dllimport) unsigned long __stdcall WaitForSingleObject(void *hHandle, unsigned long dwMilliseconds);
__declspec(dllimport) int __stdcall ResetEvent(void *hEvent);
__declspec(dllimport) int __stdcall SetEvent(void *hEvent);
__declspec(dllimport) int __stdcall CloseHandle(void *hObject);
}
void THREAD_destroy(THREAD *t);
#endif

// total size: 0xC
struct TagValuePair {
    unsigned int mTag; // offset 0x0, size 0x4
    union {
        int mInt;             // offset 0x0, size 0x4
        unsigned int mSize;   // offset 0x0, size 0x4
        float mFloat;         // offset 0x0, size 0x4
        const void *mPointer; // offset 0x0, size 0x4
    } mValue;                 // offset 0x4, size 0x4
    mutable const TagValuePair *mNext; // offset 0x8, size 0x4

    TagValuePair(unsigned int tag, int value) {
        mTag = tag;
        mValue.mInt = value;
        mNext = 0;
    }
    TagValuePair(unsigned int tag, unsigned int value) {
        mTag = tag;
        mValue.mSize = value;
        mNext = 0;
    }
    TagValuePair(unsigned int tag, float value) {
        mTag = tag;
        mValue.mFloat = value;
        mNext = 0;
    }
    TagValuePair(unsigned int tag, const void *value) {
        mTag = tag;
        mValue.mPointer = value;
        mNext = 0;
    }

    const TagValuePair &operator+(const TagValuePair &rhs) const {
        rhs.mNext = this;
        return rhs;
    }
};

// total size: 0x4
struct IAllocator {
    virtual void *Alloc(unsigned int size, const TagValuePair &flags) = 0;
    virtual void Free(void *pBlock, unsigned int size) = 0;
    virtual int AddRef() = 0;
    virtual int Release() = 0;

  protected:
    virtual ~IAllocator() {}
};

struct FILESYSOPTS {
    int size;                                // +0x00
    IAllocator *allocator;                   // +0x04
    int MaxOpenFiles;                        // +0x08
    int MaxFileOps;                          // +0x0C
    int nSearchLocs;                         // +0x10
    int nSearchPathLength;                   // +0x14
    int MaxDevices;                          // +0x18
    int ThreadStackSize;                     // +0x1C
    int (*decompresssize)(const void *);     // +0x20
    int (*decompress)(const void *, void *); // +0x24
    unsigned int LargeReadSliceSize;         // +0x28
    unsigned int AllocAlignBoundary;         // +0x2C
    int DiscType;                            // +0x30
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

extern FILESYSOPTS gFileSysOpts;

struct FILEDEVICE;
struct FILESYSHANDLE;

// Relleno: el cuerpo de ~FILEOPERATION cae en la linea 71 y los accesores
// GetId/GetPriority/GetStatus/GetCallback/IsCancelled en 86..90.
//
//
//
//
// total size: 0x48
struct FILEOPERATION : public ListNodeS {
    virtual ~FILEOPERATION() {
    }

    virtual void Exec(FILEDEVICE *device) = 0;
    virtual int64_t Complete() = 0;
    virtual void Cancel(FILEDEVICE *device);

    //
    //
    //
    //
    //
    //
    //
    //
    //
    //
    int GetId() {
        return id;
    }
    int GetPriority() {
        return priority;
    }
    int GetStatus() {
        return status;
    }
    void (*GetCallback())(int, int, void *) {
        return callback;
    }
    bool IsCancelled() {
        return cancelled == true;
    }
    void *GetUserData() {
        return userdata;
    }
    void ClearId() {
        id = 0;
    }
    void SetPriority(unsigned char _priority) {
        priority = _priority;
    }
    void SetCallback(void (*_callback)(int, int, void *)) {
        callback = _callback;
    }

    static FILEDEVICE *GetDevice(int ophandle);
    int iGetNextOpId(FILEDEVICE *dev);
    FILEOPERATION(int _priority, void *_userdata, FILEDEVICE *device);
    static void *operator new(unsigned int);
    void SetName(const char *name);
    void AddToQueue();

    int id;                              // offset 0x4, size 0x4
    bool cancelled;                      // offset 0x8, size 0x1
    signed char status;                  // offset 0xC, size 0x1
    unsigned char priority;              // offset 0xD, size 0x1
    FILESYSHANDLE *filehandle;           // offset 0x10, size 0x4
    void *userdata;                      // offset 0x14, size 0x4
    void (*callback)(int, int, void *);  // offset 0x18, size 0x4
    int64_t value;                       // offset 0x20, size 0x8
    int64_t amount;                      // offset 0x28, size 0x8
    void *data;                          // offset 0x30, size 0x4
    uint64_t totalbytes;                 // offset 0x38, size 0x8
};

// total size: 0xC
struct FileOperationQueue : public ListSingle<FILEOPERATION> {
    FileOperationQueue() {
    }

    FILEOPERATION *Find(int ophandle, bool bRemoveFromQueue);
};

// total size: 0x20
struct FILESYSHANDLE : public ListNodeS {
    FILESYSHANDLE *pParentFile; // offset 0x4, size 0x4
    int hFile;                  // offset 0x8, size 0x4
    FILEDEVICE *dev;            // offset 0xC, size 0x4
    int64_t length;             // offset 0x10, size 0x8
    int64_t offset;             // offset 0x18, size 0x8
};

// total size: 0x3D4
struct FILEDEVICE : public ListNodeS {
    volatile int running;           // offset 0x4, size 0x4
    volatile int terminate;         // offset 0x8, size 0x4
    FILEOPERATION *current;         // offset 0xC, size 0x4
    FileOperationQueue pending;     // offset 0x10, size 0xC
    FileOperationQueue completed;   // offset 0x1C, size 0xC
    RealSystem::Mutex mutex;        // offset 0x28, size 0x1C
    SIGNAL signal;                  // offset 0x44, size 0x28
    RealSystem::Mutex atomic;       // offset 0x6C, size 0x1C
    THREAD deviceThread;            // offset 0x88, size 0x318
    void *pThreadStack;             // offset 0x3A0, size 0x4
#ifdef EA_PLATFORM_WIN32
    void *attention;                // PC: HANDLE de un evento de reinicio manual (CreateEventA); drv en 0x80
#else
    SIGNAL attention;               // offset 0x3A4, size 0x28
#endif
    int minpriority;                // offset 0x3CC, size 0x4
    RealFile::DeviceDriver *drv;    // offset 0x3D0, size 0x4
};

// total size: 0x9C
struct FILESYSINFO {
    ~FILESYSINFO();

    ListSingle<FILEOPERATION> FreeOps;             // offset 0x0, size 0xC
    RealSystem::Mutex FreeOpsMutex;                // offset 0xC, size 0x1C
    ListSingle<FILESYSHANDLE> FreeFileSysHandles;  // offset 0x28, size 0xC
    RealSystem::Mutex FreeFileSysHandlesMutex;     // offset 0x34, size 0x1C
    int NextOp;                                    // offset 0x50, size 0x4
    RealSystem::Mutex NextOpMutex;                 // offset 0x54, size 0x1C
    ListSingle<FILEDEVICE> FreeDevices;            // offset 0x70, size 0xC
    ListSingle<FILEDEVICE> AllocatedDevices;       // offset 0x7C, size 0xC
    char **psearchpaths;                           // offset 0x88, size 0x4
    char *searchpath;                              // offset 0x8C, size 0x4
    FILEOPERATION *OpMemory;                       // offset 0x90, size 0x4
    FILESYSHANDLE *FileSysHandleMemory;            // offset 0x94, size 0x4
    FILEDEVICE *DeviceMemory;                      // offset 0x98, size 0x4
};

static FILESYSINFO *gpFileSysInfo;
static void *pDeviceMem = 0;

static void iStartDevice(FILEDEVICE *dev);
static FILESYSHANDLE *iAllocateFileSysHandle();
static void iFreeFileSysHandle(FILESYSHANDLE *fs_h);
static FILESYSHANDLE *iOpenFileSysHandle(const char *filename, int modeflags, FILEDEVICE *device);
static void iCloseFileSysHandle(FILESYSHANDLE *h);
static FILEOPERATION *iGetOpFromHandle(FILEDEVICE *device, int ophandle, int *ispending);
static int iDeviceCommandProcessorThreadFunc(void *param);

extern int bIsFileSysInitialized;

FILEDEVICE *FILE_nametodevice(const char *name);
int FILE_overhead();
void FILE_restore();
extern RealFile::DeviceDriver *libdevice[2];
void SYSTEM_addexit(void (*func)());

inline void *operator new(unsigned int, void *place, unsigned int) throw() {
    return place;
}
int THREAD_iscurrent(THREAD *t);
void THREAD_yield(int ms);
void SYNCTASK_run();

RealFile::DeviceDriver::DeviceDriver(const char *name) {
    strcpy(mDeviceName, name);
}

// total size: 0x14
class NullFileDriver : public RealFile::DeviceDriver {
  public:
    NullFileDriver(const char *pName) : DeviceDriver(pName) {}

    virtual EAFileHandle Open(const char *, int, int *) {
        return -1;
    }
    virtual void Close(EAFileHandle) {}
    virtual uint32_t Read(EAFileHandle, void *, unsigned int, DeviceDriver *, EAFileHandle) {
        return 0;
    }
    virtual uint64_t Seek(EAFileHandle, uint64_t, int, DeviceDriver *, EAFileHandle) {
        return 0;
    }
    virtual uint64_t Getsize(EAFileHandle) {
        return 0;
    }
};

static NullFileDriver nulldrv("null:");

static void iDefaultFilesysCallbackFunc(int, int, void *) {
}

FILEDEVICE *FILEOPERATION::GetDevice(int ophandle) {
    return &gpFileSysInfo->DeviceMemory[(unsigned char)ophandle];
}

FILEOPERATION *FileOperationQueue::Find(int ophandle, bool bRemoveFromQueue) {
    ListSingleIterator<FILEOPERATION> Itr;

    Itr = this->Begin();
    if (*Itr != 0) {
        do {
            if ((*Itr)->GetId() == ophandle) {
                if (bRemoveFromQueue) {
                    // Keep the queue's removal inline, including its empty-list case.
                    FILEOPERATION *node = *Itr;
                    FILEOPERATION *p;
                    bool foundit = false;
                    if (node == this->head) {
                        foundit = true;
                        this->nNodes--;
                        if (node == this->tail) {
                            this->head = 0;
                            this->tail = 0;
                        } else {
                            this->head = (FILEOPERATION *)node->next;
                        }
                    } else if (this->head != 0) {
                        p = this->head;
                        while (p->next != 0 && p->next != node) {
                            p = (FILEOPERATION *)p->next;
                        }
                        if (p->next != 0 && p->next == node) {
                            foundit = true;
                            this->nNodes--;
                            p->next = node->next;
                            if (node == this->tail) {
                                this->tail = p;
                            }
                        }
                    }
                    if (foundit) {
                        node->next = 0;
                    }
                }

                return *Itr;
            }
            Itr++;
        } while (*Itr != 0);
    }

    return 0;
}

static void iStartDevice(FILEDEVICE *dev) {
    gpFileSysInfo->FreeOpsMutex.Lock();

    dev->mutex.Create();

    dev->current = 0;

    SIGNAL_create(&dev->signal);

    //
    //
#ifdef EA_PLATFORM_WIN32
    dev->attention = CreateEventA(0, 1, 0, 0);
#else
    SIGNAL_create(&dev->attention);
#endif

    dev->atomic.Create();
    dev->minpriority = 255;

    dev->drv->Init();

    //
    //
    //
    //
    //
    //
    //
    //
    //
#ifndef EA_PLATFORM_WIN32
    // PC no reserva la pila del hilo (speed.exe 0x7ED751 pasa pThreadStack tal cual)
    dev->pThreadStack = gFileSysOpts.allocator->Alloc(
        gFileSysOpts.ThreadStackSize,
        TagValuePair(4, 1) + TagValuePair(2, 32u) +
            TagValuePair(1, (const void *)"File System") +
            TagValuePair(5, (const void *)"d:/packages/realcore/6.24.00/source/file/cmn/filesys.cpp") +
            TagValuePair(6, 312));
#endif

    //
    //
    //
    //
    //
    //
    //
    //
    //
    //
    //
    //
    //
    //
    //
    THREAD_create(&dev->deviceThread, iDeviceCommandProcessorThreadFunc, dev, dev->pThreadStack,
                  gFileSysOpts.ThreadStackSize, 1);

    //
    //
    //
    //
    //
    //
    //
#ifdef EA_PLATFORM_WIN32
    WaitForSingleObject(dev->attention, 0xFFFFFFFF);
    ResetEvent(dev->attention);
#else
    SIGNAL_wait(&dev->attention);
#endif

    //
    //
    gpFileSysInfo->FreeOpsMutex.Unlock();
}

int FILEOPERATION::iGetNextOpId(FILEDEVICE *dev) {
    int op;

    gpFileSysInfo->NextOpMutex.Lock();

    op = gpFileSysInfo->NextOp;
    gpFileSysInfo->NextOp = op + 1;

    //
    //
    //
    //
    //
    if (gpFileSysInfo->NextOp == 0x10000) {
        gpFileSysInfo->NextOp = 1;
    }

    gpFileSysInfo->NextOpMutex.Unlock();

    return (op << 8) | (dev - gpFileSysInfo->DeviceMemory);
}

FILEOPERATION::FILEOPERATION(int _priority, void *_userdata, FILEDEVICE *device) {
    //
    //
    //
    //
    //
    //
    if (device->running == 0) {
        iStartDevice(device);
    }

    //
    priority = _priority;
    cancelled = false;
    status = 0;
    filehandle = (FILESYSHANDLE *)-1;
    userdata = _userdata;
    callback = iDefaultFilesysCallbackFunc;
    value = 0;
    amount = 0;
    data = 0;

    id = iGetNextOpId(device);
}

void *FILEOPERATION::operator new(unsigned int) {
    FILEOPERATION *op;

    gpFileSysInfo->FreeOpsMutex.Lock();

    op = gpFileSysInfo->FreeOps.Pop();

    gpFileSysInfo->FreeOpsMutex.Unlock();

    //
    //
    //
    //
    return op;
}

void FILEOPERATION::SetName(const char *name) {
    int len;

    //
    //
    //
    if (name == 0) {
        name = "";
    }
    len = strlen(name) + 1;

    //
    //
    //
    //
    //
    //
    data = gFileSysOpts.allocator->Alloc(
        len, TagValuePair(4, 1) + TagValuePair(1, (const void *)name) +
                 TagValuePair(5, (const void *)"d:/packages/realcore/6.24.00/source/file/cmn/filesys.cpp") +
                 TagValuePair(6, 432));
    MEM_copy(data, name, len);
}

void FILEOPERATION::Cancel(FILEDEVICE *device) {
    cancelled = true;

    //
    //
    if (device->pending.Remove(this, 0)) {

        device->completed.Push(this);

        //
        callback(id, -1, userdata);
    }
}

static FILESYSHANDLE *iAllocateFileSysHandle() {
    FILESYSHANDLE *fs_h;

    gpFileSysInfo->FreeFileSysHandlesMutex.Lock();
    fs_h = gpFileSysInfo->FreeFileSysHandles.Pop();
    gpFileSysInfo->FreeFileSysHandlesMutex.Unlock();

    //
    //
    //
    //
    fs_h->hFile = -1;

    return fs_h;
}

static void iFreeFileSysHandle(FILESYSHANDLE *fs_h) {
    //
    //
    MEM_clear(fs_h, sizeof(FILESYSHANDLE));
    //
    gpFileSysInfo->FreeFileSysHandlesMutex.Lock();
    gpFileSysInfo->FreeFileSysHandles.Push(fs_h);
    gpFileSysInfo->FreeFileSysHandlesMutex.Unlock();
}

static FILESYSHANDLE *iOpenFileSysHandle(const char *filename, int modeflags,
                                         FILEDEVICE *device) {
    int ret;
    int hParentFile;
    FILESYSHANDLE *h;
    char newname[256];
    int i;
    char *searchpath;

    ret = -1;
    hParentFile = -1;

    if (device == gpFileSysInfo->DeviceMemory) {

        //
        //
        //
        //
        for (i = 0; i < gFileSysOpts.nSearchLocs; i++) {

            searchpath = gpFileSysInfo->psearchpaths[i];

            //
            //
            //
            if (searchpath == 0) {
                break;
            }

            //
            //
            //
            //
            device = FILE_nametodevice(searchpath);

            //
            //
            //
            //
            //
            //
            if (device->running == 0) {
                iStartDevice(device);
            }

            //
            //
            //
            strcpy(newname, searchpath);
            strcat(newname, "/");
            strcat(newname, filename);

            //
            ret = device->drv->Open(newname, modeflags, &hParentFile);

            //
            //
            if (ret != -1) {
                break;
            }
        }

    } else {

        //
        //
        ret = device->drv->Open(filename, modeflags, &hParentFile);
    }

    //
    //
    if (ret != -1) {

        //
        //
        //
        if (modeflags & 0x10000) {

            //
            device->drv->Close(ret);

            //
            return (FILESYSHANDLE *)1;
        }

        //
        //
        h = iAllocateFileSysHandle();

        //
        h->hFile = ret;
        h->dev = device;
        h->length = device->drv->Getsize(ret);
        h->pParentFile = h;

        if (hParentFile != -1) {

            //
            //
            //
            //
            h->pParentFile = (FILESYSHANDLE *)hParentFile;
        }

        //
        //
        return h;
    }

    //
    //
    return (FILESYSHANDLE *)-1;
}

static void iCloseFileSysHandle(FILESYSHANDLE *h) {
    h->dev->drv->Close(h->hFile);

    iFreeFileSysHandle(h);
}

static FILEOPERATION *iGetOpFromHandle(FILEDEVICE *device, int ophandle, int *ispending) {
    FILEOPERATION *result = 0;

    *ispending = 0;

    if (device->running != 0) {

        result = device->current;

        if (result != 0 && result->GetId() == ophandle) {

            *ispending = -1;

        } else {

            result = device->pending.Find(ophandle, false);

            if (result != 0) {

                *ispending = 1;

            } else {

                result = device->completed.Find(ophandle, false);
            }
        }
    }

    return result;
}

void FILEOPERATION::AddToQueue() {
    FILEDEVICE *device = GetDevice(id);
    ListSingleIterator<FILEOPERATION> Itr;
    FILEOPERATION *prev;
    unsigned int newoppriority;

    device->mutex.Lock();

    prev = 0;

    newoppriority = (priority << 24) | (id & 0x00FFFF00);

    // La forma de los stabs de la A124 y del DWARF de GameCube: Itr, prev y newoppriority en la funcion,
    // curpriority en el bloque del cuerpo y el ++ del iterador fuera de el; priority e id se leen directos
    // (el DWARF no tiene GetPriority/GetId aqui). Los dos originales tienen el bucle enrollado (la prueba
    // y el break al final; en PS2 ademas duplicados delante) y expand_end_loop solo lo enrolla con -O2:
    // a -O1 no cruza las notas de bloque (GameCube con los cflags de realcore de hoy: 83,65 %).
    for (Itr = device->pending.Begin(); *Itr != 0; Itr++) {
        unsigned int curpriority = ((*Itr)->priority << 24) | ((*Itr)->id & 0x00FFFF00);

        if (newoppriority < curpriority) {
            break;
        }

        prev = *Itr;
    }

    device->pending.InsertAfter(prev, this);

    device->mutex.Unlock();
    SIGNAL_set(&device->signal);
}

static int iDeviceCommandProcessorThreadFunc(void *param) {
    FILEDEVICE *device = (FILEDEVICE *)param;
    FILEOPERATION *op;

    device->running = 1;

#ifdef EA_PLATFORM_WIN32
    SetEvent(device->attention);
#else
    SIGNAL_set(&device->attention);
#endif

    while (device->terminate == 0) {

        device->mutex.Lock();

        device->current = 0;

        op = device->pending.Head();
        if (op != 0 && op->GetPriority() <= device->minpriority) {

            device->pending.Pop();

            device->current = op;
        }

        device->mutex.Unlock();

        if (device->current == 0) {

            SIGNAL_wait(&device->signal);

        } else {

            if (op->IsCancelled() != true) {

                op->Exec(device);
            }

            if (op->GetStatus() != 0) {

                device->mutex.Lock();

                device->completed.Push(op);

                device->current = 0;

                device->mutex.Unlock();

                op->GetCallback()(op->GetId(),
                                  op->IsCancelled() ? -1 : op->GetStatus(),
                                  op->GetUserData());

#ifdef EA_PLATFORM_WIN32
                SetEvent(device->attention);
#else
                SIGNAL_set(&device->attention);
#endif
            }
        }
    }

    device->running = 0;

    return 0;
}

bool FILE_init(void *buf, int bufsize) {
    int i;
    int nDevices;
    FILEOPERATION *op;
    FILESYSHANDLE *fs_h;
    FILEDEVICE *dev;
    FILEDEVICE *device;

    //
    //
    //
    //
    //
    //
    //
    if (buf == 0) {

        bufsize = FILE_overhead();

        pDeviceMem = gFileSysOpts.allocator->Alloc(bufsize,
                                            TagValuePair(1, (const void *)"File System"));
        buf = pDeviceMem;
    }

    //
    //
    //
    //
    //
    MEM_clear(buf, bufsize);

    gpFileSysInfo = new (buf, bufsize) FILESYSINFO;

    gpFileSysInfo->psearchpaths = (char **)(gpFileSysInfo + 1);

    gpFileSysInfo->searchpath =
        (char *)gpFileSysInfo->psearchpaths + gFileSysOpts.nSearchLocs * 4;

    // AlignAddrUp dos veces (dos `void *p` en los stabs y dos expansiones en el DWARF de GameCube).
    // En GameCube el segundo no deja codigo: combine sabe que OpMemory + n * 0x48 ya es multiplo de 8;
    // en PS2 la mascara -8 va en registro y el alineado se queda.
    gpFileSysInfo->OpMemory = (FILEOPERATION *)AlignAddrUp(
        gpFileSysInfo->searchpath + gFileSysOpts.nSearchPathLength, 8);

    gpFileSysInfo->FileSysHandleMemory = (FILESYSHANDLE *)AlignAddrUp(
        gpFileSysInfo->OpMemory + gFileSysOpts.MaxFileOps, 8);

    gpFileSysInfo->DeviceMemory =
        (FILEDEVICE *)(gpFileSysInfo->FileSysHandleMemory + gFileSysOpts.MaxOpenFiles);

    //
    //
    gpFileSysInfo->FreeOpsMutex.Create();
    gpFileSysInfo->FreeFileSysHandlesMutex.Create();

    //
    op = gpFileSysInfo->OpMemory;
    //
    //
    for (i = 0; i < gFileSysOpts.MaxFileOps; i++) {

        gpFileSysInfo->FreeOps.PushTail(op);
        op++;
    }
    //
    //
    fs_h = gpFileSysInfo->FileSysHandleMemory;
    for (i = 0; i < gFileSysOpts.MaxOpenFiles; i++) {

        gpFileSysInfo->FreeFileSysHandles.PushTail(fs_h);
        fs_h++;
    }
    //
    //
    dev = gpFileSysInfo->DeviceMemory;
    for (i = 0; i < gFileSysOpts.MaxDevices; i++) {

        gpFileSysInfo->FreeDevices.PushTail(dev);
        dev++;
    }
    //
    //
    gpFileSysInfo->NextOp = 1;
    gpFileSysInfo->NextOpMutex.Create();

    //
    //
    //
    //
    //
    //
    //
    //
    //
    //
    //
    //
    //
    //
    RealFile::AddDevice(&nulldrv);

    //
    //
    //
    //
    //
    nDevices = sizeof(libdevice) / sizeof(libdevice[0]);
    for (i = 0; i < nDevices; i++) {
        RealFile::AddDevice(libdevice[i]);
    }

    //
    //
    //
    //
    //
    //
    //
    //
    //
    //
    //
    //
    //
#ifdef EA_PLATFORM_PLAYSTATION2
    // SLES y SLUS: "host0:" con DiscType 0 y "cdrom0:" con el resto, en ese orden en .rodata.
    RealFile::SetSearchPath(gFileSysOpts.DiscType == 0 ? "host0:" : "cdrom0:");
#else
    RealFile::SetSearchPath(gFileSysOpts.DiscType == 1 ? "dvd:" : "hd:");
#endif

    //
    //
    //
    //
    iStartDevice(FILE_nametodevice(gpFileSysInfo->psearchpaths[0]));

    //
    SYSTEM_addexit(FILE_restore);

    //
    bIsFileSysInitialized = 1;

    //
    return true;
}

int FILE_overhead() {
    int operations = sizeof(FILESYSINFO) + gFileSysOpts.MaxFileOps * sizeof(FILEOPERATION);
    int handles = gFileSysOpts.MaxOpenFiles * sizeof(FILESYSHANDLE);
    handles += operations;
    handles += gFileSysOpts.MaxDevices * (sizeof(FILEDEVICE) + 1);
    handles += gFileSysOpts.nSearchLocs * 4;
    handles += gFileSysOpts.nSearchPathLength;
    return handles + 24;
}

void FILE_restore() {
    int i;

    //
    //
    //
    for (i = 0; i < gFileSysOpts.MaxDevices; i++) {

        RealFile::RemoveDevice(i);
    }

    gpFileSysInfo->NextOpMutex.Destroy();

    //
    //
    gpFileSysInfo->FreeFileSysHandlesMutex.Destroy();
    gpFileSysInfo->FreeOpsMutex.Destroy();

    //
    //
    //
    //
    //
    //
    //
    gpFileSysInfo->FILESYSINFO::~FILESYSINFO();
    gpFileSysInfo = 0;

    if (pDeviceMem != 0) {

        //
        gFileSysOpts.allocator->Free(pDeviceMem, 0);
        pDeviceMem = 0;
    }

    //
    //
    bIsFileSysInitialized = 0;
}

inline FILESYSINFO::~FILESYSINFO() {
}

// total size: 0x48
struct ExistOperation : public FILEOPERATION {
    ExistOperation(const char *name, int priority, void *userdata, FILEDEVICE *dev)
        : FILEOPERATION(priority, userdata, dev) {
        SetName(name);
    }

    virtual void Exec(FILEDEVICE *device) {
        value = (iOpenFileSysHandle((const char *)data, 0x30001, device) != (FILESYSHANDLE *)-1);
        status = 1;
    }

    virtual int64_t Complete() {
        gFileSysOpts.allocator->Free(data, 0);
        return value;
    }
};

// total size: 0x48
struct OpenOperation : public FILEOPERATION {
    OpenOperation(const char *name, unsigned int modeflags, int priority, void *userdata,
                  FILEDEVICE *dev)
        : FILEOPERATION(priority, userdata, dev) {
        SetName(name);
        value = modeflags;
    }

    virtual void Exec(FILEDEVICE *device) {
        filehandle = iOpenFileSysHandle((const char *)data, (int)value, device);

        if (filehandle != (FILESYSHANDLE *)-1) {
            status = 1;
        } else {
            status = -2;
        }
    }

    virtual int64_t Complete() {
        gFileSysOpts.allocator->Free(data, 0);
        if (IsCancelled() == true && filehandle != (FILESYSHANDLE *)-1) {

            iCloseFileSysHandle(filehandle);

        } else if (filehandle != (FILESYSHANDLE *)-1) {

            return (int)filehandle;
        }

        return 0;
    }
};

// total size: 0x48
struct CloseOperation : public FILEOPERATION {
    CloseOperation(FILESYSHANDLE *_filehandle, int priority, void *userdata, FILEDEVICE *dev)
        : FILEOPERATION(priority, userdata, dev) {
        filehandle = _filehandle;
    }

    virtual void Exec(FILEDEVICE *device) {
        status = 1;
    }

    virtual int64_t Complete() {
        iCloseFileSysHandle(filehandle);
        return 1;
    }

    virtual void Cancel(FILEDEVICE *device) {
    }
};

// total size: 0x48
struct ReadOperation : public FILEOPERATION {
    ReadOperation(FILESYSHANDLE *_filehandle, int bytes, int offset, void *buffer, int priority,
                  void *userdata, FILEDEVICE *dev)
        : FILEOPERATION(priority, userdata, dev) {
        filehandle = _filehandle;
        amount = bytes;
        value = offset;
        data = buffer;
        totalbytes = 0;
    }

    virtual void Exec(FILEDEVICE *device) {
        unsigned int readsize;
        unsigned int bytesread;
        unsigned int optimal;

        //
        //
        //
        //
        //
        //
        //
        //
        //
        //
        //
        //
        //
        //
        //
        //
        //
        //
        filehandle->pParentFile->offset =
            device->drv->Seek(filehandle->hFile, value, 0, filehandle->pParentFile->dev->drv,
                              filehandle->pParentFile->hFile);

        readsize = amount;

        //
        //
        optimal = device->drv->GetOptimalReadSize();

        if (optimal != 0 && readsize > optimal) {

            readsize = optimal;
        }

        //
        //
        //
        bytesread =
            device->drv->Read(filehandle->hFile, data, readsize,
                              filehandle->pParentFile->dev->drv, filehandle->pParentFile->hFile);

        //
        //
        //
        amount -= bytesread;
        value += bytesread;
        totalbytes += bytesread;
        data = (char *)data + bytesread;

        //
        //
        //
        //
        //
        if (amount > 0 && bytesread == readsize) {
            AddToQueue();
        } else {
            status = 1;
        }
    }

    virtual int64_t Complete() {
        return totalbytes;
    }
};

// total size: 0x48
struct ReadLargeOperation : public FILEOPERATION {
    ReadLargeOperation(FILESYSHANDLE *_filehandle, uint64_t bytes,
                       uint64_t offset, void *buffer, int priority, void *userdata,
                       FILEDEVICE *dev)
        : FILEOPERATION(priority, userdata, dev) {
        filehandle = _filehandle;
        amount = bytes;
        value = offset;
        data = buffer;
        totalbytes = 0;
    }

    virtual void Exec(FILEDEVICE *device) {
        unsigned int readsize;
        unsigned int bytesread;
        unsigned int optimal;

        //
        //
        //
        //
        //
        //
        //
        //
        //
        //
        //
        //
        //
        //
        //
        //
        //
        //
        //
        //
        //
        //
        filehandle->pParentFile->offset =
            device->drv->Seek(filehandle->hFile, value, 0, filehandle->pParentFile->dev->drv,
                              filehandle->pParentFile->hFile);

        readsize = gFileSysOpts.LargeReadSliceSize;
        readsize = ((unsigned int)amount < readsize) ? (unsigned int)amount : readsize;

        optimal = device->drv->GetOptimalReadSize();

        if (optimal != 0 && readsize > optimal) {

            readsize = optimal;
        }

        //
        //
        //
        bytesread =
            device->drv->Read(filehandle->hFile, data, readsize,
                              filehandle->pParentFile->dev->drv, filehandle->pParentFile->hFile);

        //
        //
        //
        amount -= bytesread;
        value += bytesread;
        totalbytes += bytesread;
        data = (char *)data + bytesread;

        //
        //
        //
        //
        //
        if (amount > 0 && bytesread == readsize) {
            AddToQueue();
        } else {
            status = 1;
        }
    }

    virtual int64_t Complete() {
        return totalbytes;
    }
};

// total size: 0x48
struct WriteOperation : public FILEOPERATION {
    WriteOperation(FILESYSHANDLE *_filehandle, int bytes, int offset, void *buffer, int priority,
                   void *userdata, FILEDEVICE *dev)
        : FILEOPERATION(priority, userdata, dev) {
        filehandle = _filehandle;
        amount = bytes;
        value = offset;
        data = buffer;
    }

    virtual void Exec(FILEDEVICE *device) {
        unsigned int byteswritten;

        //
        //
        //
        //
        //
        //
        //
        //
        //
        //
        //
        //
        //
        //
        //
        //
        if (value != filehandle->pParentFile->offset) {

            filehandle->pParentFile->offset =
                device->drv->Seek(filehandle->hFile, value, 0, 0, 0);
        }

        //
        //
        //
        //
        byteswritten = device->drv->Write(filehandle->hFile, data, amount, 0, 0);
        amount = byteswritten;
        filehandle->pParentFile->offset += byteswritten;
        status = 1;
    }

    virtual int64_t Complete() {
        return amount;
    }
};

// total size: 0x48
struct SizeOperation : public FILEOPERATION {
    SizeOperation(FILESYSHANDLE *_filehandle, int priority, void *userdata, FILEDEVICE *dev)
        : FILEOPERATION(priority, userdata, dev) {
        filehandle = _filehandle;
    }

    virtual void Exec(FILEDEVICE *device) {
        value = filehandle->length;
        status = 1;
    }

    virtual int64_t Complete() {
        return value;
    }
};

int FILESYS_opstatus(int ophandle) {
    int status;
    FILEDEVICE *dev;
    FILEOPERATION *op;
    int ispending;

    status = 0;
    dev = FILEOPERATION::GetDevice(ophandle);

    dev->mutex.Lock();

    op = iGetOpFromHandle(dev, ophandle, &ispending);

    if (ispending == 0) {

        if (op != 0) {

            if (op->IsCancelled() == true) {
                status = -1;
            } else {
                status = op->GetStatus();
            }

        } else {
            status = -3;
        }
    }

    dev->mutex.Unlock();
    return status;
}

int FILESYS_waitop(int ophandle) {
    FILEDEVICE *dev;
    FILEOPERATION *op;
    int ispending;

    if (ophandle == 0) {
        return -3;
    }

    dev = FILEOPERATION::GetDevice(ophandle);

    //
    if (dev->running == 0) {
        return -3;
    }

    do {
        //
        //
        dev->mutex.Lock();
        //
        op = iGetOpFromHandle(dev, ophandle, &ispending);
        ispending = (op != 0) ? (ispending & 1) : 0;
        //
        dev->mutex.Unlock();
        //
        if (ispending != 0) {
            //
            if (THREAD_iscurrent(0)) {
                //
                SYNCTASK_run();
                THREAD_yield(1);
            } else {
                //
                //
                //
                //
                //
                //
                //
                //
                //
                //
                //
                //
                //
                //
                //
                //
#ifdef EA_PLATFORM_WIN32
                dev = FILEOPERATION::GetDevice(ophandle);
                WaitForSingleObject(dev->attention, 0xFFFFFFFF);
                ResetEvent(dev->attention);
#else
                SIGNAL_wait(&FILEOPERATION::GetDevice(ophandle)->attention);
#endif
            }
        }
        //
        //
    } while (ispending != 0);

    return FILESYS_opstatus(ophandle);
}

int64_t FILESYS_completeop64(int ophandle) {
    FILEDEVICE *dev;
    FILEOPERATION *op;
    int64_t ret;

    dev = FILEOPERATION::GetDevice(ophandle);

    dev->mutex.Lock();
    op = dev->completed.Find(ophandle, true);
    dev->mutex.Unlock();

    //
    //
    //
    //
    ret = op->Complete();

    op->ClearId();

    //
    //
    //
    //
    //
    gpFileSysInfo->FreeOpsMutex.Lock();
    gpFileSysInfo->FreeOps.PushTail(op);
    gpFileSysInfo->FreeOpsMutex.Unlock();

    return ret;
}

int FILESYS_completeop(int ophandle) {
    return FILESYS_completeop64(ophandle);
}

void FILESYS_callbackop(int ophandle, void (*func)(int, int, void *)) {
    FILEDEVICE *dev;
    FILEOPERATION *op;
    int ispending;

    dev = FILEOPERATION::GetDevice(ophandle);

    //
    //
    //
    dev->mutex.Lock();

    op = iGetOpFromHandle(dev, ophandle, &ispending);

    //
    //
    //
    if (ispending != 0) {

        op->SetCallback(func);

    } else {

        //
        //
        //
        func(op->GetId(), op->GetStatus(), op->GetUserData());
    }

    //
    //
    dev->mutex.Unlock();
}

void FILESYS_priorityop(int ophandle, int priority) {
    FILEDEVICE *device;
    FILEOPERATION *op;

    //
    //
    //
    device = FILEOPERATION::GetDevice(ophandle);

    if (device != 0) {

        device->mutex.Lock();
        op = device->pending.Find(ophandle, true);
        device->mutex.Unlock();
        if (op != 0) {

            op->SetPriority(priority);
            //
            op->AddToQueue();
        }
    }
}

int FILESYS_exists(const char *name, int priority, void *userdata) {
    FILEOPERATION *op = new ExistOperation(name, priority, userdata, FILE_nametodevice(name));

    op->AddToQueue();

    return op->GetId();
}

int FILESYS_open(const char *name, unsigned int modeflags, int priority, void *userdata) {
    FILEOPERATION *op =
        new OpenOperation(name, modeflags, priority, userdata, FILE_nametodevice(name));

    op->AddToQueue();

    return op->GetId();
}

int FILESYS_close(int filehandle, int priority, void *userdata) {
    FILEOPERATION *op = new CloseOperation((FILESYSHANDLE *)filehandle, priority, userdata,
                                          ((FILESYSHANDLE *)filehandle)->dev);

    op->AddToQueue();

    return op->GetId();
}

int FILESYS_read(int filehandle, int offset, void *buffer, int bytes, int priority,
                 void *userdata) {
    FILEOPERATION *op = new ReadOperation((FILESYSHANDLE *)filehandle, bytes, offset, buffer,
                                          priority, userdata, ((FILESYSHANDLE *)filehandle)->dev);

    op->AddToQueue();

    return op->GetId();
}

int FILESYS_readlarge(int filehandle, uint64_t offset, void *buffer,
                      uint64_t bytes, int priority, void *userdata) {
    FILEOPERATION *op =
        new ReadLargeOperation((FILESYSHANDLE *)filehandle, bytes, offset, buffer, priority,
                               userdata, ((FILESYSHANDLE *)filehandle)->dev);

    op->AddToQueue();

    return op->GetId();
}

int FILESYS_write(int filehandle, int offset, void *buffer, int bytes, int priority,
                  void *userdata) {
    FILEOPERATION *op = new WriteOperation((FILESYSHANDLE *)filehandle, bytes, offset, buffer,
                                           priority, userdata, ((FILESYSHANDLE *)filehandle)->dev);

    op->AddToQueue();

    return op->GetId();
}

int FILESYS_size(int filehandle, int priority, void *userdata) {
    FILEOPERATION *op = new SizeOperation((FILESYSHANDLE *)filehandle, priority, userdata,
                                          ((FILESYSHANDLE *)filehandle)->dev);

    op->AddToQueue();

    return op->GetId();
}

int FILESYS_atomic(int (*func)(int, void *), FILEDEVICE *device, int priority, void *userdata) {
    int result;
    int oldminpriority;

    //
    //
    //
    //
    if (device->running == 0) {
        iStartDevice(device);
    }

    device->atomic.Lock();

    //
    //
    //
    //
    oldminpriority = device->minpriority;
    device->minpriority = priority;

    //
    result = func(priority, userdata);

    //
    device->minpriority = oldminpriority;
    SIGNAL_set(&device->signal);

    //
    device->atomic.Unlock();

    //
    return result;
}

namespace RealFile {

bool GetInfoFastByName(const char *name, const unsigned int modeflags, uint64_t &location,
                       uint64_t &size) {
    FILESYSHANDLE *fs_h;

    fs_h = iOpenFileSysHandle(name, modeflags | 0x20001, FILE_nametodevice(name));

    //
    //
    //
    if (fs_h != (FILESYSHANDLE *)-1) {

        GetInfoFastByHandle((int)fs_h, location, size);
        iCloseFileSysHandle(fs_h);
        return true;
    }

    //
    //
    location = 0;
    size = 0;

    return false;
}

void GetInfoFastByHandle(int filehandle, uint64_t &location, uint64_t &size) {
    FILESYSHANDLE *fs_h = (FILESYSHANDLE *)filehandle;

    //
    location = fs_h->dev->drv->QueryLocation(fs_h->hFile);
    size = fs_h->length;
}

} // namespace RealFile

FILEDEVICE *FILE_nametodevice(const char *name) {
    char devname[16];
    ListSingleIterator<FILEDEVICE> Itr;
    bool found;

    found = false;

    //
    //
    //
    //
    //
    //
    //
    if (strchr(name, ':') != 0) {

        MEM_clear(devname, sizeof(devname));

        strncpy(devname, name, strchr(name, ':') - name + 1);

    } else if (name[0] == '/' || name[0] == '\\') {

        MEM_clear(devname, sizeof(devname));

        //
        //
        const char *end = strchr(gpFileSysInfo->psearchpaths[0], ':');
        strncpy(devname, gpFileSysInfo->psearchpaths[0],
                end - gpFileSysInfo->psearchpaths[0] + 1);

    } else {

        return gpFileSysInfo->DeviceMemory;
    }

    //
    //
    //
    //
    Itr = gpFileSysInfo->AllocatedDevices.Begin();
    if (*Itr != 0) {
        for (; !found;) {
            if (strcasecmp((*Itr)->drv->GetName(), devname) == 0) {
                return *Itr;
            }
            Itr++;
            if (*Itr == 0) {
                break;
            }
        }
    }

    //
    //
    //
    //
    //
    //
    //
    //
    //
    //
    //
    //
    //
    return 0;
}

namespace RealFile {

void SetSearchPath(const char *path) {
    char *p;
    int n;

    //
    //
    //
    //
    //
    //
    //
    MEM_clear(gpFileSysInfo->searchpath, gFileSysOpts.nSearchPathLength);
    MEM_clear(gpFileSysInfo->psearchpaths, gFileSysOpts.nSearchLocs * 4);

    strcpy(gpFileSysInfo->searchpath, path);

    p = gpFileSysInfo->searchpath;
    gpFileSysInfo->psearchpaths[0] = p;
    n = 1;

    //
    //
    while ((p = strchr(p, ';')) != 0) {

        *p = 0;

        //
        if (p[-1] == '/' || p[-1] == '\\') {
            p[-1] = 0;
        }
        //
        p++;
        gpFileSysInfo->psearchpaths[n] = p;
        n++;
    }

    //
    //
    p = gpFileSysInfo->psearchpaths[n - 1];
    p += strlen(p);

    //
    if (p[-1] == '/' || p[-1] == '\\') {
        p[-1] = 0;
    }
}

void AddSearchLocation(const char *pszLoc, bool head) {
    char *c;
    char *newpath;

    c = gpFileSysInfo->searchpath;

    //
    while (c[1] != 0) {

        if (*c != 0) {

            //
            while (*++c != 0) {
            }
        }

        //
        //
        if (c[1] == 0 && c[2] != 0) {

            //
            *++c = '/';
        }

        //
        *c = ';';
    }

    //
    *c = 0;

    newpath = (char *)__builtin_alloca(gFileSysOpts.nSearchPathLength);
    MEM_clear(newpath, gFileSysOpts.nSearchPathLength);

    if (head == true) {

        strcpy(newpath, pszLoc);
        strcat(newpath, ";");
        strcat(newpath, gpFileSysInfo->searchpath);

    } else {

        strcpy(newpath, gpFileSysInfo->searchpath);
        strcat(newpath, ";");
        strcat(newpath, pszLoc);
    }

    //
    //
    SetSearchPath(newpath);
}

unsigned int AddDevice(DeviceDriver *drv) {
    FILEDEVICE *dev;

    dev = gpFileSysInfo->FreeDevices.Pop();

    //
    //
    //
    //
    dev->drv = drv;

    gpFileSysInfo->AllocatedDevices.PushTail(dev);

    //
    //
    //
    //
    //
    return dev - gpFileSysInfo->DeviceMemory;
}

void RemoveDevice(unsigned int DevId) {
    FILEDEVICE *dev;

    //
    //
    //
    dev = &gpFileSysInfo->DeviceMemory[DevId];

    //
    //
    //
    //
    if (gpFileSysInfo->AllocatedDevices.Remove(dev, 0)) {

        //
        //
        if (dev->running != 0) {

            //
            dev->terminate = 1;

            SIGNAL_set(&dev->signal);

            //
            while (dev->running != 0) {
                THREAD_yield(0);
            }

            SIGNAL_destroy(&dev->signal);

            //
            //
            //
            //
            //
            //
            //
#ifdef EA_PLATFORM_WIN32
            SetEvent(dev->attention);
            CloseHandle(dev->attention);
#else
            SIGNAL_set(&dev->attention);
            SIGNAL_destroy(&dev->attention);
#endif

            //
            //
            //
            //
            //
            //
            //
            //
            //
            //
            //
            //
            //
            //
            //
#ifdef EA_PLATFORM_WIN32
            // PC destruye el hilo y no libera pila (no la reservo: speed.exe 0x7ED66A)
            THREAD_destroy(&dev->deviceThread);
#else
            gFileSysOpts.allocator->Free(dev->pThreadStack, 0);
            dev->pThreadStack = 0;
#endif

            //
            //
            dev->mutex.Destroy();
            dev->atomic.Destroy();

            //
            dev->drv->Restore();
        }

        //
        //
        MEM_clear(dev, 4);

        gpFileSysInfo->FreeDevices.PushTail(dev);
    }
}

} // namespace RealFile
