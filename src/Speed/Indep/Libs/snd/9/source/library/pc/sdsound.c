// snd de PC: la salida por DirectSound (speed.exe 0x820063-0x820D4C) y la captura (DirectSoundCapture). La fuente de
// PC no esta; el nombre del fichero y el de las funciones que no llama nadie de fuera son nuestros. El mezclador es el
// de la CPU (mix/smixer.c): un hilo de Win32 cada 10 ms mezcla lo que falta hasta la latencia en un buffer
// secundario circular de medio segundo. Las funciones son globales: con `static` MSVC quita el argumento que
// DSOUND_release no usa, y el exe lo empuja. SNDI_ftoiround (DSOUND_mix) es en PC el asm de EA fld/fistp: en C,
// DSOUND_mix sale con __ftol2 (ver PROPUESTAS de PC42).

#ifdef EA_PLATFORM_WIN32

#include "Speed/Indep/Libs/snd/9/source/library/pc/sndpc.h"
#include "Speed/Indep/Libs/snd/9/source/library/pc/dsoundmin.h"

extern "C" int atexit(void(__cdecl *func)(void));

typedef struct MIXCONFIG {
    int outputrate;                  // offset 0x0, size 0x4
    void (*chanovercallback)(int);   // offset 0x4, size 0x4
    unsigned char inputchannels;     // offset 0x8, size 0x1
    unsigned char outputchannels;    // offset 0x9, size 0x1
    unsigned char outputinterleaved; // offset 0xA, size 0x1
} MIXCONFIG;

extern "C" {
void MIX_create(MIXCONFIG *pmc);
void MIX_destroy();
void MIX_audioslice(void **poutput, int frames);
}

void SNDDRV_mixvoicefree(int voice);
void SNDPKTPLAYI_flushcallbackdata();
void SNDSYSI_variabletimerservice();
void SNDREAL_exithandler();
// 0x822050: pone a cero `bytes` bytes (asm de x87 con fst qword). El nombre es nuestro.
void SNDI_memclear(void *p, int bytes);

namespace Snd {
extern volatile int gVariableTimerTick;
}

static float sndlatency = 30.0f;
static unsigned short sndminperiod = 15;
static short sndperiodcountdown = 1000;
static float sndmaxsleep = 200.0f;
static float sndsleepperiod = 15.0f;
static unsigned int sndmaxperiod = 10;

static int sndhalfbufferframes;
static unsigned char sndbytesperframe;
static DWORD sndlastservicetime;
static DWORD sndservicetime;
static int sndwriteframe;
static int sndstallcount;
static DWORD sndlastwritepos;
static LPDIRECTSOUND pds;
static LPDIRECTSOUNDBUFFER pdsprimary;
static LPDIRECTSOUNDBUFFER pdsbuffer;
static int sndbufferbytes;
static int sndbufferframes;
static int sndframesowed;
static int sndticks;
static int sndframesmixed;
static LPDIRECTSOUNDCAPTURE pdscapture;
static LPDIRECTSOUNDCAPTUREBUFFER pdscbuffer;
static unsigned int capbufferframes;
static unsigned int capblockframes;
static unsigned int capwriteframes;
static unsigned int capreadframes;
static unsigned int caplastpos;
static int capshift;
static HANDLE hsndthread;
static DWORD sndthreadid;
static int sndlostcount;

void DSOUND_stopthread() {
    snddrv.threadrun = 0;
    while (snddrv.threadactive) {
        Sleep(0);
    }
}

