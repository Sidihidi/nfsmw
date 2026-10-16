#include "Speed/Indep/Libs/snd/9/source/library/cmn/sndcmn.h"
#include <snd/sfilter.h>

extern "C" {
void *memset(void *dst, int c, unsigned int bytes);
}

void *SNDMEMI_allocz(int size);
void SNDMEMI_free(void *pmem);
int SNDI_findmult16(int samplerate, int delaytime);

#ifdef EA_PLATFORM_PLAYSTATION2
// snd 9.06.00-ps2: el juego puede dar su propia memoria para el bufer del eco (las dos llamadas se hacen siempre).
void *(*AllocReverbBufferCB)(const char *name, int size) = 0;
bool (*FreeReverbBufferCB)(void *pmem) = 0;
#endif

// total size: 0x34; en PS2 (snd 9.06.00-ps2) 0x40: fblevels alineado a 16, en 0x30
typedef struct ECHOSTATE {
    SFILTERNODE sfn;             // offset 0x0, size 0x1C
    float *pdelaybuf;            // offset 0x1C
    float *punaligneddelaybuf;   // offset 0x20
    int sampleindex;             // offset 0x24
    int bufsamples;              // offset 0x28
#ifdef EA_PLATFORM_PLAYSTATION2
    float fblevels __attribute__((aligned(16))); // offset 0x30
#else
    float fblevels;              // offset 0x2C
#endif
    float fbneglevels;           // offset 0x30 (PS2: 0x34)
} ECHOSTATE;

#ifdef EA_PLATFORM_PLAYSTATION2
extern "C" void SNDCALL_setreverbCBs(void *(*AllocCB)(const char *name, int size), bool (*FreeCB)(void *pmem)) {
    AllocReverbBufferCB = AllocCB;
    FreeReverbBufferCB = FreeCB;
}

// En PS2 el eco va en la VU0, cuatro muestras por vuelta: fblevels y fbneglevels se cargan juntos en vf1 (por eso
// fblevels va alineado a 16). El asm es el del original (vf1-vf15 y los registros de trabajo del binario de la A124).
static inline void SNDI_echo(ECHOSTATE *pes, int frames, void *psrcp, void *pdstp) {
    float *ptaken;

    ptaken = pes->pdelaybuf + pes->sampleindex;
    asm __volatile__(".set noreorder\n"
                     "move $6, %2\n"
                     "move $4, %0\n"
                     "move $5, %1\n"
                     "sll $6, $6, 2\n"
                     "move $7, %3\n"
                     "move $8, %4\n"
                     "add $6, $6, $7\n"
                     "lqc2 vf1, 0x0($4)\n"
                     "1:\n"
                     "lqc2 vf4, 0x0($5)\n"
                     "lqc2 vf5, 0x10($5)\n"
                     "lqc2 vf6, 0x20($5)\n"
                     "lqc2 vf7, 0x30($5)\n"
                     "lqc2 vf8, 0x0($7)\n"
                     "lqc2 vf9, 0x10($7)\n"
                     "lqc2 vf10, 0x20($7)\n"
                     "lqc2 vf11, 0x30($7)\n"
                     "vmulx.xyzw vf12, vf4, vf1x\n"
                     "vmulx.xyzw vf13, vf5, vf1x\n"
                     "vmulx.xyzw vf14, vf6, vf1x\n"
                     "vmulx.xyzw vf15, vf7, vf1x\n"
                     "vadd.xyzw vf12, vf12, vf8\n"
                     "vadd.xyzw vf13, vf13, vf9\n"
                     "vadd.xyzw vf14, vf14, vf10\n"
                     "vadd.xyzw vf15, vf15, vf11\n"
                     "vmuly.xyzw vf8, vf12, vf1y\n"
                     "vmuly.xyzw vf9, vf13, vf1y\n"
                     "vmuly.xyzw vf10, vf14, vf1y\n"
                     "vmuly.xyzw vf11, vf15, vf1y\n"
                     "vadd.xyzw vf8, vf8, vf4\n"
                     "vadd.xyzw vf9, vf9, vf5\n"
                     "vadd.xyzw vf10, vf10, vf6\n"
                     "vadd.xyzw vf11, vf11, vf7\n"
                     "addi $7, $7, 0x40\n"
                     "addi $8, $8, 0x40\n"
                     "sqc2 vf12, 0x0($5)\n"
                     "sqc2 vf13, 0x10($5)\n"
                     "sqc2 vf14, 0x20($5)\n"
                     "sqc2 vf15, 0x30($5)\n"
                     "sqc2 vf8, -0x40($8)\n"
                     "sqc2 vf9, -0x30($8)\n"
                     "sqc2 vf10, -0x20($8)\n"
                     "sqc2 vf11, -0x10($8)\n"
                     "bne $6, $7, 1b\n"
                     "addi $5, $5, 0x40\n"
                     ".set reorder"
                     :
                     : "r"(&pes->fblevels), "r"(ptaken), "r"(frames), "r"(psrcp), "r"(pdstp)
                     : "$4", "$5", "$6", "$7", "$8");
}
#else
static inline void SNDI_echo(ECHOSTATE *pes, int frames, void *psrcp, void *pdstp) {
    float *ptaken;
    float fedforward[4];
    float *psrc;
    float *pdst;
    float *pdstend;

    ptaken = pes->pdelaybuf + pes->sampleindex;
    psrc = (float *)psrcp;
    pdst = (float *)pdstp;
    pdstend = pdst + frames;
    while (pdst < pdstend) {
        fedforward[0] = ptaken[0] * pes->fblevels + psrc[0] + 1e-20f;
        fedforward[1] = ptaken[1] * pes->fblevels + psrc[1] + 1e-20f;
        fedforward[2] = ptaken[2] * pes->fblevels + psrc[2] + 1e-20f;
        fedforward[3] = ptaken[3] * pes->fblevels + psrc[3] + 1e-20f;
        pdst[0] = fedforward[0] * pes->fbneglevels + ptaken[0];
        pdst[1] = fedforward[1] * pes->fbneglevels + ptaken[1];
        pdst[2] = fedforward[2] * pes->fbneglevels + ptaken[2];
        pdst[3] = fedforward[3] * pes->fbneglevels + ptaken[3];
        ptaken[0] = fedforward[0];
        ptaken[1] = fedforward[1];
        ptaken[2] = fedforward[2];
        ptaken[3] = fedforward[3];
        psrc += 4;
        pdst += 4;
        ptaken += 4;
    }
}
#endif

