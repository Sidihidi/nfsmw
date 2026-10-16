#include <snd/sfilter.h>

// total size: 0x3C
// En PS2 (snd 9.06.00-ps2) mide 0x100: los stabs de la A124 le ponen delante los coeficientes para la VU0,
// alineados a 16 (por eso la `fir` de LPFSTATE/HPFSTATE/BPFSTATE cae en 0x20), y detras resetVU0coef.
typedef struct SNDFIRSTATE {
#ifdef EA_PLATFORM_PLAYSTATION2
    float VU0coef[12][4] __attribute__((aligned(16))); // offset 0x0, size 0xC0
#endif
    float history[8]; // offset 0x0, size 0x20 (PS2: 0xC0)
    float coef[5];    // offset 0x20, size 0x14 (PS2: 0xE0)
    float cutLow;     // offset 0x34 (PS2: 0xF4)
    float cutHigh;    // offset 0x38 (PS2: 0xF8)
#ifdef EA_PLATFORM_PLAYSTATION2
    signed char resetVU0coef; // offset 0xFC
    char pad[3];              // offset 0xFD
#endif
} SNDFIRSTATE;

void SNDI_fir8init(SNDFIRSTATE *psfs) {
    psfs->history[0] = 0.0f;
    psfs->history[1] = 0.0f;
    psfs->history[2] = 0.0f;
    psfs->history[3] = 0.0f;
    psfs->history[4] = 0.0f;
    psfs->history[5] = 0.0f;
    psfs->history[6] = 0.0f;
    psfs->history[7] = 0.0f;
#ifdef EA_PLATFORM_PLAYSTATION2
    // Los huecos de la matriz de coeficientes de la VU0 (ver SNDI_fir8); el resto lo rellena SNDI_fir8.
    psfs->VU0coef[0][1] = 0.0f;
    psfs->VU0coef[0][2] = 0.0f;
    psfs->VU0coef[0][3] = 0.0f;

    psfs->VU0coef[1][2] = 0.0f;
    psfs->VU0coef[1][3] = 0.0f;

    psfs->VU0coef[2][3] = 0.0f;

    psfs->VU0coef[9][0] = 0.0f;

    psfs->VU0coef[10][0] = 0.0f;
    psfs->VU0coef[10][1] = 0.0f;

    psfs->VU0coef[11][0] = 0.0f;
    psfs->VU0coef[11][1] = 0.0f;
    psfs->VU0coef[11][2] = 0.0f;
#endif
}

