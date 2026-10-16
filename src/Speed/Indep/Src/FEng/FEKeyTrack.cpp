#include "Speed/Indep/Src/FEng/FEKeyTrack.h"
#include "Speed/Indep/Src/FEng/FERefList.h"
#include "Speed/Indep/Src/FEng/ObjectPool.h"
#include "Speed/Indep/Src/FEng/FEKeyInterpolators.h"
#include "Speed/Indep/Src/FEng/FETypeNode.h"
#include "types.h"

ObjectPool<FEKeyNode, 256> FEKeyNode::NodePool;

void FEKeyTrack::operator=(FEKeyTrack &Src) {
    DeltaKeys.Purge();

    ParamType = Src.ParamType;
    ParamSize = Src.ParamSize;
    InterpType = Src.InterpType;
    InterpAction = Src.InterpAction;
    Length = Src.Length;
    LongOffset = Src.LongOffset;
    BaseKey.tTime = Src.BaseKey.tTime;
    BaseKey.Val = Src.BaseKey.Val;

    if (Src.IsReference()) {
        DeltaKeys.ReferenceList(Src.DeltaKeys.GetRefSource());

    } else {
        FEKeyNode *pSrcKey = Src.GetFirstDeltaKey();
        while (pSrcKey) {
            FEKeyNode *pKey = new FEKeyNode();
            pKey->tTime = pSrcKey->tTime;
            pKey->Val = pSrcKey->Val;
            DeltaKeys.AddTail(pKey);
            pSrcKey = pSrcKey->GetNext();
        }
    }
}

// AUSENTE del objetivo: zFEng.o del original no trae este simbolo y ninguna de
// las 526 unidades lo referencia. El cuerpo vacio emitia un `blr` de 4 B que
// objdiff no compara y que sobraba en .text.
// u32 FEKeyTrack::ComputeSize() {}

void *FEKeyNode::operator new(size_t) {
    FEKeyNode *pNode = NodePool.AllocSingle();
    return pNode;
}

void FEKeyNode::operator delete(void *pNode) {
    static_cast<FEKeyNode *>(pNode)->~FEKeyNode();
    NodePool.FreeSingle(static_cast<FEKeyNode *>(pNode));
}

FEKeyNode *FEKeyTrack::GetKeyAt(i32 tTime) {
    if (tTime < 0) {
        return GetBaseKey();
    }

    FEKeyNode *pKey = GetFirstDeltaKey();

    if (!pKey) {
        return GetBaseKey();
    }

    while (pKey->GetNext()) {
        if (pKey->GetKeyData()->tTime >= tTime) {
            break;
        }

        pKey = pKey->GetNext();
    };

    return pKey;
}

FEKeyNode *FEKeyTrack::GetDeltaKeyAt(i32 tTime) {
    FEKeyNode *pKey = GetFirstDeltaKey();
    if (!pKey) {
        return nullptr;
    }

    while (pKey->GetNext()) {
        if (pKey->GetKeyData()->tTime >= tTime) {
            break;
        }

        pKey = pKey->GetNext();
    };

    return pKey;
}

void FEKeyTrack::InsertKey(FEKeyNode *pNewKey) {
    FEKeyNode *pKey = GetFirstDeltaKey();

    if (pKey && pNewKey->tTime < pKey->tTime) {
        DeltaKeys.AddHead(pNewKey);
        return;
    }

    while (pKey && pKey->GetNext() && pKey->GetNext()->GetKeyData()->tTime <= pNewKey->GetKeyData()->tTime) {
        pKey = pKey->GetNext();
    }
    DeltaKeys.AddNode(pKey, pNewKey);
}

FEKeyNode *FEKeyTrack::CreateKeyAt(i32 tTime) {
    FEKeyNode *pKey = GetFirstDeltaKey();

    while (pKey && pKey->GetKeyData()->tTime < tTime) {
        pKey = pKey->GetNext();
    }

    if (pKey && pKey->GetKeyData()->tTime == tTime) {
        return pKey;
    }

    pKey = new FEKeyNode;
    pKey->GetKeyData()->tTime = tTime;
    InsertKey(pKey);
    return pKey;
}