int DSOUND_setformat(int channels, int rate) {
    WAVEFORMATEXTENSIBLE wfx;
    DSBUFFERDESC dsbd;
    HRESULT hr;
    int frames;

    wfx.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
    wfx.Format.nChannels = channels;
    wfx.Format.cbSize = 22;
    wfx.Format.nSamplesPerSec = rate;
    wfx.Format.nBlockAlign = channels * 2;
    sndbytesperframe = wfx.Format.nBlockAlign;
    wfx.Format.nAvgBytesPerSec = wfx.Format.nBlockAlign * rate;
    wfx.Format.wBitsPerSample = 16;
    wfx.Samples.wValidBitsPerSample = 16;
    wfx.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;

    if (wfx.Format.nChannels == 6) {
        wfx.dwChannelMask = KSAUDIO_SPEAKER_5POINT1;
    } else if (wfx.Format.nChannels == 4) {
        wfx.dwChannelMask = KSAUDIO_SPEAKER_QUAD;
    } else if (wfx.Format.nChannels <= 2) {
        wfx.Format.wFormatTag = WAVE_FORMAT_PCM;
        wfx.Format.cbSize = 0;
    }

    hr = pdsprimary->SetFormat(&wfx.Format);
    if (hr < 0) {
        return hr;
    }

    if (pdsbuffer != NULL) {
        pdsbuffer->Release();
        pdsbuffer = NULL;
    }

    SNDI_memclear(&dsbd, sizeof(dsbd));
    dsbd.dwSize = sizeof(dsbd);
    dsbd.dwFlags = DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_GLOBALFOCUS;
    dsbd.lpwfxFormat = &wfx.Format;

    sndticks = 0;
    sndframesmixed = 0;
    sndframesowed = 0;

    frames = (wfx.Format.nSamplesPerSec * 500 / 1000) & ~15;
    sndhalfbufferframes = frames / 2;
    sndbufferbytes = wfx.Format.nBlockAlign * frames;
    dsbd.dwBufferBytes = sndbufferbytes;
    sndbufferframes = frames;

    return pds->CreateSoundBuffer(&dsbd, &pdsbuffer, NULL);
}

int DSOUND_release(StartMode startMode, char destroymixer) {
    if (pdsbuffer != NULL) {
        pdsbuffer->Release();
        pdsbuffer = NULL;
    }
    if (pdsprimary != NULL) {
        pdsprimary->Release();
        pdsprimary = NULL;
    }
    if (pds != NULL) {
        pds->Release();
        pds = NULL;
    }
    if (destroymixer) {
        MIX_destroy();
    }
    return 0;
}

int DSOUND_captureopen(unsigned int blockframes, int bits, int rate) {
    WAVEFORMATEX wfx;
    DSCBUFFERDESC dscbd;
    int blocks;

    if (pdscapture != NULL) {
        return SNDERR_INUSE;
    }

    capwriteframes = 0;
    capreadframes = 0;
    caplastpos = 0;
    capblockframes = blockframes;

    blocks = rate * 300 / 1000 / blockframes + 1;
    if (blocks < 3) {
        blocks = 3;
    }
    capbufferframes = blocks * blockframes;

    wfx.nChannels = 1;
    wfx.wFormatTag = WAVE_FORMAT_PCM;
    wfx.nSamplesPerSec = rate;
    wfx.wBitsPerSample = bits;
    wfx.nBlockAlign = bits >> 3;
    wfx.nAvgBytesPerSec = wfx.nBlockAlign * rate;
    wfx.cbSize = 0;
    capshift = wfx.nBlockAlign >> 1;

    dscbd.dwSize = sizeof(dscbd);
    dscbd.dwFlags = 0;
    dscbd.dwBufferBytes = wfx.nBlockAlign * capbufferframes;
    dscbd.dwReserved = 0;
    dscbd.lpwfxFormat = &wfx;

    if (DirectSoundCaptureCreate(NULL, &pdscapture, NULL) == DS_OK) {
        if (pdscapture->CreateCaptureBuffer(&dscbd, &pdscbuffer, NULL) == DS_OK &&
            pdscbuffer->Start(DSCBSTART_LOOPING) == DS_OK) {
            return 0;
        }

        if (pdscapture != NULL) {
            pdscapture->Release();
        }
        pdscapture = NULL;
        pdscbuffer = NULL;
    }
    return -1;
}

int DSOUND_captureread(void *pdst) {
    DWORD cappos;
    DWORD readpos;
    void *p1;
    DWORD bytes1;

    if (pdscbuffer->GetCurrentPosition(&cappos, &readpos) != DS_OK) {
        return -1;
    }

    readpos >>= capshift;
    if (readpos >= caplastpos) {
        capwriteframes += readpos - caplastpos;
    } else {
        capwriteframes += capbufferframes - caplastpos + readpos;
    }
    caplastpos = readpos;

    if (capwriteframes - capreadframes < capblockframes) {
        return -1;
    }

    if (pdscbuffer->Lock((capreadframes % capbufferframes) << capshift, capblockframes << capshift, &p1, &bytes1,
                         NULL, NULL, 0) != DS_OK) {
        return -1;
    }

    Snd::Util::MemCpy(pdst, p1, capblockframes << capshift);

    if (pdscbuffer->Unlock(p1, bytes1, NULL, 0) != DS_OK) {
        return -1;
    }

    capreadframes += capblockframes;
    return 0;
}

