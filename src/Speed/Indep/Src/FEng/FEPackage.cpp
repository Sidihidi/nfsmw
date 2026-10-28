#include "Speed/Indep/Src/FEng/FEPackage.h"
#include "Speed/Indep/Src/FEng/FEGroup.h"
#include "Speed/Indep/Src/FEng/FEKeyInterpolators.h"
#include "Speed/Indep/Src/FEng/FEObject.h"
#include "Speed/Indep/Src/FEng/FEngine.h"
#include "Speed/Indep/Src/FEng/FETypes.h"
#include "Speed/Indep/Src/FEng/FEngStandard.h"
#include "Speed/Indep/Src/FEng/FEListBox.h"
#include "Speed/Indep/Src/FEng/FECodeListBox.h"
#include "Speed/Indep/Src/FEng/FEGameInterface.h"
#include "Speed/Indep/Src/FEng/FEMovie.h"














u32 FEPackage::uHoldDirtyFlags = 0xFFFFFFFF; // size: 0x4, address: 0x8041D158, Decl: speed/indep/src/feng/FEPackage.cpp:26



bool PackageInitStateCB::Callback(FEObject *pObj) {
    pObj->SetCurrentScript(pObj->FindScript(0x1744b3));
    pObj->pCurrentScript->CurTime = 0;
    pObj->Flags |= FEngDirtyFlagsMask;
    return true;
}



FEPackage::FEPackage() {
    Controllers = 0xff;
    OldControllers = 0xff;
    bInputEnabled = false;
    bExecuting = false;
    bUseIdleList = false;
    bIsLibrary = false;
    bStartEqualsAccept = false;
    Priority = 0;
    bErrorScreen = false;
    pFilename = nullptr;
    pParentPackage = nullptr;
    UserParam = 0;
    NumRequests = 0;
    pRequests = nullptr;
    NumMsgTargets = 0;
    pMsgTargets = nullptr;
    pCurrentButton = nullptr;
    pResourceNames = nullptr;
    MouseObjectStates = nullptr;
    NumMouseObjects = 0;
    NumLibRefs = 0;
    pLibRefs = nullptr;
}











FEPackage::~FEPackage() {
    if (pFilename) {
        delete[] pFilename;
    }
    if (pRequests) {
        delete[] pRequests;
    }
    if (pMsgTargets) {
        delete[] pMsgTargets;
    }
    if (pResourceNames) {
        delete[] pResourceNames;
    }
    if (MouseObjectStates) {
        delete[] MouseObjectStates;
    }
    FEObjectComment *pComment;
    pComment = static_cast<FEObjectComment *>(Comments.RemHead());
    while (pComment) {
        delete[] pComment->pStr;
        delete pComment;
        pComment = static_cast<FEObjectComment *>(Comments.RemHead());
    }
    if (pLibRefs) {
        delete[] pLibRefs;
    }
}


void FEPackage::SetFilename(const char *pName) {
    if (pFilename) { delete[] pFilename; }
    pFilename = nullptr;

    if (pName) {
        int Len = FEngStrLen(pName);

        pFilename = FNEW char[Len + 1];
        FEngStrCpy(pFilename, pName);
    }
}










bool FEPackage::Startup(FEGameInterface *pGameInterface) {
    bool bResult = true;
    if (!pGameInterface->LoadResources(this, NumRequests, pRequests)) {
        bResult = false;
    }
    ConnectObjectResources();
    BuildMouseObjectStateList();
    return bResult;
}










void FEPackage::Shutdown(FEGameInterface *pGameInterface) {
    if (pGameInterface) {
        pGameInterface->UnloadResources(this, NumRequests, pRequests);
    }
}




bool FEPackage::InitializePackage() {
    PackageInitStateCB initState;
    return ForAllObjects(initState);
}










FEMessageResponse *FEPackage::FindResponse(u32 MsgID) {
    FEMessageResponse *pNode = GetFirstResponse();
    while (pNode) {
        if (pNode->GetMsgID() == MsgID) {
            break;
        }
        pNode = pNode->GetNext();
    }
    return pNode;
}















