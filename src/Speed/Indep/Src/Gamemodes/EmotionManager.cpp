// Emotion system: driver emotional state summaries and pursuit responses.
//
// The entire subsystem was dead-stripped from the shipped GOWE69 build; in the
// zGameModes TU only the type/response string tables survive as the first
// global object (see the key of _GLOBAL_.I.aEmotionalSummaryTypeStrings).
//
// r49: los literales SI estan en el DOL, en 0x803EBD60..0x803EBE94 (el rango
// que splits.txt le da hoy a zGameplay).  Los seis disparadores son CamelCase
// alli, no SCREAMING_SNAKE, y NO hay ningun "Created": el DOL solo trae cinco
// fases (Initial, RampUp, Peak, RampDown, Final).  Ver docs/analisis/r49-promo.md.

#include "Speed/Indep/Src/Gamemodes/EmotionManager.hpp"
#include "Speed/Indep/Libs/Support/Utility/UVectorMath.h"


const char *aEmotionalSummaryTypeStrings[EmotionalSummary::MaxTypes] = {
    "Embarrassed",
    "Anxious",
    "Smug",
    "Ecstatic",
};

const char *aEmotionalResponseTriggerStrings[EmotionalResponse::MaxTypes] = {
    "PursuitBegins",
    "PursuitAddsCar",
    "PursuitAddsHeli",
    "PursuitAddsRoadblock",
    "PursuitEnds",
    "Arrested",
};

const char *aEmotionalResponsePhaseStrings[EmotionalResponse::MaxPhases] = {
    0, /* r49: el DOL no trae ningun "Created" entre "Arrested" e "Initial" */
    "Initial",
    "RampUp",
    "Peak",
    "RampDown",
    "Final",
};

const char *GetEmotionalSummaryString(EmotionalSummary::eType type) {
    if (type >= EmotionalSummary::Embarrassed && type < EmotionalSummary::MaxTypes) {
        return aEmotionalSummaryTypeStrings[type];
    }
    return 0;
}

const char *GetEmotionalResponseString(EmotionalResponse::eTrigger trigger) {
    if (trigger >= EmotionalResponse::PursuitBegins && trigger < EmotionalResponse::MaxTypes) {
        return aEmotionalResponseTriggerStrings[trigger];
    }
    return 0;
}

const char *GetEmotionalResponsePhaseString(EmotionalResponse::ePhase phase) {
    if (phase >= EmotionalResponse::Created && phase < EmotionalResponse::MaxPhases) {
        return aEmotionalResponsePhaseStrings[phase];
    }
    return 0;
}

EmotionalSummary::EmotionalSummary()
    : mEmotion(Embarrassed), mEmotionTarget(Embarrassed), mEmotionIntensity(0.0f), mEmotionIntensityTarget(0.0f) {}

EmotionalSummary::~EmotionalSummary() {}

EmotionalSummary::eType EmotionalSummary::GetEmotion() const {
    return mEmotion;
}

float EmotionalSummary::GetIntensity() const {
    return mEmotionIntensity;
}

EmotionalResponse::EmotionalResponse(eTrigger trigger)
    : mTrigger(trigger), mPhase(Created), mPeakIntensity(0.0f), mIntensity(0.0f), mAge(0.0f) {}

EmotionalResponse::~EmotionalResponse() {}

EmotionalResponse::eTrigger EmotionalResponse::GetTrigger() const {
    return mTrigger;
}

EmotionalResponse::ePhase EmotionalResponse::GetPhase() const {
    return mPhase;
}

float EmotionalResponse::GetIntensity() const {
    return mIntensity;
}

#ifndef EA_PLATFORM_PLAYSTATION2
EmotionManager::EmotionManager() {
    mSummary.mEmotion = EmotionalSummary::Embarrassed;
    mSummary.mEmotionTarget = EmotionalSummary::Embarrassed;
    mSummary.mEmotionIntensity = 0.0f;
    mSummary.mEmotionIntensityTarget = 0.0f;
}

EmotionManager::~EmotionManager() {}

const EmotionalSummary &EmotionManager::GetSummary() {
    return mSummary;
}
#endif

void EmotionManager::Update(Car *car) {}

float EmotionManager::GetEmotionalEvaluation(eEmotionalContext context) {
    return 0.0f;
}

void EmotionManager::SendResponseEvent(EmotionalResponse::eTrigger trigger, float intensity) {}

#ifdef EA_PLATFORM_PLAYSTATION2
#include "Speed/Indep/bWare/Inc/bList.hpp"

// La implementacion a la que apunta el EmotionManager de PS2 (48 B en los stabs de la alpha). De
// todo el subsistema, en el objeto de PS2 solo quedan estas siete funciones y los destructores;
// aqui van las siete que no dependen de la vtable de la alpha.
struct EmotionLog {
    virtual ~EmotionLog();
};

