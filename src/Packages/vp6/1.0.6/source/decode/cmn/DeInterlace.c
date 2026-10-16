#include "type_aliases.h"

/* Sin prototipo, como en el original: en GameCube las dos llamadas llevan crclr. */
extern void *memcpy();

void CFastDeInterlace(UINT8 *SrcPtr, UINT8 *DstPtr, INT32 Width, INT32 Height, INT32 Stride) {
    INT32 i, j;
    UINT32 x0, x1, x2;
    UINT8 *PrevSrcPtr, *NextSrcPtr;
    UINT8 *CurrentSrcPtr = SrcPtr;
    UINT8 *CurrentDstPtr = DstPtr;

    memcpy(CurrentDstPtr, CurrentSrcPtr, Width);

    for (i = 1; i < Height - 1; i++) {
        PrevSrcPtr = CurrentSrcPtr;
        CurrentSrcPtr += Stride;
        NextSrcPtr = CurrentSrcPtr + Stride;
        CurrentDstPtr += Stride;

        for (j = 0; j < Width; j++) {
            x0 = PrevSrcPtr[j];
            x1 = CurrentSrcPtr[j] << 1;
            x2 = NextSrcPtr[j];
            CurrentDstPtr[j] = (UINT8)((x0 + x1 + x2 + 2) >> 2);
        }
    }

    memcpy(CurrentDstPtr + Stride, CurrentSrcPtr + Stride, Width);
}
