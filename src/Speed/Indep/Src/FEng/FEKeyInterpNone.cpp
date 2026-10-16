#include "Speed/Indep/Src/FEng/FEKeyInterpolators.h"
#include "Speed/Indep/Src/FEng/FEScript.h"
#include "Speed/Indep/Src/FEng/FEngStandard.h"

// c34ord3 f5: el objetivo emite FEInterpNone(FEScript*) ANTES que
// FEInterpNone(FEKeyTrack*). NO volver a ordenarlas.
void FEInterpNone(FEScript *pScript, u8 TrackNum, i32 tTime, void *pOutData) {
    FEKeyTrack *pTrack = &pScript->pTracks[TrackNum];
    u8 *pOutDataPtr = reinterpret_cast<u8 *>(pOutData);

    u32 OutOffset = pTrack->LongOffset;

    FEInterpNone(pTrack, tTime, pOutDataPtr + OutOffset * sizeof(void *));
}

void FEInterpNone(FEKeyTrack *pTrack, i32 tTime, void *pOutDataPtr) {
    u32 KeySize = pTrack->ParamSize + 4;
    FEKeyNode *pKey;
    FEKeyNode *pPrevKey;

    if (tTime <= pTrack->Length) {
        pKey = pTrack->GetKeyAt(tTime);
        pPrevKey = static_cast<FEKeyNode *>(pKey->GetPrev());
        if (pPrevKey && pKey->tTime > tTime) {
            FEngMemCpy(pOutDataPtr, &pPrevKey->Val, KeySize - 4);
        } else {
            FEngMemCpy(pOutDataPtr, &pKey->Val, KeySize - 4);
        }
    } else {
        pKey = pTrack->GetKeyAt(pTrack->Length - tTime);
        pPrevKey = static_cast<FEKeyNode *>(pKey->GetNext());
        if (pPrevKey && pKey->tTime < tTime) {
            FEngMemCpy(pOutDataPtr, &pPrevKey->Val, KeySize - 4);
        } else {
            FEngMemCpy(pOutDataPtr, &pKey->Val, KeySize - 4);
        }
    }
}
