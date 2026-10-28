// snd de PC: el driver de plataforma (speed.exe 0x81EBF9-0x81F337), el pc/ que el arbol no tenia, en el orden del exe.
// La fuente de PC no esta. Los nombres SNDPLATFORM_* y Snd::Hal::* son los que pide el codigo comun en el sitio de la
// llamada (ssysinit.c, ssys.cpp, sbplay.c, saemsbnk...: el mismo llamante que en GameCube); las funciones que ninguna
// fuente nombra llevan nombre NEUTRO pc_snd_<direccion> (decision del usuario, ronda 82). Las dos copias fuera de linea
// del principio del tramo (0x81EBF9 SNDI_clipf y 0x81EC25 SNDI_ftoiround, de sndcmn.h) las emite MSVC solo.

#ifdef EA_PLATFORM_WIN32

#include "Speed/Indep/Libs/snd/9/source/library/pc/sndpc.h"
#include "Speed/Indep/Libs/snd/9/source/library/cmn/sndfxcmn.h"

extern "C" void *memset(void *pdst, int val, unsigned int bytes);

namespace Snd {
extern float gVariableTimerPeriod;
}

extern "C" void SNDMIX_setwetgain(int voice, int bus, float gain);
extern "C" void MIX_setpitch(int voice, int pitchmult);
void SNDI_aztospkrvol(int azimuth, float *pvolumes);

// 0x82261B: lo que sabe hacer la CPU (GetVersionExA, cpuid); 0x822C37: la calidad del remuestreo del mezclador
int pc_snd_82261B();
void pc_snd_822C37(int resamplequality);
int pc_snd_81EC4D(unsigned char *pchanmodes);

static int gotcaps;

void SNDPLATFORM_getvoicerange(int playloc, int *pminvoicerange, int *pmaxvoicerange) {
    *pminvoicerange = 0;
    *pmaxvoicerange = sndgs.sso.set.voicesmaincpu;
}

// 0x81EC47 (SNDSYS_getopts, 0x81E67E): si el driver ya tiene sus capacidades
int pc_snd_81EC47() {
    return gotcaps;
}

// 0x81EC4D: las capacidades del driver (DirectSound) en sndgs.sso.cap y los valores por defecto de sndgs.sso.set, una
// sola vez; la tabla de los 9 modos de salida se copia en pchanmodes si lo hay
int pc_snd_81EC4D(unsigned char *pchanmodes) {
    int rc;
    int i;

    if (gotcaps) {
        for (i = 0; i < 9; i++) {
            pchanmodes[i] = snddrv.chanmodes.modes[i];
        }
        return 0;
    }

    gotcaps = 1;
    memset(&snddrv.chanmodes, 0, sizeof(snddrv.chanmodes));
    rc = DSOUND_getcaps(&sndgs.sso.cap.outputchannelsmax, snddrv.chanmodes.modes);
    if (rc < 0) {
        return rc;
    }
    if (pchanmodes) {
        for (i = 0; i < 9; i++) {
            pchanmodes[i] = snddrv.chanmodes.modes[i];
        }
    }

    sndgs.sso.cap.voicesmaincpumax = 255;
    sndgs.sso.cap.outputratemin = 11025;
    sndgs.sso.cap.outputratemax = 48000;
    sndgs.sso.cap.outputchannelsmin = 1;
    sndgs.sso.set.usesse = 1;
    sndgs.sso.set.numrendermodes = 1;
    sndgs.sso.set.rendermode[0] = 0x24;
    sndgs.sso.set.voicesmaincpu = 32;
    sndgs.sso.set.maxstreams = 16;
    sndgs.sso.set.outputrate = 44100;
    sndgs.sso.set.outputchannels = 2;
    sndgs.sso.set.resamplequality = 25;
    sndgs.cputypes = pc_snd_82261B();
    return 0;
}

// 0x81ED1C (Snd::System::CapOutputMode, 0x81B3E4): la tabla de los 9 modos de salida (1 = el driver lo admite)
int pc_snd_81ED1C(SNDSYSOPTS *psso, unsigned char *pchanmodes) {
    int rc;

    memset(pchanmodes, 0, 9);
    rc = pc_snd_81EC4D(pchanmodes);
    if (rc < 0) {
        return rc;
    }
    return 0;
}

