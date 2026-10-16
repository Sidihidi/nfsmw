#ifndef BWARE_BWARE_H
#define BWARE_BWARE_H

#ifdef EA_PRAGMA_ONCE_SUPPORTED
#pragma once
#endif

#include "Strings.hpp"
#include "bMath.hpp"
#include "Speed/Indep/Libs/Support/Utility/UCrc.h"
#include "bMemory.hpp"
#include "bSlotPool.hpp"
#include "bTypes.hpp"

// #define PLAT_NEXT_GEN

#if defined(EA_PLATFORM_GAMECUBE) || defined(EA_PLATFORM_XENON)
#define NATIVE_ENDIAN_BIG
#else
#define NATIVE_ENDIAN_LITTLE
#endif

#define MEMORY_DUMP
//   #undef  NO_DEBUG_BMEMORY
//   #define   NULL 0
#define MUL 0
#define bPrintf (1) ? ((void)0) : bNullPrintf
#define bMilestonePutString bReleasePutString
#define bMilestonePrintf bReleasePrintf
#define bAssert(exp)
#define bAssertMsg(exp, msg)
#define bAssertMsg1(exp, msg, arg1)

inline void bNullPrintf(const char *format, ...) {}

inline void bNullPrintf(char terminal_channel, const char *format, ...) {}
#define bAssertMsg2(exp, msg, arg1, arg2)
#define bAssertMsg3(exp, msg, arg1, arg2, arg3)
#define bAssertMsg4(exp, msg, arg1, arg2, arg3, arg4)
#define bWarning(exp)
#define bWarningMsg(exp, msg)
#define bValid(exp)
#define bFAssert(exp)
#define ASSERT_NOTRENDERTHREAD()
#define ASSERT_ISMAINTHREAD()
#define ASSERT_ISRENDERTHREAD()
#if defined(EA_PLATFORM_WIN32)
// Only here: on GameCube bPrintf.cpp calls memset without a prototype, as the original does.
#include <cstring>
#define bMemCpy(dest, src, numbytes) memcpy(dest, src, numbytes)
#define bMemSet(dest, pattern, size) memset(dest, pattern, size)
#define bMemCmp(s1, s2, numbytes) memcmp(s1, s2, numbytes)
#endif
#define BMEMORY_TOP_BIT (1 << 6)
#define BMEMORY_MAX_POOLS 16
#define BMEMORY_POOL_MASK (BMEMORY_MAX_POOLS - 1)
#define BMEMORY_DEFAULT_POOL 0
#define BMEMORY_MAIN_POOL 0
#define BMEMORY_MAX_ALIGNMENT 4096
#define BMEMORY_POOL(n) (n & BMEMORY_POOL_MASK)
#define BMEMORY_ALLOCATE_FROM_TOP (1 << 6)
#define BMEMORY_BEST_FIT (1 << 7)
#define BMEMORY_ALIGNMENT(n) (((n >> 2) & 0x7ff) << 8)
#define BMEMORY_ALIGNMENT_OFFSET(n) (((n >> 2) & 0x7ff) << 19)
#define BMEMORY_DEFAULT_ALLOCATION_PARAMS 0
#define MEMTHROW
#define BDELETE delete
#ifdef EA_BUILD_A124
// La A124 pasa 0 como linea en los tres ficheros que usan BNEW (move a2, zero); la version de venta pasa __LINE__.
#define BNEW new (__FILE__, 0)
#else
#define BNEW new (__FILE__, __LINE__)
#endif
#define BTEMPNEW new (__FILE__, __LINE__, BMEMORY_ALLOCATE_FROM_TOP)
#define USE_SLOTALLOC(POOL)                                                                                                                          \
    void *operator new(size_t size) {                                                                                                                \
        return bOMalloc(POOL);                                                                                                                       \
    }                                                                                                                                                \
    void *operator new(size_t size, const char *name) {                                                                                              \
        return bOMalloc(POOL);                                                                                                                       \
    }                                                                                                                                                \
    void operator delete(void *ptr) {                                                                                                                \
        bFree(POOL, ptr);                                                                                                                            \
    }
#define BEGIN_SOURCELIST(_MODULE)
#define END_SOURCELIST(_MODULE)
#define THIS_SCOPE_EXECUTES_ONLY_ONCE()

