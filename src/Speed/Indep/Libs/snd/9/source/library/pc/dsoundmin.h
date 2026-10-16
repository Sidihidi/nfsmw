#ifndef SND_PC_DSOUNDMIN_H
#define SND_PC_DSOUNDMIN_H

// Lo minimo de DirectSound (dsound.h, mmreg.h y ksmedia.h del SDK de DirectX 9) que usa el driver de snd de PC,
// declarado a mano para no copiar cabeceras de Microsoft al arbol (como D3D9Min.hpp y Win32Min.hpp). Las interfaces
// son las de C++: cada metodo es un virtual __stdcall con el puntero a la interfaz como primer argumento, y el orden de
// la tabla es el del SDK (speed.exe llama a CreateSoundBuffer en +0xC, Lock en +0x2C, Unlock en +0x4C...).

#ifndef EA_PLATFORM_WIN32
#error "dsoundmin.h solo es de PC"
#endif

#include "Speed/PC/Src/Win32Min.hpp"

typedef long HRESULT;

typedef struct _GUID {
    unsigned long Data1;
    unsigned short Data2;
    unsigned short Data3;
    unsigned char Data4[8];
} GUID;

#define DS_OK 0
#define DSERR_ALLOCATED ((HRESULT)0x8878000A)
#define DSERR_NODRIVER ((HRESULT)0x88780078)

#define DSSCL_PRIORITY 0x00000002

#define DSBCAPS_PRIMARYBUFFER 0x00000001
#define DSBCAPS_GLOBALFOCUS 0x00008000
#define DSBCAPS_GETCURRENTPOSITION2 0x00010000

#define DSBPLAY_LOOPING 0x00000001

#define DSBSTATUS_PLAYING 0x00000001
#define DSBSTATUS_BUFFERLOST 0x00000002

#define DSCBSTART_LOOPING 0x00000001

#define WAVE_FORMAT_PCM 1
#define WAVE_FORMAT_EXTENSIBLE 0xFFFE

#define SPEAKER_FRONT_LEFT 0x1
#define SPEAKER_FRONT_RIGHT 0x2
#define SPEAKER_FRONT_CENTER 0x4
#define SPEAKER_LOW_FREQUENCY 0x8
#define SPEAKER_BACK_LEFT 0x10
#define SPEAKER_BACK_RIGHT 0x20

#define KSAUDIO_SPEAKER_QUAD (SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT)
#define KSAUDIO_SPEAKER_5POINT1                                                                                        \
    (SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER | SPEAKER_LOW_FREQUENCY | SPEAKER_BACK_LEFT |     \
     SPEAKER_BACK_RIGHT)

// mmreg.h los declara con empaquetado de 1 (pshpack1.h): WAVEFORMATEX mide 18 B y WAVEFORMATEXTENSIBLE 40.
#pragma pack(push, 1)
typedef struct tWAVEFORMATEX {
    WORD wFormatTag;
    WORD nChannels;
    DWORD nSamplesPerSec;
    DWORD nAvgBytesPerSec;
    WORD nBlockAlign;
    WORD wBitsPerSample;
    WORD cbSize;
} WAVEFORMATEX;

typedef struct {
    WAVEFORMATEX Format;
    union {
        WORD wValidBitsPerSample;
        WORD wSamplesPerBlock;
        WORD wReserved;
    } Samples;
    DWORD dwChannelMask;
    GUID SubFormat;
} WAVEFORMATEXTENSIBLE;
#pragma pack(pop)

extern "C" const GUID KSDATAFORMAT_SUBTYPE_PCM;

