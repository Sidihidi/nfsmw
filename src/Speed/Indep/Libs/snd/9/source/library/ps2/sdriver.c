// El driver de sonido de PlayStation 2 de snd 9.06.00-ps2 (C:/packages/snd/9.06.00-ps2/source/library/ps2/sdriver.c
// en el .mdebug de la A124). Escrito desde el desensamblado de la A124 con su mapa de lineas, los stabs (locales con
// registro y cada estructura) y, para la logica comun de los SNDPLATFORM_*, el driver de GameCube (gc/snddrv.c).

#include <string.h>
#include <eeregs.h>
#include <libgraph.h>
#include <sifdev.h>

#include "sndps2.h"
#include "../mix/smixi.h"
#include "../extern/ftoi.h"

static char *snddrviopmodulepath = "host0:";

SNDDRV snddrv;
int (*sndpdtsquerymemsize)(int, int, int *, int *);
int (*sndpdtsinit)(int, int, void *);
int (*sndpdtsencode)(void **, unsigned char *, void *);
static sceSifCmdData cmdBuf[16] __attribute__((aligned(64)));
static tCmdHandlerData cmdData;

// Los registros de la VU0 que el hilo del driver guarda antes de mezclar y restaura despues
// total size: 0x208
struct VU0REGISTERS {
    unsigned char vf1_31[496]; // offset 0x0, size 0x1F0
    unsigned char acc[16];     // offset 0x1F0, size 0x10
    unsigned int macflags;     // offset 0x200, size 0x4
    unsigned int statflags;    // offset 0x204, size 0x4
};

// sqc2 y lqc2 piden 16 bytes de alineacion; el tipo mide 0x208 en los stabs, asi que se alinea la variable
VU0REGISTERS snddrvvu0regs __attribute__((aligned(16)));
char SNDDRVthreadstack[0x6000] __attribute__((aligned(16)));
int SNDDRVthreadid;
char sndtagbuf[0x2c00] __attribute__((aligned(128)));

// Deja en los flags MAC de la VU0 los que se guardaron: por cada componente (x, y, z, w) prepara dos factores cuyo
// producto da cero, desbordamiento, subdesbordamiento o signo, y los multiplica en la VU0.
void SNDSYSI_setmacflags(unsigned int macflags) {
    float setflagargs[2][4];
    float val;
    int i;

    for (i = 0; i < 4; i++) {
        if ((macflags & 0x8800) == 0) {
            if ((macflags & 0x8) == 0) {
                val = 1.0f;
            } else {
                val = 0.0f;
            }
        } else {
            if ((macflags & 0x8) == 0) {
                val = 1e30f;
            } else {
                val = 1e-30f;
            }
        }
        setflagargs[0][i] = val;
        if (macflags & 0x80) {
            val = -val;
        }
        setflagargs[1][i] = val;
        macflags <<= 1;
    }

    asm __volatile__("qmfc2.ni $8, vf1\n"
                     "qmfc2.ni $9, vf2\n"
                     "lqc2 vf1, 0x0(%0)\n"
                     "lqc2 vf2, 0x10(%0)\n"
                     "vmul.xyzw vf1, vf1, vf2\n"
                     "qmtc2.ni $8, vf1\n"
                     "qmtc2.ni $9, vf2\n"
                     "vnop\n"
                     :
                     : "r"(setflagargs)
                     : "$8", "$9", "memory");
}

void SNDSYSI_savevu0registers(VU0REGISTERS *pr) {
    asm __volatile__("move $8, %0\n"
                     "cfc2.i $9, $vi17\n"
                     "sqc2 vf1, 0x0($8)\n"
                     "sqc2 vf2, 0x10($8)\n"
                     "sw $9, 0x200($8)\n"
                     "sqc2 vf3, 0x20($8)\n"
                     "sqc2 vf4, 0x30($8)\n"
                     "ctc2 $9, $vi16\n"
                     "sqc2 vf5, 0x40($8)\n"
                     "sqc2 vf6, 0x50($8)\n"
                     "sw $9, 0x204($8)\n"
                     "sqc2 vf7, 0x60($8)\n"
                     "sqc2 vf8, 0x70($8)\n"
                     "vmaddx.xyzw vf1, vf0, vf0x\n"
                     "sqc2 vf9, 0x80($8)\n"
                     "sqc2 vf10, 0x90($8)\n"
                     "sqc2 vf11, 0xa0($8)\n"
                     "sqc2 vf12, 0xb0($8)\n"
                     "sqc2 vf13, 0xc0($8)\n"
                     "sqc2 vf14, 0xd0($8)\n"
                     "sqc2 vf15, 0xe0($8)\n"
                     "sqc2 vf16, 0xf0($8)\n"
                     "sqc2 vf17, 0x100($8)\n"
                     "sqc2 vf18, 0x110($8)\n"
                     "sqc2 vf19, 0x120($8)\n"
                     "sqc2 vf20, 0x130($8)\n"
                     "sqc2 vf21, 0x140($8)\n"
                     "sqc2 vf22, 0x150($8)\n"
                     "sqc2 vf23, 0x160($8)\n"
                     "sqc2 vf24, 0x170($8)\n"
                     "sqc2 vf25, 0x180($8)\n"
                     "sqc2 vf26, 0x190($8)\n"
                     "sqc2 vf27, 0x1a0($8)\n"
                     "sqc2 vf28, 0x1b0($8)\n"
                     "sqc2 vf29, 0x1c0($8)\n"
                     "sqc2 vf30, 0x1d0($8)\n"
                     "sqc2 vf31, 0x1e0($8)\n"
                     "sqc2 vf1, 0x1f0($8)\n"
                     :
                     : "r"(pr)
                     : "$8", "$9", "memory");
}