EmotionLog::~EmotionLog() {}

#ifdef EA_BUILD_A124
struct EmotionSequence {
    virtual ~EmotionSequence();
};

EmotionSequence::~EmotionSequence() {}
#endif

struct EmotionManagerImpl {
    float mContextEvaluations[EmotionManager::MaxContexts]; // 0x00
    EmotionalSummary mEmotionalSummary;                      // 0x14
    bPList<EmotionalResponse> mEmotionalResponses;           // 0x24

    virtual ~EmotionManagerImpl();

    float GetNegativeIntensity(EmotionalSummary::eType type);
    float GetPositiveIntensity(EmotionalSummary::eType type);
    void IncreaseEmotion();
    void DecreaseEmotion();
    bPNode *GetFirstResponse();
    EmotionalResponse *GetNextResponse(bPNode **node);
    void ClearResponseList();
};

EmotionManagerImpl::~EmotionManagerImpl() {
    ClearResponseList();
}

float EmotionManagerImpl::GetNegativeIntensity(EmotionalSummary::eType type) {
    float intensity = 1.0f;
    if (mEmotionalSummary.mEmotion == EmotionalSummary::Smug || mEmotionalSummary.mEmotion == EmotionalSummary::Ecstatic) {
        intensity = 0.0f;
    }
    return intensity;
}

float EmotionManagerImpl::GetPositiveIntensity(EmotionalSummary::eType type) {
    float intensity = 0.0f;
    if (mEmotionalSummary.mEmotion == EmotionalSummary::Smug || mEmotionalSummary.mEmotion == EmotionalSummary::Ecstatic) {
        intensity = 1.0f;
    }
    return intensity;
}

void EmotionManagerImpl::IncreaseEmotion() {
    bool bIncreased = true;
    switch (mEmotionalSummary.mEmotion) {
        case EmotionalSummary::Embarrassed:
            mEmotionalSummary.mEmotion = EmotionalSummary::Anxious;
            break;
        case EmotionalSummary::Anxious:
            mEmotionalSummary.mEmotion = EmotionalSummary::Smug;
            break;
        case EmotionalSummary::Smug:
            mEmotionalSummary.mEmotion = EmotionalSummary::Ecstatic;
            break;
        case EmotionalSummary::Ecstatic:
            bIncreased = false;
            break;
        case EmotionalSummary::MaxTypes:
            break;
    }
    if (bIncreased) {
        mEmotionalSummary.mEmotionIntensity = GetNegativeIntensity(mEmotionalSummary.mEmotion);
    }
}

void EmotionManagerImpl::DecreaseEmotion() {
    bool bDecreased = true;
    switch (mEmotionalSummary.mEmotion) {
        case EmotionalSummary::Embarrassed:
            bDecreased = false;
            break;
        case EmotionalSummary::Anxious:
            mEmotionalSummary.mEmotion = EmotionalSummary::Embarrassed;
            break;
        case EmotionalSummary::Smug:
            mEmotionalSummary.mEmotion = EmotionalSummary::Anxious;
            break;
        case EmotionalSummary::Ecstatic:
            mEmotionalSummary.mEmotion = EmotionalSummary::Smug;
            break;
        case EmotionalSummary::MaxTypes:
            break;
    }
    if (bDecreased) {
        mEmotionalSummary.mEmotionIntensity = GetPositiveIntensity(mEmotionalSummary.mEmotion);
    }
}

bPNode *EmotionManagerImpl::GetFirstResponse() {
    bPNode *node = static_cast<bPNode *>(mEmotionalResponses.GetHead());
    if (node == static_cast<bPNode *>(mEmotionalResponses.EndOfList())) {
        return 0;
    }
    return node;
}

EmotionalResponse *EmotionManagerImpl::GetNextResponse(bPNode **ppNode) {
    bPNode *current = *ppNode;
    if (current == static_cast<bPNode *>(mEmotionalResponses.GetTail())) {
        *ppNode = 0;
        return static_cast<EmotionalResponse *>(current->GetObj());
    }
    *ppNode = current->GetNext();
    return static_cast<EmotionalResponse *>(current->GetObj());
}

void EmotionManagerImpl::ClearResponseList() {
    bPNode *pNode = GetFirstResponse();
    while (pNode) {
        bPNode *node = pNode;
        EmotionalResponse *pResponse = GetNextResponse(&pNode);
        if (pResponse) {
            delete pResponse;
        }
        node->Remove();
        delete node;
    }
}

EmotionManager::EmotionManager() : mpImpl(0) {}

EmotionManager::~EmotionManager() {
    delete mpImpl;
}

const EmotionalSummary &EmotionManager::GetSummary() {
    return mpImpl->mEmotionalSummary;
}
#endif