#ifdef DEBUG_OPT
#define ENABLE_IN_DEBUG true
#else
#define ENABLE_IN_DEBUG false
#endif

#ifdef MILESTONE_BUILD
#define ENABLE_IN_MILESTONE true
#else
#define ENABLE_IN_MILESTONE false
#endif

void *bMalloc(int size, int allocation_params);
#ifdef MILESTONE_BUILD
void *bMalloc(int size, const char *debug_text, int debug_line, int allocation_params);
#else

inline void *bMalloc(int size, const char *debug_text, int debug_line, int allocation_params) {
    return bMalloc(size, allocation_params);
}

#endif

void *bMalloc(SlotPool *slot_pool);
void *bMalloc(SlotPool *slot_pool, int num_slots, void **last_slot);
void bFree(void *ptr);
const char *bGetMallocName(void *ptr);
size_t bGetMallocSize(const void *ptr);
int bGetMallocPool(void *ptr);
int bMemoryGetAllocations(int pool_num, void **allocations, int max_allocations);

extern "C" {
#if !defined(EA_PLATFORM_WIN32)
void bMemCpy(void *dest, const void *src, unsigned int numbytes);
void bMemSet(void *dest, unsigned char pattern, unsigned int size);
int bMemCmp(const void *s1, const void *s2, unsigned int numbytes);
#endif
void bOverlappedMemCpy(void *dest, const void *src, unsigned int numbytes);
}

bool bSetMemoryPoolDebugFill(int pool_num, bool on_off);
bool bSetMemoryPoolDebugTracing(int pool_num, bool on_off);
void bSetMemoryPoolTopDirection(int pool_num, bool top_means_larger_address);

// TODO
inline void *Alloc(unsigned int bytes, int memtype, const char *name) {
    int parms;

    return bMalloc(bytes, name, 0, memtype);
}

#ifdef MILESTONE_BUILD
void *operator new(size_t size, const char *file, int line);
void *operator new[](size_t size, const char *file, int line);
// __nw__FUiPCcii (A124 0x35bc78): DebugMenuAllocate la llama con el pool de FEng.
void *operator new(size_t size, const char *file, int line, int allocation_params);
#else
// TODO move the milestone path into a cpp file?
inline void *operator new(size_t size, const char *file, int line) {
#ifdef EA_BUILD_A124
    return bMalloc(size, 0);
#elif MILESTONE_BUILD
    return bWareMalloc(size, file, line, 0);
#else
    return new char[size];
#endif
}
#endif

#ifndef MILESTONE_BUILD
inline void *operator new[](size_t size, const char *file, int line) {
    return new char[size];
}
#endif

#ifdef EA_PLATFORM_WIN32
// Los operator delete a juego con los new con nombre de arriba: en PC estan fuera de linea (speed.exe 0x652B40 y
// 0x652B50) y los funcletes de excepciones del original los llaman (548 y 6 veces) para liberar el objeto si su
// constructor lanza. Sin ellos MSVC no expande en linea los constructores que siguen a un `new ("Nombre", 0)`.
void operator delete(void *ptr, const char *file, int line);
void operator delete[](void *ptr, const char *file, int line);
#endif

inline char *bGetPlatformName() {
#ifdef EA_PLATFORM_GAMECUBE
    return "GAMECUBE";
#elif defined(EA_PLATFORM_PLAYSTATION2)
    return "PSX2";
#elif defined(EA_PLATFORM_XENON)
    return "XENON";
#elif defined(EA_PLATFORM_WIN32)
    return "PC";
#else
#error "Platform not specified";
#endif
}

void bEndianSwap64(void *value);
void bEndianSwap32(void *value);
void bEndianSwap16(void *value);
#ifdef NATIVE_ENDIAN_BIG
void bPlatEndianSwap(bVector2 *value);
void bPlatEndianSwap(bVector3 *value);
void bPlatEndianSwap(bVector4 *value);
void bPlatEndianSwap(bMatrix4 *value);
#else
// En little endian no hay nada que girar: en PS2 ni existen como funcion.
inline void bPlatEndianSwap(bVector2 *value) {}
inline void bPlatEndianSwap(bVector3 *value) {}
inline void bPlatEndianSwap(bVector4 *value) {}
inline void bPlatEndianSwap(bMatrix4 *value) {}
#endif

