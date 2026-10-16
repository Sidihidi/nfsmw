#ifndef PSX2_INC_DMAE_H
#define PSX2_INC_DMAE_H

#ifdef EA_PRAGMA_ONCE_SUPPORTED
#pragma once
#endif

// psx2/inc/DmaE.hpp del original (la ruta y el orden de los tipos salen del .mdebug de la A124: QWdata, CPU,
// DmaChannel, DmaDescriptor, sDmaSrcTag, sVifTag, eDmaTransferType, eVifCommand, DMAC y los enums del GS). Las
// lineas de los inline que expande el juego: CPU::IntsOff 27-36, CPU::IntsOn 43, la alineacion 61-66, CPU::SyncP y
// SyncL 69-70, FloatAsInteger 132, los tags de DMA 222-228 y los de GIF 417-422.

#include "Speed/PSX2/bWare/Src/ee/include/eetypes.h"
#include "Speed/PSX2/bWare/Src/ee/include/eeregs.h"

typedef union {
    u_long128 ul128;
    u_long ul64[2];
    u_int ui32[4];
    float f32[4];
} QWdata;

struct CPU {
    // El DI del SDK de Sony (eekernel.h): el bit EIE se relee hasta que el di ha surtido efecto.
    static void IntsOff() {
        u_long stat;

        do {
            asm volatile("di");
            asm volatile("sync.p");
            asm volatile("mfc0 %0, $12" : "=r"(stat));
        } while (stat & 0x10000);
    }
    static void IntsOn() {
        asm volatile("ei\n\tnop\n\tsync.p\n\tnop");
    }
    static void ZeroCount();
    static u_int GetCount();
    static void SyncP() {
        asm volatile("sync.p");
    }
    static void SyncL() {
        asm volatile("sync.l");
    }
};

struct DmaChannel {
    // total size: 0x90
    unsigned int CHCR;     // offset 0x0, size 0x4
    unsigned int pad_0[3]; // offset 0x4, size 0xC
    unsigned int MADR;     // offset 0x10, size 0x4
    unsigned int pad_1[3]; // offset 0x14, size 0xC
    unsigned int QWC;      // offset 0x20, size 0x4
    unsigned int pad_2[3]; // offset 0x24, size 0xC
    unsigned int TADR;     // offset 0x30, size 0x4
    unsigned int pad_3[3]; // offset 0x34, size 0xC
    unsigned int ASR0;     // offset 0x40, size 0x4
    unsigned int pad_4[3]; // offset 0x44, size 0xC
    unsigned int ASR1;     // offset 0x50, size 0x4
    unsigned int pad_5[3]; // offset 0x54, size 0xC
    long128 pad_6;         // offset 0x60, size 0x10
    long128 pad_7;         // offset 0x70, size 0x10
    unsigned int SADR;     // offset 0x80, size 0x4
    unsigned int pad_8[3]; // offset 0x84, size 0xC
};

struct DmaDescriptor {
    // total size: 0x8
    DmaChannel *hware;      // offset 0x0, size 0x4
    unsigned int chanFlags; // offset 0x4, size 0x4
};

struct sDmaSrcTag {
    // total size: 0x10
    unsigned int qwc : 16;       // offset 0x0, size 0x4
    unsigned int unused : 10;    // offset 0x0, size 0x4
    unsigned int pce : 2;        // offset 0x0, size 0x4
    unsigned int id : 3;         // offset 0x0, size 0x4
    unsigned int irq : 1;        // offset 0x0, size 0x4
    unsigned int addr : 31;      // offset 0x4, size 0x4
    unsigned int spr : 1;        // offset 0x4, size 0x4
    unsigned int padding[2];     // offset 0x8, size 0x8
};

struct sVifTag {
    // total size: 0x4
    unsigned int immediate : 16; // offset 0x0, size 0x4
    unsigned int num : 8;        // offset 0x0, size 0x4
    unsigned int cmd : 8;        // offset 0x0, size 0x4

    const char *commandName();
};

enum eDmaTransferType {
    eDMA_REFE = 0,
    eDMA_CNT = 1,
    eDMA_NEXT = 2,
    eDMA_REF = 3,
    eDMA_REFS = 4,
    eDMA_CALL = 5,
    eDMA_RET = 6,
    eDMA_END = 7,
};

enum eVifCommand {
    NOP = 0,
    STCYCL = 1,
    OFFSET = 2,
    BASE = 3,
    ITOP = 4,
    STMOD = 5,
    MSKPATH3 = 6,
    MARK = 7,
    FLUSHE = 16,
    FLUSH = 17,
    FLUSHA = 19,
    MSCAL = 20,
    MSCALF = 21,
    MSCNT = 23,
    STMASK = 32,
    STROW = 48,
    STCOL = 49,
    MPG = 74,
    DIRECT = 80,
    DIRECTHL = 81,
    UNPACK = 96,
};

