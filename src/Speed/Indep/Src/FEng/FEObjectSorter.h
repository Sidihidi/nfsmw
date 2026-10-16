#ifndef FEOBJECTSORTER_H_
#define FEOBJECTSORTER_H_

#include <types.h>
#include "FEObject.h"
#include "FEngStandard.h"

// File: speed/indep/src/feng/FEObjectSorter.h
// total size: 0x8
// Decl: speed/indep/src/feng/FEObjectSorter.h:24
struct SFERadixKey {
    FEObject *pobObject; // offset 0x0, size 0x4, Decl: speed/indep/src/feng/FEObjectSorter.h:25
    u32 ulKey;           // offset 0x4, size 0x4, Decl: speed/indep/src/feng/FEObjectSorter.h:26
};

// total size: 0x4004
// Decl: speed/indep/src/feng/FEObjectSorter.h:34
template <int N> class FEObjectSorter {
  private:
    u32 mulNumObjects;              // offset 0x0, size 0x4, Decl: speed/indep/src/feng/FEObjectSorter.h:36
    SFERadixKey mastFinalList[N];   // offset 0x4, size 0x2000, Decl: speed/indep/src/feng/FEObjectSorter.h:37
    SFERadixKey mastScratchList[N]; // offset 0x2004, size 0x2000, Decl: speed/indep/src/feng/FEObjectSorter.h:38

  public:
    FEObjectSorter() { // Decl: speed/indep/src/feng/FEObjectSorter.h:41
        mulNumObjects = 0;
    }

    void Zero() { // Decl: speed/indep/src/feng/FEObjectSorter.h:46
        mulNumObjects = 0;
    }

    void AddObject(FEObject *pobObject, float fZValue) { // Decl: speed/indep/src/feng/FEObjectSorter.h:51
        mastFinalList[mulNumObjects].pobObject = pobObject;
        mastFinalList[mulNumObjects].ulKey = *reinterpret_cast<u32 *>(&fZValue);
        mulNumObjects++;
    }

    u32 GetNumObjects() { // Decl: speed/indep/src/feng/FEObjectSorter.h:59
        return mulNumObjects;
    }

    SFERadixKey *GetListPtr() { // Decl: speed/indep/src/feng/FEObjectSorter.h:70
        return mastFinalList;
    }

    // Definida FUERA de la clase a proposito: dentro es implicitamente inline
    // y se expande en FEngine::Render, cuyo marco pasa de 296 a 2.376 bytes.
    void SortObjects();
};

template <int N> void FEObjectSorter<N>::SortObjects() {
    SFERadixKey *pstDestList = mastScratchList;
    SFERadixKey *pstSrcList = mastFinalList;
    i32 alElemCount[256];
    i32 alElemIndex[256];
    int lNumBytes = mulNumObjects << 3;
    int i;

    // El byte de menos peso de ulKey va primero: el ultimo en big-endian y el primero en little-endian.
#ifdef FENG_BIG_ENDIAN
    for (int b = 3; b >= 0; b--) {
#else
    for (int b = 0; b < 4; b++) {
#endif
        FEngMemSet(alElemCount, 0, sizeof(alElemCount));
        unsigned char *pucByte = reinterpret_cast<unsigned char *>(pstSrcList) + (b + 4);
        for (i = 0; i < lNumBytes; i += 8) {
            alElemCount[pucByte[i]]++;
        }
        alElemIndex[0] = 0;
        for (i = 0; i < 255; i++) {
            alElemIndex[i + 1] = alElemIndex[i] + alElemCount[i];
        }
        for (i = 0; i < static_cast<int>(mulNumObjects); i++) {
            unsigned char ucIndex = pucByte[i * 8];
            pstDestList[alElemIndex[ucIndex]] = pstSrcList[i];
            alElemIndex[ucIndex]++;
        }
        SFERadixKey *pstTemp = pstSrcList;
        pstSrcList = pstDestList;
        pstDestList = pstTemp;
    }
}


#endif