#ifdef EA_PLATFORM_PLAYSTATION2
// snd 9.06.00-ps2: el filtro de 9 coeficientes simetricos va en la VU0, cuatro muestras por vuelta. VU0coef es la
// matriz de Toeplitz 12x4 de los coeficientes (columna k, fila k + t = coef t), que se rehace cuando cambian; el asm es
// el del original (los vf y los registros de trabajo son los del binario de la A124).
void SNDI_fir8(SNDFIRSTATE *pfir, int frames, void *psrcp, void *pdstp) {
    if (pfir->resetVU0coef) {
        pfir->VU0coef[0][0] = pfir->coef[0];
        pfir->VU0coef[1][1] = pfir->coef[0];
        pfir->VU0coef[2][2] = pfir->coef[0];
        pfir->VU0coef[3][3] = pfir->coef[0];

        pfir->VU0coef[1][0] = pfir->coef[1];
        pfir->VU0coef[2][1] = pfir->coef[1];
        pfir->VU0coef[3][2] = pfir->coef[1];
        pfir->VU0coef[4][3] = pfir->coef[1];

        pfir->VU0coef[2][0] = pfir->coef[2];
        pfir->VU0coef[3][1] = pfir->coef[2];
        pfir->VU0coef[4][2] = pfir->coef[2];
        pfir->VU0coef[5][3] = pfir->coef[2];

        pfir->VU0coef[3][0] = pfir->coef[3];
        pfir->VU0coef[4][1] = pfir->coef[3];
        pfir->VU0coef[5][2] = pfir->coef[3];
        pfir->VU0coef[6][3] = pfir->coef[3];

        pfir->VU0coef[4][0] = pfir->coef[4];
        pfir->VU0coef[5][1] = pfir->coef[4];
        pfir->VU0coef[6][2] = pfir->coef[4];
        pfir->VU0coef[7][3] = pfir->coef[4];

        pfir->VU0coef[5][0] = pfir->coef[3];
        pfir->VU0coef[6][1] = pfir->coef[3];
        pfir->VU0coef[7][2] = pfir->coef[3];
        pfir->VU0coef[8][3] = pfir->coef[3];

        pfir->VU0coef[6][0] = pfir->coef[2];
        pfir->VU0coef[7][1] = pfir->coef[2];
        pfir->VU0coef[8][2] = pfir->coef[2];
        pfir->VU0coef[9][3] = pfir->coef[2];

        pfir->VU0coef[7][0] = pfir->coef[1];
        pfir->VU0coef[8][1] = pfir->coef[1];
        pfir->VU0coef[9][2] = pfir->coef[1];
        pfir->VU0coef[10][3] = pfir->coef[1];

        pfir->VU0coef[8][0] = pfir->coef[0];
        pfir->VU0coef[9][1] = pfir->coef[0];
        pfir->VU0coef[10][2] = pfir->coef[0];
        pfir->VU0coef[11][3] = pfir->coef[0];

        pfir->resetVU0coef = 0;
    }

    asm __volatile__(".set noreorder\n"
                     "move $6, %1\n"
                     "move $4, %0\n"
                     "move $5, %4\n"
                     "sll $6, $6, 2\n"
                     "move $7, %2\n"
                     "move $8, %3\n"
                     "add $6, $6, $7\n"
                     "lqc2 vf1, 0x0($4)\n"
                     "lqc2 vf2, 0x10($4)\n"
                     "lqc2 vf3, 0x20($4)\n"
                     "lqc2 vf4, 0x30($4)\n"
                     "lqc2 vf5, 0x40($4)\n"
                     "lqc2 vf6, 0x50($4)\n"
                     "lqc2 vf7, 0x60($4)\n"
                     "lqc2 vf8, 0x70($4)\n"
                     "lqc2 vf9, 0x80($4)\n"
                     "lqc2 vf10, 0x90($4)\n"
                     "lqc2 vf11, 0xa0($4)\n"
                     "lqc2 vf12, 0xb0($4)\n"
                     "lqc2 vf20, 0x0($5)\n"
                     "lqc2 vf21, 0x10($5)\n"
                     "1:\n"
                     "lqc2 vf22, 0x0($7)\n"
                     "lqc2 vf23, 0x10($7)\n"
                     "lqc2 vf24, 0x20($7)\n"
                     "lqc2 vf25, 0x30($7)\n"
                     "vmulax.xyzw ACC, vf1, vf20x\n"
                     "vmadday.xyzw ACC, vf2, vf20y\n"
                     "vmaddaz.xyzw ACC, vf3, vf20z\n"
                     "vmaddaw.xyzw ACC, vf4, vf20w\n"
                     "vmaddax.xyzw ACC, vf5, vf21x\n"
                     "vmadday.xyzw ACC, vf6, vf21y\n"
                     "vmaddaz.xyzw ACC, vf7, vf21z\n"
                     "vmaddaw.xyzw ACC, vf8, vf21w\n"
                     "vmaddax.xyzw ACC, vf9, vf22x\n"
                     "vmadday.xyzw ACC, vf10, vf22y\n"
                     "vmaddaz.xyzw ACC, vf11, vf22z\n"
                     "vmaddw.xyzw vf28, vf12, vf22w\n"
                     "vmulax.xyzw ACC, vf1, vf21x\n"
                     "vmadday.xyzw ACC, vf2, vf21y\n"
                     "vmaddaz.xyzw ACC, vf3, vf21z\n"
                     "vmaddaw.xyzw ACC, vf4, vf21w\n"
                     "vmaddax.xyzw ACC, vf5, vf22x\n"
                     "vmadday.xyzw ACC, vf6, vf22y\n"
                     "vmaddaz.xyzw ACC, vf7, vf22z\n"
                     "vmaddaw.xyzw ACC, vf8, vf22w\n"
                     "vmaddax.xyzw ACC, vf9, vf23x\n"
                     "vmadday.xyzw ACC, vf10, vf23y\n"
                     "vmaddaz.xyzw ACC, vf11, vf23z\n"
                     "vmaddw.xyzw vf29, vf12, vf23w\n"
                     "vmulax.xyzw ACC, vf1, vf22x\n"
                     "vmadday.xyzw ACC, vf2, vf22y\n"
                     "vmaddaz.xyzw ACC, vf3, vf22z\n"
                     "vmaddaw.xyzw ACC, vf4, vf22w\n"
                     "vmaddax.xyzw ACC, vf5, vf23x\n"
                     "vmadday.xyzw ACC, vf6, vf23y\n"
                     "vmaddaz.xyzw ACC, vf7, vf23z\n"
                     "vmaddaw.xyzw ACC, vf8, vf23w\n"
                     "vmaddax.xyzw ACC, vf9, vf24x\n"
                     "vmadday.xyzw ACC, vf10, vf24y\n"
                     "vmaddaz.xyzw ACC, vf11, vf24z\n"
                     "vmaddw.xyzw vf30, vf12, vf24w\n"
                     "vmulax.xyzw ACC, vf1, vf23x\n"
                     "vmadday.xyzw ACC, vf2, vf23y\n"
                     "vmaddaz.xyzw ACC, vf3, vf23z\n"
                     "vmaddaw.xyzw ACC, vf4, vf23w\n"
                     "vmaddax.xyzw ACC, vf5, vf24x\n"
                     "vmadday.xyzw ACC, vf6, vf24y\n"
                     "vmaddaz.xyzw ACC, vf7, vf24z\n"
                     "vmaddaw.xyzw ACC, vf8, vf24w\n"
                     "vmaddax.xyzw ACC, vf9, vf25x\n"
                     "vmadday.xyzw ACC, vf10, vf25y\n"
                     "vmaddaz.xyzw ACC, vf11, vf25z\n"
                     "vmaddw.xyzw vf31, vf12, vf25w\n"
                     "addi $7, $7, 0x40\n"
                     "sqc2 vf28, 0x0($8)\n"
                     "sqc2 vf29, 0x10($8)\n"
                     "sqc2 vf30, 0x20($8)\n"
                     "sqc2 vf31, 0x30($8)\n"
                     "vmove.xyzw vf20, vf24\n"
                     "vmove.xyzw vf21, vf25\n"
                     "bne $6, $7, 1b\n"
                     "addi $8, $8, 0x40\n"
                     "sqc2 vf20, 0x0($5)\n"
                     "sqc2 vf21, 0x10($5)\n"
                     ".set reorder"
                     :
                     : "r"(pfir->VU0coef), "r"(frames), "r"(psrcp), "r"(pdstp), "r"(pfir->history)
                     : "$4", "$5", "$6", "$7", "$8", "memory");
}
#else
void SNDI_fir8(SNDFIRSTATE *pfir, int frames, void *psrcp, void *pdstp) {
    float *psrc;
    float *pdst;
    int i;

    psrc = (float *)psrcp;
    pdst = (float *)pdstp;
    for (i = 0; i < frames; i++) {
        pdst[i] = pfir->coef[0] * (psrc[i] + pfir->history[7]) +
                  pfir->coef[1] * (pfir->history[0] + pfir->history[6]) +
                  pfir->coef[2] * (pfir->history[1] + pfir->history[5]) +
                  pfir->coef[3] * (pfir->history[2] + pfir->history[4]) +
                  pfir->coef[4] * pfir->history[3];
        pfir->history[7] = pfir->history[6];
        pfir->history[6] = pfir->history[5];
        pfir->history[5] = pfir->history[4];
        pfir->history[4] = pfir->history[3];
        pfir->history[3] = pfir->history[2];
        pfir->history[2] = pfir->history[1];
        pfir->history[1] = pfir->history[0];
        pfir->history[0] = psrc[i];
    }
}
#endif