void SNDSYSI_restorevu0registers(VU0REGISTERS *pr) {
    asm __volatile__("move $8, %0\n"
                     "lqc2 vf1, 0x1f0($8)\n"
                     "vaddax.xyzw ACC, vf1, vf0x\n"
                     "lqc2 vf1, 0x0($8)\n"
                     "lqc2 vf2, 0x10($8)\n"
                     "lqc2 vf3, 0x20($8)\n"
                     "lqc2 vf4, 0x30($8)\n"
                     "lqc2 vf5, 0x40($8)\n"
                     "lqc2 vf6, 0x50($8)\n"
                     :
                     : "r"(pr)
                     : "$8", "memory");

    SNDSYSI_setmacflags(pr->macflags);

    asm __volatile__("move $8, %0\n"
                     "lqc2 vf7, 0x60($8)\n"
                     "lqc2 vf8, 0x70($8)\n"
                     "lqc2 vf9, 0x80($8)\n"
                     "lqc2 vf10, 0x90($8)\n"
                     "lqc2 vf11, 0xa0($8)\n"
                     "lqc2 vf12, 0xb0($8)\n"
                     "lw $9, 0x204($8)\n"
                     "lqc2 vf13, 0xc0($8)\n"
                     "lqc2 vf14, 0xd0($8)\n"
                     "ctc2 $9, $vi16\n"
                     "lqc2 vf15, 0xe0($8)\n"
                     "lqc2 vf16, 0xf0($8)\n"
                     "lqc2 vf17, 0x100($8)\n"
                     "lqc2 vf18, 0x110($8)\n"
                     "lqc2 vf19, 0x120($8)\n"
                     "lqc2 vf20, 0x130($8)\n"
                     "lqc2 vf21, 0x140($8)\n"
                     "lqc2 vf22, 0x150($8)\n"
                     "lqc2 vf23, 0x160($8)\n"
                     "lqc2 vf24, 0x170($8)\n"
                     "lqc2 vf25, 0x180($8)\n"
                     "lqc2 vf26, 0x190($8)\n"
                     "lqc2 vf27, 0x1a0($8)\n"
                     "lqc2 vf28, 0x1b0($8)\n"
                     "lqc2 vf29, 0x1c0($8)\n"
                     "lqc2 vf30, 0x1d0($8)\n"
                     "lqc2 vf31, 0x1e0($8)\n"
                     :
                     : "r"(pr)
                     : "$8", "$9", "memory");
}

// El mezclador usa el scratchpad (sndgs.sso.set.fastramaddr): se guarda con los registros de sus canales de DMA
// antes de mezclar y se restaura despues.
void SNDDRV_savescratchpad() {
    snddrv.SP_d8_madr = *D8_MADR;
    snddrv.SP_d8_sadr = *D8_SADR;
    snddrv.SP_d8_qwc = *D8_QWC;
    snddrv.SP_d9_madr = *D9_MADR;
    snddrv.SP_d9_tadr = *D9_TADR;
    snddrv.SP_d9_sadr = *D9_SADR;
    snddrv.SP_d9_qwc = *D9_QWC;

    SNDDMAI_FromScratchPad((unsigned int *)sndgs.sso.set.fastramaddr, (unsigned int *)snddrv.SP_buff,
                           snddrv.scratchpadsize >> 4, 0x10000);
}

void SNDDRV_restorescratchpad() {
    SNDDMAI_ToScratchPad((unsigned int *)snddrv.SP_buff, (unsigned int *)sndgs.sso.set.fastramaddr,
                         snddrv.scratchpadsize >> 4, 0x10000);
    *D8_MADR = snddrv.SP_d8_madr;
    *D8_SADR = snddrv.SP_d8_sadr;
    *D8_QWC = snddrv.SP_d8_qwc;
    *D9_MADR = snddrv.SP_d9_madr;
    *D9_TADR = snddrv.SP_d9_tadr;
    *D9_SADR = snddrv.SP_d9_sadr;
    *D9_QWC = snddrv.SP_d9_qwc;
}

void SNDDRV_rpc(int command, void *pdata, int datasize) {
    int waitmode;
    volatile int curtick; // espera activa: en la pila (stabs) y releida en cada vuelta

    waitmode = command == 2;

    if (datasize) {
        Snd::Util::MemCpy(snddrv.rpcsendbuf, pdata, datasize);
    }

    while (sceSifCheckStatRpc(&snddrv.rpccd.rpcd)) {
        for (curtick = 0; curtick < 10000; curtick++) {
        }
    }

    sceSifCallRpc(&snddrv.rpccd, command, waitmode, snddrv.rpcsendbuf, datasize, 0, 0, 0, 0);
}

void SNDDRV_mixvoicefree(int voice) {
    SNDVOICEI_free(voice + sndgs.sso.set.voicesspu + sndgs.sso.set.voicesiopcpu);
}

void SNDPLATFORM_getvoicerange(int playloc, int *minvoicerange, int *maxvoicerange) {
    if (playloc & SND_PLAYLOC_SPU) {
        *minvoicerange = 0;
        *maxvoicerange = sndgs.sso.set.voicesspu;
    } else if (playloc & SND_PLAYLOC_IOPCPU) {
        *minvoicerange = sndgs.sso.set.voicesspu;
        *maxvoicerange = sndgs.sso.set.voicesspu + sndgs.sso.set.voicesiopcpu;
    } else if (playloc & SND_PLAYLOC_MAINCPU) {
        *minvoicerange = sndgs.sso.set.voicesspu + sndgs.sso.set.voicesiopcpu;
        *maxvoicerange = sndgs.voicestotal;
    }
}

int SNDPLATFORM_outputcaps() {
    sndgs.sso.cap.voicesmaincpumax = 32;
    sndgs.sso.cap.voicesiopcpumax = 16;
    sndgs.sso.cap.voicesspumax = 48;

    sndgs.sso.cap.eax = 0;

    sndgs.sso.cap.outputratemin = 24000;
    sndgs.sso.cap.outputratemax = 48000;
    sndgs.sso.cap.outputchannelsmin = 1;
    sndgs.sso.cap.outputchannelsmax = 5;
    sndgs.sso.set.outputspdifmode = 1;

    sndgs.sso.cap.numrendermodes = 6;
    sndgs.sso.cap.rendermode[0] = 0x28;
    sndgs.sso.cap.rendermode[1] = 0x24;
    sndgs.sso.cap.rendermode[2] = 0x120;
    sndgs.sso.cap.rendermode[3] = 0x48;
    sndgs.sso.cap.rendermode[4] = 0x44;
    sndgs.sso.cap.rendermode[5] = 0x140;

    sndgs.sso.set.numrendermodes = 3;
    sndgs.sso.set.rendermode[0] = 0x28;
    sndgs.sso.set.rendermode[1] = 0x24;
    sndgs.sso.set.rendermode[2] = 0x120;

    sndgs.sso.set.useeax = 0;
    sndgs.sso.set.voicesmaincpu = 8;
    sndgs.sso.set.voicesiopcpu = 8;
    sndgs.sso.set.voicesspu = 48;
    sndgs.sso.set.maxstreams = 8;
    sndgs.sso.set.outputrate = 48000;
    sndgs.sso.set.outputrateiopcpu = 48000;
    sndgs.sso.set.outputchannels = 2;
    sndgs.sso.set.routemaincpufxtoiopcpu = 0;
    sndgs.sso.set.loadiopmodules = 1;
    sndgs.sso.set.iopmodulepath = snddrviopmodulepath;

    sndgs.sso.set.resamplequality = 25;
    sndgs.sso.set.iopcpuresamplequality = 25;

    return 0;
}

