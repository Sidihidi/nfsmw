#include "Stomper.hpp"
#include "Speed/Indep/Libs/Support/Utility/UVectorMath.h"
#include "Speed/Indep/Src/Ecstasy/EcstasyE.hpp"
#include "Speed/Indep/Src/World/CarRender.hpp"
#include "types.h"

// TODO move these?
extern int32 DrawSky;
extern int32 DrawWireframe;
extern int32 EnableTexturing;
extern int32 WeatherUseCoolFogValues;
extern int32 egAlphaA;
extern int32 egAlphaB;
extern int32 egAlphaC;
extern int32 egAlphaD;
extern int32 egAlphaF;

static const float fCutiePetootieX = 0.5f;
static const float fCutiePetootieY = 0.5f;
static const float fCutiePetootieZ = 0.5f;

// total size: 0xC
struct StompEntry {
    void *Set(void *p);
    void *Clear(void *p);
    void *SetAll(void *pStack);
    void *ClearAll(void *pStack);

    int nType; // offset 0x0, size 0x4
    union {
        void *pData;                      // offset 0x0, size 0x4
        int (*pFuncInt)(bool, int);       // offset 0x0, size 0x4
        float (*pFuncFloat)(bool, float); // offset 0x0, size 0x4
    }; // offset 0x4, size 0x4
    union {
        int nData;   // offset 0x0, size 0x4
        float fData; // offset 0x0, size 0x4
    }; // offset 0x8, size 0x4
};

enum eStomper {
    eSTOMPER_NONE = 0,
    eSTOMPER_CARTOON = 1,
    eSTOMPER_WIRE_ORANGE = 2,
    eSTOMPER_NEGATIVE_BLUE_FOG = 3,
    eSTOMPER_NEGATIVE_CREAM_FOG = 4,
    eSTOMPER_FLAT_ORANGE = 5,
    eSTOMPER_BLUE_NIGHT = 6,
    eSTOMPER_HOT_MAGENTA = 7,
    eSTOMPER_MOTION_BLUR = 8,
    eSTOMPER_BLACK_AND_WHITE = 9,
    eSTOMPER_COOL_FOG = 10,
    eSTOMPER_CUTIE_PETOOTIE = 11,
    eSTOMPER_CHROME_CARS = 12,
};

int32 bCartoonMode = 0;
// static const eStomper nStomper;
// static const eStomper nStomperOld;
// static const int nStomperStackSize;
StompEntry sWireFrame[3] = {{2, &DrawSky, 0}, {2, &DrawWireframe, 1}, {7, nullptr, 0}};
StompEntry sPixelClamp[2] = {{4, (void *)&eClampTopLeft, 1}, {7, nullptr, 0}};
StompEntry sFlatShaded[2] = {{2, &EnableTexturing, 0}, {7, nullptr, 0}};
StompEntry sFlatOrange[2] = {{6, sFlatShaded, 0}, {7, nullptr, 0}};
StompEntry sWireOrange[3] = {{6, sFlatShaded, 0}, {6, sWireFrame, 0}, {7, nullptr, 0}};
StompEntry sBigHead[1] = {{7, nullptr, 0}};
StompEntry sCutiePetootie[5] = {
    {3, &CarScaleMatrix.v0.x, FloatAsInt(fCutiePetootieX)},
    {3, &CarScaleMatrix.v1.y, FloatAsInt(fCutiePetootieY)},
    {3, &CarScaleMatrix.v2.z, FloatAsInt(fCutiePetootieZ)},
    {6, sBigHead, 0},
    {7, nullptr, 0},
};
StompEntry sCartoonMode[5] = {{6, sCutiePetootie, 0}, {6, sPixelClamp, 0}, {2, &bCartoonMode, 1}, {6, sBigHead, 0}, {7, nullptr, 0}};
StompEntry sCoolFog[2] = {{2, &WeatherUseCoolFogValues, 1}, {7, nullptr, 0}};
StompEntry sChromeCars[6] = {{2, &egAlphaA, 2}, {2, &egAlphaB, 2}, {2, &egAlphaC, 1}, {2, &egAlphaD, 0}, {2, &egAlphaF, 128}, {7, nullptr, 0}};
StompEntry sDummyStomper[1];

StompEntry *StomperTable[7] = {nullptr, sCartoonMode, sWireOrange, sFlatOrange, sCoolFog, sCutiePetootie, sChromeCars};

