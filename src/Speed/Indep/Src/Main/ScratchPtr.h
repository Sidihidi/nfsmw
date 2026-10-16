#ifndef SCRATCHPPTR_H
#define SCRATCHPPTR_H

#include <types.h>
#ifdef EA_PLATFORM_PLAYSTATION2
#include "Speed/Indep/bWare/Inc/bWare.hpp"
#endif

// total size: 0x4
template <typename T> class ScratchPtr {
  public:
    static void Push(void *workspace);
    static void Pop();

    T **_Alloc();
    ScratchPtr();
    ~ScratchPtr();

    const T &operator*() const {
        return **mRef;
    }

    T &operator*() {
        return **mRef;
    }

    const T *operator->() const {
        return *mRef;
    }

    T *operator->() {
        return *mRef;
    }

  private:
    T **mRef; // offset 0x0, size 0x4

    static void *mWorkSpace;
    static T *mPointer[T::MaxInstances];
    static T mRAMBuffer[T::MaxInstances];
};

template <typename T> T **ScratchPtr<T>::_Alloc() {
    for (unsigned int i = 0; i < sizeof(mPointer) / sizeof(T *); ++i) {
        if (mPointer[i] == nullptr) {
            if (mWorkSpace == nullptr) {
                mPointer[i] = &mRAMBuffer[i];
            } else {
                T *spbuffer = reinterpret_cast<T *>(mWorkSpace);
                mPointer[i] = &spbuffer[i];
            }
            return &mPointer[i];
        }
    }
    return nullptr;
}

template <typename T> ScratchPtr<T>::ScratchPtr() {
    mRef = nullptr;
    mRef = _Alloc();
    new (*mRef) T();
}

template <typename T> ScratchPtr<T>::~ScratchPtr() {
    *mRef = nullptr;
}

// Las PS2 pasan los T vivos al scratchpad (workspace) y de vuelta; GameCube los deja siempre en mRAMBuffer.
template <typename T> void ScratchPtr<T>::Push(void *workspace) {
#ifdef EA_PLATFORM_PLAYSTATION2
    if (workspace != nullptr) {
        mWorkSpace = workspace;
        bMemCpy(workspace, mRAMBuffer, sizeof(mRAMBuffer));
        T *spbuffer = reinterpret_cast<T *>(mWorkSpace);
        for (unsigned int i = 0; i < sizeof(mPointer) / sizeof(T *); ++i) {
            if (mPointer[i] != nullptr) {
                mPointer[i] = &spbuffer[i];
            }
        }
    }
#endif
}

template <typename T> void ScratchPtr<T>::Pop() {
#ifdef EA_PLATFORM_PLAYSTATION2
    if (mWorkSpace != nullptr) {
        bMemCpy(mRAMBuffer, mWorkSpace, sizeof(mRAMBuffer));
        for (unsigned int i = 0; i < sizeof(mPointer) / sizeof(T *); ++i) {
            if (mPointer[i] != nullptr) {
                mPointer[i] = &mRAMBuffer[i];
            }
        }
        mWorkSpace = nullptr;
    }
#endif
}

#define IMPLEMENT_SCRATCHPTR(DATATYPE)                                                                                                               \
    template <> void *ScratchPtr<DATATYPE>::mWorkSpace = NULL;                                                                                       \
    ALIGNVEC DATATYPE ScratchPtr<DATATYPE>::mRAMBuffer[DATATYPE::MaxInstances];                                                                      \
    template <> ALIGNVEC DATATYPE *ScratchPtr<DATATYPE>::mPointer[DATATYPE::MaxInstances];

#endif
