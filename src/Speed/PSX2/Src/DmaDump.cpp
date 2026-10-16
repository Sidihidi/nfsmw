#include "Speed/Indep/bWare/Inc/bWare.hpp"
#include "Speed/Indep/bWare/Inc/bPrintf.hpp"
#include "Speed/PSX2/Inc/DmaE.hpp"
#include "Speed/PSX2/bWare/Src/ee/include/libdma.h"
#include "Speed/PSX2/bWare/Src/ee/include/eestruct.h"

// psx2/src/DmaDump.cpp de la A124: el volcado de cadenas de DMA para depurar (DMAC y los interpretes de los paquetes
// de VIF y de GIF). Las lineas que imprimian no estan en el ejecutable; lo que queda recorre los paquetes. Los tipos
// son los de los stabs, en su orden, y las tablas de cadenas, las del .data del ELF.

DmaDescriptor DMAC::dmac_bases[10] = {
    {reinterpret_cast<DmaChannel *>(0x10008000), 0}, {reinterpret_cast<DmaChannel *>(0x10009000), 0},
    {reinterpret_cast<DmaChannel *>(0x1000A000), 0}, {reinterpret_cast<DmaChannel *>(0x1000B000), 0},
    {reinterpret_cast<DmaChannel *>(0x1000B400), 0}, {reinterpret_cast<DmaChannel *>(0x1000C000), 0},
    {reinterpret_cast<DmaChannel *>(0x1000C400), 0}, {reinterpret_cast<DmaChannel *>(0x1000C800), 0},
    {reinterpret_cast<DmaChannel *>(0x1000D000), 0}, {reinterpret_cast<DmaChannel *>(0x1000D400), 0},
};

// Un GIF tag visto por campos (todos de 64 bits).
struct GifTag {
    // total size: 0x10
    unsigned long nloop : 15;
    unsigned long eop : 1;
    unsigned long pad16 : 16;
    unsigned long id : 14;
    unsigned long pre : 1;
    unsigned long prim : 11;
    unsigned long flg : 2;
    unsigned long nreg : 4;
    unsigned long regs0 : 4;
    unsigned long regs1 : 4;
    unsigned long regs2 : 4;
    unsigned long regs3 : 4;
    unsigned long regs4 : 4;
    unsigned long regs5 : 4;
    unsigned long regs6 : 4;
    unsigned long regs7 : 4;
    unsigned long regs8 : 4;
    unsigned long regs9 : 4;
    unsigned long regs10 : 4;
    unsigned long regs11 : 4;
    unsigned long regs12 : 4;
    unsigned long regs13 : 4;
    unsigned long regs14 : 4;
    unsigned long regs15 : 4;
};

enum GifDataDest {
    GDD_GIFTAG = 0,
    GDD_PACKED = 1,
    GDD_REGLIST = 2,
    GDD_IMAGE = 3,
    GDD_ERROR = 4,
};

enum GSRegisterState {
    GSREG_STATE_UNKNOWN = 0,
    GSREG_STATE_SET = 1,
    GSREG_STATE_SET_INVALID = 2,
};

struct GSRegister {
    // total size: 0x10
    GSRegisterState State; // offset 0x0, size 0x4
    unsigned long Data;    // offset 0x8, size 0x8
};

struct GifController {
    // total size: 0x1040
    u_long128 LastGifTag;           // offset 0x0, size 0x10
    unsigned int LastGifTagAddress; // offset 0x10, size 0x4
    GSRegister GSRegisters[256];    // offset 0x18, size 0x1000
    GifDataDest DataDest;           // offset 0x1018, size 0x4
    unsigned int ExpectedBytes;     // offset 0x101C, size 0x4
    u_long128 Data;                 // offset 0x1020, size 0x10
    unsigned char *CurrentDataPtr;  // offset 0x1030, size 0x4
    unsigned int RegisterIndex;     // offset 0x1034, size 0x4

    GifController();
    void Reset();
    void ProcessData(void *data_start, unsigned int byte_size);