int SNDPLATFORM_outputcaps() {
    int rc;

    rc = pc_snd_81EC4D(NULL);
    if (rc < 0) {
        return rc;
    }
    return 0;
}

int SNDPLATFORM_outputset() {
    int i;
    float period;

    pc_snd_822C37(sndgs.sso.set.resamplequality);
    if (sndgs.installed) {
        int resamplequality = sndgs.sso.set.resamplequality;

        sndgs.sso.set = sndgs.prevset;
        sndgs.sso.set.resamplequality = resamplequality;
        return 0;
    }

    if (sndgs.sso.set.outputchannels < sndgs.sso.cap.outputchannelsmin) {
        sndgs.sso.set.outputchannels = sndgs.sso.cap.outputchannelsmin;
    }
    if (sndgs.sso.set.outputchannels > sndgs.sso.cap.outputchannelsmax) {
        sndgs.sso.set.outputchannels = sndgs.sso.cap.outputchannelsmax;
    }
    if (sndgs.sso.set.outputrate < sndgs.sso.cap.outputratemin) {
        sndgs.sso.set.outputrate = sndgs.sso.cap.outputratemin;
    }
    if (sndgs.sso.set.outputrate > sndgs.sso.cap.outputratemax) {
        sndgs.sso.set.outputrate = sndgs.sso.cap.outputratemax;
    }
    if (sndgs.sso.set.voicesmaincpu > sndgs.sso.cap.voicesmaincpumax) {
        sndgs.sso.set.voicesmaincpu = sndgs.sso.cap.voicesmaincpumax;
    }
    if (sndgs.sso.set.maxstreams > 32) {
        sndgs.sso.set.maxstreams = 32;
    }
    if (sndgs.sso.set.useeax && !sndgs.sso.cap.eax) {
        sndgs.sso.set.useeax = 0;
    }
    if (sndgs.sso.set.outputchannels == 4 && sndgs.sso.set.outputlfe) {
        sndgs.sso.set.outputlfe = 0;
    }
    if (sndgs.sso.set.outputchannels == 2 && sndgs.sso.set.outputlfe) {
        sndgs.sso.set.outputlfe = 0;
    }
    if (sndgs.sso.set.outputchannels == 6 && !sndgs.sso.set.outputlfe) {
        sndgs.sso.set.outputchannels = 5;
        sndgs.sso.set.outputlfe = 1;
    }

    sndgs.voicestotal = sndgs.sso.set.voicesmaincpu;
    if (sndgs.voicestotal > 256) {
        sndgs.voicestotal = 256;
    }

    for (;;) {
        i = 0;
        while (i < sndgs.sso.set.numrendermodes && sndgs.sso.set.rendermode[i] != 0) {
            i++;
        }
        if (i >= sndgs.sso.set.numrendermodes) {
            break;
        }
        sndgs.sso.set.numrendermodes--;
        for (; i < sndgs.sso.set.numrendermodes; i++) {
            sndgs.sso.set.rendermode[i] = sndgs.sso.set.rendermode[i + 1];
        }
    }

    period = sndgs.sso.set.updateperiod;
    if (sndgs.sso.set.updateperiod == -1.0f) {
        period = 20.0f;
    }
    period = SNDI_clipf(period, 10.0f, 100.0f);
    Snd::gVariableTimerPeriod = period;
    snddrv.updateperiod = 10.0f / period;

    if (sndgs.sso.set.usesse && (pc_snd_82261B() & 2)) {
        sndgs.cputypes |= 2;
    } else {
        sndgs.cputypes &= ~2;
        sndgs.sso.set.usesse = 0;
    }
    return 0;
}

int SNDPLATFORM_stop(int voice) {
    SNDDRVVOICE *pv = &snddrv.pvoice[voice];

    pv->pfnstop(pv->state);
    return 0;
}

void Snd::Hal::SetDry(int voice) {
    SetVol(voice);
}

// 0x81EF5E (lo llama 0x81F9FB): nada que hacer en PC
int pc_snd_81EF5E() {
    return 0;
}

