#ifndef FEMODEL_H_
#define FEMODEL_H_

#include "Speed/Indep/Src/FEng/FEObject.h"












// File: speed/indep/src/feng/FEModel.h
// total size: 0x44
// Decl: speed/indep/src/feng/FEModel.h:20
class FEModelData : public FEObjData {};








// total size: 0x5C
// Decl: speed/indep/src/feng/FEModel.h:31
class FEModel : public FEObject {
  public:
    FEModel() { Type = FE_Model; } // Decl: speed/indep/src/feng/FEModel.h:33
    FEModel(const FEModel &Model, bool bReference) : FEObject(Model, bReference) {}
    ~FEModel() override {}

    FEObject *Clone(bool bReference) override { return FNEW FEModel(*this, bReference); } // Decl: speed/indep/src/feng/FEModel.h:37

    FEModelData *GetModelData() { return reinterpret_cast<FEModelData *>(pData); } // Decl: speed/indep/src/feng/FEModel.h:39
};

#endif
