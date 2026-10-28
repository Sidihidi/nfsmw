/* VEDA r46 ROTA en la r51.  El orden del `.bss` SI tiene mando, pero no desde
   las definiciones: lo fija la posicion de la PRIMERA declaracion de cada
   nombre en la cadena del espacio de nombres, y `gVariableTimerPeriod` ya
   estaba declarada en `sndcmn.h` antes de que este fichero abra su bloque
   `namespace Snd`.  Por eso las seis permutaciones de las definiciones --y un
   `extern` puesto DESPUES del include-- salian identicas.  Declarando el tick
   ANTES del include entra el primero en la cadena y el volcado final del
   `.lcomm` sale lista, tick, periodo, que es el del objetivo.  */
namespace Snd {
extern int gVariableTimerTick;
};

#include "./sndcmn.h"

namespace Snd {
// La lista sale antes que las otras dos porque tiene constructor y se emite
// junto a su init; las dos POD caen en la volcada final, y el orden de esa
// volcada es el de la cadena del espacio de nombres (ver la nota de arriba).
CListDStack gVariableTimerList;
int gVariableTimerTick;
float gVariableTimerPeriod;
};


void SNDI_mutexlock();
void SNDI_mutexunlock();

void SNDSYSI_variabletimerservice() {
    Snd::VariableTimerClient *pClient;
    /* El objetivo SI lo define: su `.data` son 12 B --lastTick, gMutexLockFn y
       gMutexUnlockFn-- y aqui estaba puesto como `extern` con el nombre del
       troceador. Con inicializador explicito GCC 2.9 lo emite en `.data` aunque
       el valor sea cero; sin el se va a `.bss` y sobran 4 B alli. */
    static unsigned int lastTick = 0;

    if (Snd::gVariableTimerTick != (int)lastTick) {
        lastTick = Snd::gVariableTimerTick;
        pClient = (Snd::VariableTimerClient *)Snd::gVariableTimerList.GetHead();
        while (pClient != 0) {
            CListDNode *pNode = pClient->ln.GetNext();

            pClient->pClientFunc(pClient->pClientData);
            pClient = (Snd::VariableTimerClient *)pNode;
        }
    }
}

#ifdef EA_PLATFORM_PLAYSTATION2
// El reloj del driver de PS2 (sdriver.c lo llama con 94): pone audiotick al dia llamando al servidor de 100 Hz. En
// GameCube no existe (su DWARF salta de SNDSYSI_variabletimerservice a SNDSYSI_100hzserver con las mismas lineas).
void SNDSYSI_timerservice(int timerhertz) {
    if (sndgs.installed) {
        if (timerhertz != sndgs.currenttimerhz) {
            sndgs.currenttimerhz = timerhertz;
            sndgs.audiotick = 0;
            sndgs.servertick = 0;
        }
        sndgs.servertick++;
        {
            unsigned int target = sndgs.servertick * 100 / timerhertz;

            while (sndgs.audiotick <= target) {
                SNDSYSI_100hzserver();
            }
        }
    }
}
#endif

void iSNDserve();