int DSOUND_captureclose() {
    if (pdscapture != NULL) {
        pdscapture->Release();
    }
    pdscapture = NULL;
    pdscbuffer = NULL;
    return 0;
}

int DSOUND_close(StartMode startMode) {
    DSOUND_stopthread();
    DSOUND_release(startMode, 1);
    return 0;
}

int SNDPLATFORM_getlatency() {
    DWORD playpos;
    int frames;

    if (pds == NULL) {
        return 0;
    }
    if (pdsbuffer->GetCurrentPosition(&playpos, NULL) < 0) {
        return 0;
    }

    playpos = (playpos / 2) / (sndgs.sso.set.outputchannels + sndgs.sso.set.outputlfe);
    frames = sndwriteframe - playpos;
    if (frames < 0) {
        frames += sndbufferframes;
    }

    return frames * 1000 / sndgs.sso.set.outputrate;
}

HWND DSOUND_getwindow() {
    HWND hwnd;

    hwnd = GetForegroundWindow();
    if (hwnd == NULL) {
        hwnd = GetDesktopWindow();
    }
    return hwnd;
}

void DSOUND_mix(void **ppbuf, int frames) {
    short *p;
    int n;

    p = (short *)*ppbuf;

    while (frames > 0) {
        if (sndframesowed <= 0) {
            static float curTick = 0.1f;

            sndticks++;
            SNDSYSI_100hzserver();

            Snd::gVariableTimerTick = SNDI_ftoiround(curTick += snddrv.updateperiod);
            if (curTick > 30.0f) {
                curTick -= 30.0f;
            }

            SNDSYSI_variabletimerservice();

            sndframesowed = (sndgs.sso.set.outputrate * sndticks / 100 - sndframesmixed) & 0xFFFFFF0;
            sndframesmixed += sndframesowed;

            if (sndticks > 30000) {
                sndticks = 0;
                sndframesmixed = 0;
            }
        }

        n = sndframesowed < frames ? sndframesowed : frames;
        sndframesowed -= n;
        frames -= n;

        MIX_audioslice((void **)&p, n);
        SNDPKTPLAYI_flushcallbackdata();

        p += (sndgs.sso.set.outputchannels + sndgs.sso.set.outputlfe) * n;
    }
}

int SNDPLATFORM_restore(StartMode startMode) {
    DSOUND_close(startMode);

    SNDSYS_entercritical();
    SNDMEMI_free(snddrv.pvoice);
    SNDSYS_leavecritical();

    return 0;
}

int DSOUND_getcaps(unsigned char *pmaxchannels, unsigned char *pchanmodes) {
    DSCAPS caps;
    DSBUFFERDESC dsbd;
    HRESULT hr;
    int ret;
    DWORD rate;

    pdsprimary = NULL;
    pdsbuffer = NULL;

    hr = DirectSoundCreate(NULL, &pds, NULL);
    if (hr != DS_OK) {
        if (hr == DSERR_ALLOCATED) {
            ret = SNDERR_INUSE;
        } else {
            ret = hr == DSERR_NODRIVER ? SNDERR_NO_DRIVER : SNDERR_GENERAL;
        }
        pds = NULL;
    } else {
        pds->SetCooperativeLevel(DSOUND_getwindow(), DSSCL_PRIORITY);

        caps.dwSize = sizeof(caps);
        if (pds->GetCaps(&caps) != DS_OK) {
            ret = -1;
        } else {
            SNDI_memclear(&dsbd, sizeof(dsbd));
            dsbd.dwSize = sizeof(dsbd);
            dsbd.dwFlags = DSBCAPS_PRIMARYBUFFER;

            ret = pds->CreateSoundBuffer(&dsbd, &pdsprimary, NULL);
            if (ret >= 0) {
                rate = caps.dwMaxSecondarySampleRate;
                if (rate > 44100) {
                    rate = 44100;
                } else if (rate <= 0) {
                    rate = 11025;
                }

                *pmaxchannels = 0;
                if (DSOUND_setformat(1, rate) >= 0) {
                    if (pchanmodes != NULL) {
                        pchanmodes[0] = 1;
                    }
                    *pmaxchannels = 1;
                }
                if (DSOUND_setformat(2, rate) >= 0) {
                    if (pchanmodes != NULL) {
                        pchanmodes[1] = 1;
                    }
                    *pmaxchannels = 2;
                }
                if (DSOUND_setformat(4, rate) >= 0) {
                    if (pchanmodes != NULL) {
                        pchanmodes[2] = 1;
                    }
                    *pmaxchannels = 4;
                }
                if (DSOUND_setformat(6, rate) >= 0) {
                    if (pchanmodes != NULL) {
                        pchanmodes[3] = 1;
                    }
                    *pmaxchannels = 5;
                }
            }
        }
    }

    if (pdsbuffer != NULL) {
        pdsbuffer->Release();
        pdsbuffer = NULL;
    }
    if (pdsprimary != NULL) {
        pdsprimary->Release();
        pdsprimary = NULL;
    }
    if (pds != NULL) {
        pds->Release();
    }
    pds = NULL;

    return ret;
}