bool FEPackage::ForAllChildren(FEGroup *pGroup, FEObjectCallback &Callback) {
    FEObject *pObj = pGroup->GetFirstChild();
    while (pObj) {
        if (!Callback.Callback(pObj)) {
            return false;
        }
        if (pObj->Type == 5 && !ForAllChildren(static_cast<FEGroup *>(pObj), Callback)) {
            return false;
        }
        pObj = static_cast<FEObject *>(pObj->GetNext());
    }
    return true;
}














bool FEPackage::ForAllObjects(FEObjectCallback &Callback) {
    FEObject *pObj = static_cast<FEObject *>(Objects.GetHead());
    while (pObj) {
        if (!Callback.Callback(pObj)) {
            return false;
        }
        if (pObj->Type == 5 && !ForAllChildren(static_cast<FEGroup *>(pObj), Callback)) {
            return false;
        }
        pObj = static_cast<FEObject *>(pObj->GetNext());
    }
    return true;
}






// File: speed/indep/src/feng/FEPackage.cpp
// total size: 0xC
// Decl: speed/indep/src/feng/FEPackage.cpp:240
class FEFindByHash : public FEObjectCallback {
  public:
    u32 Hash;         // offset 0x4, size 0x4, Decl: speed/indep/src/feng/FEPackage.cpp:242
    FEObject *pFound; // offset 0x8, size 0x4, Decl: speed/indep/src/feng/FEPackage.cpp:243

    bool Callback(FEObject *pObj) override { // Decl: speed/indep/src/feng/FEPackage.cpp:245
        if (pObj->NameHash == Hash) {
            pFound = pObj;
            return false;
        }
        return true;
    }
};







// AUSENTE del objetivo: zFEng.o del original no trae este simbolo y ninguna de
// las 526 unidades lo referencia. El cuerpo vacio emitia un `blr` de 4 B que
// objdiff no compara y que sobraba en .text.
// FEObject *FEPackage::FindObjectByName(const char *pName) {}









FEObject *FEPackage::FindObjectByHash(u32 NameHash) {
    FEFindByHash Find;
    Find.Hash = NameHash;
    Find.pFound = nullptr;
    ForAllObjects(Find);
    return Find.pFound;
}




// total size: 0xC
// Decl: speed/indep/src/feng/FEPackage.cpp:286
class FEFindByGUID : public FEObjectCallback {
  public:
    u32 GUID;         // offset 0x4, size 0x4, Decl: speed/indep/src/feng/FEPackage.cpp:288
    FEObject *pFound; // offset 0x8, size 0x4, Decl: speed/indep/src/feng/FEPackage.cpp:289

    bool Callback(FEObject *pObj) override { // Decl: speed/indep/src/feng/FEPackage.cpp:291
        if (pObj->GUID == GUID) {
            pFound = pObj;
            return false;
        }
        return true;
    }
};













FEObject *FEPackage::FindObjectByGUID(u32 GUID) {
    FEFindByGUID Find;
    Find.GUID = GUID;
    Find.pFound = nullptr;
    ForAllObjects(Find);
    return Find.pFound;
}





