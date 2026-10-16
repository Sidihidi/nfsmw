#include <snd/sfilter.h>

// total size: 0x20
typedef struct AMPLFSTATE {
    SFILTERNODE sfn; // offset 0x0, size 0x1C
    float gain;      // offset 0x1C
} AMPLFSTATE;

int SFILTER_amplf(void *pstate, int frames, void *psrcp, void *pdstp, int requester) {
    AMPLFSTATE *pas = (AMPLFSTATE *)pstate;
    int temp;

    if (pas->sfn.pfnnext) {
        temp = pas->sfn.pfnnext->filterfn(pas->sfn.pfnnext, frames, pdstp, psrcp,
                                          pas->sfn.requester);
        if (temp <= 0) {
            return temp;
        }
    }
#ifdef EA_PLATFORM_PLAYSTATION2
    // snd 9.06.00-ps2: la ganancia con la VU0, de 16 en 16 muestras (lqc2/vmulx/sqc2 en el binario)
    asm __volatile__(".set noreorder\n"
                     "move $4, %0\n"
                     "qmtc2.ni %1, vf1\n"
                     "sll $4, $4, 2\n"
                     "move $6, %2\n"
                     "move $7, %3\n"
                     "add $4, $4, $6\n"
                     "1:\n"
                     "lqc2 vf2, 0x0($6)\n"
                     "lqc2 vf3, 0x10($6)\n"
                     "lqc2 vf4, 0x20($6)\n"
                     "lqc2 vf5, 0x30($6)\n"
                     "addi $6, $6, 0x40\n"
                     "vmulx.xyzw vf2, vf2, vf1x\n"
                     "vmulx.xyzw vf3, vf3, vf1x\n"
                     "vmulx.xyzw vf4, vf4, vf1x\n"
                     "vmulx.xyzw vf5, vf5, vf1x\n"
                     "sqc2 vf2, 0x0($7)\n"
                     "sqc2 vf3, 0x10($7)\n"
                     "sqc2 vf4, 0x20($7)\n"
                     "sqc2 vf5, 0x30($7)\n"
                     "bne $4, $6, 1b\n"
                     "addi $7, $7, 0x40\n"
                     ".set reorder"
                     :
                     : "r"(frames), "r"(pas->gain), "r"(psrcp), "r"(pdstp)
                     : "$4", "$6", "$7", "memory");
#else
    {
        float *psrc = (float *)psrcp;
        float *pdst = (float *)pdstp;
        int i;

        for (i = 0; i < frames; i++) {
            pdst[i] = pas->gain * psrc[i];
        }
    }
#endif
    return frames;
}

int SFILTER_createAMPLF(AMPLFSTATE *pas) {
    pas->sfn.pfnnext = 0; pas->sfn.pfnnext2 = 0; pas->sfn.pfnprev1 = 0; pas->sfn.pfnprev2 = 0;
    pas->sfn.clean = 0;
    pas->sfn.restorefn = 0;
    pas->sfn.filterfn = SFILTER_amplf;
    pas->gain = 0.0f;
    return 0;
}

void SFILTER_modifyAMPLF(AMPLFSTATE *pas, int *pparams) {
    pas->gain = (float)pparams[0] * (1.0f / 256.0f);
}
