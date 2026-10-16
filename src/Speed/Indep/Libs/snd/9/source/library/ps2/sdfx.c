// Los efectos del driver de PS2 (C:/packages/snd/9.06.00-ps2/source/library/ps2/sdfx.c): la reverberacion del SPU2 (un
// tipo de su lista, con la memoria que cada uno necesita al final de los 2 MB), la del IOP (una definicion que se le
// manda por la RPC) y la de la CPU principal (el mezclador).

#include "sndps2.h"
#include "../cmn/sndfxcmn.h"
#include "../mix/smixi.h"

int hwFxBusID = -1;

// La memoria de trabajo de cada tipo de reverberacion del SPU2 (apagada, sala, estudio A, B y C, auditorio, espacio,
// eco, retardo y tuberia), mas 0x20000 de margen.
int snddrvfxsizes[10] = {0x10,    0x226C0, 0x21F40, 0x24840, 0x26FE0,
                         0x2ADE0, 0x2F6C0, 0x38040, 0x38040, 0x23C00};

void SNDDRV_setiopfx(void *pFxDefinition) {
    if (sndgs.sso.set.routemaincpufxtoiopcpu) {
        sndmix.fxaddiop = SNDMIXI_fxadd;
        *sndmix.wetbusactive = 1;
    }

    SNDDRV_rpc(2, pFxDefinition, SNDI_getb((char *)pFxDefinition + 4, 4));
}

void SNDDRV_setiopvol(void *pData) {
    if (snddrv.peetoiop) {
        snddrv.peetoiop->effectOutputLevelIop =
            SNDI_ftoitruncpositive(((GlobalFxProcessorData *)pData)->fxBusOutputLevel[0] * 127.0f);
    }
}

void SNDDRV_setspufxmem(int type) {
    snddrv.mm.reverbaddr = (0x200000 - snddrvfxsizes[type]) >> 6;
}

void SNDDRV_setspuvol(void *pData) {
    if (snddrv.peetoiop) {
        snddrv.peetoiop->effectvolleft = ((GlobalFxProcessorData *)pData)->fxBusOutputLevel[0] * 32767.0f;
        snddrv.peetoiop->effectvolright = ((GlobalFxProcessorData *)pData)->fxBusOutputLevel[1] * 32767.0f;
    }
}

int Snd::Hal::SetPresetFx(void *pData, FxPreset fxPreset) {
    signed char type;

    hwFxBusID = ((GlobalFxProcessorData *)pData)->busId;

    SNDSYS_entercritical();

    switch (fxPreset) {
    case FXPRESET_REVERB_CONCERTHALL:
        type = 5;
        break;
    case FXPRESET_REVERB_ROOM:
    case FXPRESET_REVERB_BATHROOM:
    case FXPRESET_REVERB_LIVINGROOM:
    case FXPRESET_REVERB_CARPETEDHALLWAY:
        type = 1;
        break;
    case FXPRESET_REVERB_ARENA:
    case FXPRESET_REVERB_HANGAR:
    case FXPRESET_REVERB_STONECORRIDOR:
        type = 6;
        break;
    case FXPRESET_REVERB_STONEROOM:
    case FXPRESET_REVERB_AUDITORIUM:
    case FXPRESET_REVERB_CAVE:
    case FXPRESET_REVERB_QUARRY:
        type = 4;
        break;
    case FXPRESET_REVERB_HALLWAY:
    case FXPRESET_REVERB_ALLEY:
    case FXPRESET_REVERB_PARKINGLOT:
        type = 3;
        break;
    case FXPRESET_REVERB_SEWERPIPE:
        type = 9;
        break;
    case FXPRESET_REVERB_GENERIC:
    case FXPRESET_REVERB_PADDEDCELL:
    case FXPRESET_REVERB_FOREST:
    case FXPRESET_REVERB_CITY:
    case FXPRESET_REVERB_MOUNTAINS:
    case FXPRESET_REVERB_PLAIN:
    case FXPRESET_REVERB_UNDERWATER:
        type = 2;
        break;
    default:
        type = 0;
        break;
    }

    SNDDRV_setspufxmem(type);

    snddrv.peetoiop->effecttype = type;
    snddrv.peetoiop->effectdelay = 0;
    snddrv.peetoiop->effectfeedback = 0;

    SNDDRV_setspuvol(pData);

    SNDSYS_leavecritical();

    return 0;
}

unsigned int Snd::Hal::SetCustomFx(void *pData, void *pFxDefinition) {
    GlobalFxProcessorData *pfxData = (GlobalFxProcessorData *)pData;
    unsigned int retVal = 0;

    SNDSYS_entercritical();

    if (pfxData->device == DEVICE_MAIN) {
        retVal = MIX_initreverb(sndgs.sso.set.outputrate, pfxData->busId, pFxDefinition);
    } else if (pfxData->device == DEVICE_IOP) {
        SNDDRV_setiopfx(pFxDefinition);
        if (sndgs.sso.set.routemaincpufxtoiopcpu) {
            MIX_inbufInit(pfxData->busId);
        }
        SNDDRV_setiopvol(pData);
    }

    SNDSYS_leavecritical();

    return retVal;
}

unsigned int Snd::Hal::Reset(void *pData) {
    GlobalFxProcessorData *pfxData = (GlobalFxProcessorData *)pData;
    int playloc;
    unsigned int retVal = 0;
    static unsigned char fxOff[4] = {'F', 'X', 0x0A, 0x00};

    SNDSYS_entercritical();

    if (pfxData->device == DEVICE_MAIN) {
        playloc = SND_PLAYLOC_MAINCPU;
    } else if (pfxData->device == DEVICE_IOP) {
        playloc = SND_PLAYLOC_IOPCPU;
    } else if (pfxData->device == DEVICE_HARDWARE) {
        playloc = SND_PLAYLOC_SPU;
    } else {
        return SNDERR_NO_DRIVER;
    }

    if (playloc == SND_PLAYLOC_MAINCPU) {
        retVal = MIX_restorereverb(pfxData->busId, pfxData->instanceHandle);
    } else if (playloc == SND_PLAYLOC_IOPCPU) {
        SNDDRV_rpc(2, fxOff, 4);
        if (sndgs.sso.set.routemaincpufxtoiopcpu) {
            sndmix.fxaddiop = 0;
            MIX_inbufRestore(pfxData->busId);
        }
    } else {
        SNDDRV_setspufxmem(0);
        snddrv.peetoiop->effecttype = 0;
        hwFxBusID = -1;
    }

    SNDSYS_leavecritical();

    return retVal;
}