void FEPackage::IssueScriptMessages(FEngine *pEngine, FEObject *pObjPtr, FEScript *pScript, i32 tFrom, i32 tTo) {
    FEEvent *pEvents = &pScript->Events[0];
    int Count = pScript->Events.GetCount();

    if (tTo < tFrom) {
        return;
    }

    if (tTo == pScript->Length) {
        tTo++;
    }

    int i = 0;
    while (i < Count) {
        if (pEvents[i].tTime >= static_cast<u32>(tFrom)) {
            break;
        }
        i++;
    }








    while (i < Count && pEvents[i].tTime < static_cast<u32>(tTo)) {
        switch (pEvents[i].Target) {
            case 0:
                if (pEvents[i].EventID == 0x1B3909AA) {
                    FEObject *pTargetPtr = this->FindObjectByGUID(0);
                    this->SetCurrentButton(pTargetPtr, true);
                } else {
                    pEngine->QueueMessage(pEvents[i].EventID, pObjPtr, this, reinterpret_cast<FEObject *>(0), 0);
                }
                break;
            case 0xFFFFFFFF:
                pEngine->SendMessageToGame(pEvents[i].EventID, pObjPtr, this, 0);
                break;
            case 0xFFFFFFFC:
                pEngine->QueueMessage(pEvents[i].EventID, pObjPtr, this, reinterpret_cast<FEObject *>(0xFFFFFFFC), 0);
                break;
            case 0xFFFFFFFB:
                pEngine->QueueMessage(pEvents[i].EventID, pObjPtr, this, reinterpret_cast<FEObject *>(0xFFFFFFFB), 0);
                break;
            case 0xFFFFFFFA:
                pEngine->QueueMessage(pEvents[i].EventID, pObjPtr, this, reinterpret_cast<FEObject *>(0xFFFFFFFA), 0);
                break;
            default: {
                FEObject *pTargetPtr = this->FindObjectByGUID(pEvents[i].Target);
                if (pEvents[i].EventID == 0x1B3909AA) {
                    FEObject *pTargetPtr = this->FindObjectByGUID(pEvents[i].Target);
                    this->SetCurrentButton(pTargetPtr, true);
                    break;
                }
                if (pObjPtr) {
                    pEngine->QueueMessage(pEvents[i].EventID, pObjPtr, this, pTargetPtr, 0);
                }
                break;
            }
        }
        i++;
    }
}

















void FEPackage::UpdateGroup(FEGroup *pGroup, i32 tDeltaTicks) {
    FEObject *pChild = pGroup->GetFirstChild();
    while (pChild) {
        UpdateObject(pChild, tDeltaTicks);
        pChild->Flags |= pGroup->Flags & FEngDirtyFlagsMask;
        pChild = static_cast<FEObject *>(pChild->next);
    }
}

// c36feng: el objeto extraido tiene eFrameCounter UNDEF y symbols.txt lo pone
// en .data:0x8041AC38 (Ecstasy.cpp): esta TU solo lo referencia.
extern uint32 eFrameCounter; // Decl: speed/indep/src/feng/FEPackage.cpp:400
uint32 eFrameCounterOLD;  // size: 0x4, address: 0x80473E0C, Decl: speed/indep/src/feng/FEPackage.cpp:401
uint32 objCount;          // size: 0x4, address: 0x80473E10, Decl: speed/indep/src/feng/FEPackage.cpp:402







// total size: 0x8
class MouseStateArrayOffsetUpdater : public FEObjectCallback {
  public:
    FEPackage *pPack; // offset 0x4, size 0x4

    bool Callback(FEObject *pObj) override {
        if (pObj->Flags & FF_MouseObject) {
            pPack->UpdateMouseObjectOffsets(pObj);
        }
        return true;
    };
};



