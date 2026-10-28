#ifndef EAXSOUND_STREAM_SPEECHMANAGER_H
#define EAXSOUND_STREAM_SPEECHMANAGER_H

#define _SPEECH_MANAGER_H_

#ifdef EA_PRAGMA_ONCE_SUPPORTED
#pragma once
#endif

#include "Speed/Indep/Libs/Support/Utility/UStandard.h"
#include "Speed/Indep/Src/EAXSound/SFX_base.hpp"
#include "Speed/Indep/Src/Generated/AttribSys/Classes/speech.h"
#include "Speed/Indep/Src/Misc/Timer.hpp"
#include "Speed/Indep/Src/Speech/EAXCharacter.h"
#include "Speed/Indep/Src/Speech/SpeechCache.h"
#include "Speed/Indep/Src/EAXSound/SND_GEN/NISAudio.hpp"
#include "SpeechModule.hpp"

struct InterfaceId;
struct FunctionHandle;
struct SlotPool;
namespace Csis {
struct InterfaceId;
struct FunctionHandle;
}



struct CLUMP_IDX_FILEtag;

namespace Csis {
static inline Result CacheHandlesEventsNIS() {
    gNIS_Select_StartHandle.Set(&NIS_Select_StartId);
#ifdef EA_BUILD_A124
    // la A124 trae los interfaces de NIS en otro orden (NISAudio.hpp:230-233 de su .mdebug)
    gNIS_Select_EndHandle.Set(&NIS_Select_EndId);
    gSoundFX_SelectHandle.Set(&SoundFX_SelectId);
    gNIS_Select_BlacklistHandle.Set(&NIS_Select_BlacklistId);
#else
    gSoundFX_SelectHandle.Set(&SoundFX_SelectId);
    gNIS_Select_BlacklistHandle.Set(&NIS_Select_BlacklistId);
    gNIS_Select_EndHandle.Set(&NIS_Select_EndId);
#endif
    return RESULT_OK;
}
}

namespace Speech {

// gcdatos13: los enums que rodean a SpeechValRtnType en el DWARF de GC
// (mw_dwarfdump.nothpp:125895-125944) y en los stabs de la A124
// (PS2_types.nothpp:9693-9745, 10018), mismos valores en ambos volcados.
enum SpeechObsFlags {
    kObserve_None = 0,
    kObserve_Player = 4,
    kObserve_Cops = 8,
    kObserve_All = 12,
    kObserve_FORCE_INT = 2147483647,
};

enum SpeechValRtnType {
    kKeepEvt = 0,
    kIntEvt = 1,
    kDitchEvt = 2,
    kEvtNotFound = 3,
    kDeferEvt = 4,
    MAX_RETURN_TYPES = 5,
};

enum InterruptType {
    Static = 0,
    Expletive = 1,
    Composed = 2,
    MAX_INTERRUPT_TYPES = 3,
};

enum SchedSpeechEventTypes {
    kRequests = 0,
    kFiltered = 1,
    kPlayback = 2,
    kInterrupts = 3,
    MAX_SPCHEVT_TYPES = 4,
};

struct ScheduledSpeechEvent;

struct SPCHSampleRequest {
    SPCHType_SampleRequestData data; // offset 0x0, size 0x20
    ScheduledSpeechEvent *owner;     // offset 0x20, size 0x4
    unsigned int offset;             // offset 0x24, size 0x4
    unsigned char sample_index;      // offset 0x28, size 0x1

    bool operator<(const SPCHSampleRequest &from) const {
        return offset < from.offset;
    }
};

DECLARE_CONTAINER_TYPE(SampleReqList);

class SampleReqList : public UTL::Std::vector<SPCHSampleRequest, _type_SampleReqList>, public AudioMemBase {
  public:
};

struct History {
    History() {
        this->time = Timer(0);
        this->count = 0;
        this->speakers = 0;
    }

    void Touch(unsigned short spkrID) {
        this->time = WorldTimer;
        this->count = this->count + 1;
        if (spkrID < 10 && ((((this->speakers >> spkrID) ^ 1) & 1) != 0)) {
            this->speakers = static_cast<unsigned short>(this->speakers | (1 << spkrID));
        }
    }

    Timer time;             // offset 0x0, size 0x4
    unsigned short count;   // offset 0x4, size 0x2
    unsigned short speakers; // offset 0x6, size 0x2
};

struct HistoryPair {
    HistoryPair() : id(kSPCH1_EventID_MaxEventID), history() {}

    SPCHType_1_EventID id; // offset 0x0, size 0x4
    History history;       // offset 0x4, size 0x8

