#include <snd/sfilter.h>

void *SNDMEMI_allocz(int size);
void SNDMEMI_free(void *pmem);

// total size: 0x24
typedef struct MIXERSTATE {
    SFILTERNODE sfn;   // offset 0x0, size 0x1C
    float *ptempbuf;   // offset 0x1C
    int tempbufframes; // offset 0x20
} MIXERSTATE;

int SFILTER_mixer(void *pstate, int frames, void *psrcp, void *pdstp, int requester) {
    MIXERSTATE *pms = (MIXERSTATE *)pstate;
    int totalbufsize;
    int retframes1;
    int retframes2;

    if (frames > pms->tempbufframes) {
        if (pms->ptempbuf) {
            SNDMEMI_free(pms->ptempbuf);
        }
        pms->tempbufframes = frames;
        totalbufsize = frames * sizeof(float);
        pms->ptempbuf = (float *)SNDMEMI_allocz(totalbufsize);
    }
    retframes1 = pms->sfn.pfnnext->filterfn(pms->sfn.pfnnext, frames, psrcp, pdstp, 1);
    if (retframes1 <= 0) {
        return retframes1;
    }
    retframes2 = pms->sfn.pfnnext2->filterfn(pms->sfn.pfnnext2, frames, psrcp, pms->ptempbuf, 2);
    if (retframes2 <= 0) {
        return retframes2;
    }
#ifdef EA_PLATFORM_PLAYSTATION2
    // snd 9.06.00-ps2: la suma de los dos buffers con la VU0, de 16 en 16 muestras (lqc2/vadd/sqc2 en el binario)
    asm __volatile__(".set noreorder\n"
                     "move $4, %0\n"
                     "sll $4, $4, 2\n"
                     "move $5, %1\n"
                     "move $6, %2\n"
                     "add $4, $4, $5\n"
                     "1:\n"
                     "lqc2 vf2, 0x0($5)\n"
                     "lqc2 vf3, 0x10($5)\n"
                     "lqc2 vf4, 0x20($5)\n"
                     "lqc2 vf5, 0x30($5)\n"
                     "lqc2 vf6, 0x0($6)\n"
                     "lqc2 vf7, 0x10($6)\n"
                     "lqc2 vf8, 0x20($6)\n"
                     "lqc2 vf9, 0x30($6)\n"
                     "addi $5, $5, 0x40\n"
                     "vadd.xyzw vf6, vf6, vf2\n"
                     "vadd.xyzw vf7, vf7, vf3\n"
                     "vadd.xyzw vf8, vf8, vf4\n"
                     "vadd.xyzw vf9, vf9, vf5\n"
                     "sqc2 vf6, 0x0($6)\n"
                     "sqc2 vf7, 0x10($6)\n"
                     "sqc2 vf8, 0x20($6)\n"
                     "sqc2 vf9, 0x30($6)\n"
                     "bne $4, $5, 1b\n"
                     "addi $6, $6, 0x40\n"
                     ".set reorder"
                     :
                     : "r"(frames), "r"(pms->ptempbuf), "r"(pdstp)
                     : "$4", "$5", "$6", "memory");
#else
    {
        float *pdst = (float *)pdstp;
        int i;

        for (i = 0; i < frames; i++) {
            pdst[i] = pdst[i] + pms->ptempbuf[i];
        }
    }
#endif
    return frames;
}

void SFILTER_mixerrestore(void *pstate) {
    MIXERSTATE *pms = (MIXERSTATE *)pstate;

    if (pms->ptempbuf) {
        SNDMEMI_free(pms->ptempbuf);
    }
}

int SFILTER_createMIX(MIXERSTATE *pms) {
    pms->sfn.pfnnext = 0; pms->sfn.pfnnext2 = 0; pms->sfn.pfnprev1 = 0; pms->sfn.pfnprev2 = 0;
    pms->sfn.clean = 0;
    pms->ptempbuf = 0;
    pms->sfn.restorefn = SFILTER_mixerrestore;
    pms->sfn.filterfn = SFILTER_mixer;
    pms->tempbufframes = 0;
    return 0;
}