int SNDPLATFORM_ModeSetUp(SNDSYSOPTS *pSndSysOpts, Snd::OutputMode outputmode) {
    switch (outputmode) {
    case Snd::OUTPUTMODE_MONO:
        pSndSysOpts->set.outputchannels = 1;
        break;
    case Snd::OUTPUTMODE_STEREO:
    case Snd::OUTPUTMODE_PROLOGIC2:
        pSndSysOpts->set.outputchannels = 2;
        break;
    case Snd::OUTPUTMODE_4POINT0:
        pSndSysOpts->set.outputchannels = 4;
        break;
    case Snd::OUTPUTMODE_5POINT1:
        pSndSysOpts->set.outputchannels = 5;
        pSndSysOpts->set.outputlfe = 1;
        break;
    }
    return 0;
}

int SNDPLATFORM_setpitch(int voice) {
    CHANPUB *ppubv;
    float floatpitch;
    int pitch;
    int i;

    ppubv = &sndgs.chan[voice];
    floatpitch = ppubv->samplerate * 16.0f / sndgs.sso.set.outputrate * ppubv->finalpitch;
    pitch = SNDI_ftoiround(floatpitch);
    for (i = 0; i < ppubv->numchan; i++) {
        MIX_setpitch(ppubv->voices[i], pitch);
    }
    return 0;
}

// 0x81F025 (lo llama 0x81BB45): vacia en PC
void pc_snd_81F025() {}

// La descripcion de la voz que recibe 0x823691 (lado del mezclador de PC, sin escribir). Los nombres son nuestros.
typedef struct PCSNDVOICEDESC {
    unsigned short samplerate; // offset 0x0, size 0x2
    short voice;               // offset 0x2, size 0x2
    unsigned char numchan;     // offset 0x4, size 0x1
    unsigned char samplerep;   // offset 0x5, size 0x1
    unsigned char unknown6;    // offset 0x6, size 0x1
    int unknown8;              // offset 0x8, size 0x4
    int unknownC;              // offset 0xC, size 0x4
    unsigned int frames;       // offset 0x10, size 0x4
    int unknown14[6];          // offset 0x14, size 0x18
    int unknown2C[6];          // offset 0x2C, size 0x18
    int unknown44[6];          // offset 0x44, size 0x18
    short unknown5C[6];        // offset 0x5C, size 0xC
} PCSNDVOICEDESC;

// las funciones de la voz del mezclador de PC (0x823587-0x823795, sin escribir)
int pc_snd_823691(void *pstate, PCSNDVOICEDESC *pdesc);
int pc_snd_823739(void *pstate);
int pc_snd_823763(void *pstate);
void pc_snd_823587(void *pstate, int value);
void pc_snd_8235D9(void *pstate, int value);
void pc_snd_823606(void *pstate, int value);
void pc_snd_823654(void *pstate, int value);
void pc_snd_823681(void *pstate, int value);

