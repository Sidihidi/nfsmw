#ifndef INTERFACES_IMENUZONETRIGGER_H
#define INTERFACES_IMENUZONETRIGGER_H

#ifdef EA_PRAGMA_ONCE_SUPPORTED
#pragma once
#endif

#include "Speed/Indep/Libs/Support/Utility/UCOM.h"

class GRuntimeInstance;

// total size: 0x8
class IMenuZoneTrigger : public UTL::COM::IUnknown {
  public:
    static HINTERFACE _IHandle() {
        return (HINTERFACE)_IHandle;
    }

    IMenuZoneTrigger(UTL::COM::Object *owner) : UTL::COM::IUnknown(owner, _IHandle()) {}

    virtual ~IMenuZoneTrigger() {}

    virtual bool ShouldSeeMenuZoneCluster() = 0;
    virtual bool IsPlayerInsideTrigger() = 0;
    virtual bool IsType(const char *type) = 0;
    // La A124 avisa al disparador del autoguardado y no tiene el logo de Cingular (su vtable, en los stabs).
#ifdef EA_BUILD_A124
    virtual void EnterTriggerForAutoSave() = 0;
    virtual void ExitTriggerForAutoSave() = 0;
#endif
    virtual void EnterTrigger(GRuntimeInstance *pRaceActivity) = 0;
    virtual void EnterTrigger(const char *pName) = 0;
    virtual void ExitTrigger() = 0;
#ifndef EA_BUILD_A124
    virtual void RequestCingularLogo() = 0;
#endif
    virtual void RequestEventInfoDialog(int index) = 0;
    virtual void RequestZoneInfoDialog(int index) = 0;
    virtual void RequestDoAction() = 0;
};

#endif
