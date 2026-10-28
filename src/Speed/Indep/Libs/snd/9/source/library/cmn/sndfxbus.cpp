#include "Speed/Indep/Libs/snd/9/source/library/cmn/sndcmn.h"
#include "Speed/Indep/Libs/snd/9/source/library/cmn/sndenum.h"
#include "Speed/Indep/Libs/snd/9/source/library/cmn/sndfxcmn.h"
#include "csis/csis.h"
#include "snd/sndo.h"
#include <cstring>

namespace Snd {

GlobalFxProcessor::~GlobalFxProcessor() {
    GlobalFxProcessorData *pGlobalFxProcessorData = reinterpret_cast<GlobalFxProcessorData *>(this);
#ifdef EA_PLATFORM_XENON
    // Xenon: la comparacion del tag es con signo (`cmpw cr6, r11, r9` en
    // 0x826155AC, como la de Reset en 0x826158C8); el cast a unsigned la
    // vuelve `cmplw` en el cl de X360.
    if (pGlobalFxProcessorData != NULL && pGlobalFxProcessorData->fxBusTag == 0x46584253) {
#else
    if (pGlobalFxProcessorData != NULL && (unsigned int)pGlobalFxProcessorData->fxBusTag == 0x46584253) {
#endif
        if (!pGlobalFxProcessorData->isReset) {
            this->Reset();
        }

        SNDI_memclear(pGlobalFxProcessorData, sizeof(GlobalFxProcessorData));

        SNDSYS_entercritical();
        SNDMEMI_free(this);
        SNDSYS_leavecritical();
    }
}

#ifdef EA_PLATFORM_XENON
Csis::Result GlobalFxProcessor::CreateInstance(Device device, GlobalFxProcessor **ppGlobalFxProcessor) {
#else
Csis::Result GlobalFxProcessor::CreateInstance(Device device, int bus, GlobalFxProcessor **ppGlobalFxProcessor) {
#endif
    int i;

    SNDSYS_entercritical();
    GlobalFxProcessorData *pGlobalFxProcessorData = reinterpret_cast<GlobalFxProcessorData *>(
        SNDMEMI_allocz(sizeof(GlobalFxProcessorData))
    );
    SNDSYS_leavecritical();

    SNDI_memclear(pGlobalFxProcessorData, sizeof(GlobalFxProcessorData));

    if (pGlobalFxProcessorData == NULL) {
        return Csis::RESULT_ERR_ALLOCATE;
    }

    pGlobalFxProcessorData->fxBusTag = 0x46584253;
#ifdef EA_PLATFORM_XENON
    pGlobalFxProcessorData->busId = 0;
#else
    pGlobalFxProcessorData->busId = bus;
#endif
    for (i = 0; i < 6; i++) {
        pGlobalFxProcessorData->fxBusOutputLevel[i] = 1.0f;
    }
#ifndef EA_PLATFORM_XENON
    // Xenon: sin campo `device` (ver sndfxcmn.h); su CreateInstance
    // (fn_826156B8) pasa de los niveles al `stw r5, 0x20(r31)` del
    // isReset de 0x826157C4 sin tienda intermedia.
    pGlobalFxProcessorData->device = device;
#endif
    pGlobalFxProcessorData->isReset = 1;

    *ppGlobalFxProcessor = reinterpret_cast<GlobalFxProcessor *>(pGlobalFxProcessorData);

    return Csis::RESULT_OK;
}

Csis::Result GlobalFxProcessor::Release() {
    this->~GlobalFxProcessor();
    return Csis::RESULT_OK;
}

Csis::Result GlobalFxProcessor::SetCustom(void *pFxDefinition) {
    GlobalFxProcessorData *mp = reinterpret_cast<GlobalFxProcessorData *>(this);
    if (!mp->isReset) {
        this->Reset();
    }
    mp->instanceHandle = Snd::Hal::SetCustomFx(mp, pFxDefinition);
    mp->isReset = 0;

    return Csis::RESULT_OK;
}

Csis::Result GlobalFxProcessor::Reset() {
    GlobalFxProcessorData *mp = reinterpret_cast<GlobalFxProcessorData *>(this);
    if (mp->fxBusTag == 0x46584253) {
        if (!mp->isReset) {
#ifdef EA_PLATFORM_XENON
            // Xenon: el Reset del EGM (fn_826158A8, al que llama el
            // destructor en 0x826155C0) ignora el retorno del HAL y
            // guarda un 0 (`li r7, 0` / `stw r7, 0x24(r31)` en
            // 0x826158E0-0x826158E4, tras el `bl` de 0x826158DC); el
            // Hal::Reset de Xenon (fn_82618C30) ni siquiera deja un
            // retorno en r3 (lo recarga para el leavecritical en
            // 0x82618C94). En GC si se guarda (`stw r3, 0x28(r31)` en
            // 0x8036073C).
            Snd::Hal::Reset(mp);
            mp->instanceHandle = 0;
#else
            mp->instanceHandle = Snd::Hal::Reset(mp);
#endif
        }
        mp->isReset = 1;
    }

    return Csis::RESULT_OK;
}

// TODO: This match feels kinda bad. Are we really keeping rodata from a
//      stripped function?
Csis::Result GlobalFxProcessor::SetOutputLevel(float level) {
    int i;
    GlobalFxProcessorData *mp = reinterpret_cast<GlobalFxProcessorData *>(this);

    if (level > 1.0f) {
        level = 1.0f;
    } else if (level < 0.0f) {
        level = 0.0f;
    }

    for (i = 0; i < 6; i++) {
        mp->fxBusOutputLevel[i] = level;
    }

    return Csis::RESULT_OK;
}

}
