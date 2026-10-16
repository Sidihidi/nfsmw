#include "../cmn/pbdll.h"

/* El descodificador trabaja en la memoria rapida del EE (scratchpad, 16 kB): los
   coeficientes de los seis bloques, las dos tablas de descuantizacion y dos buffers
   de 64 muestras van en los primeros 0x500 bytes de los 0x1000 que usa; el resto,
   hasta ScratchpadEnd, es la pila de VP6_DecodeFrameToYUV (vfwpbdll_if.c). */
void VP6_UseScratchpad(PB_INSTANCE *pbi, char *address) {
    pbi->ScratchpadStart = address;
    pbi->ScratchpadEnd = address + 0x1000;

    pbi->mbi.Coeffs = (Q_LIST_ENTRY(*)[64])address;
    pbi->quantizer->dequant_coeffs[0] = (Q_LIST_ENTRY *)(address + 0x300);
    pbi->quantizer->dequant_coeffs[1] = (Q_LIST_ENTRY *)(address + 0x380);
    pbi->ReconDataBuffer = (INT16 *)(address + 0x400);
    pbi->TmpDataBuffer = (INT16 *)(address + 0x480);
}