struct DMAC {
  private:
    static DmaDescriptor dmac_bases[10];
    static sDmaSrcTag *tagStack[2];
    static int tagStackIndex;
    static unsigned short mostRecentSTCYCL;

  public:
    static void Reset();
    static void CycleStealMode(int on);
    static unsigned int GetDmacADDR(void *address);
    static void DumpGifTag(void *tag);
    static int DumpVifPacket(int channel, void *packet);
    static void DumpDmaPacket(int channel, void *packet);
    static void FastSync(volatile DmaChannel *channel, unsigned int mask);
    static void SyncGsDma();
    static void SyncToSPRDma();
    static void SyncVif1Dma();
    static void SyncVif0Dma();
    static void ToSPR(void *spr_address, unsigned int size, unsigned int mode);

  private:
    static DmaChannel *GetChannel(int channel);
    static void SpadDMA();
    static void SendChain(volatile DmaChannel *channel, void *chain);
    static void SendChain(int channel, void *chain);
    static void pushTag(sDmaSrcTag *tag);
    static sDmaSrcTag *popTag();
    static sDmaSrcTag *getNextDmaTag(sDmaSrcTag *tag);
    static void *dmaTransferAddress(sDmaSrcTag *tag);
    static void dmaAddress(sDmaSrcTag *tag);
    static int vifUnpackNumInts(sVifTag *tag);
    static sVifTag *getNextVifTag(sVifTag *tag);
};

enum DMA_CHANNEL {
    DMA_VIF0 = 0,
    DMA_VIF1 = 1,
    DMA_GIF = 2,
    DMA_fromIPU = 3,
    DMA_toIPU = 4,
    DMA_SIF0 = 5,
    DMA_SIF1 = 6,
    DMA_SIF2 = 7,
    DMA_fromSPR = 8,
    DMA_toSPR = 9,
};

// El id del tag de DMA ya desplazado a su sitio en el byte 3 (eDmaChain::Add lo escribe con un sb).
enum DMA_TYPES {
    DMA_REFE = 0,
    DMA_CNT = 16,
    DMA_NEXT = 32,
    DMA_REF = 48,
    DMA_REFS = 64,
    DMA_CALL = 80,
    DMA_RET = 96,
    DMA_END = 112,
};

enum DMA_PCE {
    PCE_DEFAULT = 0,
    PCE_RESERVED = 1,
    PCE_DISABLED = 2,
    PCE_ENABLED = 3,
};

enum tagVifUnpackTypes {
    S_32 = 0,
    S_16 = 1,
    S_8 = 2,
    V2_32 = 4,
    V2_16 = 5,
    V2_8 = 6,
    V3_32 = 8,
    V3_16 = 9,
    V3_8 = 10,
    V4_32 = 12,
    V4_16 = 13,
    V4_8 = 14,
    V4_5 = 15,
    VWR_MASK = 16,
};

enum tagVifUnpackFlags {
    VUNPACK_UNSIGNED = 16384,
    VUNPACK_TOPS = 32768,
};

enum tagVifMasks {
    VMASK_WRITE = 0,
    VMASK_ROW = 1,
    VMASK_COL = 2,
    VMASK_MASK = 3,
};

enum tagVifModes {
    VMODE_NORMAL = 0,
    VMODE_OFFSET = 1,
    VMODE_DIFFERENCE = 2,
};

enum tagPrimAttributes {
    PRIM_POINT = 0,
    PRIM_LINE = 1,
    PRIM_LINESTRIP = 2,
    PRIM_TRI = 3,
    PRIM_TRISTRIP = 4,
    PRIM_TRIFAN = 5,
    PRIM_SPRITE = 6,
    PRIM_IIP = 8,
    PRIM_TME = 16,
    PRIM_FGE = 32,
    PRIM_ABE = 64,
    PRIM_AA1 = 128,
    PRIM_FST = 256,
    PRIM_CTXT1 = 0,
    PRIM_CTXT2 = 512,
    PRIM_FIX = 1024,
};

enum tagGifPackedModes {
    GIF_PACKED = 0,
    GIF_REGLIST = 1,
    GIF_IMAGE = 2,
};

enum GS_PSM {
    PSMCT32 = 0,
    PSMCT24 = 1,
    PSMCT16 = 2,
    PSMCT16S = 10,
    PSMT8 = 19,
    PSMT4 = 20,
    PSMT8H = 27,
    PSMT4HL = 36,
    PSMT4HH = 44,
    PSMZ32 = 48,
    PSMZ24 = 49,
    PSMZ16 = 50,
    PSMZ16S = 58,
};