void bInitSharedStringPool(int size);
void bCloseSharedStringPool();

inline void bPlatEndianSwap(uint64 *value) {
#ifdef NATIVE_ENDIAN_BIG
    bEndianSwap64(value);
#endif
}

inline void bPlatEndianSwap(int32 *value) {
#ifdef NATIVE_ENDIAN_BIG
    bEndianSwap32(value);
#endif
}

inline void bPlatEndianSwap(uint32 *value) {
#ifdef NATIVE_ENDIAN_BIG
    bEndianSwap32(value);
#endif
}

inline void bPlatEndianSwap(int16 *value) {
#ifdef NATIVE_ENDIAN_BIG
    bEndianSwap16(value);
#endif
}

inline void bPlatEndianSwap(uint16 *value) {
#ifdef NATIVE_ENDIAN_BIG
    bEndianSwap16(value);
#endif
}

inline void bPlatEndianSwap(uint8 *value) {}

inline void bPlatEndianSwap(int8 *value) {}

inline void bPlatEndianSwap(float *value) {
#ifdef NATIVE_ENDIAN_BIG
    bEndianSwap32(value);
#endif
}

inline void bPlatEndianSwap(UCrc32 *c) {
    unsigned int val = c->GetValue();
    bPlatEndianSwap(&val);
    *c = UCrc32(val);
}

inline bool bIsDigit(char c) {
    return c >= '0' && c <= '9';
}

inline bool bStrEqual(const char *s1, const char *s2) {
    return bStrICmp(s1, s2) == 0;
}

// El DWARF del original lista bStrDup justo detras de bStrEqual (antes de bToUpper/bToLower/bIsDigit), con la
// local `char *s2`. Es la funcion que mete la ruta de bware.hpp en el pool comun de .rodata de las 33 unidades
// de juego: su BNEW expande __FILE__ DENTRO de bWare.hpp (d:/mw/speed/indep/bware/inc/bware.hpp, segundo
// literal del pool, detras de GAMECUBE y delante de bad_alloc). La ruta va como literal fijo porque es la del
// build de EA (la nuestra no existe en el binario), con la misma convencion que hlafile.cpp:278 y filesys.cpp.
inline char *bStrDup(const char *s1) {
#ifdef EA_BUILD_A124
    // El prototipo A124 se compilo desde el workspace p4_apex (prefijo PSX2 de 0x439800); las de venta
    // (GC y SLUS) desde d:/mw/speed (0x803A4234 y 0x3c3300).
    char *s2 = new ("d:/p4_apex1666_d1001856/mw/speed/indep/bware/inc/bware.hpp", __LINE__) char[bStrLen(s1) + 1];
#else
    char *s2 = new ("d:/mw/speed/indep/bware/inc/bware.hpp", __LINE__) char[bStrLen(s1) + 1];
#endif
    bStrCpy(s2, s1);
    return s2;
}

// <new> parsea detras de bStrDup y delante de UVectorMath.h: su declaracion de la clase bad_alloc es la que
// pone el tercer literal del pool (cc1plus emite el nombre en cuanto la clase queda completa; medido: un TU
// con solo #include <new> ya emite "bad_alloc"). En el original entra por esta cadena porque en las 33
// unidades bad_alloc va DELANTE de los %f de Coord3AsString (UVectorMath.h, un fichero posterior).
// Guardado a no-Win32: el include en MSVC cambia los objetos de PC (medido: 24 unidades se mueven y el
// informe de objdiff deja de emparejar); las PS2 y GC no lo noten porque sus <new> son de la propia cadena.
#ifndef EA_PLATFORM_WIN32
#include <new>
#endif

// SLUS tambien lee bCodeineVersion (0x538438: MyRebootIOP, bReleasePutString, Joylog::StopReplaying): en PS2 existe
// aunque la build no sea milestone.
#if defined(MILESTONE_BUILD) || defined(EA_PLATFORM_PLAYSTATION2)
extern float bCodeineVersion;
#endif
void bWareInit();

#ifdef EA_PLATFORM_PLAYSTATION2
extern int bSonyToolConnected;
extern int bSuperBenderConnected;