int SNDPLATFORM_outputset() {
    SNDSYSSET sss;

    sss = sndgs.sso.set;

    if (sndgs.installed) {
        sndgs.sso.set = sndgs.prevset;
        if (sndgs.prevset.outputspdifmode != sss.outputspdifmode) {
            return 0;
        }
    }

    if (sss.outputrate <= 24000) {
        sss.outputrate = 24000;
    } else if (sss.outputrate <= 36000) {
        sss.outputrate = 36000;
    } else {
        sss.outputrate = 48000;
    }

    if (sss.outputrateiopcpu <= 24000) {
        sss.outputrateiopcpu = 24000;
    } else if (sss.outputrateiopcpu <= 36000) {
        sss.outputrateiopcpu = 36000;
    } else {
        sss.outputrateiopcpu = 48000;
    }

    if (sss.outputspdifmode & 2) {
        sss.voicesspu = 24;
        if (sss.outputchannels < 4) {
            sss.outputchannels = 4;
        }
    }

    if (sss.outputspdifmode & 1) {
        if (sss.outputchannels > 2) {
            sss.outputchannels = 2;
        }
        sss.outputlfe = 0;
    }

    if (sss.dtsquality > 100) {
        sss.dtsquality = 100;
    }

    if (sss.outputchannels < sndgs.sso.cap.outputchannelsmin) {
        sss.outputchannels = sndgs.sso.cap.outputchannelsmin;
    } else if (sss.outputchannels > sndgs.sso.cap.outputchannelsmax) {
        sss.outputchannels = sndgs.sso.cap.outputchannelsmax;
    }

    if (sss.outputchannels == 5) {
        sss.outputlfe = 1;
    } else {
        sss.outputlfe = 0;
    }

    if (sss.voicesmaincpu > sndgs.sso.cap.voicesmaincpumax) {
        sss.voicesmaincpu = sndgs.sso.cap.voicesmaincpumax;
    }
    if (sss.voicesiopcpu > sndgs.sso.cap.voicesiopcpumax) {
        sss.voicesiopcpu = sndgs.sso.cap.voicesiopcpumax;
    }
    if (sss.voicesspu > sndgs.sso.cap.voicesspumax) {
        sss.voicesspu = sndgs.sso.cap.voicesspumax;
    }
    if (sss.maxstreams > 32) {
        sss.maxstreams = 32;
    }

    MIX_setresamplequality(sss.resamplequality);

    if (sndgs.sso.set.fastramaddr) {
        if ((unsigned int)sss.fastramaddr & 0xf) {
            sss.fastramaddr = (void *)((((unsigned int)sss.fastramaddr + 15) >> 4) << 4);
            sss.fastramsize -= 16;
        }
    }

    if (sndgs.installed) {
        sndgs.sso.set.resamplequality = sss.resamplequality;
        sndgs.sso.set.iopcpuresamplequality = sss.iopcpuresamplequality;
        snddrv.peetoiop->iopcpuresamplequality = sss.iopcpuresamplequality;
        return 0;
    }

    sndgs.voicestotal = sss.voicesmaincpu + sss.voicesiopcpu + sss.voicesspu;
    sndgs.sso.set = sss;

    return 0;
}

void SNDDRV_updatevoices() {
    int voice;
    int i;
    int j;

    for (i = 0; i < sndgs.sso.set.voicesspu; i++) {
        if (snddrv.peetoiop->sv[i].startaddr) {
            if (sndgs.chan[i].masterchan == -1) {
                if (snddrv.ioptoee.sv[i].startaddr != snddrv.peetoiop->sv[i].startaddr) {
                    snddrv.ioptoee.sv[i].curaddr = snddrv.peetoiop->sv[i].startaddr & 0x7fffff;
                }
                if (snddrv.ioptoee.sv[i].curaddr == 0) {
                    for (j = 0; j < sndgs.chan[i].numchan; j++) {
                        voice = sndgs.chan[i].voices[j];
                        snddrv.peetoiop->sv[voice].startaddr = 0;
                        SNDVOICEI_free(voice);
                        if (sndgs.sso.set.outputspdifmode == 2) {
                            snddrv.peetoiop->sv[voice + 24].startaddr = 0;
                        }
                    }
                }
            }
        }
    }

    for (i = 0; i < sndgs.sso.set.voicesiopcpu; i++) {
        if (snddrv.peetoiop->iv[i].startaddr) {
            if (sndgs.chan[i + sndgs.sso.set.voicesspu].masterchan == -1) {
                if (snddrv.ioptoee.iv[i].startaddr != snddrv.peetoiop->iv[i].startaddr) {
                    snddrv.ioptoee.iv[i].curframe = 0;
                }
                if (snddrv.ioptoee.iv[i].curframe == -1) {
                    for (j = 0; j < sndgs.chan[i + sndgs.sso.set.voicesspu].numchan; j++) {
                        voice = sndgs.chan[i + sndgs.sso.set.voicesspu].voices[j];
                        snddrv.peetoiop->iv[voice - sndgs.sso.set.voicesspu].startaddr = 0;
                        SNDVOICEI_free(voice);
                    }
                }
            }
        }
    }
}

void SNDDRV_iopinterrupt(void *p, void *q) {
    tCommandData *h = (tCommandData *)p;
    SemaParam sp;

    switch (h->cmd) {
    case 2:
        Snd::Dma::CompletionHandler(h->Hdr.opt);
        break;
    case 0:
        snddrv.sifdma.addr = h->Hdr.opt;
        iReferSemaStatus(snddrv.serversema, &sp);
        if (sp.currentCount < sp.maxCount) {
            iSignalSema(snddrv.serversema);
        }
        break;
    case 5:
        Snd::Dma::CompletionHandler(h->Hdr.opt);
        break;
    case 3:
        break;
    }
}

