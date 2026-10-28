#include "FEMessageResponse.h"
#include "FEngStandard.h"
#include "ObjectPool.h"






FEResponse::~FEResponse() {
    ReleaseParam();
}




FEResponse &FEResponse::operator=(FEResponse &rhs) {
    ReleaseParam();
    ResponseID = rhs.ResponseID;
    if (FEResponse::HasString(ResponseID)) {
        SetParam(reinterpret_cast<const char *>(rhs.ResponseParam));
    } else {
        ResponseParam = rhs.ResponseParam;
    }
    ResponseTarget = rhs.ResponseTarget;
    return *this;
}






void FEResponse::SetParam(const char *pString) {
    ReleaseParam();
    if (pString) {
        u32 Len = FEngStrLen(pString);

        char *pPathCopy = FNEW char[Len + 1];
        FEngStrCpy(pPathCopy, pString);
        ResponseParam = reinterpret_cast<u32>(pPathCopy);
    }
}






void FEResponse::ReleaseParam() {
    if (HasString() && ResponseParam) {
        delete[] reinterpret_cast<char *>(ResponseParam);
    }
    ResponseParam = 0;
}

ObjectPool<FEMessageResponse, 64> FEMessageResponse::NodePool;





FEMessageResponse::~FEMessageResponse() {
    PurgeResponses();
}









void *FEMessageResponse::operator new(size_t) {
    FEMessageResponse *pNode = NodePool.AllocSingle();
    pNode->Init();
    return pNode;
}





void FEMessageResponse::operator delete(void *pNode) {
    FEMessageResponse *pDeleteNode = static_cast<FEMessageResponse *>(pNode);
    pDeleteNode->~FEMessageResponse();
    NodePool.FreeSingle(pDeleteNode);
}

















void FEMessageResponse::PurgeResponses() {
    delete[] pResponseList;
    pResponseList = nullptr;
    Count = 0;
}






void FEMessageResponse::SetCount(u32 NewCount) {


    if (NewCount != Count) {


        if (NewCount == 0) {

            PurgeResponses();
        } else {



            FEResponse *pNewList = FNEW FEResponse[NewCount];
            u32 copyCount = Count;
#ifdef EA_PLATFORM_XENON
            if (NewCount < copyCount) {
#else
            if (copyCount > NewCount) {
#endif
                copyCount = NewCount;
            }
            u32 i = 0;
            while (i < copyCount) {
                pNewList[i] = pResponseList[i];
                i++;
            }
            delete[] pResponseList;
            pResponseList = pNewList;
            Count = NewCount;
        }
    }
}




























































u32 FEMessageResponse::FindResponse(u32 CommandID) const {
    for (u32 i = 0; i < Count; i++) {
        if (pResponseList[i].ResponseID == CommandID) {
            return i;
        }
    }
    return 0xFFFFFFFF;
}

/* CERRADA CONTRA EL DWARF: dos locales a nivel de funcion, `Nest` (r10) y `Result` (r4, el MISMO registro que
 * Index: es el indice que corre), y ninguna para Count ni para pResponseList. Con `count` como local, el
 * `return Count` de salida costaba `bne`+`mr`+`blr` en vez del `beqlr` del objetivo (se habian medido `goto`,
 * `while` con la comprobacion al final y la forma directa: 74,9 / 72,2 / 62,6 %).
 * En MR_Else las dos PS2 calculan Nest - 1 con un `movz`: el original decrementa (`Nest--`), y GameCube lo pliega
 * a 0 porque sabe que Nest vale 1. */





u32 FEMessageResponse::FindConditionBranchTarget(u32 Index) const {
    u32 Nest;
    u32 Result;

    if (Index == this->Count - 1) {
        return this->Count;
    }
    Nest = 1;
    Result = Index;
    do {
        Result++;
        switch (this->pResponseList[Result].ResponseID) {
            case MR_IfScriptEquals:
            case MR_IfScriptNotEquals:
                Nest++;
                break;
            case MR_Else:
                if (Nest == 1) {
                    Nest--;
                }
                break;
            case MR_EndIf:
                Nest--;
                break;
        }
    } while (Result < this->Count && Nest != 0);
    return Result;
}