// El depurador SuperBender de PS2 (bware/src/psx2/SuperBenderPSX2.cpp y bFunkPlat.cpp); el enum sale de los stabs de la
// A124.
enum eSuperBenderDebuggerLevel {
    SB_DEBUGGER_LEVEL_NONE = 0,
    SB_DEBUGGER_LEVEL_MID = 1,
    SB_DEBUGGER_LEVEL_FULL = 2,
};

void bEnableInterrupts();
void bFigureOutPSX2Platform();
// bware/src/psx2/SuperBenderPSX2.cpp: la linea de ordenes que manda el depurador (OnlineManager::Initialize de la A124).
int SuperBenderGetCommandLineArgc();
char **SuperBenderGetCommandLineArgv();

// psx2/src/RebootIOP.cpp, que en las unidades va detras de Platform.cpp.
int bIsThereACDInTheDrive();
void InitializeDVD();
void RebootIOP();
void LoadStartupIRXModules();
void LoadNetworkIRXModules(bool connect_super_bender);
void LoadRemainingIRXModules();
#endif

inline int bIsCodeineConnected() {
#if defined(MILESTONE_BUILD) || defined(EA_PLATFORM_PLAYSTATION2)
    return bCodeineVersion > 0.0f;
#else
    return 0;
#endif
}

// bware.hpp:96 y 97 en la A124, y en este orden en el DWARF de GC (entre bIsCodeineConnected y bIsPlatGameCube):
// MyRebootIOP lee bSonyToolConnected y NCSelector::Act (uiOnlineLogin.cpp) bSuperBenderConnected.
inline int bIsPlatDTL_T10000() {
#ifdef EA_PLATFORM_PLAYSTATION2
    return bSonyToolConnected;
#else
    return 0;
#endif
}

inline int bIsPlatSuperBender() {
#ifdef EA_PLATFORM_PLAYSTATION2
    return bSuperBenderConnected;
#else
    return 0;
#endif
}

inline int bIsBFunkAvailable() {
    return bIsCodeineConnected();
}

unsigned int bCalculateCrc32(const void *data, int size, unsigned int prev_crc32);

inline int bMemoryGetPoolNum(int allocation_params) {
    return allocation_params & 0xf;
}

inline int bMemoryGetAlignment(int allocation_params) {
    int alignment = allocation_params >> 6 & 0x1ffc;

    return alignment;
}

inline int bMemoryGetBestFit(int allocation_params) {
    return allocation_params & 0x80;
}

inline int bMemoryGetTopBit(int allocation_params) {
    return allocation_params & 0x40;
}

inline int bMemoryGetAlignmentOffset(int allocation_params) {
    return (allocation_params >> 17) & 0x1ffc;
}

inline void bEndianSwap(short *value) {
    bEndianSwap16(value);
}

void bAssertFailMsg(char *fmt, const char *filename, int line_number, ...);

// Xbox 360: el juego llama a memcpy y memset de la CRT del XDK (0x826FC570 y 0x826FCDF0) y su ejecutable no trae el
// bMemCpy ni el bMemSet de bWare. Al final del fichero para no mover el __LINE__ de los BNEW de arriba.
// bMemCmp es el memcmp intrinseco (/Oi): bucle lbz/subf./cntlzw en los operator== de FEDatabase.cpp (0x822E9B08).
#ifdef EA_PLATFORM_XENON
#include <cstring>
#define bMemCpy(dest, src, numbytes) memcpy(dest, src, numbytes)
#define bMemSet(dest, pattern, size) memset(dest, pattern, size)
#define bMemCmp(s1, s2, numbytes) memcmp(s1, s2, numbytes)
#endif

// Xbox 360: los trozos que se cargan de disco (texturas de la GPU) van alineados a 4 kB; GC, PS2 y PC, a 128 B.
// NFS.exe pasa 0x40000 (BMEMORY_ALIGNMENT(0x1000)) donde los demas pasan 0x2000: TrackStreamer::InitMemoryPool
// (0x824F3060), bGetFile, CarLoader::DefragmentPool, eStreamPackLoader::CreateStreamingPack...
#ifdef EA_PLATFORM_XENON
#define BCHUNK_ALIGNMENT 0x1000
#else
#define BCHUNK_ALIGNMENT 128
#endif

#endif
