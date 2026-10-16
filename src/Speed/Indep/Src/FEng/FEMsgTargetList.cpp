#include "Speed/Indep/Src/FEng/FEMsgTargetList.h"
#include "Speed/Indep/Src/FEng/FEObject.h"
#include "Speed/Indep/Src/FEng/FEObjectCallback.h"
#include "Speed/Indep/Src/FEng/FEngStandard.h"
#include "Speed/Indep/Src/FEng/FEPackage.h"





void FEMsgTargetList::Allocate(u32 NewAlloc) {
    if (NewAlloc == 0) {

        if (pTargets) {
            delete[] reinterpret_cast<char *>(pTargets);
        }
        pTargets = nullptr;
        Count = 0;
        Alloc = 0;
    } else if (NewAlloc != Alloc) {


        FEObject **pNewTargets = FNEW FEObject *[NewAlloc];
        if (NewAlloc < Alloc) {
            FEngMemCpy(pNewTargets, pTargets, NewAlloc * sizeof(FEObject *));
        } else {
            FEngMemCpy(pNewTargets, pTargets, Alloc * sizeof(FEObject *));
        }
        if (pTargets) {
            delete[] reinterpret_cast<char *>(pTargets);
        }
        pTargets = pNewTargets;
    }
    Alloc = NewAlloc;
}






void FEMsgTargetList::AppendTarget(FEObject *pObject) {
    if (Count == Alloc) {
        Allocate(Count + 1);
    }
    pTargets[Count++] = pObject;
}


static void GrowArrayBy(u32 *&pArray, u32 &Size, u32 GrowBy) {
    u32 *pNewList;
    u32 NewSize;

    NewSize = Size + GrowBy;
    pNewList = FNEW u32[NewSize];
    FEngMemCpy(pNewList, pArray, Size * sizeof(u32));
    if (pArray) { delete[] pArray; }
    pArray = pNewList;
}


static u32 MsgIndex(u32 *pList, u32 Size, u32 MsgID) {
    for (u32 i = 0; i < Size; i++) {

        if (pList[i] == MsgID) {
            return i;
        }
    }
    return 0xFFFFFFFF;
}





// File: speed/indep/src/feng/FEMsgTargetList.cpp
// total size: 0x14
// Decl: speed/indep/src/feng/FEMsgTargetList.cpp:79
class FECountMsgs : public FEObjectCallback {
  public:
    u32 *pMsgList, *pCountList; // offset 0x4, size 0x4, Decl: speed/indep/src/feng/FEMsgTargetList.cpp:81
    u32 MsgCount, MsgAlloc;     // offset 0xC, size 0x4, Decl: speed/indep/src/feng/FEMsgTargetList.cpp:82

    FECountMsgs() {}           // Decl: speed/indep/src/feng/FEMsgTargetList.cpp:84
    ~FECountMsgs() override { if (pMsgList) { delete[] pMsgList; } if (pCountList) { delete[] pCountList; } } // Decl: speed/indep/src/feng/FEMsgTargetList.cpp:85

    bool Callback(struct FEObject *pObj) override;
};


// Decl: speed/indep/src/feng/FEMsgTargetList.cpp:91
bool FECountMsgs::Callback(FEObject *pObj) {
    FEMessageResponse *pResp;
    u32 Index;

    pResp = pObj->GetFirstResponse();
    while (pResp) {

        Index = MsgIndex(pMsgList, MsgCount, pResp->GetMsgID());
        if (Index == 0xFFFFFFFF) {

            if (MsgCount == MsgAlloc) {

                GrowArrayBy(pMsgList, MsgAlloc, 32);
                GrowArrayBy(pCountList, MsgAlloc, 32);
                MsgAlloc += 32;
            }

            pMsgList[MsgCount] = pResp->GetMsgID();
            pCountList[MsgCount] = 1;
            MsgCount++;
        } else {

            pCountList[Index]++;
        }
        pResp = pResp->GetNext();
    }
    return true;
}







// total size: 0xC
// Decl: speed/indep/src/feng/FEMsgTargetList.cpp:129
class FEBuildMsgs : public FEObjectCallback {
  public:
    FEMsgTargetList *pList; // offset 0x4, size 0x4, Decl: speed/indep/src/feng/FEMsgTargetList.cpp:131
    u32 Count;              // offset 0x8, size 0x4, Decl: speed/indep/src/feng/FEMsgTargetList.cpp:132

    FEBuildMsgs() {}
    bool Callback(struct FEObject *pObj) override;
};
// Decl: speed/indep/src/feng/FEMsgTargetList.cpp:137
bool FEBuildMsgs::Callback(FEObject *pObj) {
    FEMessageResponse *pResp;
    u32 i;

    pResp = pObj->GetFirstResponse();
    while (pResp) {

        for (i = 0; i < Count; i++) {

            if (pList[i].GetMsgID() == pResp->GetMsgID()) {

                pList[i].AppendTarget(pObj);
                break;
            }
        }

        pResp = pResp->GetNext();
    }

    return true;
}


































// Decl: speed/indep/src/feng/FEMsgTargetList.cpp:195
// Range: 0x801856DC -> 0x8018571C
// this: r3
FEMsgTargetList *FEPackage::GetMessageTargets(u32 MsgID) {
    for (u32 i = 0; i < NumMsgTargets; i++) {
        if (pMsgTargets[i].GetMsgID() == MsgID) {
            return &pMsgTargets[i];
        }
    }
    return nullptr;
}