    bool operator<(const HistoryPair &rhs) const {
        return id < rhs.id;
    }
};

struct SpeechEventPair {
    unsigned int hash;     // offset 0x0, size 0x4
    SPCHType_1_EventID id; // offset 0x4, size 0x4

    bool operator<(const SpeechEventPair &rhs) const {
        return id < rhs.id;
    }
};

struct SpeechHashIDMap : public UTL::FixedVector<SpeechEventPair, 264, 16>, public AudioMemBase {
  public:
    SpeechHashIDMap() {}

    void Add(unsigned int hash, SPCHType_1_EventID id);
    SPCHType_1_EventID GetID(unsigned int hash);
    unsigned int GetHash(SPCHType_1_EventID id);
};

struct EventHistory : public UTL::FixedVector<HistoryPair, 264, 16>, public AudioMemBase {
  public:
    EventHistory() {}

    void Init();
    History *Find(SPCHType_1_EventID id);
    int GetCount(SPCHType_1_EventID id);
    Timer GetTime(SPCHType_1_EventID id);
    bool HasSaid(unsigned short speakerID, SPCHType_1_EventID id);
    unsigned short GetSpeakers(SPCHType_1_EventID id);
    History *Touch(SPCHType_1_EventID id, unsigned short speaker);
    void Reset();
};

DECLARE_CONTAINER_TYPE(SchedSpchEvents);

struct SchedSpchEvents : public UTL::Std::vector<ScheduledSpeechEvent *, _type_SchedSpchEvents>, public AudioMemBase {
  public:
    SchedSpchEvents() {}
    virtual ~SchedSpchEvents() {}
};

struct SPCHEventList : public UTL::Std::list<SPCHType_1_EventID, _type_list>, public AudioMemBase {
  public:
};

// gcdatos13: tras los iteradores de SPCHEventList en los stabs (PS2_types.nothpp:122280)
// y delante de ScheduledSpeechEvent en el DWARF (mw_dwarfdump.nothpp:125941).
enum eSpeechIOMode {
    NOMINAL = 0,
    CACHE_ONLY = 1,
    DISK_ONLY = 2,
    MAX_SPEECHIOMODES = 3,
};

enum SpeechEventFlags {
    BW_DEFERRED = 1,
    DELAYED = 2,
};

struct ScheduledSpeechEvent {
    Csis::InterfaceId *iid;             // offset 0x0, size 0x4
    Csis::FunctionHandle *fh;           // offset 0x4, size 0x4
    SPCHType_1_EventID ID;              // offset 0x8, size 0x4
    EAXCharacter *actor;                // offset 0xC, size 0x4
    Timer entry_time;                   // offset 0x10, size 0x4
    Timer playback_time;                // offset 0x14, size 0x4
    Timer finish_time;                  // offset 0x18, size 0x4
    SpeechSampleData *assoc_samples[7]; // offset 0x1C, size 0x1C
    unsigned char assoc_samples_count;  // offset 0x38, size 0x1
    unsigned char assoc_samples_prep;   // offset 0x39, size 0x1
    unsigned char curndx;               // offset 0x3A, size 0x1
    unsigned char priority;             // offset 0x3B, size 0x1
    short frameindex;                   // offset 0x3C, size 0x2
    short flags;                        // offset 0x3E, size 0x2

    ScheduledSpeechEvent();
    ~ScheduledSpeechEvent();

    static void *operator new(unsigned int base_size, unsigned int xtra);
    static void operator delete(void *ptr);
    static void operator delete(void *ptr, unsigned int xtra);

    static bool sort_nested_priority(const ScheduledSpeechEvent *lhs, const ScheduledSpeechEvent *rhs);
    void AddSample(SpeechSampleData *sample, unsigned char specific_index);
#ifdef EA_BUILD_A124
    void *GetData();
#else
    void *GetData(unsigned int *datasize = nullptr);
#endif
    unsigned char ReserveSample();
};

// gcdatos13: delante de Manager en los stabs de la A124 (PS2_types.nothpp:43172) y el
// DWARF de GC (mw_dwarfdump.nothpp:125968): la mascara de PostValidate.
enum FilterMask {
    EXPIRY = 1,
    MAXPLAY = 2,
    DEADAIR = 4,
    INTERVAL = 8,
    RANGE = 16,
    DEPENDANCY = 32,
    RENDERED = 64,
    LOS = 128,
    HEAT = 256,
    SPEED = 512,
    ALL = -1,
};

class Manager {
  public:
    template <typename T>
    static void ScheduleSpeech(T &data, Csis::InterfaceId &iid, Csis::FunctionHandle &fh, EAXCharacter *actor);