void SNDDRV_strcat(char *pdst, char *psrc) {
    while (*pdst) {
        pdst++;
    }
    while (*psrc) {
        *pdst = *psrc;
        pdst++;
        psrc++;
    }
    *pdst = 0;
}

void SNDDRV_resettagbuf() {
    SNDIOPTAGVOICECMD *psitvc;
    SNDIOPTAGMIXBUF *psitmb;
    SNDIOPTAGDTSBUF *psitdb;
    int eemixsamples;
    int i;

    snddrv.ptagbuf = (unsigned char *)((unsigned int)sndtagbuf | 0x20000000);
    memset(snddrv.ptagbuf, 0, sizeof(sndtagbuf));

    eemixsamples = (sndgs.sso.set.outputrate * 512) / 48000;

    snddrv.tagindex = 0;

    *(int *)&snddrv.ptagbuf[snddrv.tagindex] = 0;
    snddrv.tagindex += sizeof(int);
    snddrv.psith = (SNDIOPTAGHEADER *)&snddrv.ptagbuf[snddrv.tagindex];
    snddrv.tagindex += sizeof(SNDIOPTAGHEADER);

    *(int *)&snddrv.ptagbuf[snddrv.tagindex] = 3;
    snddrv.tagindex += sizeof(int);
    psitvc = (SNDIOPTAGVOICECMD *)&snddrv.ptagbuf[snddrv.tagindex];
    snddrv.tagindex += sizeof(SNDIOPTAGVOICECMD);
    snddrv.peetoiop = (SNDDRVEETOIOP *)&snddrv.ptagbuf[snddrv.tagindex];
    psitvc->datasize = sizeof(SNDDRVEETOIOP);
    snddrv.tagindex += sizeof(SNDDRVEETOIOP);

    while (snddrv.tagindex & 0xf) {
        snddrv.tagindex++;
        psitvc->datasize++;
    }

    if (sndgs.sso.set.routemaincpufxtoiopcpu) {
        *(int *)&snddrv.ptagbuf[snddrv.tagindex] = 2;
        snddrv.tagindex += sizeof(int) + sizeof(SNDIOPTAGFXBUF);
        MIX_setwetbuffer(&snddrv.ptagbuf[snddrv.tagindex]);
        snddrv.tagindex += eemixsamples * sizeof(float);
    }

    *(int *)&snddrv.ptagbuf[snddrv.tagindex] = 1;
    snddrv.tagindex += sizeof(int);
    psitmb = (SNDIOPTAGMIXBUF *)&snddrv.ptagbuf[snddrv.tagindex];
    snddrv.tagindex += sizeof(SNDIOPTAGMIXBUF);

    for (i = 0; i < sndgs.sso.set.outputchannels + sndgs.sso.set.outputlfe; i++) {
        snddrv.pmixbuf[i] = &snddrv.ptagbuf[snddrv.tagindex + i * eemixsamples * sizeof(short)];
    }

    psitmb->bufsize = eemixsamples * sizeof(short) * (sndgs.sso.set.outputchannels + sndgs.sso.set.outputlfe);

    if (sndgs.sso.set.outputchannels == 1) {
        snddrv.pmixbuf[1] = &snddrv.ptagbuf[snddrv.tagindex + eemixsamples * sizeof(short)];
        psitmb->bufsize += eemixsamples * sizeof(short);
    }

    snddrv.tagindex += psitmb->bufsize;

    if (sndgs.sso.set.outputspdifmode & 2) {
        *(int *)&snddrv.ptagbuf[snddrv.tagindex] = 4;
        snddrv.tagindex += sizeof(int);
        psitdb = (SNDIOPTAGDTSBUF *)&snddrv.ptagbuf[snddrv.tagindex]; // no se rellena: el original solo lee el indice
        snddrv.tagindex += sizeof(SNDIOPTAGDTSBUF);
        snddrv.pdtsbuf = &snddrv.ptagbuf[snddrv.tagindex];
        snddrv.tagindex += 2048;
    }

    *(int *)&snddrv.ptagbuf[snddrv.tagindex] = 5;
    snddrv.tagindex += sizeof(int);
    snddrv.psitf = (SNDIOPTAGFOOTER *)&snddrv.ptagbuf[snddrv.tagindex];
    snddrv.tagindex += sizeof(SNDIOPTAGFOOTER);

    *(int *)&snddrv.ptagbuf[snddrv.tagindex] = 6;
    snddrv.tagindex += 16;

    snddrv.psith->tagbufsize = snddrv.tagindex;

    snddrv.sifdma.data = (unsigned int)sndtagbuf;
    snddrv.sifdma.size = snddrv.tagindex;
    snddrv.sifdma.mode = 0;
}

int SNDDRV_vblankcb(int ca) {
    Snd::gVariableTimerTick++;
    ExitHandler();
    return 0;
}

void SNDDRV_thread(void *data) {
    for (;;) {
        WaitSema(snddrv.serversema);

        if (!snddrv.installed) {
            continue;
        }

        Snd::gMutexLockFn();

        if (sndgs.profileenter) {
            sndgs.profileenter();
        }

        SNDSYSI_timerservice(94);

        if (snddrv.vblhandlerid < 0) {
            Snd::gVariableTimerTick++;
        }

        SNDSYSI_variabletimerservice();

        Snd::Util::MemCpy(&snddrv.ioptoee, &snddrv.ioptoeedma, sizeof(SNDDRVIOPTOEE));

        SNDDRV_updatevoices();

        SNDSYSI_savevu0registers(&snddrvvu0regs);

        if (snddrv.scratchpadsize) {
            SNDDRV_savescratchpad();
        }

        MIX_audio(snddrv.pmixbuf, snddrv.framestomix);

        if (sndgs.sso.set.outputspdifmode & 2) {
            sndpdtsencode(snddrv.pdtsmixbuf, snddrv.pdtsbuf, snddrv.pdtstempbuf);
        } else if (sndgs.sso.set.outputchannels == 1) {
            Snd::Util::MemCpy(snddrv.pmixbuf[1], snddrv.pmixbuf[0], snddrv.framestomix * sizeof(short));
        }

        if (snddrv.scratchpadsize) {
            SNDDRV_restorescratchpad();
        }

        SNDSYSI_restorevu0registers(&snddrvvu0regs);

        SNDPKTPLAYI_flushcallbackdata();

        snddrv.psith->audiotick = sndgs.servertick;
        snddrv.psitf->audiotick = sndgs.servertick;

        FlushCache(WRITEBACK_DCACHE);
        sceSifSetDma(&snddrv.sifdma, 1);

        if (sndgs.profileleave) {
            sndgs.profileleave();
        }

        Snd::gMutexUnlockFn();
    }
}

