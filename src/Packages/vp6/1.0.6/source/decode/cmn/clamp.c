#include "postp.h"

/* En la A124 casa con SrcPtr declarado delante de DestPtr (el orden que dan
   los stabs y tambien el DWARF de GameCube), pero GameCube solo sale identico
   con este orden: los dos compiladores reparten al reves los dos punteros de la
   fila siguiente. Se deja el de GameCube, que no puede cambiar. */
void ClampLevels_C(POSTPROC_INSTANCE *pbi, INT32 BlackClamp, INT32 WhiteClamp, UINT8 *Src, UINT8 *Dst) {
    int i;
    int row;
    int col;
    unsigned char clamped[256];
    int width;
    int height;
    UINT8 *DestPtr;
    UINT8 *SrcPtr;
    UINT32 LineLength;

    width = pbi->HFragments * 8;
    height = pbi->VFragments * 8;
    SrcPtr = Src + pbi->ReconYDataOffset;
    DestPtr = Dst + pbi->ReconYDataOffset;
    LineLength = pbi->YStride;

    for (i = 0; i < 256; i++) {
        clamped[i] = i;
        if (i < BlackClamp) {
            clamped[i] = BlackClamp;
        }
        if (i > 255 - WhiteClamp) {
            clamped[i] = ~WhiteClamp;
        }
    }

    for (row = 0; row < height; row++) {
        for (col = 0; col < width; col++) {
            SrcPtr[col] = clamped[DestPtr[col]];
        }
        SrcPtr += LineLength;
        DestPtr += LineLength;
    }
}