void FEPackage::UpdateObject(FEObject *pObject, i32 tDeltaTicks) {
    if (eFrameCounterOLD == eFrameCounter) {
        objCount++;
    } else {
        objCount = 0;
        eFrameCounterOLD = eFrameCounter;
    }

    if (pObject->Flags & (FF_DirtyCode | FF_DirtyColor | FF_DirtyTransform)) {
        pObject->Flags |= FF_Dirty;
    } else {
        pObject->Flags &= FEPackage::uHoldDirtyFlags | ~FF_Dirty;
    }

    FEScript *pScript;
    int tPrevTime;
    int tOverTime;
    int ScrLength;
    pScript = pObject->pCurrentScript;
    ScrLength = pScript->Length;
    tPrevTime = pScript->CurTime;
    pScript->CurTime = tPrevTime + iTickIncrement;
    if (pScript->CurTime < 0) {
        pScript->CurTime = 0;
    }

    if (pScript->CurTime >= ScrLength) {
        if (bExecuting) {
            if (pScript->pChainTo) {
                UpdateObjectTracks(pObject, pScript);
                tOverTime = pScript->CurTime - ScrLength;
                pScript->CurTime = 0;
                if (pScript->Events.GetCount()) {
                    IssueScriptMessages(pEnginePtr, pObject, pScript, tPrevTime, ScrLength);
                }
                pScript = pScript->pChainTo;
                pObject->SetCurrentScript(pScript);
                pScript->CurTime = tOverTime;
                if (pScript->Events.GetCount()) {
                    IssueScriptMessages(pEnginePtr, pObject, pScript, 0, pScript->CurTime);
                }
            } else {
                switch (pScript->Flags & 3) {
                    case 0:
                        if (pScript->Events.GetCount()) {
                            IssueScriptMessages(pEnginePtr, pObject, pScript, tPrevTime, ScrLength);
                        }
                        pScript->CurTime = pScript->Length + 1;
                        break;
                    case 1:
                        if (pScript->Length > 0) {
                            if (pScript->Events.GetCount()) {
                                IssueScriptMessages(pEnginePtr, pObject, pScript, tPrevTime, ScrLength);
                            }
                            pScript->CurTime = pScript->CurTime % pScript->Length;
                            if (pScript->Events.GetCount()) {
                                IssueScriptMessages(pEnginePtr, pObject, pScript, 0, pScript->CurTime);
                            }
                            pObject->SetupMoveToTracks();
                        } else {
                            pScript->CurTime = 0;
                        }
                        break;
                    case 2:
                        if (pScript->Length > 0) {
                            pScript->CurTime = pScript->CurTime % (pScript->Length * 2);
                        } else {
                            pScript->CurTime = 0;
                        }
                        break;
                }
            }
        }
    } else {
        if (bExecuting) {
            if (pScript->Events.GetCount() != 0) {
                switch (pScript->Flags & 3) {
                    case 0:
                        IssueScriptMessages(pEnginePtr, pObject, pScript, tPrevTime, pScript->CurTime);
                        break;
                    case 1:
                        if (pScript->CurTime < tPrevTime) {
                            IssueScriptMessages(pEnginePtr, pObject, pScript, tPrevTime, ScrLength);
                            IssueScriptMessages(pEnginePtr, pObject, pScript, 0, pScript->CurTime);
                            break;
                        }
                        IssueScriptMessages(pEnginePtr, pObject, pScript, tPrevTime, pScript->CurTime);
                        break;
                    case 2:
                        if (tPrevTime < ScrLength) {
                            IssueScriptMessages(pEnginePtr, pObject, pScript, tPrevTime, pScript->CurTime);
                        } else {
                            IssueScriptMessages(pEnginePtr, pObject, pScript, tPrevTime - ScrLength, 0);
                            IssueScriptMessages(pEnginePtr, pObject, pScript, 0, pScript->CurTime);
                        }
                        break;
                }
            }
        }
    }

    if (!bExecuting || tPrevTime != pScript->CurTime || tPrevTime != pScript->Length + 1 || (pObject->Flags & FF_DirtyCode)) {
        UpdateObjectTracks(pObject, pScript);
    }

    switch (pObject->Type) {
        case FE_Group:
            UpdateGroup(static_cast<FEGroup *>(pObject), tDeltaTicks);
            break;
        case FE_List:
            static_cast<FEListBox *>(pObject)->Update(static_cast<float>(tDeltaTicks));
            break;
        case FE_CodeList:
            static_cast<FECodeListBox *>(pObject)->Update(static_cast<float>(tDeltaTicks));
            break;
        case FE_Movie:
            if (bExecuting) {
                static_cast<FEMovie *>(pObject)->Update(tDeltaTicks);
            }
            break;
    }

    if (bExecuting == true && tPrevTime == pScript->CurTime && tPrevTime == pScript->Length + 1 && !(pObject->Flags & FF_DirtyCode)) {
        pObject->Flags &= FEPackage::uHoldDirtyFlags | ~(FF_DirtyTransform | FF_DirtyColor);
    }

    pObject->Flags &= FEPackage::uHoldDirtyFlags | ~FF_DirtyCode;
    if (pObject->Flags & (FF_DirtyCode | FF_DirtyColor | FF_DirtyTransform)) {
        pObject->Flags |= FF_Dirty;
    }
}































































