int SNDPLATFORM_init(StartMode startMode) {
    char irxpath[128];
    static ThreadParam tp;
    static SemaParam sp;
    static SNDIOPRPCINIT sndioprpcinit;
    MIXCONFIG mc;
    unsigned int i;
    int size;
    int dtsbufpermsize;
    int dtsbuftempsize;
    static int initedonce = 0;

    snddrv.scratchpadsize = 0;

    SNDSYS_entercritical();

    if (startMode == COLD) {
        snddrv.mm.pr = (SNDDRVMEMREC *)SNDMEMI_allocz(sndgs.sso.set.maxbanks * sizeof(SNDDRVMEMREC));
    }

    if (sndgs.sso.set.outputrate == 24000) {
        snddrv.framestomix = 256;
    } else if (sndgs.sso.set.outputrate == 36000) {
        snddrv.framestomix = 384;
    } else {
        snddrv.framestomix = 512;
    }

    SNDDRV_resettagbuf();

    snddrv.peetoiop->drybusbits = 0xffffffffffffL;

    if (sndgs.sso.set.outputspdifmode & 2) {
        if (sndgs.sso.set.outputchannels == 5) {
            sndpdtsquerymemsize(2, sndgs.sso.set.dtsquality, &dtsbufpermsize, &dtsbuftempsize);
        } else {
            sndpdtsquerymemsize(1, sndgs.sso.set.dtsquality, &dtsbufpermsize, &dtsbuftempsize);
        }

        if (sndgs.sso.set.fastramaddr && dtsbuftempsize < sndgs.sso.set.fastramsize) {
            snddrv.pdtspermbuf = SNDMEMI_allocz(dtsbufpermsize);
            snddrv.pdtstempbuf = sndgs.sso.set.fastramaddr;
            snddrv.scratchpadsize = dtsbuftempsize;
        } else {
            snddrv.pdtspermbuf = SNDMEMI_allocz(dtsbufpermsize + dtsbuftempsize);
            snddrv.pdtstempbuf = (char *)snddrv.pdtspermbuf + dtsbufpermsize;
        }

        if (sndgs.sso.set.outputchannels == 5) {
            sndpdtsinit(2, sndgs.sso.set.dtsquality, snddrv.pdtspermbuf);
        } else {
            sndpdtsinit(1, sndgs.sso.set.dtsquality, snddrv.pdtspermbuf);
        }

        size = (sndgs.sso.set.outputchannels + sndgs.sso.set.outputlfe) * 1024 + 64;
        snddrv.palloceddtsbuf = (unsigned char *)SNDMEMI_allocz(size);
        memset(snddrv.palloceddtsbuf, 0, size);
        snddrv.porigdtsmixbuf = (unsigned char *)((((unsigned int)snddrv.palloceddtsbuf + 63) >> 6) << 6);

        if (sndgs.sso.set.outputchannels == 4) {
            snddrv.pdtsmixbuf[0] = snddrv.porigdtsmixbuf + 3072;
            snddrv.pdtsmixbuf[1] = snddrv.porigdtsmixbuf;
            snddrv.pdtsmixbuf[2] = snddrv.porigdtsmixbuf + 2048;
            snddrv.pdtsmixbuf[3] = snddrv.porigdtsmixbuf + 1024;
        } else {
            snddrv.pdtsmixbuf[0] = snddrv.porigdtsmixbuf;
            snddrv.pdtsmixbuf[1] = snddrv.porigdtsmixbuf + 4096;
            snddrv.pdtsmixbuf[2] = snddrv.porigdtsmixbuf + 1024;
            snddrv.pdtsmixbuf[3] = snddrv.porigdtsmixbuf + 3072;
            snddrv.pdtsmixbuf[4] = snddrv.porigdtsmixbuf + 2048;
            snddrv.pdtsmixbuf[5] = snddrv.porigdtsmixbuf + 5120;
        }
    }

    snddrv.peetoiop->effectvolleft = 0;
    snddrv.peetoiop->effectvolright = 0;
    snddrv.peetoiop->effectOutputLevelIop = 127;
    snddrv.peetoiop->iopcpuresamplequality = sndgs.sso.set.iopcpuresamplequality;

    if (startMode == COLD) {
        snddrv.mm.startaddr = 321;
        snddrv.mm.endaddr = 0x8000;
    }

    SNDSYS_leavecritical();

    sp.maxCount = 3;
    sp.initCount = 0;
    snddrv.serversema = CreateSema(&sp);

    if (!initedonce) {
        sceSifInitRpc(0);
        sceSifInitIopHeap();
    }

    if (sndgs.sso.set.loadiopmodules) {
        if (!initedonce) {
            irxpath[0] = 0;
            SNDDRV_strcat(irxpath, sndgs.sso.set.iopmodulepath);
            SNDDRV_strcat(irxpath, "libsd.irx");
            if (sceSifLoadModule(irxpath, 0, 0) < 0) {
                if (sceSifLoadModule("host0:/usr/local/sce/iop/modules/libsd.irx", 0, 0) < 0) {
                    return -1;
                }
            }

            irxpath[0] = 0;
            SNDDRV_strcat(irxpath, sndgs.sso.set.iopmodulepath);
            SNDDRV_strcat(irxpath, "snddrv.irx");
            if (sceSifLoadModule(irxpath, 0, 0) < 0) {
                if (sceSifLoadModule("host0:snddrv.irx", 0, 0) < 0) {
                    return -1;
                }
            }
        }
    }

    tp.entry = SNDDRV_thread;
    tp.stack = SNDDRVthreadstack;
    tp.stackSize = sizeof(SNDDRVthreadstack);
    tp.initPriority = 6;
    tp.gpReg = &_gp;
    tp.option = 0;
    SNDDRVthreadid = CreateThread(&tp);
    StartThread(SNDDRVthreadid, 0);

    ChangeThreadPriority(GetThreadId(), 100);

    SNDSYS_entercritical();
    sndmix.fxadd = 0;
    sndmix.fxaddiop = 0;
    SNDDRV_setspufxmem(0);
    snddrv.peetoiop->effecttype = 0;
    snddrv.peetoiop->effectdelay = 0;
    snddrv.peetoiop->effectfeedback = 0;
    SNDSYS_leavecritical();

    for (i = 0; i < sndgs.sso.set.numrendermodes; i++) {
        if (sndgs.sso.set.rendermode[i] & 4) {
            mc.inputchannels = sndgs.sso.set.voicesmaincpu;
            mc.outputchannels = sndgs.sso.set.outputchannels + sndgs.sso.set.outputlfe;
            mc.outputrate = sndgs.sso.set.outputrate;
            mc.chanovercallback = SNDDRV_mixvoicefree;
            mc.outputinterleaved = 0;
            MIX_create(&mc);
            break;
        }
    }

    if (sndgs.sso.set.fastramaddr) {
        if (sndgs.sso.set.fastramsize - SNDMIXfastramfree > snddrv.scratchpadsize) {
            snddrv.scratchpadsize = sndgs.sso.set.fastramsize - SNDMIXfastramfree;
        }
    }

    if (snddrv.scratchpadsize) {
        SNDSYS_entercritical();
        snddrv.SP_buff = SNDMEMI_allocz(snddrv.scratchpadsize + 16);
        SNDSYS_leavecritical();
    }

    if (!initedonce) {
        DI();
        sceSifSetCmdBuffer(cmdBuf, 16);
        sceSifAddCmdHandler(1, SNDDRV_iopinterrupt, &cmdData);
        EI();

        do {
            if (sceSifBindRpc(&snddrv.rpccd, 0x534e44, 0) < 0) { // 'SND'
                return -1;
            }
        } while (snddrv.rpccd.serve == 0);
    }

    // el periodo del temporizador en milisegundos: el de la vsync con -1 (campo NTSC o PAL); si no, el de una mezcla
    // de 512 muestras a 48 kHz (93,75 por segundo)
    if (sndgs.sso.set.updateperiod == -1.0f) {
        switch (sceGsGetGParam()->sceGsOutMode) {
        case SCE_GS_NTSC:
        case 0x50: // SCE_GS_DTV480P
            Snd::gVariableTimerPeriod = 1000.0f * (1.0f / 59.94f);
            break;
        default:
            Snd::gVariableTimerPeriod = 1000.0f * (1.0f / 50.0f);
            break;
        }

        DisableIntc(INTC_VBLANK_S);
        snddrv.vblhandlerid = AddIntcHandler(INTC_VBLANK_S, SNDDRV_vblankcb, 0);
        EnableIntc(INTC_VBLANK_S);
    } else {
        Snd::gVariableTimerPeriod = 1000.0f * (1.0f / 93.75f);
        snddrv.vblhandlerid = -1;
    }

    sndioprpcinit.ver = 0x90600;
    sndioprpcinit.ioptoeeaddr = (unsigned int)&snddrv.ioptoeedma;
    sndioprpcinit.dtspcmaddr = (unsigned int)snddrv.porigdtsmixbuf;
    sndioprpcinit.outputchannels = sndgs.sso.set.outputchannels;
    sndioprpcinit.outputlfe = sndgs.sso.set.outputlfe;
    sndioprpcinit.voicesiopcpu = sndgs.sso.set.voicesiopcpu;
    sndioprpcinit.outputratemaincpu = sndgs.sso.set.outputrate;
    sndioprpcinit.outputrateiopcpu = sndgs.sso.set.outputrateiopcpu;

    if (sndgs.sso.set.outputchannels >= 4) {
        sndioprpcinit.routespufxtoiopcpu = sndgs.sso.set.routespufxtoiopcpu;
    } else {
        sndioprpcinit.routespufxtoiopcpu = 0;
    }

    SNDDRV_rpc(0, &sndioprpcinit, sizeof(sndioprpcinit));

    initedonce = 1;

    snddrv.installed = 1;

    if (Snd::Dma::Init() < 0) {
        return -1;
    }
    return 0;
}

