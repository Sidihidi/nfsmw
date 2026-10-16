#ifndef FEGROUP_H_
#define FEGROUP_H_

#include "FEObject.h"
#include "types.h"














// File: speed/indep/src/feng/FEGroup.h
// total size: 0x6C
// Decl: speed/indep/src/feng/FEGroup.h:23
class FEGroup : public FEObject {
  private:
    FEMinList Children; // offset 0x5C, size 0x10, Decl: speed/indep/src/feng/FEGroup.h:25

  public:
    FEGroup() { Type = FE_Group; } // Decl: speed/indep/src/feng/FEGroup.h:28
    FEGroup(const FEGroup &Object, bool bCloneChildren, bool bReference);

    FEObject *Clone(bool bReference) override { return FNEW FEGroup(*this, true, bReference); } // Decl: speed/indep/src/feng/FEGroup.h:31

    void AddObject(FEObject *pObj) { Children.AddTail(pObj); } // Decl: speed/indep/src/feng/FEGroup.h:33
    void AddObjectAfter(FEObject *pObj, FEObject *pAddAfter) { // Decl: speed/indep/src/feng/FEGroup.h:34
        Children.AddNode(pAddAfter, pObj);
    }

    void RemoveObject(FEObject *pObj) { // Decl: speed/indep/src/feng/FEGroup.h:36
        Children.RemNode(pObj);
    }

    u32 GetNumChildren() const {
        return Children.GetNumElements();
    }

    FEObject *GetFirstChild() const {
        return static_cast<FEObject *>(Children.GetHead());
    }
    FEObject *GetLastChild() const {
        return static_cast<FEObject *>(Children.GetTail());
    }

    FEObject *FindChild(u32 NameHash) const;
    FEObject *FindChild(const char *pName) const;

    FEObject *FindChildRecursive(u32 NameHash) const;
    FEObject *FindChildRecursive(const char *pName) const;
};

#endif