  private:
    void SetDataDest(GifDataDest dest, unsigned int expected_bytes);
    void DecodeGSRegister(unsigned int reg, unsigned long data);
    int SetGSRegisterData(unsigned int reg, unsigned long data);
    void HandleGifTag(void *data);
    void HandlePackedData(void *data);
    void HandleRegListData(void *data);
    void HandleImageData(void *data);
};

const char *_error = "<error>";

// Las tres tablas se rellenan al arrancar (_GLOBAL_$I$ de zEcstasy en las dos PS2: las entradas libres son _error,
// que es una variable); por eso en el .data del ELF son ceros.
const char *GSRegNames[256] = {
    "PRIM", "RGBAQ", "ST", "UV", "XYZF2", "XYZ2", "TEX0_1", "TEX0_2", "CLAMP_1", "CLAMP_2", "FOG", _error, "XYZF3",
    "XYZ3", "A+D", _error, _error, _error, _error, _error, "TEX1_1", "TEX1_2", "TEX2_1", "TEX2_2", "XYOFFSET_1",
    "XYOFFSET_2", "PRMODECONT", "PRMODE", "TEXCLUT", _error, _error, _error, _error, _error, "SCANMASK", _error,
    _error, _error, _error, _error, _error, _error, _error, _error, _error, _error, _error, _error, _error, _error,
    _error, _error, "MIPTBP1_1", "MIPTBP1_2", "MIPTBP2_1", "MIPTBP2_2", _error, _error, _error, "TEXA", _error,
    "FOGCOL", _error, "TEXFLUSH", "SCISSOR_1", "SCISSOR_2", "ALPHA_1", "ALPHA_2", "DIMX", "DTHE", "COLCLAMP",
    "TEST_1", "TEST_2", "PABE", "FBA_1", "FBA_2", "FRAME_1", "FRAME_2", "ZBUF_1", "ZBUF_2", "BITBLTBUF", "TRXPOS",
    "TRXREG", "TRXDIR", "HWREG", _error, _error, _error, _error, _error, _error, _error, _error, _error, _error,
    _error, "SIGNAL", "FINISH", "LABEL", _error, _error, _error, _error, _error, _error, _error, _error, _error,
    _error, _error, _error, _error, _error, _error, _error, _error, _error, _error, _error, _error, _error, _error,
    _error, _error, _error, _error, _error, "NOP", _error, _error, _error, _error, _error, _error, _error, _error,
    _error, _error, _error, _error, _error, _error, _error, _error, _error, _error, _error, _error, _error, _error,
    _error, _error, _error, _error, _error, _error, _error, _error, _error, _error, _error, _error, _error, _error,
    _error, _error, _error, _error, _error, _error, _error, _error, _error, _error, _error, _error, _error, _error,
    _error, _error, _error, _error, _error, _error, _error, _error, _error, _error, _error, _error, _error, _error,
    _error, _error, _error, _error, _error, _error, _error, _error, _error, _error, _error, _error, _error, _error,
    _error, _error, _error, _error, _error, _error, _error, _error, _error, _error, _error, _error, _error, _error,
    _error, _error, _error, _error, _error, _error, _error, _error, _error, _error, _error, _error, _error, _error,
    _error, _error, _error, _error, _error, _error, _error, _error, _error, _error, _error, _error, _error, _error,
    _error, _error, _error, _error, _error, _error, _error, _error,
};
const char *GifTagPrimTypes[8] = {
    "POINT", "LINE", "LINESTRIP", "TRIANGLE", "TRISTRIP", "TRIFAN", "SPRITE", _error,
};
const char *GifTagFlagTypes[4] = {
    "PACKED ", "REGLIST", "IMAGE  ", _error,
};

GifController::GifController() {
    Reset();
}

void GifController::Reset() {
    LastGifTag = 0;
    LastGifTagAddress = 0;

    DataDest = GDD_GIFTAG;
    ExpectedBytes = 0;
    CurrentDataPtr = reinterpret_cast<unsigned char *>(&Data);
    RegisterIndex = 0;

    bMemSet(GSRegisters, 0, sizeof(GSRegisters));
}