void FEPackage::UpdateObjectTracks(FEObject *pObject, FEScript *pScript) {
    int tTime = pScript->CurTime;
    u8 TrackCount;
    u8 Track;
    FEKeyTrack *pTrack = pScript->pTracks;
    u8 *pDestPtr = pObject->pData;

    if (bExecuting) {
        if ((pTrack != nullptr) && pTrack->LongOffset == 0) {
            if (pTrack->InterpAction & 0x80) {
                pObject->Flags &= FEPackage::uHoldDirtyFlags | ~FF_DirtyColor;
            } else {
                pObject->Flags |= FF_DirtyColor;
            }
            FEKeyInterpFast(pTrack, tTime, pDestPtr);
        } else {
            pObject->Flags &= FEPackage::uHoldDirtyFlags | ~FF_DirtyColor;
        }
        u32 UpdateMask = 0x80;
        if (pObject->GetObjData()->Col.a) {
            TrackCount = pScript->TrackCount;
            for (Track = 0; Track < TrackCount; Track++) {
                UpdateMask = pTrack->InterpAction & UpdateMask;
                FEKeyInterpFast(pTrack, tTime, pDestPtr + pTrack->LongOffset * 4);
                pTrack++;
            }
        }
        if (UpdateMask) {
            pObject->Flags &= FEPackage::uHoldDirtyFlags | ~FF_DirtyTransform;
        } else {
            pObject->Flags |= FF_DirtyTransform;
        }
    } else {
        if ((pTrack != nullptr) && pTrack->LongOffset == 0) {
            FEKeyInterp(pTrack, tTime, pDestPtr);
        }
        if (pObject->GetObjData()->Col.a) {
            TrackCount = pScript->TrackCount;

            for (Track = 0; Track < TrackCount; Track++) {
                FEKeyInterp(pTrack, tTime, pDestPtr + pTrack->LongOffset * 4);
                pTrack++;
            }
        }
    }

    if (pObject->Flags & (FF_DirtyCode | FF_DirtyColor | FF_DirtyTransform)) {
        pObject->Flags = pObject->Flags | FF_Dirty;
    }
}

















































// Decl: speed/indep/src/feng/FEPackage.cpp:767
void FEPackage::Update(FEngine *pEngine, i32 tDeltaTicks) {
    FEObject *pObject = static_cast<FEObject *>(Objects.GetHead());
    pEnginePtr = pEngine;
    iTickIncrement = tDeltaTicks;
    while (pObject) {
        UpdateObject(pObject, tDeltaTicks);
        pObject = pObject->GetNext();
    }
    if (NumMouseObjects > 0) {
        NumMouseObjectsCounter = 0;
        MouseStateArrayOffsetUpdater the_udater;
        the_udater.pPack = this;
        ForAllObjects(the_udater);
    }
}











void FEPackage::SetCurrentButton(FEObject *pNewButton, bool bSendMsgs) {
    if (bSendMsgs) {
        if (pCurrentButton) {
            pEnginePtr->QueueMessage(0x55d1e635, nullptr, this, pCurrentButton, 0);
            pEnginePtr->QueueMessage(0x55d1e635, pCurrentButton, this, reinterpret_cast<FEObject *>(0xfffffffb), 0);
        }
        if (pNewButton) {
            pEnginePtr->QueueMessage(0xabc08912, nullptr, this, pNewButton, 0);
            pEnginePtr->QueueMessage(0xabc08912, pNewButton, this, reinterpret_cast<FEObject *>(0xfffffffb), 0);
        }
    }
    pCurrentButton = pNewButton;
}















void GetNumSpawnResponses(FEMessageResponse *pMsgResponse, u32 &NumLoad, u32 &StringLength) {
    u32 i;

    while (pMsgResponse) {


        for (i = 0; i < pMsgResponse->GetCount(); i++) {

            if (pMsgResponse->GetResponse(i)->SpawnsPackage() && pMsgResponse->GetResponse(i)->HasString()) {

                NumLoad++;
                StringLength += FEngStrLen(reinterpret_cast<const char *>(pMsgResponse->GetResponse(i)->ResponseParam)) + 1;
            }
        }

        pMsgResponse = pMsgResponse->GetNext();
    }
}


