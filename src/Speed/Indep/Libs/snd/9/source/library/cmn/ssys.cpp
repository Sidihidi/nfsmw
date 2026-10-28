#include "./sndcmn.h"

// Declaraciones que en el original viven en cabeceras internas de la libreria
// que aqui no existen todavia. No se toca ninguna cabecera compartida.
enum StartMode {
    COLD = 0,
    WARM = 1,
};

// total size: 0xC
typedef struct SNDPROFVOICES {
    unsigned short currentmaincpu; // offset 0x0, size 0x2
    unsigned char currentiopcpu;   // offset 0x2, size 0x1
    unsigned char currentspu;      // offset 0x3, size 0x1
    unsigned short currentdsp;     // offset 0x4, size 0x2
    unsigned short currentds2dhw;  // offset 0x6, size 0x2
    unsigned short currentds3dhw;  // offset 0x8, size 0x2
    char pad[2];                   // offset 0xA, size 0x2
} SNDPROFVOICES;

extern CListStack *fxBusesLinkList;

int SNDPLATFORM_init(StartMode startMode);
int SNDPLATFORM_restore(StartMode startMode);
int SNDPLATFORM_ModeSetUp(SNDSYSOPTS *pSndSysOpts, Snd::OutputMode outputmode);

void SNDI_spkrconfig();
void SNDI_precalcaztospkrvol();
void SNDI_freespkrtable();

int SNDSYS_getopts(SNDSYSOPTS *psso);
int SNDSYS_setopts(SNDSYSOPTS *psso);
int SNDSYSI_init(void *psndheap, int heapsize, int headerversion);
int SNDSYS_restore();
int SNDSYSI_chanpubinit();
int SNDSYSI_updatesso(SNDSYSOPTS *pSndSysOpts);

#ifdef __cplusplus
extern "C" {
#endif
int SNDPROFILE_voices(SNDPROFVOICES *pspv);
#ifdef __cplusplus
}
#endif

#ifdef EA_PLATFORM_WIN32
// PC (speed.exe 0x81B3C8, System::CapOutputMode): the driver of pc/ answers with a table of 9 output modes
// (1 = supported). The driver function (0x81ED1C, pc/snddrv.c) has no name in any source: neutral name. Global,
// outside namespace Snd, like its definition.
int pc_snd_81ED1C(SNDSYSOPTS *psso, unsigned char *pchanmodes);
#endif

