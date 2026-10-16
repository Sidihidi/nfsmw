#ifndef FEOBJECTCALLBACK_H_
#define FEOBJECTCALLBACK_H_

// c3: el objetivo importa _vt.16FEObjectCallback y _._16FEObjectCallback en
// zFEng y zFe2; solo zFe los DEFINE, y como GLOBAL (no weak). Eso es la firma
// de #pragma interface + #pragma implementation en zFe.
// En PS2 no: la A124 llama a su inicializador estatico de zFe _GLOBAL_$I$gOnlineMainMenu (el primer global es la
// variable de uiMain.cpp), emite _$_16FEObjectCallback en el lote final de inline diferidas y no emite el
// constructor. Con #pragma interface + implementation EE-GCC emite el destructor el primero (el nombre pasa a
// _GLOBAL_$I$_$_16FEObjectCallback) y el constructor fuera de linea. SLUS igual: su inicializador mide 0xB0.
#ifndef EA_PLATFORM_PLAYSTATION2
#pragma interface
#endif

#include "FEObject.h"

// File: speed/indep/src/feng/FEObjectCallback.h
// total size: 0x4
// Decl: speed/indep/src/feng/FEObjectCallback.h:20
class FEObjectCallback {
  public:
    FEObjectCallback() {}
    virtual ~FEObjectCallback() {} // Decl: speed/indep/src/feng/FEObjectCallback.h:20
    virtual bool Callback(FEObject *obj) = 0;
};

static const u32 DEFAULT_NUM_FEOBJECTS = 1024; // size: 0x4, Decl: speed/indep/src/feng/FEObjectSorter.h:30

#endif
