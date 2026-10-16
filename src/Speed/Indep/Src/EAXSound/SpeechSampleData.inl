// SpeechSampleData::Construct/Destruct y los metodos de ScheduledSpeechEvent. En la version de venta (GameCube y SLUS)
// van en EaxSoundTypes.cpp (zEAXSound); la A124 los tenia al principio de SpeechCache.cpp (zSpeech, lineas 42-120
// de su .mdebug). Se escriben una vez y los incluye el fichero de cada version, dentro de namespace Speech.

void SpeechSampleData::Destruct(SpeechSampleData *ptr) {
    ptr->~SpeechSampleData();
    gSpeechCache.Free(ptr);
}

SpeechSampleData *SpeechSampleData::Construct(SPCHType_SampleRequestData *data, unsigned int key, bool is_cached) {
    unsigned int total;
    if (is_cached) {
        total = data->numBytes + 0x40;
    } else {
        total = 0x40;
    }

    void *ptr = gSpeechCache.Alloc(total, key);
    if (ptr == nullptr) {
        return nullptr;
    }

    return new (ptr) SpeechSampleData(data, is_cached);
}

ScheduledSpeechEvent::ScheduledSpeechEvent()
    : iid(nullptr), //
      fh(nullptr), //
      ID(kSPCH1_EventID_MaxEventID), //
      actor(nullptr), //
      entry_time(WorldTimer), //
      playback_time(WorldTimer), //
      finish_time(0), //
      assoc_samples_count(0), //
      assoc_samples_prep(0), //
      curndx(0), //
      priority(0), //
      frameindex(Manager::m_frameindex), //
      flags(0) {
    Manager::m_frameindex = static_cast<short>(Manager::m_frameindex + 1);

    for (short i = 0; i < 7; i = static_cast<short>(i + 1)) {
        this->assoc_samples[i] = nullptr;
    }
}

ScheduledSpeechEvent::~ScheduledSpeechEvent() {
    for (short i = 0; i < 7; ++i) {
        SpeechSampleData *stitch = this->assoc_samples[i];
        if (stitch != nullptr && stitch->lock == true) {
            stitch->Unlock();
        }
        this->assoc_samples[i] = nullptr;
    }

    this->assoc_samples_prep = 0;
    this->curndx = 0;
    SampleReqList &requests = Manager::GetSampleRequests();
    if (requests.size() != 0) {
        for (SampleReqList::iterator i = requests.begin(); i != requests.end();) {
            if (i->owner == this) {
                requests.erase(i);
            } else {
                ++i;
            }
        }
    }
}

void *ScheduledSpeechEvent::operator new(unsigned int base_size, unsigned int xtra) {
    unsigned int total = base_size + xtra;
    (void)total;

    if (!gSpeechCache.GetEventPool()) {
        return NullPointer;
    }
    if (gSpeechCache.GetEventPool() != nullptr && gSpeechCache.GetEventPool()->IsFull()) {
        return NullPointer;
    }
    return gSpeechCache.GetEventPool()->Malloc(1, nullptr);
}

void ScheduledSpeechEvent::operator delete(void *ptr) {
    gSpeechCache.GetEventPool()->Free(ptr);
}

bool ScheduledSpeechEvent::sort_nested_priority(const ScheduledSpeechEvent *lhs, const ScheduledSpeechEvent *rhs) {
    if (lhs->priority == rhs->priority) {
        if (lhs->entry_time == rhs->entry_time) {
            return lhs->frameindex < rhs->frameindex;
        }
        return lhs->entry_time < rhs->entry_time;
    }
    return lhs->priority > rhs->priority;
}

#ifdef EA_BUILD_A124
void *ScheduledSpeechEvent::GetData() {
    return this + 1;
}
#else
void *ScheduledSpeechEvent::GetData(unsigned int *datasize) {
    unsigned int ptr = reinterpret_cast<unsigned int>(this + 1);
    if (datasize != nullptr) {
        *datasize = 0x40;
    }
    return reinterpret_cast<void *>(ptr);
}
#endif

unsigned char ScheduledSpeechEvent::ReserveSample() {
    unsigned char requested_index = this->curndx;
    this->curndx++;
    return requested_index;
}

void ScheduledSpeechEvent::AddSample(SpeechSampleData *sample, unsigned char specific_index) {
    sample->Lock();
    if (specific_index != 0xFF) {
        this->assoc_samples[specific_index] = sample;
        return;
    }

    this->assoc_samples[this->curndx] = sample;
    this->curndx = static_cast<unsigned char>(this->curndx + 1);
}