// 0x81F026 (la llaman 0x8224DE y 0x8262DA): arranca la voz en el mezclador de PC
int pc_snd_81F026(int voice, int unknown8, int unknownC, int arg4, int arg5, int lowpasscutoff, int highpasscutoff,
                  int *punknown14, int *punknown2C, int *punknown44, int arg11, unsigned char unknown6) {
    SNDDRVVOICE *pv;
    CHANPUB *ppubv;
    PCSNDVOICEDESC desc;
    int i;
    int rc;

    ppubv = &sndgs.chan[voice];
    pv = &snddrv.pvoice[voice];
    pv->pstate[0] = pv->state;
    pv->pstate[1] = pv->state;
    pv->pstate[2] = pv->state;
    pv->pstate[3] = pv->state;
    pv->pstate[4] = pv->state;
    pv->pstate[5] = pv->state;
    pv->pstate[6] = pv->state;
    pv->unknown84 = 0;
    desc.voice = voice;
    desc.samplerep = ppubv->samplerep;
    desc.numchan = ppubv->numchan;
    desc.unknown6 = unknown6;
    desc.samplerate = ppubv->samplerate;
    desc.unknown8 = unknown8;
    desc.unknownC = unknownC;
    desc.frames = ppubv->frames;
    pv->pfnlowpass = pc_snd_823606;
    pv->pfnhighpass = pc_snd_823654;
    pv->pfntimemult = pc_snd_8235D9;
    pv->pfn80 = pc_snd_823681;
    pv->pfnstop = pc_snd_823763;
    pv->pfn74 = pc_snd_823587;
    for (i = 0; i < ppubv->numchan; i++) {
        desc.unknown2C[i] = punknown2C[i];
        desc.unknown14[i] = punknown14[i];
        desc.unknown44[i] = punknown44[i];
    }
    pc_snd_823691(pv->state, &desc);
    SNDPLATFORM_setpitch(voice);
    for (i = 0; i < ppubv->numchan; i++) {
        Snd::Hal::SetPan(ppubv->voices[i]);
    }
    if (pv->pfnlowpass) {
        pv->pfnlowpass(pv->pstate[4], lowpasscutoff);
    }
    if (pv->pfnhighpass) {
        pv->pfnhighpass(pv->pstate[4], highpasscutoff);
    }
    if (pv->pfntimemult) {
        pv->pfntimemult(pv->pstate[3], sndgs.chan[voice].timemult);
    }
    rc = pc_snd_823739(pv->state);
    if (rc < 0) {
        return rc;
    }
    for (i = 0; i < Snd::gMaxFxBuses; i++) {
        SNDMIX_setwetgain(voice, i, ppubv->pFxVolume[i].fxLevel * ppubv->finalvol);
    }
    return 0;
}

int SNDPLATFORM_timemult(int voice, int timemult) {
    SNDDRVVOICE *pv = &snddrv.pvoice[voice];

    if (pv->pfntimemult) {
        pv->pfntimemult(pv->pstate[3], timemult);
    }
    return 0;
}

void Snd::Hal::SetPan(int voice) {
    SNDDRVVOICE *pv;
    CHANPUB *ppubv;
    int i;

    pv = &snddrv.pvoice[voice];
    ppubv = &sndgs.chan[voice];
    if (ppubv->isLfe) {
        for (i = 0; i < Snd::gTotalOutputChannels; i++) {
            pv->balances[i] = 0.0f;
        }
        pv->balances[5] = 1.0f;
    } else {
        SNDI_aztospkrvol(ppubv->azimuth, pv->balances);
    }
    SetVol(voice);
}

int SNDPLATFORM_setfxlevel(int voice, int bus) {
    CHANPUB *ppubv = &sndgs.chan[voice];

    SNDMIX_setwetgain(voice, bus, ppubv->pFxVolume[bus].fxLevel * ppubv->finalvol);
    return 0;
}

// 0x823795 y 0x821801: el bus de efectos del mezclador de PC (sin escribir)
int pc_snd_823795(int channel);
void pc_snd_821801(int channel, float level, unsigned int instancehandle);

// 0x81F294 (la llama Snd::GlobalFxProcessor, 0x81D99E, con -1 = todos los canales): el nivel de salida del bus de
// efectos; solo el de la CPU
int pc_snd_81F294(GlobalFxProcessorData *pdata, int channel) {
    int i;

    if (pdata->device == Snd::DEVICE_MAIN) {
        if (channel != -1) {
            int mixchannel = pc_snd_823795(channel);

            pc_snd_821801(mixchannel, pdata->fxBusOutputLevel[channel], pdata->instanceHandle);
        } else {
            for (i = 0; i < 6; i++) {
                pc_snd_821801(i, pdata->fxBusOutputLevel[i], pdata->instanceHandle);
            }
        }
        return 0;
    }
    return -15;
}

void SNDPLATFORM_lowpass(int voice, int cutofffreq) {
    SNDDRVVOICE *pv = &snddrv.pvoice[voice];

    if (pv->pfnlowpass) {
        pv->pfnlowpass(pv->pstate[4], cutofffreq);
    }
}

void SNDPLATFORM_highpass(int voice, int cutofffreq) {
    SNDDRVVOICE *pv = &snddrv.pvoice[voice];

    if (pv->pfnhighpass) {
        pv->pfnhighpass(pv->pstate[5], cutofffreq);
    }
}

#endif