int SNDPLATFORM_restore(StartMode startMode) {
    ThreadParam info;
    unsigned int settlingtick;

    Snd::Dma::Restore();

    while (sndgs.incritical) {
        SNDSYS_leavecritical();
    }

    settlingtick = sndgs.audiotick + 10;

    while (sndgs.audiotick < settlingtick) {
        SNDSYS_service();
    }

    if (snddrv.vblhandlerid >= 0) {
        DisableIntc(INTC_VBLANK_S);
        if (RemoveIntcHandler(INTC_VBLANK_S, snddrv.vblhandlerid) > 0) {
            EnableIntc(INTC_VBLANK_S);
        }
    }

    SNDDRV_rpc(1, 0, 0);

    ReferThreadStatus(SNDDRVthreadid, &info);

    if (info.status == THS_DORMANT) {
        DeleteThread(SNDDRVthreadid);
    } else {
        TerminateThread(SNDDRVthreadid);
        DeleteThread(SNDDRVthreadid);
    }

    DeleteSema(snddrv.serversema);

    MIX_setwetbuffer(0);

    snddrv.installed = 0;

    SNDSYS_entercritical();

    if (snddrv.palloceddtsbuf) {
        SNDMEMI_free(snddrv.palloceddtsbuf);
        snddrv.palloceddtsbuf = 0;
        snddrv.porigdtsmixbuf = 0;
    }

    if (snddrv.pdtspermbuf) {
        SNDMEMI_free(snddrv.pdtspermbuf);
        snddrv.pdtspermbuf = 0;
    }

    if (snddrv.SP_buff) {
        SNDMEMI_free(snddrv.SP_buff);
        snddrv.SP_buff = 0;
    }

    MIX_destroy();

    if (startMode == COLD) {
        SNDMEMI_free(snddrv.mm.pr);
    }

    SNDSYS_leavecritical();

    return 0;
}

void iSNDserve() {
}

int SNDPLATFORM_stop(int voice) {
    CHANPUB *ppubv;
    int i;

    ppubv = &sndgs.chan[voice];

    if (ppubv->rendermode & SND_PLAYLOC_SPU) {
        for (i = 0; i < ppubv->numchan; i++) {
            snddrv.peetoiop->sv[ppubv->voices[i]].startaddr = 0;
            SNDVOICEI_free(ppubv->voices[i]);
            if (sndgs.sso.set.outputspdifmode == 2) {
                snddrv.peetoiop->sv[ppubv->voices[i] + 24].startaddr = 0;
            }
        }
    } else if (ppubv->rendermode & SND_PLAYLOC_IOPCPU) {
        for (i = 0; i < ppubv->numchan; i++) {
            snddrv.peetoiop->iv[ppubv->voices[i] - sndgs.sso.set.voicesspu].startaddr = 0;
            SNDVOICEI_free(ppubv->voices[i]);
        }
    } else {
        for (i = 0; i < ppubv->numchan; i++) {
            MIX_stop(ppubv->voices[i] - (sndgs.sso.set.voicesspu + sndgs.sso.set.voicesiopcpu));
            SNDVOICEI_free(ppubv->voices[i]);
        }
    }

    return 0;
}

