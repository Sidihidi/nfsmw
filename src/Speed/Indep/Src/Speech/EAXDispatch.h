#ifndef SPEECH_EAXDISPATCH_H
#define SPEECH_EAXDISPATCH_H

#ifdef EA_PRAGMA_ONCE_SUPPORTED
#pragma once
#endif

#include "EAXCharacter.h"
#include "Speed/Indep/Src/EAXSound/SND_GEN/COPSPEECH.hpp"

class EAXCop;

// total size: 0x40
// Decl: 24
class EAXDispatch : public EAXCharacter {
    // PC: ??0EAXDispatch@@AAE@H@Z (private); solo lo construye SoundAI (new EAXDispatch(1) en SoundAI.cpp).
    friend class SoundAI;
    EAXDispatch(int sID) : EAXCharacter(sID, nullptr, 0, 0) {}

  public:
    ~EAXDispatch() override;

    void Update() override;

    void BackupReply(EAXCop *cop, int yes, int type);
    void ArrestReply();
    void PursuitUpdate(EAXCop *cop);
    void PursuitEscalationGeneric();
    void PursuitEscalation();
    void BackupUpdate(EAXCop *cop, int yes);
    void BreakAway();
    void GoAhead();
    void TimeExpired();
    void Report911(Csis::Type_pursuit_type infraction);
    void RBUpdate(EAXCop *cop, signed char true_false);
    void RBReply(EAXCop *cop, signed char true_false, unsigned int type);
    void JurisShift(Csis::Type_jurisdiction jurisdiction);
    void BackupETA();
    void VehicleDescription();
    void NoVehicleDescription();
    // EVReply, VehicleVinyls y VehicleCustomPaint NO existen en el ELF (ni el
    // simbolo ni las tres instanciaciones de ScheduleSpeech que arrastraban).
    // Eran codigo anadido en la descompilacion; fuera.
    // gcdatos6: ojo, si tienen DIEs concretos de definicion en la CU zSpeech de GC
    // (low_pc=-1, locales data / ai+data+color+pcar / ai+data+pcar e inlines) y
    // stabs en la A124 (56/104/80 B): son funciones muertas del original. Escritas
    // con el esqueleto de sus DIEs y MEDIDAS x5 por duplicado: codigo/fn y datos GC
    // identicos, pero la .rodata de zSpeech en PS2 baja (A124 17,1051->17,0449,
    // SLUS 15,7812->15,6900) sin ganancia en GC. Revertidas; no reintentar sin
    // encontrar la forma que EE-GCC interna igual que el original.
    void SubRBReply();
};

#endif