bool FEKeyTrack::GenerateNewKeyAt(i32 tTime) {
    FEKeyNode *pKey = GetKeyAt(tTime);

    if (!pKey || pKey->GetKeyData()->tTime == tTime) {
        return false;
    }

    switch (ParamType) {
        case PT_Bool:
            break;
        case PT_Int: {
            i32 Val;
            FEKeyInterp(this, tTime, &Val);
            return SetKeyValueAt(tTime, Val);
        }
        case PT_Vector2: {
            FEVector2 Val;
            FEKeyInterp(this, tTime, &Val);
            return SetKeyValueAt(tTime, Val);
        }
        case PT_Vector3: {
            FEVector3 Val;
            FEKeyInterp(this, tTime, &Val);
            return SetKeyValueAt(tTime, Val);
        }
        case PT_Quaternion: {
            FEQuaternion Val;
            FEKeyInterp(this, tTime, &Val);
            return SetKeyValueAt(tTime, Val);
        }
        case PT_Color: {
            FEColor Val;
            FEKeyInterp(this, tTime, &Val);
            return SetKeyValueAt(tTime, Val);
        }
    }
    return false;
}

bool FEKeyTrack::SetKeyValueAt(i32 tTime, i32 &Val) {
    FEKeyNode *pKey;
    bool bCreated = false;

    if (tTime == -1) {
        BaseKey.Val = Val;
    } else {
        pKey = GetKeyAt(tTime);
        if (pKey->GetKeyData()->tTime != tTime) {
            bCreated = true;
            pKey = CreateKeyAt(tTime);
        }
        pKey->Val = Val - reinterpret_cast<i32 &>(BaseKey.Val);
    }
    InterpAction &= 0x7F;
    return bCreated;
}

bool FEKeyTrack::SetKeyValueAt(i32 tTime, FEVector3 &Val) {
    FEKeyNode *pKey;
    bool bCreated = false;

    if (tTime == -1) {
        BaseKey.Val = Val;
    } else {
        pKey = GetKeyAt(tTime);
        if (pKey->GetKeyData()->tTime != tTime) {
            bCreated = true;
            pKey = CreateKeyAt(tTime);
        }
        pKey->Val = Val - reinterpret_cast<FEVector3 &>(BaseKey.Val);
    }
    InterpAction &= 0x7F;
    return bCreated;
}

bool FEKeyTrack::SetKeyValueAt(i32 tTime, FEVector2 &Val) {
    FEKeyNode *pKey;
    bool bCreated = false;

    if (tTime == -1) {
        BaseKey.Val = Val;
    } else {
        pKey = GetKeyAt(tTime);
        if (pKey->GetKeyData()->tTime != tTime) {
            bCreated = true;
            pKey = CreateKeyAt(tTime);
        }
        pKey->Val = Val - reinterpret_cast<FEVector2 &>(BaseKey.Val);
    }
    InterpAction &= 0x7F;
    return bCreated;
}

bool FEKeyTrack::SetKeyValueAt(i32 tTime, FEColor &Val) {
    FEKeyNode *pKey;
    bool bCreated = false;

    if (tTime == -1) {
        BaseKey.Val = Val;
    } else {
        pKey = GetKeyAt(tTime);
        if (pKey->GetKeyData()->tTime != tTime) {
            bCreated = true;
            pKey = CreateKeyAt(tTime);
        }
        pKey->Val = Val - reinterpret_cast<FEColor &>(BaseKey.Val);
    }
    InterpAction &= 0x7F;
    return bCreated;
}

bool FEKeyTrack::SetKeyValueAt(i32 tTime, FEQuaternion &Val) {
    FEKeyNode *pKey;
    bool bCreated = false;

    if (tTime == -1) {
        BaseKey.Val = Val;
    } else {
        pKey = GetKeyAt(tTime);
        if (pKey->tTime != tTime) {
            bCreated = true;
            pKey = CreateKeyAt(tTime);
        }
        {
            FEQuaternion RefQuat = *static_cast<FEQuaternion *>(BaseKey.GetKeyData()->Val);
            RefQuat.Conjugate();
            pKey->Val = Val * RefQuat;
        }
    }
    InterpAction &= 0x7F;
    return bCreated;
}
