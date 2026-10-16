#include <snd/sfilter.h>

typedef void RSFFN(int, float *, float *, unsigned int *, unsigned int *, unsigned int, unsigned int);

extern "C" RSFFN rsflc;

typedef struct RSFSTATE {
    SFILTERNODE sfn;
    unsigned int incr16_16;
    unsigned int acc16_16;
#ifndef EA_PLATFORM_PLAYSTATION2
    RSFFN *rsf;
#endif
    short firstdone;
    short centersample;
    float history[4];
} RSFSTATE;

int SFILTER_rsf(void *pstate, int outputframes, void *psrcp, void *pdstp, int requester);

void SFILTER_rsfsetpitch(RSFSTATE *prss, int incrementor) {
    prss->incr16_16 = incrementor;
}

int SFILTER_rsf(void *pstate, int outputframes, void *psrcp, void *pdstp, int requester) {
    RSFSTATE *prss;
#ifndef EA_PLATFORM_PLAYSTATION2
    unsigned int incwhole;
    unsigned int incfrac;
    unsigned int accfrac;
#endif
    float *psrc;
    float *pdst;
    float *ptemp;
    int inputframes;
    int temp;
    int i;
#ifndef EA_PLATFORM_PLAYSTATION2
    unsigned int accwhole;
#endif

    prss = (RSFSTATE *)pstate;
    psrc = (float *)psrcp;
    pdst = (float *)pdstp;
    temp = outputframes;

    if (prss->incr16_16 == 0x10000) {
        inputframes = outputframes;
        if (prss->firstdone == 0) {
            inputframes = outputframes + (4 - prss->centersample);
        } else {
            for (i = 0; i < 4 - prss->centersample; i++) {
                *pdst++ = prss->history[i + prss->centersample];
            }
        }
        if (prss->sfn.pfnnext != 0) {
            temp = prss->sfn.pfnnext->filterfn(prss->sfn.pfnnext, inputframes, psrcp, pdst,
                                              prss->sfn.requester);
        }
        if (temp <= 0) {
            if (prss->firstdone == 0) {
                prss->firstdone = 1;
            }
            prss->history[0] = 0.0f;
            prss->history[1] = 0.0f;
            prss->history[2] = 0.0f;
            prss->history[3] = 0.0f;
            return temp;
        }
        if (prss->firstdone == 0) {
            prss->firstdone = 1;
            ptemp = pdst + (outputframes - prss->centersample);
        } else {
            ptemp = pdst + (outputframes - 4);
        }
        prss->history[0] = ptemp[0];
        prss->history[1] = ptemp[1];
        prss->history[2] = ptemp[2];
        prss->history[3] = ptemp[3];
        return outputframes;
    } else {
        inputframes = (prss->incr16_16 * outputframes + prss->acc16_16) >> 16;
        if (prss->firstdone == 0) {
            inputframes = inputframes + (4 - prss->centersample);
            for (i = 0; i < prss->centersample; i++) {
                *psrc++ = 0.0f;
            }
        } else {
            psrc[0] = prss->history[0];
            psrc[1] = prss->history[1];
            psrc[2] = prss->history[2];
            psrc[3] = prss->history[3];
            psrc += 4;
        }
        if (inputframes > 0 && prss->sfn.pfnnext != 0) {
            temp = prss->sfn.pfnnext->filterfn(prss->sfn.pfnnext, inputframes, pdstp, psrc,
                                              prss->sfn.requester);
            if (temp <= 0) {
                if (prss->firstdone == 0) {
                    prss->firstdone = 1;
                }
                prss->history[0] = 0.0f;
                prss->history[1] = 0.0f;
                prss->history[2] = 0.0f;
                prss->history[3] = 0.0f;
                return temp;
            }
        }
        if (prss->firstdone != 0) {
            psrc -= 4 - prss->centersample;
        } else {
            prss->firstdone = 1;
        }
#ifdef EA_PLATFORM_PLAYSTATION2
        // snd 9.06.00-ps2: interpolacion lineal de 16 en 16 muestras con MMI y la VU0; actualiza acc16_16
        asm __volatile__(".set noreorder\n"
                         "move $4, %0\n"
                         "move $5, %1\n"
                         "move $6, %2\n"
                         "move $7, %3\n"
                         "lw $10, 0x1c($4)\n"
                         "sll $8, $10, 2\n"
                         "pextlw $8, $8, $8\n"
                         "pextlw $8, $8, $8\n"
                         "sll $11, $10, 1\n"
                         "add $12, $11, $10\n"
                         "pextlw $9, $11, $0\n"
                         "pextlw $14, $12, $10\n"
                         "pextlw $9, $14, $9\n"
                         "paddw $10, $9, $8\n"
                         "paddw $11, $10, $8\n"
                         "paddw $12, $11, $8\n"
                         "psllw $8, $8, 2\n"
                         "lw $13, 0x20($4)\n"
                         "pextlw $13, $13, $13\n"
                         "pextlw $13, $13, $13\n"
                         "lui $14, 0x3780\n"
                         "qmtc2.ni $14, vf1\n"
                         "sll $5, $5, 2\n"
                         "add $5, $5, $7\n"
                         "mtsab $0, 4\n"
                         "1:\n"
                         "paddw $14, $13, $9\n"
                         "paddw $15, $13, $10\n"
                         "paddw $16, $13, $11\n"
                         "paddw $17, $13, $12\n"
                         "paddw $13, $13, $8\n"
                         "pinteh $18, $0, $14\n"
                         "pinteh $19, $0, $15\n"
                         "pinteh $20, $0, $16\n"
                         "pinteh $21, $0, $17\n"
                         "qmtc2.ni $18, vf2\n"
                         "qmtc2.ni $19, vf3\n"
                         "qmtc2.ni $20, vf4\n"
                         "qmtc2.ni $21, vf5\n"
                         "vitof0.xyzw vf2, vf2\n"
                         "vitof0.xyzw vf3, vf3\n"
                         "pextlw $18, $6, $6\n"
                         "vitof0.xyzw vf4, vf4\n"
                         "vitof0.xyzw vf5, vf5\n"
                         "vmulx.xyzw vf2, vf2, vf1x\n"
                         "vmulx.xyzw vf3, vf3, vf1x\n"
                         "pextlw $18, $18, $18\n"
                         "vmulx.xyzw vf4, vf4, vf1x\n"
                         "vmulx.xyzw vf5, vf5, vf1x\n"
                         "psrlw $14, $14, 16\n"
                         "psrlw $15, $15, 16\n"
                         "psrlw $16, $16, 16\n"
                         "psrlw $17, $17, 16\n"
                         "psllw $14, $14, 2\n"
                         "psllw $15, $15, 2\n"
                         "psllw $16, $16, 2\n"
                         "psllw $17, $17, 2\n"
                         "paddw $14, $14, $18\n"
                         "paddw $15, $15, $18\n"
                         "paddw $16, $16, $18\n"
                         "paddw $17, $17, $18\n"
                         "qfsrv $18, $0, $14\n"
                         "lw $19, 0x0($14)\n"
                         "lw $20, 0x4($14)\n"
                         "qfsrv $14, $0, $18\n"
                         "lw $21, 0x0($18)\n"
                         "lw $22, 0x4($18)\n"
                         "dsrl32 $18, $14, 0\n"
                         "lw $23, 0x0($14)\n"
                         "lw $24, 0x4($14)\n"
                         "pextlw $19, $23, $19\n"
                         "lw $23, 0x0($18)\n"
                         "lw $14, 0x4($18)\n"
                         "pextlw $20, $24, $20\n"
                         "pextlw $21, $23, $21\n"
                         "pextlw $22, $14, $22\n"
                         "pextlw $19, $21, $19\n"
                         "pextlw $20, $22, $20\n"
                         "qmtc2.ni $19, vf6\n"
                         "qmtc2.ni $20, vf10\n"
                         "qfsrv $18, $0, $15\n"
                         "lw $19, 0x0($15)\n"
                         "lw $20, 0x4($15)\n"
                         "qfsrv $15, $0, $18\n"
                         "lw $21, 0x0($18)\n"
                         "lw $22, 0x4($18)\n"
                         "dsrl32 $18, $15, 0\n"
                         "lw $23, 0x0($15)\n"
                         "lw $24, 0x4($15)\n"
                         "pextlw $19, $23, $19\n"
                         "lw $23, 0x0($18)\n"
                         "lw $14, 0x4($18)\n"
                         "pextlw $20, $24, $20\n"
                         "pextlw $21, $23, $21\n"
                         "pextlw $22, $14, $22\n"
                         "pextlw $19, $21, $19\n"
                         "pextlw $20, $22, $20\n"
                         "qmtc2.ni $19, vf7\n"
                         "qmtc2.ni $20, vf11\n"
                         "qfsrv $18, $0, $16\n"
                         "lw $19, 0x0($16)\n"
                         "lw $20, 0x4($16)\n"
                         "qfsrv $16, $0, $18\n"
                         "lw $21, 0x0($18)\n"
                         "lw $22, 0x4($18)\n"
                         "dsrl32 $18, $16, 0\n"
                         "lw $23, 0x0($16)\n"
                         "lw $24, 0x4($16)\n"
                         "pextlw $19, $23, $19\n"
                         "lw $23, 0x0($18)\n"
                         "lw $14, 0x4($18)\n"
                         "pextlw $20, $24, $20\n"
                         "pextlw $21, $23, $21\n"
                         "pextlw $22, $14, $22\n"
                         "pextlw $19, $21, $19\n"
                         "pextlw $20, $22, $20\n"
                         "qmtc2.ni $19, vf8\n"
                         "qmtc2.ni $20, vf12\n"
                         "qfsrv $18, $0, $17\n"
                         "lw $19, 0x0($17)\n"
                         "lw $20, 0x4($17)\n"
                         "qfsrv $17, $0, $18\n"
                         "lw $21, 0x0($18)\n"
                         "lw $22, 0x4($18)\n"
                         "dsrl32 $18, $17, 0\n"
                         "lw $23, 0x0($17)\n"
                         "lw $24, 0x4($17)\n"
                         "pextlw $19, $23, $19\n"
                         "lw $23, 0x0($18)\n"
                         "lw $14, 0x4($18)\n"
                         "pextlw $20, $24, $20\n"
                         "pextlw $21, $23, $21\n"
                         "pextlw $22, $14, $22\n"
                         "pextlw $19, $21, $19\n"
                         "pextlw $20, $22, $20\n"
                         "qmtc2.ni $19, vf9\n"
                         "qmtc2.ni $20, vf13\n"
                         "addi $7, $7, 0x40\n"
                         "vsub.xyzw vf10, vf10, vf6\n"
                         "vsub.xyzw vf11, vf11, vf7\n"
                         "vsub.xyzw vf12, vf12, vf8\n"
                         "vsub.xyzw vf13, vf13, vf9\n"
                         "vmul.xyzw vf10, vf10, vf2\n"
                         "vmul.xyzw vf11, vf11, vf3\n"
                         "vmul.xyzw vf12, vf12, vf4\n"
                         "vmul.xyzw vf13, vf13, vf5\n"
                         "vadd.xyzw vf10, vf10, vf6\n"
                         "vadd.xyzw vf11, vf11, vf7\n"
                         "vadd.xyzw vf12, vf12, vf8\n"
                         "vadd.xyzw vf13, vf13, vf9\n"
                         "sqc2 vf10, -0x40($7)\n"
                         "sqc2 vf11, -0x30($7)\n"
                         "sqc2 vf12, -0x20($7)\n"
                         "bne $5, $7, 1b\n"
                         "sqc2 vf13, -0x10($7)\n"
                         "sw $13, 0x20($4)\n"
                         ".set reorder"
                         :
                         : "r"(prss), "r"(outputframes), "r"(psrc), "r"(pdst)
                         : "$4", "$5", "$6", "$7", "$8", "$9", "$10", "$11", "$12", "$13", "$14", "$15", "$16", "$17",
                           "$18", "$19", "$20", "$21", "$22", "$23", "$24");
#else
        incfrac = prss->incr16_16 << 16;
        incwhole = prss->incr16_16 >> 16;
        accfrac = prss->acc16_16 << 16;
        accwhole = 0;
        prss->rsf(outputframes, psrc, (float *)pdstp, &accwhole, &accfrac, incwhole, incfrac);
        prss->acc16_16 = (accwhole << 16) | (accfrac >> 16);
#endif
        ptemp = psrc + ((prss->acc16_16 >> 16) - prss->centersample);
        prss->history[0] = ptemp[0];
        prss->history[1] = ptemp[1];
        prss->history[2] = ptemp[2];
        prss->history[3] = ptemp[3];
        prss->acc16_16 &= 0xFFFF;
    }
    return outputframes;
}

void SFILTER_rsfinit(RSFSTATE *prss, int cpu, int method) {
    int i;

    prss->sfn.filterfn = SFILTER_rsf;
    prss->sfn.restorefn = 0;
    prss->acc16_16 = 0;
    prss->firstdone = 0;
    prss->centersample = 0;
#ifndef EA_PLATFORM_PLAYSTATION2
    if (method == 0) {
        prss->rsf = rsflc;
    }
#endif
    for (i = 0; i <= 3; i++) {
        prss->history[i] = 0.0f;
    }
}