typedef struct _DSCAPS {
    DWORD dwSize;
    DWORD dwFlags;
    DWORD dwMinSecondarySampleRate;
    DWORD dwMaxSecondarySampleRate;
    DWORD dwPrimaryBuffers;
    DWORD dwMaxHwMixingAllBuffers;
    DWORD dwMaxHwMixingStaticBuffers;
    DWORD dwMaxHwMixingStreamingBuffers;
    DWORD dwFreeHwMixingAllBuffers;
    DWORD dwFreeHwMixingStaticBuffers;
    DWORD dwFreeHwMixingStreamingBuffers;
    DWORD dwMaxHw3DAllBuffers;
    DWORD dwMaxHw3DStaticBuffers;
    DWORD dwMaxHw3DStreamingBuffers;
    DWORD dwFreeHw3DAllBuffers;
    DWORD dwFreeHw3DStaticBuffers;
    DWORD dwFreeHw3DStreamingBuffers;
    DWORD dwTotalHwMemBytes;
    DWORD dwFreeHwMemBytes;
    DWORD dwMaxContigFreeHwMemBytes;
    DWORD dwUnlockTransferRateHwBuffers;
    DWORD dwPlayCpuOverheadSwBuffers;
    DWORD dwReserved1;
    DWORD dwReserved2;
} DSCAPS;

typedef struct _DSBUFFERDESC {
    DWORD dwSize;
    DWORD dwFlags;
    DWORD dwBufferBytes;
    DWORD dwReserved;
    WAVEFORMATEX *lpwfxFormat;
    GUID guid3DAlgorithm;
} DSBUFFERDESC;

typedef struct _DSCEFFECTDESC DSCEFFECTDESC;

typedef struct _DSCBUFFERDESC {
    DWORD dwSize;
    DWORD dwFlags;
    DWORD dwBufferBytes;
    DWORD dwReserved;
    WAVEFORMATEX *lpwfxFormat;
    DWORD dwFXCount;
    DSCEFFECTDESC *lpDSCFXDesc;
} DSCBUFFERDESC;

struct IUnknown {
    virtual HRESULT __stdcall QueryInterface(const GUID &riid, void **ppvObj) = 0;
    virtual unsigned long __stdcall AddRef() = 0;
    virtual unsigned long __stdcall Release() = 0;
};

struct IDirectSoundBuffer : public IUnknown {
    virtual HRESULT __stdcall GetCaps(void *pDSBufferCaps) = 0;
    virtual HRESULT __stdcall GetCurrentPosition(DWORD *pdwCurrentPlayCursor, DWORD *pdwCurrentWriteCursor) = 0;
    virtual HRESULT __stdcall GetFormat(WAVEFORMATEX *pwfxFormat, DWORD dwSizeAllocated, DWORD *pdwSizeWritten) = 0;
    virtual HRESULT __stdcall GetVolume(long *plVolume) = 0;
    virtual HRESULT __stdcall GetPan(long *plPan) = 0;
    virtual HRESULT __stdcall GetFrequency(DWORD *pdwFrequency) = 0;
    virtual HRESULT __stdcall GetStatus(DWORD *pdwStatus) = 0;
    virtual HRESULT __stdcall Initialize(struct IDirectSound *pDirectSound, const DSBUFFERDESC *pcDSBufferDesc) = 0;
    virtual HRESULT __stdcall Lock(DWORD dwOffset, DWORD dwBytes, void **ppvAudioPtr1, DWORD *pdwAudioBytes1,
                                   void **ppvAudioPtr2, DWORD *pdwAudioBytes2, DWORD dwFlags) = 0;
    virtual HRESULT __stdcall Play(DWORD dwReserved1, DWORD dwPriority, DWORD dwFlags) = 0;
    virtual HRESULT __stdcall SetCurrentPosition(DWORD dwNewPosition) = 0;
    virtual HRESULT __stdcall SetFormat(const WAVEFORMATEX *pcfxFormat) = 0;
    virtual HRESULT __stdcall SetVolume(long lVolume) = 0;
    virtual HRESULT __stdcall SetPan(long lPan) = 0;
    virtual HRESULT __stdcall SetFrequency(DWORD dwFrequency) = 0;
    virtual HRESULT __stdcall Stop() = 0;
    virtual HRESULT __stdcall Unlock(void *pvAudioPtr1, DWORD dwAudioBytes1, void *pvAudioPtr2,
                                     DWORD dwAudioBytes2) = 0;
    virtual HRESULT __stdcall Restore() = 0;
};

