#ifndef EAXSOUND_STREAM_GAMESPEECH_H
#define EAXSOUND_STREAM_GAMESPEECH_H

#ifdef EA_PRAGMA_ONCE_SUPPORTED
#pragma once
#endif

#include "Speed/Indep/Libs/Support/Utility/UStandard.h"
#include "Speed/Indep/Src/EAXSound/STICH_Playback.h"
#include "SpeechModule.hpp"

struct CLUMP_IDX_FILEtag;

namespace Speech {

struct SpeechSampleData;
struct ScheduledSpeechEvent;

#ifdef EA_PLATFORM_PLAYSTATION2
DECLARE_CONTAINER_TYPE(SpeechSampleList);

// Las dos PS2 la tienen (stabs de la A124, que la declaran delante de SpeechSampleVec; SLUS la usa en Cache::Dump): una
// lista de muestras con AudioMemBase detras, 0xC. La A124 guarda asi las muestras pendientes de GameSpeech.
struct SpeechSampleList : public UTL::Std::list<SpeechSampleData *, _type_SpeechSampleList>, public AudioMemBase {};
#endif

DECLARE_CONTAINER_TYPE(SpeechSampleVec);

struct SpeechSampleVec : public UTL::Std::vector<SpeechSampleData *, _type_SpeechSampleVec>, public AudioMemBase {
    SpeechSampleData *Find(int handle) const;
};

// total size: 0xD0 (0xB0 en la A124)
struct GameSpeech : public Module {
    static char *m_tempCharPtr;
    static CLUMP_IDX_FILEtag *m_clumpIdx;
    static char *m_csisData;
    static int m_channel;
    static char *m_eventDat;

    std::deque<int> mLoadState;            // offset 0x58, size 0x30
    int m_currentIntensity;                // offset 0x88, size 0x4
    int m_speechCycle;                     // offset 0x8C, size 0x4
#ifdef EA_BUILD_A124
    // La A124 guarda un puntero a una lista que crea el constructor (stabs: 4 bytes en 0x88; la clase mide 0xB0).
    SpeechSampleList *m_pendingList;
#else
    SpeechSampleVec m_pendingList;         // offset 0x90, size 0x14
#endif
    ScheduledSpeechEvent *m_currEvent;     // offset 0xA4, size 0x4
    unsigned int m_currEventTime;          // offset 0xA8, size 0x4
    // La A124 no los tiene: sus stabs pasan de m_currEventTime a m_Chirper.
#ifndef EA_BUILD_A124
    int m_currEventClarity;                // offset 0xAC, size 0x4
    short m_currEventSpeakerID;            // offset 0xB0, size 0x2
#endif
    cStichWrapper *m_Chirper;              // offset 0xB4, size 0x4
    SND_Params m_ChirpParams;              // offset 0xB8, size 0x18

    GameSpeech();
    ~GameSpeech() override;
    void Init(int channel) override;
    static void LoadingCallback(int param, int error_status);
    void LoadBanks() override;
    int TestSentenceRuleCallback(int eventID, int ruleID, int parmValue) override;
    int SetSentenceRuleCallback(int eventID, int ruleID, int parmValue) override;
    SPCHType_EventRuleResult EventRuleCallback(int eventID) override;
    void Update() override;
  protected:
#ifndef EA_BUILD_A124 // la A124 mira el siguiente evento dentro de Update
    void CheckNextEvent();
#endif
  public:
    ScheduledSpeechEvent *GetCurrentEvent() {
        return this->m_currEvent;
    }
    bool IsBusy() {
        return TestFlag(8);
    }
  protected:
    void IssuePlayback(ScheduledSpeechEvent *nextevent);
    void ClearCompletedRequests();
  public:
    void ReleaseResource() override;
    unsigned int SampleRequestCallback(SPCHType_SampleRequestData *data) override;
    void IssueSampleRequests();
  protected:
    bool ShouldPause();
    void RadioChirp(unsigned char type);
    void UpdateChirps();
    int GetVolForSpeaker(int id);
  public:
    char *GetCSIptr() override {
        return m_csisData;
    }
    int GetChannel() override {
        return m_channel;
    }
    char *GetEventDat() override {
        return m_eventDat;
    }
    bool IsDataLoaded() override { return TestFlag(1); }
};

} // namespace Speech

#endif