void SetSpawnResponses(FEMessageResponse *pMsgResponse, u32 &NumLoad, u32 &StringLength, FELoadPackageDescArray *pDesc) {
    u32 i;

    while (pMsgResponse) {


        for (i = 0; i < pMsgResponse->GetCount(); i++) {

            if (pMsgResponse->GetResponse(i)->SpawnsPackage() && pMsgResponse->GetResponse(i)->HasString()) {

                char *pString = reinterpret_cast<char *>(pDesc) + StringLength;
                FEngStrCpy(pString, reinterpret_cast<const char *>(pMsgResponse->GetResponse(i)->ResponseParam));

                pDesc->Descs[NumLoad].pPackageName = pString;
                pDesc->Descs[NumLoad].Type = pMsgResponse->GetResponse(i)->ResponseID;

                StringLength += FEngStrLen(pString) + 1;
                NumLoad++;
            }
        }

        pMsgResponse = pMsgResponse->GetNext();
    }
}

// total size: 0xC
// Decl: speed/indep/src/feng/FEPackage.cpp:869
class FEGetNumSpawnResponses : public FEObjectCallback {
  public:
    u32 *pNumLoad;      // offset 0x4, size 0x4, Decl: speed/indep/src/feng/FEPackage.cpp:871
    u32 *pStringLength; // offset 0x8, size 0x4, Decl: speed/indep/src/feng/FEPackage.cpp:872

    FEGetNumSpawnResponses(u32 &TheNumLoad, u32 &TheStringLength) : pNumLoad(&TheNumLoad), pStringLength(&TheStringLength) {} // Decl: speed/indep/src/feng/FEPackage.cpp:874


    bool Callback(FEObject *pObj) override { // Decl: speed/indep/src/feng/FEPackage.cpp:877
        GetNumSpawnResponses(pObj->GetFirstResponse(), *pNumLoad, *pStringLength);
        return true;
    }
};

// total size: 0x10
// Decl: speed/indep/src/feng/FEPackage.cpp:885
class FESetSpawnResponses : public FEObjectCallback {
  public:
    u32 *pNumLoad;                 // offset 0x4, size 0x4, Decl: speed/indep/src/feng/FEPackage.cpp:887
    u32 *pStringLength;            // offset 0x8, size 0x4, Decl: speed/indep/src/feng/FEPackage.cpp:888
    FELoadPackageDescArray *pDesc; // offset 0xC, size 0x4, Decl: speed/indep/src/feng/FEPackage.cpp:889

    FESetSpawnResponses(u32 &TheNumLoad, u32 &TheStringLength, FELoadPackageDescArray *pTheDesc) : pNumLoad(&TheNumLoad), pStringLength(&TheStringLength), pDesc(pTheDesc) {} // Decl: speed/indep/src/feng/FEPackage.cpp:891


    bool Callback(FEObject *pObj) override { // Decl: speed/indep/src/feng/FEPackage.cpp:894
        SetSpawnResponses(pObj->GetFirstResponse(), *pNumLoad, *pStringLength, pDesc);
        return true;
    }
};

FELoadPackageDescArray *FEPackage::GetLoadPackageDescArray() {
    u32 StringLength = 0;
    u32 NumLoad = 0;
    GetNumSpawnResponses(GetFirstResponse(), NumLoad, StringLength);
    FEGetNumSpawnResponses Counter(NumLoad, StringLength);
    ForAllObjects(Counter);
    if (NumLoad == 0) { return nullptr; }
    u32 Size = sizeof(u32) + NumLoad * sizeof(FELoadPackageDesc) + StringLength;
    FELoadPackageDescArray *pDesc = reinterpret_cast<FELoadPackageDescArray *>(FNEW char[Size]);
    pDesc->Num = NumLoad;
    StringLength = sizeof(u32) + NumLoad * sizeof(FELoadPackageDesc);
    NumLoad = 0;
    SetSpawnResponses(GetFirstResponse(), NumLoad, StringLength, pDesc);
    FESetSpawnResponses Setter(NumLoad, StringLength, pDesc);
    ForAllObjects(Setter);
    return pDesc;
}
// total size: 0xC
// Decl: speed/indep/src/feng/FEPackage.cpp:968
class ResourceConnector : public FEObjectCallback {
  public:
    FEPackage *pPack;             // offset 0x4, size 0x4, Decl: speed/indep/src/feng/FEPackage.cpp:970
    FEResourceRequest **pReqList; // offset 0x8, size 0x4, Decl: speed/indep/src/feng/FEPackage.cpp:971

