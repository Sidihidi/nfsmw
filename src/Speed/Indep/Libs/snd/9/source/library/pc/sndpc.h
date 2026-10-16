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

// La voz del driver de PC: SNDPLATFORM_init reserva sndgs.voicestotal de 0x88 B (0x820CCF). Los campos no se conocen
// todavia (los usan las funciones de 0x81EF3E-0x81F314, sin escribir).
typedef struct SNDDRVVOICE {
    unsigned char unknown[0x88];
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