int SFILTER_echo(void *pstate, int frames, void *psrcp, void *pdstp, int requester) {
    ECHOSTATE *pes = (ECHOSTATE *)pstate;
    float *ptempsrc;
    float *ptempdst;
    int moduleframes;
    int temp;

    if (pes->sfn.pfnnext) {
        temp = pes->sfn.pfnnext->filterfn(pes->sfn.pfnnext, frames, pdstp, psrcp,
                                          pes->sfn.requester);
        if (temp <= 0) {
            return temp;
        }
    }
    ptempsrc = (float *)psrcp;
    ptempdst = (float *)pdstp;
    moduleframes = frames;
    if (moduleframes > 0) {
        do {
            if (pes->sampleindex >= pes->bufsamples) {
                pes->sampleindex = 0;
            }
            temp = pes->bufsamples - pes->sampleindex;
            if (temp > moduleframes) {
                temp = moduleframes;
            }
            SNDI_echo(pes, temp, ptempsrc, ptempdst);
            moduleframes -= temp;
            pes->sampleindex += temp;
            ptempsrc += temp;
            ptempdst += temp;
        } while (moduleframes > 0);
    }
    return frames;
}

void SFILTER_echorestore(void *pstate) {
    ECHOSTATE *pes = (ECHOSTATE *)pstate;

    if (pes->punaligneddelaybuf != 0) {
#ifdef EA_PLATFORM_PLAYSTATION2
        if (!FreeReverbBufferCB(pes->punaligneddelaybuf)) {
            SNDMEMI_free(pes->punaligneddelaybuf);
        }
#else
        SNDMEMI_free(pes->punaligneddelaybuf);
#endif
        pes->punaligneddelaybuf = 0;
        pes->pdelaybuf = 0;
    }
}

int SFILTER_createECHO(ECHOSTATE *pes) {
    pes->sfn.pfnnext = 0; pes->sfn.pfnnext2 = 0; pes->sfn.pfnprev1 = 0; pes->sfn.pfnprev2 = 0;
    pes->sfn.clean = 0;
    pes->punaligneddelaybuf = 0;
    pes->sfn.restorefn = SFILTER_echorestore;
    pes->sfn.filterfn = SFILTER_echo;
    pes->pdelaybuf = 0;
    return 0;
}

void SFILTER_modifyECHO(ECHOSTATE *pes, int *pparams) {
    int feedback;
    int samplerate;
    int delaytime;
    int delaybufsize;

    feedback = pparams[0] >> 1;
    samplerate = pparams[1] >> 8;
    delaytime = pparams[2] >> 8;
    SFILTER_echorestore(pes);
    pes->fblevels = (float)feedback * 0.007874016f;
    pes->fbneglevels = -pes->fblevels;
    pes->bufsamples = SNDI_findmult16(samplerate, delaytime);
    delaybufsize = pes->bufsamples * sizeof(float);
#ifdef EA_PLATFORM_PLAYSTATION2
    pes->punaligneddelaybuf = (float *)AllocReverbBufferCB("Echo Filter Delay Buffer", delaybufsize + 16);
    if (pes->punaligneddelaybuf == 0) {
        pes->punaligneddelaybuf = (float *)SNDMEMI_allocz(delaybufsize + 16);
    }
    pes->pdelaybuf = (float *)(((sndptruint)pes->punaligneddelaybuf + 15) >> 4 << 4);
#else
    pes->punaligneddelaybuf = (float *)SNDMEMI_allocz(delaybufsize + 16);
    pes->pdelaybuf = (float *)(((sndptruint)pes->punaligneddelaybuf + 15) & ~15);
#endif
    memset(pes->pdelaybuf, 0, delaybufsize);
    pes->sampleindex = 0;
}