    bool Callback(FEObject *pObj) override; // Decl: speed/indep/src/feng/FEPackage.cpp:958

    void ConnectListBoxResources(FEListBox *pList); // Decl: speed/indep/src/feng/FEPackage.cpp:986
};


void FEPackage::FreeLoadPackageDescArray(FELoadPackageDescArray *pDescArray) const {
    delete[] reinterpret_cast<char *>(pDescArray);
}


























bool ResourceConnector::Callback(FEObject *pObj) override { // Decl: speed/indep/src/feng/FEPackage.cpp:958
    switch (pObj->Type) {
        case FE_List:
            ConnectListBoxResources(static_cast<FEListBox *>(pObj));
        case FE_CodeList:
        case FE_Group:
            break;
        default:
            if (pObj->ResourceIndex != 0xFFFF) {
                pObj->UserParam = (*pReqList)[pObj->ResourceIndex].UserParam;
                pObj->Handle = (*pReqList)[pObj->ResourceIndex].Handle;
            }
    }
    return true;
};













void ResourceConnector::ConnectListBoxResources(FEListBox *pList) { // Decl: speed/indep/src/feng/FEPackage.cpp:986
    pList->SetCurrentColumn(0);
    pList->SetCurrentRow(0);

    u32 Rows = pList->GetNumRows();
    u32 Cols = pList->GetNumColumns();
    for (u32 j = 0; j < Rows; j++) {
        for (u32 i = 0; i < Cols; i++) {
            u32 ulIndex = pList->GetCurrentCellData()->stResource.ResourceIndex;
            if (ulIndex != INVALID_LIST_ENTRY) {
                pList->SetCellResource((*pReqList)[ulIndex].Handle, (*pReqList)[ulIndex].UserParam, ulIndex);
            } else {
                pList->SetCellResource(0, 0, INVALID_LIST_ENTRY);
            }
            pList->IncrementCellByColumn();
        }
    }
};









void FEPackage::ConnectObjectResources() {
    ResourceConnector resConnector;
    resConnector.pPack = this;
    resConnector.pReqList = &pRequests;
    ForAllObjects(resConnector);
}

































// total size: 0x8
// Decl: speed/indep/src/feng/FEPackage.cpp:1055
class MouseStateObjectCounter : public FEObjectCallback {
  public:
    int NumMouseObjects; // offset 0x4, size 0x4, Decl: speed/indep/src/feng/FEPackage.cpp:1057

    bool Callback(FEObject *pObj) override { // Decl: speed/indep/src/feng/FEPackage.cpp:1059
        if (pObj->Flags & FF_MouseObject) {
            NumMouseObjects++;
        }
        return true;
    }
};


// total size: 0x8
// Decl: speed/indep/src/feng/FEPackage.cpp:1070
class MouseStateArrayBuilder : public FEObjectCallback {
  public:
    FEPackage *pPack; // offset 0x4, size 0x4, Decl: speed/indep/src/feng/FEPackage.cpp:1072

    bool Callback(struct FEObject *pObj) override { // Decl: speed/indep/src/feng/FEPackage.cpp:1074
        if (pObj->Flags & FF_MouseObject) {
            pPack->AddMouseObjectState(pObj);
        }
        return true;
    }
};



FEObjectMouseState::FEObjectMouseState() {
    pObject = nullptr;
    Offset.v = 0.0f;
    Offset.h = 0.0f;
    Flags = 0;
}


FEObjectMouseState::~FEObjectMouseState() {}




void FEPackage::BuildMouseObjectStateList() {

    if (MouseObjectStates) {

        delete[] MouseObjectStates;
        MouseObjectStates = nullptr;
        NumMouseObjects = 0;
    }

    MouseStateObjectCounter the_counter;
    the_counter.NumMouseObjects = 0;
    ForAllObjects(the_counter);


    if (the_counter.NumMouseObjects > 0) {

        MouseObjectStates = FNEW FEObjectMouseState[the_counter.NumMouseObjects];

        MouseStateArrayBuilder the_builder;
        the_builder.pPack = this;
        ForAllObjects(the_builder);
    }
}


