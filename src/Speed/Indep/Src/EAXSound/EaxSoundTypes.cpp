#include <types.h>

#include <algorithm>
#include <new>

#include "Speed/Indep/Src/EAXSound/EAXSoundTypes.h"
#include "Speed/Indep/Src/Speech/SoundAI.h"
#include "Speed/Indep/Src/Interfaces/Simables/ISimable.h"
#include "Speed/Indep/Src/EAXSound/Stream/SpeechManager.hpp"
#include "Speed/Indep/Src/Misc/Timer.hpp"
#include "Speed/Indep/Src/Speech/EAXCop.h"

extern void *NullPointer;
struct ISndAttachable;

#include "Speed/Indep/Src/Generated/AttribSys/Classes/speech.h"

namespace Sound {

inline Wheel::~Wheel() {}

} // namespace Sound

namespace Speech {

void copMap::Add(HSIMABLE__ *hsimable, EAXCop *cop) {
    copPair p;
    p.hsimable = hsimable;
    p.cop = cop;
    iterator iter = std::upper_bound(this->begin(), this->end(), p);
    this->insert(iter, p);
}

EAXCop *copMap::Remove(HSIMABLE__ *hsimable) {
    EAXCop *result = nullptr;
    copPair p;
    p.hsimable = hsimable;
    p.cop = nullptr;

    iterator iter = std::lower_bound(this->begin(), this->end(), p);
    if (iter != this->end() && iter->hsimable == hsimable) {
        result = iter->cop;
        this->erase(iter);
    }

    return result;
}

void copMap::ModifyHandle(HSIMABLE__ *hsimable, HSIMABLE__ *newhandle) {
    iterator iter;
    for (iter = this->begin(); iter != this->end(); ++iter) {
        if (iter->hsimable == hsimable) {
            iter->hsimable = newhandle;
            break;
        }
    }
    std::sort(this->begin(), this->end());
}

EAXCop *copMap::Find(HSIMABLE__ *hsimable) const {
    copPair p;
    p.hsimable = hsimable;
    p.cop = nullptr;

    const_iterator iter = std::lower_bound(this->begin(), this->end(), p);
    if (iter != this->end() && iter->hsimable == hsimable) {
        return iter->cop;
    }

    return nullptr;
}

void SpeechHashIDMap::Add(unsigned int hash, SPCHType_1_EventID id) {
    SpeechEventPair pair;
    pair.hash = hash;
    pair.id = id;
    this->insert(std::upper_bound(this->begin(), this->end(), pair), pair);
}

SPCHType_1_EventID SpeechHashIDMap::GetID(unsigned int hash) {
    for (const_iterator i = this->begin(); i != this->end(); ++i) {
        const SpeechEventPair &p = *i;
        if (p.hash == hash) {
            return p.id;
        }
    }

    return kSPCH1_EventID_MaxEventID;
}

unsigned int SpeechHashIDMap::GetHash(SPCHType_1_EventID id) {
    SpeechEventPair p;
    p.hash = 0;
    p.id = id;

    const_iterator iter = std::lower_bound(this->begin(), this->end(), p);
    if (iter != this->end() && iter->id == id) {
        return iter->hash;
    }

    return 0;
}

void EventHistory::Init() {
    const Attrib::Class *speechevents = Attrib::Database::Get().GetClass(0xC593DD47);
    unsigned int eventkey = speechevents->GetFirstCollection();
    while (eventkey != 0) {
        HistoryPair p;
        Attrib::Gen::speech event_collection(eventkey, 0, nullptr);
        p.id = event_collection.SpeechID();
        {
            iterator iter = std::upper_bound(this->begin(), this->end(), p);
            insert(iter, p);
        }
        eventkey = speechevents->GetNextCollection(eventkey);
    }

    this->Reset();
}

History *EventHistory::Find(SPCHType_1_EventID id) {
    HistoryPair p;
    p.id = id;

    iterator it = std::lower_bound(this->begin(), this->end(), p);
    if (it != this->end() && it->id == id) {
        return &it->history;
    }

    return nullptr;
}

int EventHistory::GetCount(SPCHType_1_EventID id) {
    History *hist = this->Find(id);
    return hist ? hist->count : -1;
}

Timer EventHistory::GetTime(SPCHType_1_EventID id) {
    History *hist = this->Find(id);
    if (hist != nullptr) {
        return hist->time;
    }

    return Timer(0);
}

History *EventHistory::Touch(SPCHType_1_EventID id, unsigned short speaker) {
    History *hist = this->Find(id);
    if (hist == nullptr) {
        return nullptr;
    }

    hist->Touch(speaker);
    return hist;
}

void EventHistory::Reset() {
    for (iterator i = this->begin(); i != this->end(); ++i) {
        History &hist = i->history;
        hist.count = 0;
        hist.time = Timer(0);
        hist.speakers = 0;
    }
}

#ifndef EA_BUILD_A124
#include "Speed/Indep/Src/EAXSound/SpeechSampleData.inl"
#endif

} // namespace Speech

// Instanciaciones del STLport de GameCube (`_STL`, `_List_iterator`, `_Nonconst_traits`): solo alli existen.
#ifdef EA_PLATFORM_GAMECUBE
namespace _STL {
template Speech::SpeechEventPair *__lower_bound<Speech::SpeechEventPair *, Speech::SpeechEventPair,
                                                std::less<Speech::SpeechEventPair>, int>(
    Speech::SpeechEventPair *, Speech::SpeechEventPair *, Speech::SpeechEventPair const &, std::less<Speech::SpeechEventPair>, int *);

template std::_List_iterator<int, std::_Nonconst_traits<int> > find<std::_List_iterator<int, std::_Nonconst_traits<int> >, int>(
    std::_List_iterator<int, std::_Nonconst_traits<int> >, std::_List_iterator<int, std::_Nonconst_traits<int> >, int const &);

template ISndAttachable **find<ISndAttachable **, ISndAttachable *>(
    ISndAttachable **, ISndAttachable **, ISndAttachable * const &);
}
#endif