enum GS_TEST {
    SHIFT_ATE = 0,
    SHIFT_ATST = 1,
    SHIFT_AREF = 4,
    SHIFT_AFAIL = 12,
    SHIFT_DATE = 14,
    SHIFT_DATM = 15,
    SHIFT_ZTE = 16,
    SHIFT_ZTST = 17,
    ATE_OFF = 0,
    ATE_ON = 1,
    ATST_NEVER = 0,
    ATST_ALWAYS = 1,
    ATST_LESS = 2,
    ATST_LEQUAL = 3,
    ATST_EQUAL = 4,
    ATST_GEQUAL = 5,
    ATST_GREATER = 6,
    ATST_NOTEQUAL = 7,
    AFAIL_KEEP = 0,
    AFAIL_FB_ONLY = 1,
    AFAIL_ZB_ONLY = 2,
    AFAIL_RGB_ONLY = 3,
    DATE_OFF = 0,
    DATE_ON = 1,
    ZTE_OFF = 0,
    ZTE_ON = 1,
    ZTST_NEVER = 0,
    ZTST_ALWAYS = 1,
    ZTST_GEQUAL = 2,
    ZTST_GREATER = 3,
};

// Un quadword visto por partes: los paquetes de DMA/VIF/GIF se escriben por palabras de 64 y de 32 bits.
union _DATA16 {
    u_long128 q;
    u_long d[2];
    unsigned int w[4];
    unsigned short s[8];
    unsigned char c[16];
    float f[4];
};

typedef _DATA16 DATA16;

// Un codigo de VIF: el comando en el byte alto, NUM en el tercero y el inmediato de 16 bits. El orden de los OR es el del
// original (el UNPACK de AttachVUdata hace `(inmediato | comando) | NUM`, al reves que SCE_VIF1_SET_CODE).
#define VIF_CODE(cmd, num, immediate) (((cmd) << 24) | ((num) << 16) | (immediate))

// Alinea un puntero hacia arriba (la A124 lo expande con el `addiu n-1; and -n` de siempre).
inline void *AlignPointer(void *address, int alignment) {
    return reinterpret_cast<void *>((reinterpret_cast<unsigned int>(address) + alignment - 1) & ~(alignment - 1));
}

// La direccion fisica (los cuatro bits altos fuera) y la vista sin cache acelerada (0x30000000).
inline void *PhysicalAddress(void *address) {
    return reinterpret_cast<void *>(reinterpret_cast<unsigned int>(address) & 0x0FFFFFFF);
}

inline void *UncachedAcceleratedAddress(void *address) {
    return reinterpret_cast<void *>(reinterpret_cast<unsigned int>(address) | 0x30000000);
}

inline int FloatAsInteger(float x) {
    return *reinterpret_cast<int *>(&x);
}

// La inversa (la usa MapPaletteIndexToScreenZ: linea 136 del original).
inline float IntegerAsFloat(int x) {
    return *reinterpret_cast<float *>(&x);
}

// Los 64 bits bajos de un tag de DMA: QWC, el id en los bits 28-30 y la direccion en la palabra alta. El original tiene
// dos formas (lineas 222-224 y 227-228) que solo se distinguen en el orden de los OR, y ese orden se ve en el codigo
// cuando la direccion no es constante: (dir | id) | qwc en la primera y (dir | qwc) | id en la segunda.
inline u_long MakeDmaTag(unsigned int qwc, unsigned int id, const void *address = 0) {
    return ((u_long)address << 32) | ((u_long)id << 28) | qwc;
}

inline u_long MakeDmaTagA(const void *address, unsigned int qwc, unsigned int id) {
    return ((u_long)address << 32) | qwc | ((u_long)id << 28);
}

// Las dos palabras bajas de un GIF tag y el tag de 64 bits entero.
inline unsigned int MakeGifTagLo(unsigned int nloop, unsigned int eop) {
    return (eop << 15) | nloop;
}

inline unsigned int MakeGifTagHi(unsigned int pre, unsigned int prim, unsigned int flg, unsigned int nreg) {
    return (pre << 14) | (prim << 15) | (flg << 26) | (nreg << 28);
}

inline u_long MakeGifTag(unsigned int nloop, unsigned int eop, unsigned int pre, unsigned int prim, unsigned int flg,
                         unsigned int nreg) {
    return MakeGifTagLo(nloop, eop) | (static_cast<u_long>(MakeGifTagHi(pre, prim, flg, nreg)) << 32);
}

#endif
