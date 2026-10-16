#ifndef QUANTIZE_H
#define QUANTIZE_H

#include "codec_common.h"

typedef struct {
    UINT32 FrameQIndex;
    UINT32 ThisFrameQuantizerValue;
    INT16 round[8];
    INT16 mult[8];
    INT16 zbin[8];
    UINT32 LastQuantizerValue;
    UINT32 QThreshTable[64];
    UINT32 *transIndex;
    UINT8 quant_index[64];
    Q_LIST_ENTRY *dequant_coeffs[2];
    Q_LIST_ENTRY *dequant_coeffsAlloc[2];
    INT32 QuantCoeffs[2][64];
    INT32 QuantRound[2][64];
    INT32 ZeroBinSize[2][64];
    INT32 ZlrZbinCorrections[2][64];
} QUANTIZER;

extern const Q_LIST_ENTRY VP6_DcQuant[64];
extern const UINT8 VP6_QTableSelect[6];
extern void (*VP6_BuildQuantIndex)(QUANTIZER *qi);
extern void (*VP6_quantize)(QUANTIZER *qi, INT16 *DCT_block, Q_LIST_ENTRY *quantized_list, UINT8 bp);

extern void VP6_InitQTables(QUANTIZER *qi, UINT8 Vp3VersionNo);
extern void VP6_BuildQuantIndex_Generic(QUANTIZER *qi);
extern void VP6_init_dequantizer(QUANTIZER *qi, UINT8 Vp3VersionNo);
extern void VP6_UpdateQ(QUANTIZER *qi, UINT8 Vp3VersionNo);
extern void VP6_DeleteQuantizer(QUANTIZER **qi);
extern QUANTIZER *VP6_CreateQuantizer(void);

#endif
