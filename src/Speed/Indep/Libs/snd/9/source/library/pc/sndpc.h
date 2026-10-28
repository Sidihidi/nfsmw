#ifndef SND_PC_SNDPC_H
#define SND_PC_SNDPC_H

// Plataforma de PC de snd (DirectSound). La fuente original de PC no esta: lo que hay aqui sale de speed.exe.

#ifndef EA_PLATFORM_WIN32
#error "sndpc.h solo es de PC"
#endif

#include "Speed/Indep/Libs/snd/9/source/library/cmn/sndcmn.h"

enum StartMode {
    COLD = 0,
    WARM = 1,
};

// La voz del driver de PC: SNDPLATFORM_init reserva sndgs.voicestotal de 0x88 B (0x820CCF). Lo que se sabe sale de
// pc/snddrv.c (0x81EF3E-0x81F314): los balances como en GameCube (Snd::Hal::SetPan) y las funciones del filtro de la
// voz, que reciben un estado (state o uno de pstate). Los nombres de los campos son nuestros.
typedef struct SNDDRVVOICE {
    float balances[6];                            // offset 0x0, size 0x18
    unsigned char state[0x38];                    // offset 0x18, size 0x38
    void *pstate[7];                              // offset 0x50, size 0x1C
    int (*pfnstop)(void *pstate);                 // offset 0x6C, size 0x4 (SNDPLATFORM_stop)
    void (*pfntimemult)(void *pstate, int value); // offset 0x70, size 0x4 (SNDPLATFORM_timemult)
    void (*pfn74)(void *pstate, int value);       // offset 0x74, size 0x4
    void (*pfnlowpass)(void *pstate, int value);  // offset 0x78, size 0x4 (SNDPLATFORM_lowpass)
    void (*pfnhighpass)(void *pstate, int value); // offset 0x7C, size 0x4 (SNDPLATFORM_highpass)
    void (*pfn80)(void *pstate, int value);       // offset 0x80, size 0x4
    int unknown84;                                // offset 0x84, size 0x4
} SNDDRVVOICE;

// El estado del driver de PC (0x9C2000 en speed.exe). chanmodes lo rellena DSOUND_getcaps y se copia entero (9 B) en
// 0x81EC4D; updateperiod es el periodo del temporizador variable, como en GameCube.
typedef struct SNDDRVCHANMODES {
    unsigned char modes[9];
} SNDDRVCHANMODES;

typedef struct SNDDRVSTATE {
    SNDDRVCHANMODES chanmodes; // offset 0x0, size 0x9
    float updateperiod;        // offset 0xC, size 0x4
    char threadactive;         // offset 0x10, size 0x1
    char threadrun;            // offset 0x11, size 0x1
    char exithandlerset;       // offset 0x12, size 0x1
    SNDDRVVOICE *pvoice;       // offset 0x14, size 0x4
} SNDDRVSTATE;

extern SNDDRVSTATE snddrv;

int SNDPLATFORM_init(StartMode startMode);
int SNDPLATFORM_restore(StartMode startMode);
int SNDPLATFORM_getlatency();

int DSOUND_getcaps(unsigned char *pmaxchannels, unsigned char *pchanmodes);
int DSOUND_captureopen(unsigned int blockframes, int bits, int rate);
int DSOUND_captureread(void *pdst);
int DSOUND_captureclose();

#endif