void GifController::SetDataDest(GifDataDest dest, unsigned int expected_bytes) {
    DataDest = dest;
    ExpectedBytes = expected_bytes;
    CurrentDataPtr = reinterpret_cast<unsigned char *>(&Data);
}

void GifController::DecodeGSRegister(unsigned int reg, unsigned long data) {
}

void GifController::HandleGifTag(void *data) {
    GifTag *gif_tag = static_cast<GifTag *>(data);

    LastGifTag = *static_cast<u_long128 *>(data);
    LastGifTagAddress = reinterpret_cast<unsigned int>(data);

    switch (gif_tag->flg) {
        case GIF_PACKED:
        case GIF_REGLIST:
            DecodeGSRegister(SCE_GS_PRIM, gif_tag->prim);

            if (gif_tag->nloop) {
                GifDataDest dest = gif_tag->flg == GIF_PACKED ? GDD_PACKED : GDD_REGLIST;

                SetDataDest(dest, gif_tag->nloop * gif_tag->nreg * 16);
            }
            RegisterIndex = 0;
            break;

        case GIF_IMAGE:
            if (gif_tag->nloop) {
                SetDataDest(GDD_IMAGE, gif_tag->nloop * 16);
            }
            break;

        // FLG 3 no es ningun modo (tagGifPackedModes acaba en GIF_IMAGE), pero el original lo tiene como caso aparte.
        case 3:
            break;
    }
}

void GifController::HandlePackedData(void *data) {
}

void GifController::HandleRegListData(void *data) {
}

void GifController::HandleImageData(void *data) {
}

void GifController::ProcessData(void *data_start, unsigned int byte_size) {
    while (byte_size != 0 && DataDest != GDD_ERROR) {
        unsigned int space_left = 16 - (CurrentDataPtr - reinterpret_cast<unsigned char *>(&Data));
        unsigned int copy_bytes = space_left < byte_size ? space_left : byte_size;

        bMemCpy(CurrentDataPtr, data_start, copy_bytes);

        CurrentDataPtr += copy_bytes;

        if (CurrentDataPtr - reinterpret_cast<unsigned char *>(&Data) >= 16) {
            switch (DataDest) {
                case GDD_GIFTAG: HandleGifTag(&Data); break;
                case GDD_PACKED: HandlePackedData(&Data); break;
                case GDD_REGLIST: HandleRegListData(&Data); break;
                case GDD_IMAGE: HandleImageData(&Data); break;
                case GDD_ERROR: break;
            }
            CurrentDataPtr = reinterpret_cast<unsigned char *>(&Data);
        }
        byte_size -= copy_bytes;
    }
}

struct VifTag {
    // total size: 0x4
    unsigned int immediate : 16;
    unsigned int num : 8;
    unsigned int cmd : 8;
};

enum VifCommand {
    VIFCMD_NOP = 0,
    VIFCMD_STCYCL = 1,
    VIFCMD_OFFSET = 2,
    VIFCMD_BASE = 3,
    VIFCMD_ITOP = 4,
    VIFCMD_STMOD = 5,
    VIFCMD_MSKPATH3 = 6,
    VIFCMD_MARK = 7,
    VIFCMD_FLUSHE = 16,
    VIFCMD_FLUSH = 17,
    VIFCMD_FLUSHA = 19,
    VIFCMD_MSCAL = 20,
    VIFCMD_MSCALF = 21,
    VIFCMD_MSCNT = 23,
    VIFCMD_STMASK = 32,
    VIFCMD_STROW = 48,
    VIFCMD_STCOL = 49,
    VIFCMD_MPG = 74,
    VIFCMD_DIRECT = 80,
    VIFCMD_DIRECTHL = 81,
    VIFCMD_UNPACK = 96,
};

const char *VifUnpackModes[16] = {
    "S_32 ", "S_16 ", "S_8  ", "*Error*", "V2_32", "V2_16", "V2_8 ", "*Error*",
    "V3_32", "V3_16", "V2_8 ", "*Error*", "V4_32", "V4_16", "V4_8 ", "V4_5 ",
};