void SNDSYSI_100hzserver() {
    int i;

    sndgs.audiotick++;
    iSNDserve();

    for (i = 0; i < sndgs.numserverclients100hz; i++) {
        sndgs.serverclient100hz[i]();
    }

    for (i = 0; i < sndgs.voicestotal; i++) {
        CHANPUB *pc = &sndgs.chan[i];
        ENVELOPE *pe;
        int recalc;

        if (pc->status != 1) {
            continue;
        }

        if (sndgs.chan[i].handle >= 0 && pc->ppitchlfo != 0) {
            pc->curpitchlfoentry++;
            if (pc->curpitchlfoentry >= pc->pitchlfolength) {
                pc->curpitchlfoentry = 0;
            }
            pc->detunepitch = 0;
            iSNDcalcpitch(i);
            SNDPLATFORM_setpitch(i);
        }

        recalc = 0;

        if (pc->pvollfo != 0) {
            recalc = 1;
            pc->curvollfoentry++;
            if (pc->curvollfoentry >= pc->vollfolength) {
                pc->curvollfoentry = 0;
            }
        }

        if (pc->fadePer100Hz != 0.0f) {
            recalc = 1;
            pc->programmedVol += pc->fadePer100Hz;
            if (pc->fadePer100Hz < 0.0f) {
                if (pc->programmedVol <= pc->fadeTargetVol) {
                    pc->programmedVol = pc->fadeTargetVol;
                    pc->fadePer100Hz = 0.0f;
                }
            } else {
                if (pc->programmedVol >= pc->fadeTargetVol) {
                    pc->programmedVol = pc->fadeTargetVol;
                    pc->fadePer100Hz = 0.0f;
                }
            }
            if (pc->programmedVol < 0.0f) {
                SNDstop(pc->handle);
                continue;
            }
        }

        pc->envduration--;
        if (pc->envpertick != 0) {
            recalc = 1;
            pc->envvol += pc->envpertick;
        }

        if (pc->envduration == 0) {
            pc->curenvelope++;
            if (pc->curenvelope >= pc->numenvelopes) {
                SNDstop(pc->handle);
                continue;
            }
            pe = &pc->paenvelope[pc->curenvelope];
            pc->envduration = pe->duration;
            if (pc->envduration < 0) {
                pc->envduration = 0x7FFFFFFF;
            }
            pc->envpertick = ((pe->targetvol << 16) - pc->envvol) / pc->envduration;
        }

        if (recalc) {
            iSNDcalcvol(i);
            Snd::Hal::SetVol(i);
        }
    }
}


namespace Snd {

void DefaultMutexLockFn() {
    SNDI_mutexlock();
}

void DefaultMutexUnlockFn() {
    SNDI_mutexunlock();
}

void (*gMutexLockFn)() = DefaultMutexLockFn;
void (*gMutexUnlockFn)() = DefaultMutexUnlockFn;

} // namespace Snd

#ifdef EA_PLATFORM_XENON
// Xenon (EGM 0x8260D998/0x8260D9C8): la seccion critica no va por los
// punteros anteriores. Lee el puntero global de 0x82A7F9F0 al nucleo snd de
// Xenon (lo guarda su constructor, 0x8260A108: pide 0xD4 B, crea dos
// EA::Thread::Mutex incrustados en +0x78 y +0xA8 y guarda el primero en
// +0x5C); si el campo de funcion de +0x54/+0x58 esta a cero hace
// Lock(kTimeoutNone)/Unlock() de ese Mutex (Unlock expandido:
// --mnLockCount en +0x20 y RtlLeaveCriticalSection) y si no llama a la
// funcion. Sin el incritical++ de las demas plataformas.
// El tipo es el de sndcmn.h (con el asignador de +0x20, que usan las inline
// de SNDMEMI_allocz/SNDMEMI_free).
#include "eathread/eathread_mutex.h"

XenonUnknown82A7F9F0_t *XenonUnknown82A7F9F0;

static const EA::Thread::ThreadTime kSndTimeoutNone = 0xFFFFFFFF;

void SNDSYS_entercritical() {
    // El acceso va por un local: la copia suelta (0x8260D998) sale igual de
    // las dos formas, pero al fundirse en los llamantes el local es el que
    // da el reparto del EGM — en MIX_create, el prologo entero
    // (0x82617590-0x826175EC: la copia de MIXCONFIG en r6/r5/r4 con
    // `mr r3, r31` y la tienda de SetMemCpy de 0x826175E4 delante del
    // `lwz r10, 0x54(r11)`); con el acceso directo repetido, el asignador
    // del cl de X360 aparca la segunda palabra de la copia en un
    // callee-saved y esa tienda cae detras del `cmplwi`.
    XenonUnknown82A7F9F0_t *p = XenonUnknown82A7F9F0;

    if (!p->enterFn) {
        p->mutex->Lock(kSndTimeoutNone);
    } else {
        p->enterFn();
    }
}

void SNDSYS_leavecritical() {
    if (!XenonUnknown82A7F9F0->leaveFn) {
        XenonUnknown82A7F9F0->mutex->Unlock();
    } else {
        XenonUnknown82A7F9F0->leaveFn();
    }
}
#else
void SNDSYS_entercritical() {
    Snd::gMutexLockFn();
    sndgs.incritical++;
}

void SNDSYS_leavecritical() {
    sndgs.incritical--;
    Snd::gMutexUnlockFn();
}
#endif