    static int FlushSpeechForActor(EAXCharacter *actor);
    static int GetGlobalHistoryCount(SPCHType_1_EventID id);
    static ScheduledSpeechEvent *ScheduleSpeechPartII(unsigned int sample_size, void *sample_data, Csis::InterfaceId &iid, Csis::FunctionHandle &fh, EAXCharacter *actor);
    static int IndirectSpeechEvent(ScheduledSpeechEvent *evt, bool test_only);
    static int TestSentenceRuleCallback(EventSpec *event_info, int rule_id, int parm_value, int user_num);
    static int ReparmCallback(int rule_id, unsigned int *parms);
    static void SetSentenceRuleCallback(EventSpec *event_info, int rule_id, int parm_value, int user_num);
    static SPCHType_EventRuleResult EventRuleCallback(EventSpec *event_info);
    static void ClearPlayback();
    static void Init(SPEECH_MODE mode);
    static void Init2();
    static void Destroy();
    static void Deduce();
    static void Update(float dt);
    static Module *GetSpeechModule(int id);
    static void AttachSFXOBJ(SpeechModuleIndex module, SFX_Base *psb, eSFXOBJ_MAIN_TYPES type);
    static bool IsPlaying(SpeechModuleIndex module);
    static bool IsCopSpeechPlaying(SPCHType_1_EventID event);
    static bool IsCopSpeechBusy();
    static Timer GetTimeSinceLastEvent(SpeechModuleIndex module);
    static void SpchLibAbort(const char *format, ...);
    static int SampleRequestCallback(SPCHType_SampleRequestData *data);
    static int LoadSpeechBank(CLUMP_IDX_FILEtag *index, int &type, int &number, SPEECH_BANK *sb);
    static int AddHeaders(char **dest, SPEECH_BANK *banks, int numBanks
#ifndef EA_BUILD_A124
                          ,
                          Module *module
#endif
    );
    static int GetTicker();
  private:
    static void PopulateHashMap();
  public:
    static bool IsCacheable(SPCHType_1_EventID event_id);
    static bool HasBeenSaid(SPCHType_1_EventID event_id);
  private:
    static bool ServiceInterruptEvents();
    static void ServiceFilteredEvents();
  public:
    static bool RecallSpeechEvent(SPCHType_1_EventID recall_id);
    static void ResetGlobalHistory();
    static void Expire(ScheduledSpeechEvent *event);
    static bool IsQueued(SPCHType_1_EventID evtID, int indices);
    static float IsEventDead(ScheduledSpeechEvent *evt);
    static void NotifyEventCompletion(ScheduledSpeechEvent *evt, bool playback_complete);
    static ScheduledSpeechEvent *GetNextEvent();
    static SpeechValRtnType PostValidate(ScheduledSpeechEvent *evt, unsigned int mask);
    static SpeechValRtnType PreValidate(ScheduledSpeechEvent &evt);
  private:
    static bool CanPlayback(Attrib::Gen::speech &event_attribs);
    static void CalcProbPlayback();
#ifdef EA_BUILD_A124
    static void VerifySampleIntegrity();
#endif
  public:
    static SPCHType_1_EventID GetLastEventID() {
        if (mEvtHistory.empty()) {
            return kSPCH1_EventID_MaxEventID;
        }
        return mEvtHistory.front();
    }
    static EventHistory &GetHistory() { return mGlobalHistory; }
    static SpeechHashIDMap &GetHashIDMap() { return mHashMap; }
    static SampleReqList &GetSampleRequests() { return mSampleRequests; }

    static Module *m_SpeechModule[NUM_SPEECH_MODULES];
    static SPEECH_MODE m_speechMode;
    static int m_numberSpeechBanks;
    static bool m_SPEECH_initted;
    static char *m_SPEECH_bankPtrMem;
    static bool m_speechDisable;
    static int m_gameSpeechInitted;
    static int m_NISAudioInitted;
    static float m_clock_in_ms;
    static float m_timestep;
    static float m_deadair;
    static ScheduledSpeechEvent *mCurrentEvent;
    static short m_frameindex;
    static float mProbPlayback;
    static short mLastSpeakerID;
    static SchedSpchEvents mEvents[4];
    static SPCHEventList mEvtHistory;
    static SpeechHashIDMap mHashMap;
    static EventHistory mGlobalHistory;
    static SampleReqList mSampleRequests;
    static Timer mSampleReqTimer;
};

}; // namespace Speech

#ifndef SCHEDULE_SPEECH
#define SCHEDULE_SPEECH(_EVENT_, _DATA_, _CALLER_)                                                                                                         Speech::Manager::ScheduleSpeech<Csis::_EVENT_##Struct>(_DATA_, Csis::_EVENT_##Id, Csis::g##_EVENT_##Handle, _CALLER_)
#endif

#endif