int DSOUND_open(StartMode startMode, char createmixer) {
    DSBUFFERDESC dsbd;
    HRESULT hr;
    int ret;
    void *p;
    DWORD bytes;

    ret = -1;
    sndlatency = 30.0f;
    sndlastservicetime = 0;
    sndservicetime = 0;
    sndsleepperiod = 15.0f;
    sndwriteframe = 0;
    sndminperiod = 15;
    sndperiodcountdown = 1000;

    if (createmixer) {
        MIXCONFIG mc;

        mc.inputchannels = sndgs.sso.set.voicesmaincpu;
        mc.outputrate = sndgs.sso.set.outputrate;
        mc.outputchannels = sndgs.sso.set.outputchannels + sndgs.sso.set.outputlfe;
        mc.outputinterleaved = mc.outputchannels > 1;
        mc.chanovercallback = SNDDRV_mixvoicefree;
        MIX_create(&mc);
    }

    pdsprimary = NULL;
    pdsbuffer = NULL;

    hr = DirectSoundCreate(NULL, &pds, NULL);
    if (hr != DS_OK) {
        if (hr == DSERR_ALLOCATED) {
            ret = SNDERR_INUSE;
        } else {
            ret = hr == DSERR_NODRIVER ? SNDERR_NO_DRIVER : SNDERR_GENERAL;
        }
        pds = NULL;
    } else {
        pds->SetCooperativeLevel(DSOUND_getwindow(), DSSCL_PRIORITY);

        SNDI_memclear(&dsbd, sizeof(dsbd));
        dsbd.dwSize = sizeof(dsbd);
        dsbd.dwFlags = DSBCAPS_PRIMARYBUFFER;

        if (pds->CreateSoundBuffer(&dsbd, &pdsprimary, NULL) >= 0 &&
            DSOUND_setformat(sndgs.sso.set.outputchannels + sndgs.sso.set.outputlfe, sndgs.sso.set.outputrate) >= 0) {
            if (pdsbuffer->Lock(0, sndbufferbytes, &p, &bytes, NULL, NULL, 0) >= 0) {
                SNDI_memclear(p, sndbufferbytes);
                pdsbuffer->Unlock(p, sndbufferbytes, NULL, 0);
            }
            if (pdsbuffer->Play(0, 0, DSBPLAY_LOOPING) >= 0) {
                return 0;
            }
        }
    }

    DSOUND_release(startMode, 1);
    return ret;
}