const char *VifStModes[4] = {"Normal    ", "Offset    ", "Difference", "*Error*"};

enum VifDataDest {
    VDD_NONE = 0,
    VDD_GIF = 1,
    VDD_MPG = 2,
    VDD_UNPACK = 3,
    VDD_STMASK = 4,
    VDD_STROW = 5,
    VDD_STCOL = 6,
};

struct VifController {
    // total size: 0x24
    unsigned int ID;                  // offset 0x0, size 0x4
    GifController *Gif;               // offset 0x4, size 0x4
    unsigned int ErrorOccured;        // offset 0x8, size 0x4
    unsigned int ExpectedBytes;       // offset 0xC, size 0x4
    unsigned int DataProcessingBreak; // offset 0x10, size 0x4
    VifDataDest DataDest;             // offset 0x14, size 0x4
    unsigned short MostRecentSTCYCL;  // offset 0x18, size 0x2
    unsigned int UnpackMode;          // offset 0x1C, size 0x4
    unsigned int StMode;              // offset 0x20, size 0x4

    VifController(int id, GifController *gif) {
        ID = id;
        Gif = gif;
        Reset();
    }
    void Reset();
    void ProcessData(void *vif_data_start, unsigned int byte_size);

  protected:
    void HandleInvalidTag(VifTag *vif_tag, const char *command_name);
    void SetExpectedData(VifDataDest dest, unsigned int byte_size);
};

void VifController::Reset() {
    ErrorOccured = 0;

    ExpectedBytes = 0;
    DataProcessingBreak = 0;
    DataDest = VDD_NONE;

    MostRecentSTCYCL = 0xFFFF;

    UnpackMode = 0;
    StMode = 0;
}

void VifController::HandleInvalidTag(VifTag *vif_tag, const char *command_name) {
}

void VifController::SetExpectedData(VifDataDest dest, unsigned int byte_size) {
    ExpectedBytes = byte_size;
    DataProcessingBreak = 0;
    DataDest = dest;
}