// STRIPPED en GameCube; las dos PS2 la tienen
void *StompEntry::Set(void *p) {
    switch (nType) {
        case 0:
            *reinterpret_cast<unsigned char *>(p) = *reinterpret_cast<unsigned char *>(this->pData);
            *reinterpret_cast<unsigned char *>(this->pData) = this->nData;
            return static_cast<char *>(p) + 4;
        case 1:
            *reinterpret_cast<unsigned short *>(p) = *reinterpret_cast<unsigned short *>(this->pData);
            *reinterpret_cast<unsigned short *>(this->pData) = this->nData;
            return static_cast<char *>(p) + 4;
        case 2:
            *reinterpret_cast<unsigned int *>(p) = *reinterpret_cast<unsigned int *>(this->pData);
            *reinterpret_cast<unsigned int *>(this->pData) = this->nData;
            return static_cast<char *>(p) + 4;
        case 3:
            *reinterpret_cast<float *>(p) = *reinterpret_cast<float *>(this->pData);
            *reinterpret_cast<float *>(this->pData) = this->fData;
            return static_cast<char *>(p) + 4;
        case 4:
            *reinterpret_cast<int *>(p) = this->pFuncInt(true, this->nData);
            return static_cast<char *>(p) + 4;
        case 5:
            *reinterpret_cast<float *>(p) = this->pFuncFloat(true, this->fData);
            return static_cast<char *>(p) + 4;
        case 6: {
            StompEntry *pEntry = static_cast<StompEntry *>(this->pData);
            return pEntry->SetAll(p);
        }
    }
    return p;
}

void *StompEntry::Clear(void *p) {
    switch (nType) {
        case 0:
            *reinterpret_cast<unsigned char *>(this->pData) = *reinterpret_cast<unsigned char *>(p);
            return static_cast<char *>(p) + 4;
        case 1:
            *reinterpret_cast<unsigned short *>(this->pData) = *reinterpret_cast<unsigned short *>(p);
            return static_cast<char *>(p) + 4;
        case 2:
            *reinterpret_cast<unsigned int *>(this->pData) = *reinterpret_cast<unsigned int *>(p);
            return static_cast<char *>(p) + 4;
        case 3:
            *reinterpret_cast<float *>(this->pData) = *reinterpret_cast<float *>(p);
            return static_cast<char *>(p) + 4;
        case 4:
            this->pFuncInt(false, *reinterpret_cast<int *>(p));
            return static_cast<char *>(p) + 4;
        case 5:
            this->pFuncFloat(false, *reinterpret_cast<float *>(p));
            return static_cast<char *>(p) + 4;
        case 6: {
            StompEntry *pEntry = static_cast<StompEntry *>(this->pData);
            return pEntry->ClearAll(p);
        }
    }
    return p;
}

// STRIPPED en GameCube; las dos PS2 la tienen
void *StompEntry::SetAll(void *pStack) {
    StompEntry *pStompEntry = this;
    while (pStompEntry->nType != 7) {
        pStack = pStompEntry->Set(pStack);
        pStompEntry++;
    }
    return pStack;
}

void *StompEntry::ClearAll(void *pStack) {
    StompEntry *pStompEntry = this;
    while (pStompEntry->nType != 7) {
        pStack = pStompEntry->Clear(pStack);
        pStompEntry++;
    }
    return pStack;
}

// total size: 0xC
class Stomper {
  public:
    Stomper();
    void Set(eStomper nStomper);
    void Clear();
    void Suspend();
    void Resume();

  private:
    bool bEnabled;     // offset 0x0, size 0x1
    char *pStack;      // offset 0x4, size 0x4
    eStomper nCurrent; // offset 0x8, size 0x4
};

Stomper *pStomper = nullptr;

void InitStomper() {
    pStomper = new ("Stomper", 0) Stomper();
}

// STRIPPED: void CloseStomper()

Stomper::Stomper() {
    this->bEnabled = true;
    this->pStack = new ("Stomper Stack", 0) char[0x400];
    this->nCurrent = eSTOMPER_NONE;
}

// STRIPPED en GameCube; las dos PS2 la tienen
void Stomper::Set(eStomper nStomper) {
    if (nStomper > eSTOMPER_NONE && nStomper < eSTOMPER_HOT_MAGENTA) {
        StomperTable[nStomper]->SetAll(this->pStack);
        this->nCurrent = nStomper;
    }
}

void Stomper::Clear() {
    if (this->nCurrent > eSTOMPER_NONE && this->nCurrent < eSTOMPER_HOT_MAGENTA) {
        StomperTable[this->nCurrent]->ClearAll(this->pStack);
    }
    this->nCurrent = eSTOMPER_NONE;
}

// STRIPPED en GameCube; las dos PS2 la tienen
void Stomper::Suspend() {
    eStomper nStomper = this->nCurrent;

    this->Clear();
    this->nCurrent = nStomper;
}

// STRIPPED en GameCube; las dos PS2 la tienen
void Stomper::Resume() {
    this->Set(this->nCurrent);
}

void ResetRenderEggs() {
    if (pStomper != nullptr) {
        pStomper->Clear();
    }
}

// STRIPPED en GameCube; las dos PS2 la tienen
void SuspendRenderEggs() {
    if (pStomper != nullptr) {
        pStomper->Suspend();
    }
}

// STRIPPED en GameCube; la A124 la tiene
void ResumeRenderEggs() {
    if (pStomper != nullptr) {
        pStomper->Resume();
    }
}

void ActivateAnyRenderEggs() {}