bool OffsetCalculatron(u32 mouseable, FEObject *obj, FEPoint &Offset) {
    if (mouseable == obj->NameHash) {
        FEObjData *pData = obj->GetObjData();
        Offset.h += pData->Pos.x;
        Offset.v += pData->Pos.y;
        return true;
    } else if (obj->Type == FE_Group && static_cast<FEGroup *>(obj)->FindChildRecursive(mouseable)) {
        FEObjData *pData = obj->GetObjData();
        Offset.h += pData->Pos.x;
        Offset.v += pData->Pos.y;
        FEObject *pChild = static_cast<FEGroup *>(obj)->GetFirstChild();
        while (pChild) {
            OffsetCalculatron(mouseable, pChild, Offset);
            pChild = static_cast<FEObject *>(pChild->GetNext());
        }
        return true;
    }
    return false;
}












void FEPackage::AddMouseObjectState(FEObject *obj) {
    if (!obj) {
        return;
    }
    FEObject *pObj = GetFirstObject();
    u32 mouseable = obj->NameHash;
    while (pObj) {
        if (pObj->Type == FE_Group) {
            if (static_cast<FEGroup *>(pObj)->FindChildRecursive(mouseable) || mouseable == pObj->NameHash) {
                FEPoint p;
                if (OffsetCalculatron(mouseable, pObj, p)) {
                    MouseObjectStates[NumMouseObjects].Offset = p;
                    break;
                }
            }
        } else if (mouseable == pObj->NameHash) {
            FEPoint p;
            if (OffsetCalculatron(mouseable, pObj, p)) {
                MouseObjectStates[NumMouseObjects].Offset = p;
                break;
            }
        }
        pObj = static_cast<FEObject *>(pObj->GetNext());
    }
    MouseObjectStates[NumMouseObjects].pObject = obj;
    NumMouseObjects++;
}











void FEPackage::UpdateMouseObjectOffsets(FEObject *obj) {
    if (!obj) {
        return;
    }
    FEObject *pObj = GetFirstObject();
    u32 mouseable = obj->NameHash;
    while (pObj) {
        if (pObj->Type == FE_Group) {
            if (static_cast<FEGroup *>(pObj)->FindChildRecursive(mouseable) || mouseable == pObj->NameHash) {
                FEPoint p;
                if (OffsetCalculatron(mouseable, pObj, p)) {
                    MouseObjectStates[NumMouseObjectsCounter++].Offset = p;
                    break;
                }
            }
        } else if (mouseable == pObj->NameHash) {
            FEPoint p;
            if (OffsetCalculatron(mouseable, pObj, p)) {
                MouseObjectStates[NumMouseObjectsCounter++].Offset = p;
                break;
            }
        }
        pObj = static_cast<FEObject *>(pObj->GetNext());
    }
}






























































































void FEPackage::SetNumLibraryRefs(u32 NewCount) {
    if (NewCount == 0) {

        if (pLibRefs) {
            delete[] reinterpret_cast<char *>(pLibRefs);
        }
        pLibRefs = nullptr;
    } else {

        FELibraryRef *pNewList = FNEW FELibraryRef[NewCount];
        u32 CopyCount = NewCount;
        if (NewCount > NumLibRefs) {
            CopyCount = NumLibRefs;
        }
        if (CopyCount != 0) {
            FEngMemCpy(pNewList, pLibRefs, CopyCount * sizeof(FELibraryRef));
        }
        if (pLibRefs) {
            delete[] reinterpret_cast<char *>(pLibRefs);
        }
        pLibRefs = pNewList;
        NumLibRefs = NewCount;
    }
}




























FELibraryRef *FEPackage::FindLibraryReference(u32 ObjGUID) const {
    for (u32 i = 0; i < NumLibRefs; i++) {
        if (pLibRefs[i].ObjGUID == ObjGUID) {
            return &pLibRefs[i];
        }
    }
    return nullptr;
}