// Recorre un paquete de VIF palabra a palabra: decodifica cada VIF tag y, si trae datos detras (UNPACK, MPG,
// DIRECT...), se salta los bytes que le tocan. Los mensajes de cada comando no estan en el ejecutable.
void VifController::ProcessData(void *vif_data_start, unsigned int byte_size) {
    unsigned int *data_ptr;
    int bytes_left;

    if (ErrorOccured) {
        return;
    }

    if (byte_size & 3) {
        ErrorOccured = 1;
        return;
    }

    bytes_left = byte_size;
    data_ptr = static_cast<unsigned int *>(vif_data_start);

    while (bytes_left != 0 && !ErrorOccured) {
        bytes_left -= 4;

        switch (DataDest) {
            case VDD_NONE: {
                long command = reinterpret_cast<VifTag *>(data_ptr)->cmd;
                long immediate = reinterpret_cast<VifTag *>(data_ptr)->immediate;
                long num = reinterpret_cast<VifTag *>(data_ptr)->num;

                command &= 0x7F;

                switch (command) {
                    case VIFCMD_NOP:
                    case VIFCMD_ITOP:
                    case VIFCMD_MARK:
                    case VIFCMD_FLUSHE:
                    case VIFCMD_MSCAL:
                    case VIFCMD_MSCNT:
                        break;

                    case VIFCMD_STMOD:
                        StMode = immediate & 3;
                        break;

                    case VIFCMD_MPG:
                        if (num == 0) {
                            num = 256;
                        }
                        SetExpectedData(VDD_MPG, num * 8);
                        break;

                    case VIFCMD_STCYCL:
                        MostRecentSTCYCL = immediate;
                        break;

                    case VIFCMD_STMASK:
                        SetExpectedData(VDD_STMASK, 4);
                        break;

                    case VIFCMD_STROW:
                        SetExpectedData(VDD_STROW, 16);
                        break;

                    case VIFCMD_STCOL:
                        SetExpectedData(VDD_STCOL, 16);
                        break;

                    default: {
                        if ((command & VIFCMD_UNPACK) == VIFCMD_UNPACK) {
                            if (MostRecentSTCYCL == 0xFFFF) {
                                ErrorOccured = 1;
                            } else {
                                int needed_ints;
                                unsigned int cl;
                                unsigned int wl;
                                unsigned int vl;
                                unsigned int vn;

                                cl = MostRecentSTCYCL & 0xFF;
                                wl = MostRecentSTCYCL >> 8;
                                vl = command & 3;
                                vn = (command >> 2) & 3;

                                if (wl <= cl) {
                                    needed_ints = (32 >> vl) * (vn + 1) * num;
                                    needed_ints = (needed_ints + 31) / 32;
                                } else {
                                    unsigned int min = num % wl < cl ? num % wl : cl;
                                    unsigned int n = cl * (num / wl) + min;

                                    needed_ints = (32 >> vl) * (vn + 1) * n;
                                    needed_ints = (needed_ints + 31) / 32;
                                }

                                UnpackMode = command & 0xF;

                                SetExpectedData(VDD_UNPACK, needed_ints * 4);
                            }
                        } else {
                            if (ID == 0) {
                                switch (command) {
                                    case VIFCMD_OFFSET: HandleInvalidTag(reinterpret_cast<VifTag *>(data_ptr), "OFFSET (Invalid on Vif0)\n"); break;
                                    case VIFCMD_BASE: HandleInvalidTag(reinterpret_cast<VifTag *>(data_ptr), "BASE (Invalid on Vif0)\n"); break;
                                    case VIFCMD_MSKPATH3: HandleInvalidTag(reinterpret_cast<VifTag *>(data_ptr), "MSKPATH3 (Invalid on Vif0)\n"); break;
                                    case VIFCMD_FLUSH: HandleInvalidTag(reinterpret_cast<VifTag *>(data_ptr), "FLUSH (Invalid on Vif0)\n"); break;
                                    case VIFCMD_FLUSHA: HandleInvalidTag(reinterpret_cast<VifTag *>(data_ptr), "FLUSHA (Invalid on Vif0)\n"); break;
                                    case VIFCMD_MSCALF: HandleInvalidTag(reinterpret_cast<VifTag *>(data_ptr), "MSCALF (Invalid on Vif0)\n"); break;
                                    case VIFCMD_DIRECT: HandleInvalidTag(reinterpret_cast<VifTag *>(data_ptr), "DIRECT (Invalid on Vif0)\n"); break;
                                    case VIFCMD_DIRECTHL: HandleInvalidTag(reinterpret_cast<VifTag *>(data_ptr), "DIRECTHL (Invalid on Vif0)\n"); break;
                                }
                            }

                            switch (command) {
                                case VIFCMD_OFFSET:
                                case VIFCMD_BASE:
                                case VIFCMD_MSKPATH3:
                                case VIFCMD_FLUSH:
                                case VIFCMD_FLUSHA:
                                case VIFCMD_MSCALF:
                                    break;

                                case VIFCMD_DIRECT:
                                case VIFCMD_DIRECTHL:
                                    SetExpectedData(VDD_GIF, immediate * 16);
                                    break;

                                default:
                                    HandleInvalidTag(reinterpret_cast<VifTag *>(data_ptr), "<Unknown Command>");
                                    break;
                            }
                        }
                        break;
                    }
                }
                break;
            }

            case VDD_GIF:
                ExpectedBytes -= 4;
                break;

            case VDD_MPG:
                ExpectedBytes -= 4;
                break;

            case VDD_UNPACK:
                ExpectedBytes -= 4;
                break;

            case VDD_STMASK:
                ExpectedBytes -= 4;
                break;

            case VDD_STROW:
                ExpectedBytes -= 4;
                break;

            case VDD_STCOL:
                ExpectedBytes -= 4;
                break;
        }

        if (ExpectedBytes == 0) {
            DataDest = VDD_NONE;
        }

        data_ptr++;
    }

    if (ExpectedBytes) {
        DataProcessingBreak = 1;
    }
}