int SNDPLATFORM_setpitch(int voice) {
    unsigned int pitch;
    CHANPUB *ppubv;
    int i;

    ppubv = &sndgs.chan[voice];

    if (ppubv->rendermode & SND_PLAYLOC_SPU) {
        pitch = (ppubv->samplerate << 12) / 48000;
        pitch = (pitch * ppubv->finalpitch) >> 12;
        if (pitch > 0x3fff) {
            pitch = 0x3fff;
        }
        for (i = 0; i < ppubv->numchan; i++) {
            snddrv.peetoiop->sv[ppubv->voices[i]].pitch = pitch;
            if (sndgs.sso.set.outputspdifmode == 2) {
                snddrv.peetoiop->sv[ppubv->voices[i] + 24].pitch = pitch;
            }
        }
    } else if (ppubv->rendermode & SND_PLAYLOC_IOPCPU) {
        pitch = (ppubv->samplerate << 12) / sndgs.sso.set.outputrateiopcpu;
        pitch = (pitch * ppubv->finalpitch) >> 12;
        for (i = 0; i < ppubv->numchan; i++) {
            snddrv.peetoiop->iv[ppubv->voices[i] - sndgs.sso.set.voicesspu].pitch = pitch;
        }
    } else {
        pitch = (ppubv->samplerate << 12) / sndgs.sso.set.outputrate;
        pitch = (pitch * ppubv->finalpitch) >> 8;
        for (i = 0; i < ppubv->numchan; i++) {
            MIX_setpitch(ppubv->voices[i] - (sndgs.sso.set.voicesspu + sndgs.sso.set.voicesiopcpu), pitch);
        }
    }

    return 0;
}

int SNDPLATFORM_setfxlevel(int voice, int bus) {
    CHANPUB *ppubv;
    int i;
    int wetgain;

    ppubv = &sndgs.chan[voice];

    if (ppubv->rendermode & SND_PLAYLOC_SPU) {
        for (i = 0; i < ppubv->numchan; i++) {
            if (ppubv->pFxVolume[bus].fxLevel != 0.0f) {
                snddrv.peetoiop->wetbusbits |= (SNDINT64)1 << ppubv->voices[i];
            } else {
                snddrv.peetoiop->wetbusbits &= ~((SNDINT64)1 << ppubv->voices[i]);
            }
            if (sndgs.sso.set.outputspdifmode == 2) {
                if (ppubv->pFxVolume[bus].fxLevel != 0.0f) {
                    snddrv.peetoiop->wetbusbits |= (SNDINT64)1 << (ppubv->voices[i] + 24);
                } else {
                    snddrv.peetoiop->wetbusbits &= ~((SNDINT64)1 << (ppubv->voices[i] + 24));
                }
            }
        }
    } else if (ppubv->rendermode & SND_PLAYLOC_IOPCPU) {
        wetgain = ftoi(ppubv->finalvol * ppubv->pFxVolume[bus].fxLevel * 127.0f);
        for (i = 0; i < ppubv->numchan; i++) {
            snddrv.peetoiop->iv[ppubv->voices[i] - sndgs.sso.set.voicesspu].wetbusgain[bus] = wetgain;
        }
    } else {
        SNDMIX_setwetgain(voice - (sndgs.sso.set.voicesspu + sndgs.sso.set.voicesiopcpu), bus,
                          ppubv->finalvol * ppubv->pFxVolume[bus].fxLevel);
    }

    return 0;
}

void Snd::Hal::SetVolInternal(int voice) {
    SNDDRVVOICE *pHalVoice;
    CHANPUB *ppubv;
    float gain;
    int i;

    ppubv = &sndgs.chan[voice];
    pHalVoice = &snddrv.voice[voice];

    if (ppubv->rendermode & SND_PLAYLOC_SPU) {
        SNDDRVEETOSPUVOICE *pSpuVoice;

        pSpuVoice = &snddrv.peetoiop->sv[voice];
        gain = ppubv->finalvol * 127.0f;
        if (sndgs.sso.set.outputspdifmode != 2) {
            pSpuVoice->vol[0] = ftoi(pHalVoice->balances[1] * gain);
            pSpuVoice->vol[1] = ftoi(pHalVoice->balances[0] * gain);
        } else if (sndgs.sso.set.outputchannels == 4) {
            pSpuVoice->vol[0] = ftoi(pHalVoice->balances[3] * gain);
            pSpuVoice->vol[1] = ftoi(pHalVoice->balances[0] * gain);
            pSpuVoice = &snddrv.peetoiop->sv[voice + 24];
            pSpuVoice->vol[0] = ftoi(pHalVoice->balances[2] * gain);
            pSpuVoice->vol[1] = ftoi(pHalVoice->balances[1] * gain);
        } else {
            pSpuVoice->vol[0] = ftoi(pHalVoice->balances[4] * gain);
            pSpuVoice->vol[1] = ftoi(pHalVoice->balances[1] * gain);
            pSpuVoice = &snddrv.peetoiop->sv[voice + 24];
            pSpuVoice->vol[0] = ftoi(pHalVoice->balances[3] * gain);
            pSpuVoice->vol[1] = ftoi(pHalVoice->balances[2] * gain);
        }
    } else if (ppubv->rendermode & SND_PLAYLOC_IOPCPU) {
        SNDDRVEETOIOPVOICE *pIopVoice;

        pIopVoice = &snddrv.peetoiop->iv[voice - sndgs.sso.set.voicesspu];
        gain = ppubv->finalvol * ppubv->drylevel;
        for (i = 0; i < gTotalOutputChannels; i++) {
            pIopVoice->drybusgain[i] = ftoi(pHalVoice->balances[i] * gain);
        }
    } else {
        int mixerVoice;

        mixerVoice = voice - (sndgs.sso.set.voicesspu + sndgs.sso.set.voicesiopcpu);
        gain = ppubv->finalvol * ppubv->drylevel * (1.0f / 127.0f);
        for (i = 0; i < gTotalOutputChannels; i++) {
            SNDMIX_setdrygain(mixerVoice, i, pHalVoice->balances[i] * gain);
        }
    }
}