namespace Snd {

// total size: 0x8
typedef struct VoicesNext {
    unsigned char value; // offset 0x0, size 0x1
    int changed;         // offset 0x4, size 0x4
} VoicesNext;

// total size: 0x8
typedef struct OutputSampleRateNext {
    unsigned short value; // offset 0x0, size 0x2
    int changed;          // offset 0x4, size 0x4
} OutputSampleRateNext;

// total size: 0x8
typedef struct OutputModeNext {
    OutputMode value; // offset 0x0, size 0x4
    int changed;      // offset 0x4, size 0x4
} OutputModeNext;

extern unsigned char gTotalOutputChannels;

inline Result SndErrToResult(int oldError) {
    Result newError;

    switch (oldError) {
    case SND_OK:
        newError = RESULT_OK;
        break;
    case SNDERR_FILE_NOT_FOUND:
        newError = RESULT_ERR_FILEERROR;
        break;
    case SNDERR_INUSE:
        newError = RESULT_ERR_ALREADYINUSE;
        break;
    case SNDERR_UNSUPPORTED:
        newError = RESULT_ERR_UNSUPPORTEDOPT;
        break;
    case SNDERR_NOT_INITED:
        newError = RESULT_ERR_NOTINIT;
        break;
    case SNDERR_NO_DRIVER:
        newError = RESULT_ERR_DRIVER;
        break;
    case SNDERR_ALLOCATE:
    case SNDERR_MEM:
        newError = RESULT_ERR_MEMERROR;
        break;
    case SNDERR_INV_PARAM:
        newError = RESULT_ERR_INVPARAM;
        break;
    case SNDERR_FORMAT:
        newError = RESULT_ERR_INVDATAFORMAT;
        break;
    default:
        newError = RESULT_ERR_GENERAL;
        break;
    }

    return newError;
}

inline Result SafeGetOpts(SNDSYSOPTS *pSndSysOpts) {
    int result = SNDSYS_getopts(pSndSysOpts);
    return SndErrToResult(result);
}

inline Result SafeSetOpts(SNDSYSOPTS *pSndSysOpts) {
    int result = SNDSYS_setopts(pSndSysOpts);
    return SndErrToResult(result);
}

void *pheap = NULL;
OutputMode outputmode = OUTPUTMODE_PROLOGIC2;
VoicesNext gVoicesNext[4];
OutputSampleRateNext gOutputSampleRateNext[4];
OutputModeNext gOutputModeNext;
int heapsize;

Result System::SetMaxBanks(int banks) {
    SNDSYSOPTS sso;

    SafeGetOpts(&sso);
    sso.set.maxbanks = banks;
    SafeSetOpts(&sso);
    return RESULT_OK;
}

#ifdef EA_PLATFORM_WIN32
// The PC snd (speed.exe 0x81B297-0x81B7AE) has the whole System API of sndo.h, in header order.
Result System::GetMaxBanks(int *pbanks) {
    SNDSYSOPTS sso;

    SafeGetOpts(&sso);
    *pbanks = sso.set.maxbanks;
    return RESULT_OK;
}

Result System::SetMaxStreams(int maxstreams) {
    SNDSYSOPTS sso;

    SafeGetOpts(&sso);
    sso.set.maxstreams = maxstreams;
    SafeSetOpts(&sso);
    return RESULT_OK;
}

Result System::SetStealEqualPriorityVoices(bool stealequalpriority) {
    SNDSYSOPTS sso;

    SafeGetOpts(&sso);
    sso.set.stealequalpriorityvoices = stealequalpriority;
    SafeSetOpts(&sso);
    return RESULT_OK;
}

Result System::SetRandomSeed(unsigned int seed) {
    SNDSYSOPTS sso;

    SafeGetOpts(&sso);
    sso.set.randomseed = seed;
    SafeSetOpts(&sso);
    return RESULT_OK;
}

Result System::SetUpdateRate(float rate) {
    SNDSYSOPTS sso;

    if (rate >= 0.0f) {
        rate = 1000.0f / rate;
    }
    SafeGetOpts(&sso);
    sso.set.updateperiod = rate;
    SafeSetOpts(&sso);
    return RESULT_OK;
}

Result System::GetUpdateRate(float *prate) {
    SNDSYSOPTS sso;

    SafeGetOpts(&sso);
    *prate = sso.set.updateperiod;
    if (sso.set.updateperiod >= 0.0f) {
        *prate = 1000.0f / sso.set.updateperiod;
    }
    return RESULT_OK;
}
#endif

#ifdef EA_PLATFORM_WIN32
Result System::CapOutputMode(OutputMode mode, bool *psupported) {
    static unsigned char chanmodes[9];
    SNDSYSOPTS sso;
    int result;

    *psupported = false;
    result = pc_snd_81ED1C(&sso, chanmodes);
    if (result < 0) {
        return SndErrToResult(result);
    }
    if (chanmodes[mode] == 1) {
        *psupported = true;
    }
    return RESULT_OK;
}
#else
Result System::CapOutputMode(OutputMode mode, bool *psupported) {
    SNDSYSOPTS sso;

    *psupported = false;
    SafeGetOpts(&sso);

    switch (mode) {
    case OUTPUTMODE_STEREO:
        if (sso.cap.outputchannelsmax > 1) {
            *psupported = true;
        }
        break;
    case OUTPUTMODE_MONO:
    case OUTPUTMODE_PROLOGIC2:
        *psupported = true;
        break;
    case OUTPUTMODE_4POINT0:
    case OUTPUTMODE_5POINT1:
    case OUTPUTMODE_DTS4POINT0:
    case OUTPUTMODE_DTS5POINT1:
        break;
    }

    return RESULT_OK;
}
#endif

#if defined(EA_PLATFORM_PLAYSTATION2) || defined(EA_PLATFORM_WIN32)
Result System::GetOutputModeName(OutputMode mode, const char **pmodename) {
    switch (mode) {
    case OUTPUTMODE_MONO:
        *pmodename = "Mono";
        break;
    case OUTPUTMODE_STEREO:
        *pmodename = "Stereo";
        break;
    case OUTPUTMODE_4POINT0:
        *pmodename = "4.0";
        break;
    case OUTPUTMODE_5POINT1:
        *pmodename = "5.1";
        break;
    case OUTPUTMODE_DTS4POINT0:
        *pmodename = "DTS 4.0";
        break;
    case OUTPUTMODE_DTS5POINT1:
        *pmodename = "DTS 5.1";
        break;
    case OUTPUTMODE_DD5POINT1:
        *pmodename = "Dolby Digital 5.1";
        break;
    case OUTPUTMODE_PROLOGIC:
        *pmodename = "Dolby ProLogic";
        break;
    case OUTPUTMODE_PROLOGIC2:
        *pmodename = "Dolby ProLogic II";
        break;
    default:
        return RESULT_ERR_INVOPT;
    }

    return RESULT_OK;
}
#endif

Result System::SetOutputMode(OutputMode mode) {
    bool modesupported;

    CapOutputMode(mode, &modesupported);
    if (!modesupported) {
        return RESULT_ERR_INVOPT;
    }

    if (sndgs.installed == 0) {
        outputmode = mode;
    } else {
        gOutputModeNext.value = mode;
        gOutputModeNext.changed = 1;
    }

    return RESULT_OK;
}

#ifdef EA_PLATFORM_WIN32
Result System::GetOutputMode(OutputMode *pmode) {
    *pmode = outputmode;
    return RESULT_OK;
}

Result System::CapOutputSampleRate(Device device, int requestedsamplerate, int *pclosestsamplerate) {
    SNDSYSOPTS sso;

    SafeGetOpts(&sso);

    switch (device) {
    case DEVICE_MAIN:
        if (requestedsamplerate < sso.cap.outputratemin) {
            *pclosestsamplerate = sso.cap.outputratemin;
        } else if (requestedsamplerate > sso.cap.outputratemax) {
            *pclosestsamplerate = sso.cap.outputratemax;
        } else {
            *pclosestsamplerate = requestedsamplerate;
        }
        break;
    default:
        return RESULT_ERR_INVOPT;
    }

    return RESULT_OK;
}
#endif

Result System::SetOutputSampleRate(Device device, int samplerate) {
    SNDSYSOPTS sso;

    SafeGetOpts(&sso);

    if (samplerate > 65535) {
        samplerate = 65535;
    }

    switch (device) {
    case DEVICE_MAIN:
        if (sndgs.installed == 0) {
            sso.set.outputrate = samplerate;
        } else {
            gOutputSampleRateNext[device].value = samplerate;
            gOutputSampleRateNext[device].changed = 1;
        }
        break;
#ifdef EA_PLATFORM_PLAYSTATION2
    case DEVICE_IOP:
        if (sndgs.installed == 0) {
            sso.set.outputrateiopcpu = samplerate;
        } else {
            gOutputSampleRateNext[device].value = samplerate;
            gOutputSampleRateNext[device].changed = 1;
        }
        break;
#endif
    default:
        return RESULT_ERR_INVOPT;
    }

    if (sndgs.installed == 0) {
        SafeSetOpts(&sso);
    }

    return RESULT_OK;
}

#ifdef EA_PLATFORM_WIN32
Result System::GetOutputSampleRate(Device device, int *psamplerate) {
    SNDSYSOPTS sso;

    *psamplerate = 0;
    SafeGetOpts(&sso);

    switch (device) {
    case DEVICE_MAIN:
        *psamplerate = sso.set.outputrate;
        break;
    default:
        return RESULT_ERR_INVOPT;
    }

    return RESULT_OK;
}

Result System::CapVoices(Device device, int *pvoices) {
    SNDSYSOPTS sso;

    *pvoices = 0;
    SafeGetOpts(&sso);

    switch (device) {
    case DEVICE_MAIN:
        *pvoices = sso.cap.voicesmaincpumax;
        break;
    default:
        return RESULT_ERR_INVOPT;
    }

    return RESULT_OK;
}
#endif

Result System::SetVoices(Device device, int voices) {
    SNDSYSOPTS sso;

    SafeGetOpts(&sso);

    switch (device) {
    case DEVICE_MAIN:
        if (sndgs.installed == 0) {
            sso.set.voicesmaincpu = voices;
        } else {
            gVoicesNext[device].value = voices;
            gVoicesNext[device].changed = 1;
        }
        break;
#ifdef EA_PLATFORM_PLAYSTATION2
    case DEVICE_IOP:
        if (sndgs.installed == 0) {
            sso.set.voicesiopcpu = voices;
        } else {
            gVoicesNext[device].value = voices;
            gVoicesNext[device].changed = 1;
        }
        break;
    case DEVICE_HARDWARE:
        if (sndgs.installed == 0) {
            sso.set.voicesspu = voices;
        } else {
            gVoicesNext[device].value = voices;
            gVoicesNext[device].changed = 1;
        }
        break;
#elif !defined(EA_PLATFORM_WIN32) // speed.exe 0x81B611: only DEVICE_MAIN
    case DEVICE_HARDWARE:
        if (sndgs.installed == 0) {
            sso.set.voicesdsp = voices;
        } else {
            gVoicesNext[device].value = voices;
            gVoicesNext[device].changed = 1;
        }
        break;
#endif
    default:
        return RESULT_ERR_INVOPT;
    }

    if (sndgs.installed == 0) {
        SafeSetOpts(&sso);
    }

    return RESULT_OK;
}

#ifdef EA_PLATFORM_PLAYSTATION2
Result System::SetSndLoadsIopModules(bool loadIopModules) {
    SNDSYSOPTS sso;

    SafeGetOpts(&sso);
    sso.set.loadiopmodules = loadIopModules;
    SafeSetOpts(&sso);
    return RESULT_OK;
}
#elif defined(EA_PLATFORM_WIN32)
Result System::GetVoices(Device device, int *pvoices) {
    SNDSYSOPTS sso;

    SafeGetOpts(&sso);

    switch (device) {
    case DEVICE_MAIN:
        *pvoices = sso.set.voicesmaincpu;
        break;
    default:
        return RESULT_ERR_INVOPT;
    }

    return RESULT_OK;
}

Result System::SetCompatibilityLevel(CompatibilityMode mode) {
    SNDSYSOPTS sso;

    SafeGetOpts(&sso);
    sso.set.compatibility = mode;
    SafeSetOpts(&sso);
    return RESULT_OK;
}

Result System::SetResampleQuality(Device device, float quality) {
    SNDSYSOPTS sso;

    SafeGetOpts(&sso);

    switch (device) {
    case DEVICE_MAIN:
        sso.set.resamplequality = quality * 100.0f;
        break;
    default:
        return RESULT_ERR_INVOPT;
    }

    SafeSetOpts(&sso);
    return RESULT_OK;
}

Result System::GetResampleQuality(Device device, float *pQuality) {
    SNDSYSOPTS sso;

    SafeGetOpts(&sso);

    switch (device) {
    case DEVICE_MAIN:
        *pQuality = sso.set.resamplequality * 0.01f;
        break;
    default:
        return RESULT_ERR_INVOPT;
    }

    return RESULT_OK;
}

Result System::SetSse(bool useSse) {
    SNDSYSOPTS sso;

    SafeGetOpts(&sso);
    sso.set.usesse = useSse;
    SafeSetOpts(&sso);
    return RESULT_OK;
}

Result System::SetDtsQuality(float quality) {
    return RESULT_ERR_INVOPT;
}

Result System::SetRouteMainFxToIop(bool enable) {
    return RESULT_ERR_INVOPT;
}

Result System::SetRouteHardwareFxToIop(bool enable) {
    return RESULT_ERR_INVOPT;
}

Result System::SetSndLoadsIopModules(bool loadIopModules) {
    return RESULT_ERR_INVOPT;
}

Result System::SetIopModulePath(const char *iopModulePath) {
    return RESULT_ERR_INVOPT;
}

Result System::SetSndInitsAram(bool sndInitsAram) {
    return RESULT_ERR_INVOPT;
}

Result System::SetAbortHandler(void (*abortMessage)(const char *)) {
    SNDSYSOPTS sso;

    SafeGetOpts(&sso);
    sso.vec.abortmsg = (void (*)(char *))abortMessage;
    SafeSetOpts(&sso);
    return RESULT_OK;
}
#else
Result System::SetSndInitsAram(bool sndInitsAram) {
    SNDSYSOPTS sso;

    SafeGetOpts(&sso);
    sso.set.initaram = sndInitsAram;
    SafeSetOpts(&sso);
    return RESULT_OK;
}
#endif

Result System::Init(const int headerversion) {
    int i;
    int result;
    SNDSYSOPTS sso;

    result = SafeGetOpts(&sso);
    if (result < 0) {
        return SndErrToResult(result);
    }

    for (i = 0; i < DEVICE_MAX; i++) {
        gVoicesNext[i].changed = 0;
        gOutputSampleRateNext[i].changed = 0;
    }

    gOutputModeNext.changed = 0;
    sndgs.logmixing = 0;
    sndgs.logcallbackfn = NULL;
#ifdef EA_PLATFORM_WIN32
    sso.set.numrendermodes = 1;
    sso.set.rendermode[0] = 0x24;
#endif

    SNDPLATFORM_ModeSetUp(&sso, outputmode);
    SafeSetOpts(&sso);
    SNDSYSI_init(pheap, heapsize, headerversion);
    return RESULT_OK;
}

Result System::ReInit() {
    SNDPROFVOICES spv;
    SNDSYSOPTS sso;
    StartMode startMode = WARM;
    int i;

    if (sndgs.installed != 1) {
        return RESULT_ERR_INVOPT;
    }

    if (sndgs.aemsstopmodulebanks != NULL) {
        sndgs.aemsstopmodulebanks();
    }
    if (sndgs.aemsstreampurge != NULL) {
        sndgs.aemsstreampurge();
    }

    if (SNDPROFILE_voices(&spv) > 0) {
        return RESULT_ERR_INVOPT;
    }

    for (i = 0; i < gMaxFxBuses; i++) {
        if (!fxBusesLinkList[i].IsEmpty()) {
            return RESULT_ERR_INVOPT;
        }
    }

    SNDI_freespkrtable();
    SNDPLATFORM_restore(startMode);

    SafeGetOpts(&sso);
    SNDSYSI_updatesso(&sso);
    SNDPLATFORM_ModeSetUp(&sso, outputmode);
    sndgs.installed = 0;
    SafeSetOpts(&sso);

    sndgs.installed = 1;
    SNDSYS_entercritical();
    SNDMEMI_free(sndgs.chan);
    SNDSYSI_chanpubinit();
    SNDSYS_leavecritical();
    SNDPLATFORM_init(startMode);

    gTotalOutputChannels = sndgs.sso.set.outputchannels + sndgs.sso.set.outputlfe;
    SNDI_spkrconfig();
    SNDI_precalcaztospkrvol();
    return RESULT_OK;
}

bool System::IsInited() {
    return sndgs.installed == 1;
}

Result System::Restore() {
    SNDSYS_restore();
    pheap = NULL;
    return RESULT_OK;
}

Result Memory::SetHeap(Device device, void *pmem, int size) {
    switch (device) {
    case DEVICE_MAIN:
        pheap = pmem;
        heapsize = size;
        break;

#ifdef EA_PLATFORM_GAMECUBE
    case DEVICE_HARDWARE: {
        SNDSYSOPTS sso;

        SafeGetOpts(&sso);
        sso.set.arampooladdr = (sndptruint)pmem;
        sso.set.arampoolsize = size;
        SafeSetOpts(&sso);
        break;
    }
#endif

    default:
        return RESULT_ERR_INVOPT;
    }

    return RESULT_OK;
}

// PC: speed.exe 0x81B96B. GameCube la compila y la estripa (Range 0xFFFFFFFF en su DWARF), pero la .rodata de
// ssys.cpp en su DOL (4 B) no tiene lo que esta forma deja (switch y constantes): alli su cuerpo no se sabe.
#ifdef EA_PLATFORM_WIN32
Result Memory::GetHeap(Device device, void **ppmem, int *psize) {
    *ppmem = NULL;
    *psize = 0;

    switch (device) {
    case DEVICE_MAIN:
        if (sndgs.installed != 1) {
            return RESULT_ERR_NOTINIT;
        }
        *ppmem = pheap;
        *psize = heapsize;
        break;

    default:
        return RESULT_ERR_INVOPT;
    }

    return RESULT_OK;
}
#endif

Result Memory::SetHeapThreshold(Device device, float threshold) {
    switch (device) {
    case DEVICE_MAIN: {
        SNDSYSOPTS sso;

        SafeGetOpts(&sso);
        sso.set.sndheapthreshold = (unsigned char)(threshold * 100.0f);
        SafeSetOpts(&sso);
        break;
    }

    default:
        return RESULT_ERR_INVOPT;
    }

    return RESULT_OK;
}

// PC: speed.exe 0x81B9E8 (en GameCube, como GetHeap: estripada y sin su 0.01f en la .rodata).
#ifdef EA_PLATFORM_WIN32
Result Memory::GetHeapThreshold(Device device, float *pthreshold) {
    *pthreshold = 0.0f;

    switch (device) {
    case DEVICE_MAIN: {
        SNDSYSOPTS sso;

        SafeGetOpts(&sso);
        *pthreshold = sso.set.sndheapthreshold * 0.01f;
        break;
    }

    default:
        return RESULT_ERR_INVOPT;
    }

    return RESULT_OK;
}
#endif

} // namespace Snd