const char *DmaDestChannelStrings[10] = {"VIF0", "VIF1", "GIF ", "RAM ", "IPU ", "SIF0", "SIF1", "SIF2", "RAM ", "SPR "};
const char *DmaTypeString[8] = {"REFE", "CNT ", "NEXT", "REF ", "REFS", "CALL", "RET ", "END "};
const char *TTEStateString[2] = {"Off", "On "};

// Donde esta la memoria de origen de cada canal: 0 RAM, 1 SPR (fromSPR) y 2 IPU (fromIPU).
unsigned char DmaSrcChannelIDs[10] = {0, 0, 0, 2, 0, 0, 0, 0, 1, 0};

struct DmaTag {
    // total size: 0x10
    unsigned int qwc : 16;
    unsigned int unused : 10;
    unsigned int pce : 2;
    unsigned int id : 3;
    unsigned int irq : 1;
    unsigned int addr : 31;
    unsigned int spr : 1;
    unsigned int pad[2];
};

void *GetDmaTransferAddress(DmaTag *dma_tag) {
    unsigned int id = dma_tag->id << 4;

    switch (id) {
        case DMA_CNT:
        case DMA_NEXT:
        case DMA_CALL:
        case DMA_RET:
        case DMA_END:
            return dma_tag + 1;

        case DMA_REFE:
        case DMA_REF:
        case DMA_REFS:
            return reinterpret_cast<void *>(dma_tag->addr);
    }
    return dma_tag + 1;
}

char *GetDmaSourceAddressString(char *dest, int channel, DmaTag *dma_tag) {
    void *transfer_address = GetDmaTransferAddress(dma_tag);

    switch (DmaSrcChannelIDs[channel]) {
        case 0: bSPrintf(dest, "0x%08x (RAM)", transfer_address); break;
        case 1: bSPrintf(dest, "0x%08x (SPR)", transfer_address); break;
        case 2: bSPrintf(dest, "IPU"); break;
    }
    return dest;
}

DmaTag *GetNextDmaTag(DmaTag *dma_tag) {
    switch (dma_tag->id << 4) {
        case DMA_REFE:
        case DMA_CALL:
        case DMA_RET:
        case DMA_END:
            return 0;
        case DMA_CNT:
            return dma_tag + (dma_tag->qwc + 1);
        case DMA_NEXT:
            return reinterpret_cast<DmaTag *>(dma_tag->addr);
        case DMA_REF:
        case DMA_REFS:
            return dma_tag + 1;
    }
    return 0;
}

void DumpDmaChain(int channel, void *start_dma_tag_ptr) {
    int tte_state = sceDmaGetChan(channel)->chcr.TTE;

    GifController Gif;
    VifController Vif0(0, 0);
    VifController Vif1(1, &Gif);

    DmaTag *dma_tag = static_cast<DmaTag *>(start_dma_tag_ptr);

    while (dma_tag) {
        if ((reinterpret_cast<unsigned int>(dma_tag) & 0xF) == 0) {
            char src_text[64];

            GetDmaSourceAddressString(src_text, channel, dma_tag);

            unsigned char *tte_data = tte_state ? reinterpret_cast<unsigned char *>(dma_tag) + 8 : 0;

            unsigned char *transfer_address = static_cast<unsigned char *>(GetDmaTransferAddress(dma_tag));
            unsigned int transfer_byte_size = dma_tag->qwc * 16;

            if (tte_data && tte_data + 8 == transfer_address) {
                transfer_byte_size += 8;
                transfer_address = tte_data;
                tte_data = 0;
            }

            switch (channel) {
                case DMA_VIF0:
                    if (tte_data) {
                        Vif0.ProcessData(tte_data, 8);
                    }
                    Vif0.ProcessData(transfer_address, transfer_byte_size);
                    break;

                case DMA_VIF1:
                    if (tte_data) {
                        Vif1.ProcessData(tte_data, 8);
                    }
                    Vif1.ProcessData(transfer_address, transfer_byte_size);
                    break;

                case DMA_GIF:
                    Gif.ProcessData(transfer_address, transfer_byte_size);
                    break;
            }
        }
        dma_tag = GetNextDmaTag(dma_tag);
    }
}