void Snd::Hal::SetDry(int voice) {
    CHANPUB *ppubv;

    ppubv = &sndgs.chan[voice];

    if (ppubv->rendermode & SND_PLAYLOC_SPU) {
        if (ppubv->drylevel) {
            snddrv.peetoiop->drybusbits |= (SNDINT64)1 << voice;
        } else {
            snddrv.peetoiop->drybusbits &= ~((SNDINT64)1 << voice);
        }
        if (sndgs.sso.set.outputspdifmode == 2) {
            if (ppubv->drylevel) {
                snddrv.peetoiop->drybusbits |= (SNDINT64)1 << (voice + 24);
            } else {
                snddrv.peetoiop->drybusbits &= ~((SNDINT64)1 << (voice + 24));
            }
        }
    } else {
        SetVol(voice);
    }
}

int SNDPLATFORM_timemult(int voice, int timemult) {
    CHANPUB *ppubv;
    int i;

    ppubv = &sndgs.chan[voice];
    if (ppubv->rendermode & SND_PLAYLOC_MAINCPU) {
        for (i = 0; i < ppubv->numchan; i++) {
            MIX_settimemult(ppubv->voices[i] - (sndgs.sso.set.voicesspu + sndgs.sso.set.voicesiopcpu), timemult);
        }
    }
    return 0;
}

void SNDPLATFORM_lowpass(int voice, int cutofffreq) {
    CHANPUB *ppubv;
    float leakage;
    int i;

    ppubv = &sndgs.chan[voice];
    if (ppubv->rendermode & SND_PLAYLOC_MAINCPU) {
        leakage = cutofffreq * (1.0f / ((float)sndgs.sso.set.outputrate * 0.5f));
        for (i = 0; i < ppubv->numchan; i++) {
            MIX_setlowpass(ppubv->voices[i] - (sndgs.sso.set.voicesspu + sndgs.sso.set.voicesiopcpu), leakage);
        }
    }
}

void SNDPLATFORM_highpass(int voice, int cutofffreq) {
    CHANPUB *ppubv;
    int i;

    ppubv = &sndgs.chan[voice];
    if (ppubv->rendermode & SND_PLAYLOC_MAINCPU) {
        for (i = 0; i < ppubv->numchan; i++) {
            MIX_sethighpass(ppubv->voices[i] - (sndgs.sso.set.voicesspu + sndgs.sso.set.voicesiopcpu), cutofffreq);
        }
    }
}

void SNDI_mutexalloc() {
    static SemaParam sp;

    sp.maxCount = 20;
    sp.initCount = 1;
    snddrv.mutexowner = -1;
    snddrv.mutexlocks = 0;
    snddrv.mutex = CreateSema(&sp);
}

void SNDI_mutexfree() {
    DeleteSema(snddrv.mutex);
}

void SNDI_mutexlock() {
    int tid;

    tid = GetThreadId();

    if (snddrv.mutexowner == tid) {
        snddrv.mutexlocks++;
        return;
    }
    WaitSema(snddrv.mutex);
    snddrv.mutexowner = tid;
    snddrv.mutexlocks++;
}

void SNDI_mutexunlock() {
    SemaParam sp;
    int tid;

    tid = GetThreadId();

    if (snddrv.mutexowner == tid) {
        snddrv.mutexlocks--;
        if (snddrv.mutexlocks == 0) {
            snddrv.mutexowner = -1;
        }
    }

    if (snddrv.mutexlocks <= 0) {
        ReferSemaStatus(snddrv.mutex, &sp);
        if (sp.currentCount < sp.maxCount) {
            SignalSema(snddrv.mutex);
        }
    }
}

int SNDPLATFORM_ModeSetUp(SNDSYSOPTS *pSndSysOpts, Snd::OutputMode outputmode) {
    pSndSysOpts->set.numrendermodes = 0;

    if (outputmode == Snd::OUTPUTMODE_PROLOGIC2) {
        pSndSysOpts->set.rendermode[0] = 0x44;
        pSndSysOpts->set.numrendermodes = 1;
    }

    pSndSysOpts->set.rendermode[pSndSysOpts->set.numrendermodes] = 0x24;
    pSndSysOpts->set.numrendermodes++;

    if (pSndSysOpts->set.voicesiopcpu) {
        if (outputmode == Snd::OUTPUTMODE_PROLOGIC2) {
            pSndSysOpts->set.rendermode[pSndSysOpts->set.numrendermodes] = 0x140;
            pSndSysOpts->set.numrendermodes++;
        }
        pSndSysOpts->set.rendermode[pSndSysOpts->set.numrendermodes] = 0x120;
        pSndSysOpts->set.numrendermodes++;
    }

    if (outputmode == Snd::OUTPUTMODE_PROLOGIC2) {
        pSndSysOpts->set.rendermode[pSndSysOpts->set.numrendermodes] = 0x48;
        pSndSysOpts->set.numrendermodes++;
    }

    pSndSysOpts->set.rendermode[pSndSysOpts->set.numrendermodes] = 0x28;
    pSndSysOpts->set.numrendermodes++;

    pSndSysOpts->set.outputlfe = 0;
    pSndSysOpts->set.outputspdifmode = 1;

    switch (outputmode) {
    case Snd::OUTPUTMODE_MONO:
        pSndSysOpts->set.outputchannels = 1;
        break;
    case Snd::OUTPUTMODE_DTS4POINT0:
        pSndSysOpts->set.outputchannels = 4;
        pSndSysOpts->set.outputspdifmode = 2;
        break;
    case Snd::OUTPUTMODE_DTS5POINT1:
        pSndSysOpts->set.outputchannels = 5;
        pSndSysOpts->set.outputspdifmode = 2;
        break;
    case Snd::OUTPUTMODE_STEREO:
    case Snd::OUTPUTMODE_PROLOGIC2:
        pSndSysOpts->set.outputchannels = 2;
        break;
    }

    if (pSndSysOpts->set.outputrate != pSndSysOpts->set.outputrateiopcpu) {
        if (pSndSysOpts->set.outputrate != 36000 || pSndSysOpts->set.outputrateiopcpu != 24000) {
            pSndSysOpts->set.outputrateiopcpu = pSndSysOpts->set.outputrate;
        }
    }

    return 0;
}