void DSOUND_service() {
    DWORD status;
    DWORD writepos;
    char restarted;
    int frames[2];
    int offset;
    int target;
    int i;
    void *p1;
    DWORD bytes1;
    DWORD elapsed;
    DWORD now;

    if (pds == NULL) {
        return;
    }

    pdsbuffer->GetStatus(&status);
    if (!(status & DSBSTATUS_BUFFERLOST) && sndlostcount <= 100) {
        if (status & DSBSTATUS_PLAYING) {
            restarted = 0;
            do {
                if (pdsbuffer->GetCurrentPosition(NULL, &writepos) < 0) {
                    return;
                }
                if (sndlastwritepos != writepos) {
                    sndlastwritepos = writepos;
                    sndstallcount = 0;
                    break;
                }
                if (++sndstallcount >= 100) {
                    DSOUND_release(WARM, 0);
                    DSOUND_open(WARM, 0);
                    restarted = 1;
                }
            } while (restarted == 1);

            writepos /= sndbytesperframe;
            target = (int)((float)sndgs.sso.set.outputrate * ((sndlatency + sndsleepperiod) * 0.001f)) + writepos;
            target &= 0xFFFFFF0;
            if (target >= sndbufferframes) {
                target -= sndbufferframes;
            }

            frames[0] = target - sndwriteframe;
            if (frames[0] < 0 && frames[0] > -sndhalfbufferframes) {
                return;
            }
            if (frames[0] > sndhalfbufferframes) {
                return;
            }
            if (frames[0] >= 0) {
                frames[1] = 0;
            } else {
                frames[0] = sndbufferframes - sndwriteframe;
                frames[1] = target;
            }

            offset = sndwriteframe;
            for (i = 0; i < 2; i++) {
                if (frames[i] != 0) {
                    pdsbuffer->Lock(offset * (sndgs.sso.set.outputchannels + sndgs.sso.set.outputlfe) * 2,
                                    frames[i] * (sndgs.sso.set.outputchannels + sndgs.sso.set.outputlfe) * 2, &p1,
                                    &bytes1, NULL, NULL, 0);
                    DSOUND_mix(&p1, frames[i]);
                    pdsbuffer->Unlock(p1, frames[i] * (sndgs.sso.set.outputchannels + sndgs.sso.set.outputlfe) * 2,
                                      NULL, 0);
                    offset = 0;
                }
            }

            now = timeGetTime();
            elapsed = now - sndlastservicetime;
            if (elapsed > 300) {
                elapsed = sndmaxperiod;
            }
            if (elapsed > sndmaxperiod) {
                sndmaxperiod = elapsed;
            }
            if (elapsed > sndminperiod) {
                sndminperiod = elapsed;
                sndsleepperiod = elapsed;
            }
            if (--sndperiodcountdown <= 0) {
                if (sndmaxperiod < sndminperiod) {
                    sndminperiod = sndmaxperiod;
                }
                sndperiodcountdown = 3000;
                sndmaxperiod = 10;
                sndsleepperiod = sndminperiod;
                if (sndsleepperiod < 10.0f) {
                    sndsleepperiod = 10.0f;
                }
            }
            if (sndsleepperiod > sndmaxsleep) {
                sndsleepperiod = sndmaxsleep;
            }

            sndwriteframe = target;
        } else {
            pdsbuffer->Play(0, 0, DSBPLAY_LOOPING);
            sndlostcount = 0;
        }
    } else {
        pdsbuffer->Restore();
        sndlostcount = 0;
    }
}

void DSOUND_servicelocked() {
    Snd::gMutexLockFn();
    DSOUND_service();
    Snd::gMutexUnlockFn();
}

DWORD WINAPI DSOUND_thread(void *param) {
    DWORD next;
    DWORD sleep;

    timeBeginPeriod(1);
    next = timeGetTime();
    sndservicetime = next;

    while (snddrv.threadrun) {
        if (sndgs.profileenter != NULL) {
            sndgs.profileenter();
        }

        sndlastservicetime = sndservicetime;
        sndservicetime = timeGetTime();
        DSOUND_servicelocked();

        next += 10;
        sleep = next - timeGetTime();
        if ((int)sleep < 0 || (int)sleep > 200) {
            sleep = 1;
        }

        if (sndgs.profileleave != NULL) {
            sndgs.profileleave();
        }
        Sleep(sleep);
    }

    timeEndPeriod(1);
    snddrv.threadactive = 0;
    return 0;
}

void DSOUND_startthread() {
    snddrv.threadactive = 1;
    snddrv.threadrun = 1;
    hsndthread = CreateThread(NULL, 0, DSOUND_thread, NULL, 0, &sndthreadid);
    SetThreadPriority(hsndthread, THREAD_PRIORITY_TIME_CRITICAL);
}

int DSOUND_init(StartMode startMode) {
    int ret;

    if (!snddrv.exithandlerset) {
        atexit(SNDREAL_exithandler);
        snddrv.exithandlerset = 1;
    }

    ret = DSOUND_open(startMode, 1);
    if (ret >= 0) {
        DSOUND_startthread();
    }
    return ret;
}

int SNDPLATFORM_init(StartMode startMode) {
    SNDSYS_entercritical();
    snddrv.pvoice = (SNDDRVVOICE *)SNDMEMI_allocz(sndgs.voicestotal * sizeof(SNDDRVVOICE));
    SNDI_memclear(snddrv.pvoice, sndgs.voicestotal * sizeof(SNDDRVVOICE));
    SNDSYS_leavecritical();

    switch (sndgs.sso.set.compatibility) {
    case -1:
        sndlatency -= 10.0f;
        break;
    case 1:
        sndlatency += 20.0f;
        break;
    case 2:
        sndlatency += 50.0f;
        break;
    }

    return DSOUND_init(startMode);
}

#endif