const char *RegNames[16] = {"PRIM", "RGBAQ",   "ST",      "UV",  "XYZF2",  "XYZ2",  "TEX0_1", "TEX0_2",
                            "CLAMP_1", "CLAMP_2", "FOG", "error!", "XYZF3", "XYZ3", "A+D",    "NOP"};

int DMAC::tagStackIndex = 0;
unsigned short DMAC::mostRecentSTCYCL = 0xFFFF;

const char *sVifTag::commandName() {
    // Sin local (los stabs no tienen ninguna): el if compara con signo y el switch va sobre los 8 bits sin signo.
    if (static_cast<char>(cmd & 0x7F) >= UNPACK) {
        return "UNPACK";
    }

    switch (static_cast<unsigned char>(cmd & 0x7F)) {
        case NOP:
            return "NOP";
        case STCYCL:
            return "STCYCL";
        case OFFSET:
            return "OFFSET";
        case BASE:
            return "BASE";
        case ITOP:
            return "ITOP";
        case STMOD:
            return "STMOD";
        case MSKPATH3:
            return "MSKPATH3";
        case MARK:
            return "MARK";
        case FLUSHE:
            return "FLUSHE";
        case FLUSH:
            return "FLUSH";
        case FLUSHA:
            return "FLUSHA";
        case MSCAL:
            return "MSCAL";
        case MSCALF:
            return "MSCALF";
        case MSCNT:
            return "MSCNT";
        case STMASK:
            return "STMASK";
        case STROW:
            return "STROW";
        case STCOL:
            return "STCOL";
        case MPG:
            return "MPG";
        case DIRECT:
            return "DIRECT";
        case DIRECTHL:
            return "DIRECTHL";
    }
    return "???";
}

void DMAC::pushTag(sDmaSrcTag *tag) {
    tagStack[tagStackIndex++] = tag;
}

sDmaSrcTag *DMAC::popTag() {
    return tagStack[--tagStackIndex];
}

sDmaSrcTag *DMAC::getNextDmaTag(sDmaSrcTag *tag) {
    switch (tag->id) {
        case eDMA_REF:
        case eDMA_REFS:
            return tag + 1;

        case eDMA_CNT:
            return tag + (tag->qwc + 1);

        case eDMA_RET:
            return popTag();

        case eDMA_CALL:
            pushTag(tag + 1);

        case eDMA_NEXT:
            return reinterpret_cast<sDmaSrcTag *>(tag->addr);
    }
    return 0;
}

char *dmaDestinations[10] = {"VIF0", "VIF1", "GIF", "RAM", "IPU", "SIF0", "SIF1", "SIF2", "RAM", "SPR"};
char *dmaSources[10] = {"RAM", "RAM", "RAM", "IPU", "RAM", "RAM", "RAM", "RAM", "SPR", "RAM"};
char *dmaTypes[8] = {"REFE", "CNT", "NEXT", "REF", "REFS", "CALL", "RET", "END"};

int DMAC::vifUnpackNumInts(sVifTag *tag) {
    unsigned int ret = tag->cmd & 0x7F;
    unsigned int CL = mostRecentSTCYCL & 0xFF;
    unsigned int WL = mostRecentSTCYCL >> 8;
    unsigned int vl = ret & 3;
    unsigned int vn = (ret >> 2) & 3;

    if (WL <= CL) {
        ret = (32 >> vl) * (vn + 1) * tag->num;
        ret = (ret + 31) >> 5;
        return ret;
    } else {
        unsigned int n = CL * (tag->num / WL) + ((tag->num % WL) < CL ? tag->num % WL : CL);

        ret = (32 >> vl) * (vn + 1) * n;
        ret = (ret + 31) >> 5;
        return ret;
    }
}