struct IDirectSound : public IUnknown {
    virtual HRESULT __stdcall CreateSoundBuffer(const DSBUFFERDESC *pcDSBufferDesc, IDirectSoundBuffer **ppDSBuffer,
                                                IUnknown *pUnkOuter) = 0;
    virtual HRESULT __stdcall GetCaps(DSCAPS *pDSCaps) = 0;
    virtual HRESULT __stdcall DuplicateSoundBuffer(IDirectSoundBuffer *pDSBufferOriginal,
                                                   IDirectSoundBuffer **ppDSBufferDuplicate) = 0;
    virtual HRESULT __stdcall SetCooperativeLevel(HWND hwnd, DWORD dwLevel) = 0;
    virtual HRESULT __stdcall Compact() = 0;
    virtual HRESULT __stdcall GetSpeakerConfig(DWORD *pdwSpeakerConfig) = 0;
    virtual HRESULT __stdcall SetSpeakerConfig(DWORD dwSpeakerConfig) = 0;
    virtual HRESULT __stdcall Initialize(const GUID *pcGuidDevice) = 0;
};

struct IDirectSoundCaptureBuffer : public IUnknown {
    virtual HRESULT __stdcall GetCaps(void *pDSCBCaps) = 0;
    virtual HRESULT __stdcall GetCurrentPosition(DWORD *pdwCapturePosition, DWORD *pdwReadPosition) = 0;
    virtual HRESULT __stdcall GetFormat(WAVEFORMATEX *pwfxFormat, DWORD dwSizeAllocated, DWORD *pdwSizeWritten) = 0;
    virtual HRESULT __stdcall GetStatus(DWORD *pdwStatus) = 0;
    virtual HRESULT __stdcall Initialize(struct IDirectSoundCapture *pDirectSoundCapture,
                                         const DSCBUFFERDESC *pcDSCBufferDesc) = 0;
    virtual HRESULT __stdcall Lock(DWORD dwOffset, DWORD dwBytes, void **ppvAudioPtr1, DWORD *pdwAudioBytes1,
                                   void **ppvAudioPtr2, DWORD *pdwAudioBytes2, DWORD dwFlags) = 0;
    virtual HRESULT __stdcall Start(DWORD dwFlags) = 0;
    virtual HRESULT __stdcall Stop() = 0;
    virtual HRESULT __stdcall Unlock(void *pvAudioPtr1, DWORD dwAudioBytes1, void *pvAudioPtr2,
                                     DWORD dwAudioBytes2) = 0;
};

struct IDirectSoundCapture : public IUnknown {
    virtual HRESULT __stdcall CreateCaptureBuffer(const DSCBUFFERDESC *pcDSCBufferDesc,
                                                  IDirectSoundCaptureBuffer **ppDSCBuffer, IUnknown *pUnkOuter) = 0;
    virtual HRESULT __stdcall GetCaps(void *pDSCCaps) = 0;
    virtual HRESULT __stdcall Initialize(const GUID *pcGuidDevice) = 0;
};

typedef IDirectSound *LPDIRECTSOUND;
typedef IDirectSoundBuffer *LPDIRECTSOUNDBUFFER;
typedef IDirectSoundCapture *LPDIRECTSOUNDCAPTURE;
typedef IDirectSoundCaptureBuffer *LPDIRECTSOUNDCAPTUREBUFFER;

// dsound.h: speed.exe las llama por un salto a la tabla de importaciones (`call DirectSoundCreate`, sin dllimport).
extern "C" {
HRESULT WINAPI DirectSoundCreate(const GUID *pcGuidDevice, LPDIRECTSOUND *ppDS, IUnknown *pUnkOuter);
HRESULT WINAPI DirectSoundCaptureCreate(const GUID *pcGuidDevice, LPDIRECTSOUNDCAPTURE *ppDSC, IUnknown *pUnkOuter);
}

#endif