sVifTag *DMAC::getNextVifTag(sVifTag *tag) {
    char cmd = tag->cmd & 0x7F;

    if (cmd >= UNPACK) {
        return tag + 1 + vifUnpackNumInts(tag);
    }

    switch (cmd) {
        default:
        case NOP:
        case STCYCL:
        case OFFSET:
        case BASE:
        case ITOP:
        case STMOD:
        case MSKPATH3:
        case MARK:
        case FLUSHE:
        case FLUSH:
        case FLUSHA:
        case MSCAL:
        case MSCALF:
        case MSCNT:
            return tag + 1;

        case STMASK:
            return tag + 2;

        case STROW:
        case STCOL:
            return tag + 5;

        case MPG:
            if (tag->num == 0) {
                return tag + 1 + 512;
            }
            return tag + 1 + tag->num * 2;

        case DIRECT:
        case DIRECTHL:
            return tag + 1 + tag->immediate * 4;
    }
}

int DMAC::DumpVifPacket(int len, void *packet) {
    int last_command_is_direct = 0;

    for (sVifTag *tag = static_cast<sVifTag *>(packet); reinterpret_cast<int>(tag) < reinterpret_cast<int>(packet) + len;
         tag = getNextVifTag(tag)) {
        last_command_is_direct = 0;

        if (static_cast<char>(tag->cmd & 0x7F) < UNPACK) {
            switch (static_cast<char>(tag->cmd & 0x7F)) {
                case NOP: break;
                case STCYCL: break;
                case OFFSET: break;
                case BASE: break;
                case ITOP: break;
                case STMOD: break;
                case MSKPATH3: break;
                case MARK: break;
                case FLUSHE: break;
                case FLUSH: break;
                case FLUSHA: break;
                case MSCAL: break;
                case MSCALF: break;
                case MSCNT: break;
                case STMASK: break;
                case STROW: break;
                case STCOL: break;
                case MPG: break;
                case DIRECT:
                case DIRECTHL:
                    last_command_is_direct = 1;
                    break;
            }
        }

        if ((tag->cmd & 0x7F) == STCYCL) {
            mostRecentSTCYCL = tag->immediate;
        }
    }
    return last_command_is_direct;
}

// La espera es asm de EA (el binario lo prueba: nop sueltos y bc0t/bc0f, que sondean la condicion CPCOND0 que el DMAC
// levanta al acabar el canal marcado en D_PCR): cuatro nop y despues cuatro sondeos por vuelta, contando en n.
void DMAC::FastSync(volatile DmaChannel *dmach, unsigned int latchBit) {
    if (dmach->CHCR & 0x100) {
        *D_PCR = latchBit;
        *D_STAT = latchBit;
        CPU::SyncP();
        CPU::SyncL();
        asm volatile("nop");

        if (dmach->CHCR & 0x100) {
            int n = 0;

            // La A124 espera cuatro nop antes del primer sondeo y la version de venta dieciocho.
            asm volatile(".set noreorder\n"
                         "\tnop\n"
                         "\tnop\n"
                         "\tnop\n"
                         "\tnop\n"
#ifndef EA_BUILD_A124
                         "\tnop\n"
                         "\tnop\n"
                         "\tnop\n"
                         "\tnop\n"
                         "\tnop\n"
                         "\tnop\n"
                         "\tnop\n"
                         "\tnop\n"
                         "\tnop\n"
                         "\tnop\n"
                         "\tnop\n"
                         "\tnop\n"
                         "\tnop\n"
                         "\tnop\n"
#endif
                         "1:\n"
                         "\tbc0t 2f\n"
                         "\taddiu %0, %0, 1\n"
                         "\tbc0t 2f\n"
                         "\taddiu %0, %0, 1\n"
                         "\tbc0t 2f\n"
                         "\taddiu %0, %0, 1\n"
                         "\tbc0f 1b\n"
                         "\taddiu %0, %0, 1\n"
                         "2:\n"
                         "\t.set reorder"
                         : "+r"(n));
        }
    }
}

void DMAC::SyncVif1Dma() {
    FastSync(reinterpret_cast<volatile DmaChannel *>(0x10009000), 2);
}
