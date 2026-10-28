#include "Speed/Indep/Src/EAXSound/Stream/SpeechManager.hpp"
#include "Speed/Indep/Src/Generated/AttribSys/Classes/speech.h"
#include "Speed/Indep/Libs/Support/Utility/UStandard.h"
#include "Speed/Indep/Src/Misc/Config.h"
#include "Speed/Indep/Src/Misc/QueuedFile.hpp"
#include "Speed/Indep/bWare/Inc/bSlotPool.hpp"
#include "Speed/Indep/bWare/Inc/bWare.hpp"
#include "Speed/Indep/bWare/Inc/bMemory.hpp"
#ifdef EA_BUILD_A124
#include "Speed/Indep/Src/Frontend/DebugMenu.h"
#include "Speed/Indep/Src/Speech/SoundAI.h"
#endif

// r75: fuera 2 extern duplicados (bMalloc, bFree); bWare.hpp ya los declara.
// (bMemoryPrintAllocations se queda: no esta en ningun cabecero alcanzable).
extern void bMemoryPrintAllocations(int pool_num, int from_allocation, int to_allocation);
extern Timer WorldTimer;
int SpeechMemoryPool = 3;
bool SPEECH_CACHE_STATS = false;
#ifndef EA_BUILD_A124
int PRINT_SPEECH_CACHE_IO = 0;
#endif
int flushcount_uncached = 0;
int flushcount_lru = 0;
int flushcount_inactive_spkrs = 0;
int flushcount_all = 0;
namespace {
bool IsSpeakerActive(const Speech::Cache::VoiceIDs *speakers, int speaker_id) {
    if (!speakers) {
        return false;
    }

    Speech::Cache::VoiceIDs::const_iterator it = speakers->begin();
    while (it != speakers->end()) {
        if (*it == speaker_id) {
            return true;
        }
        ++it;
    }

    return false;
}
} // namespace

Speech::Cache gSpeechCache;

namespace Speech {

#ifdef EA_BUILD_A124
#include "Speed/Indep/Src/EAXSound/SpeechSampleData.inl"
#endif

struct SpeechLoadCBData {
    Module *object;
    SpeechSampleData *data;
};

Cache::Cache()
    : mSpeakers(0), //
      mCache(0), //
      mCacheSize(0), //
#ifdef EA_BUILD_A124
      mDefragBuf(0), //
#endif
      mInitialMemFree(0), //
      mIndex(0) {}

void Cache::Init(int memsize) {
    if (!IsSpeechEnabled || (mCache != 0)) {
        return;
    }

    mCacheSize = memsize;
    // El DWARF expande aqui el bMalloc INLINE de 4 argumentos, no el de 2: la
    // cadena muerta que le pasa el original es la que deja el `high` del
    // literal en un pseudo callee-saved y permite subirlo delante de la
    // llamada (`lis r30, ...@ha`).
    mCache = bMalloc(memsize, "Speech Cache Memory Pool", 0, 0);

    // El DWARF de Cache::Init NO declara ninguna local.
    SpeechMemoryPool = bGetFreeMemoryPoolNum();
    bInitMemoryPool(SpeechMemoryPool, mCache, memsize, "Speech Cache Memory Pool");

    mSpeakers = new ("Speech Cache speaker list", 0) VoiceIDs;

    mEventPool = bNewSlotPool(0x80, 0xA0, "CACHE: Speech Event slotpool", SpeechMemoryPool);
    mEventPool->ClearFlag(SLOTPOOL_FLAG_OVERFLOW_IF_FULL);

    mInitialMemFree = bCountFreeMemory(SpeechMemoryPool);
    mIndex.Reserve(100);
}

void Cache::Dump() {
    if (mEventPool) {
        bDeleteSlotPool(mEventPool);
    }

    if (mCache) {
        bFree(mCache);
        bCloseMemoryPool(SpeechMemoryPool);
        mCache = 0;
        mCacheSize = 0;
    }

    unsigned int index = mIndex.GetNextValidIndex(-1);
    while (mIndex.ValidIndex(index)) {
        SpeechSampleData::Destruct(mIndex.GetPtrAtIndex(index));
        index = mIndex.GetNextValidIndex(index);
    }

    mIndex.Clear();

    if (mSpeakers) {
        mSpeakers->clear();
        delete mSpeakers;
    }
    mSpeakers = 0;
}

SlotPool *Cache::GetEventPool() {
    return mEventPool;
}

Cache::~Cache() {
    Dump();
}

bool Cache::IsCached(SPCHType_SampleRequestData *data, bool check_preparedness) {
    if (!IsSpeechEnabled) {
        return false;
    }

    unsigned long long indexKey = CreateKey(data->bankNum, data->sampleOffset);
    SpeechSampleData *sample = mIndex.Find(indexKey);
    if (!sample) {
        return false;
    }

    return check_preparedness ? sample->ready : true;
}

unsigned long long Cache::CreateKey(int bank, int offset) {
    unsigned long long x = static_cast<unsigned long long>(bank);
    unsigned long long y = static_cast<unsigned long long>(offset);
    unsigned long long result;
    x = (x | x << 16) & 0xffff0000ffffULL;
    x = (x | x << 8) & 0xff00ff00ff00ffULL;
    x = (x | x << 4) & 0xf0f0f0f0f0f0f0fULL;
    x = (x | x << 2) & 0x3333333333333333ULL;
    x = (x | x << 1) & 0x5555555555555555ULL;

    y = (y | y << 16) & 0xffff0000ffffULL;
    y = (y | y << 8) & 0xff00ff00ff00ffULL;
    y = (y | y << 4) & 0xf0f0f0f0f0f0f0fULL;
    y = (y | y << 2) & 0x3333333333333333ULL;
    y = (y | y << 1) & 0x5555555555555555ULL;

    result = x | y << 1;
    return result;
}

SpeechSampleData *Cache::GetUncached(Module *module, SPCHType_SampleRequestData *data) {
    unsigned long long indexKey;
    SpeechSampleData *rval;

    indexKey = CreateKey(data->bankNum, data->sampleOffset);
    rval = mIndex.Find(indexKey);
    if (!rval) {
        rval = MakeSpaceFor(data, false);
        rval->cached = false;
        rval->ready = true;
        rval->t_req = WorldTimer;
        rval->t_play = Timer(0);
        rval->Unlock();
        rval->dataoffset = module->GetBankOffset(data->bankNum) + data->sampleOffset;
        mIndex.Add(indexKey, rval);
    }

    return rval;
}

SpeechSampleData *Cache::GetSample(Module *, SPCHType_SampleRequestData *data) {
    unsigned long long indexKey;
    SpeechSampleData *rval;

    indexKey = CreateKey(data->bankNum, data->sampleOffset);
    rval = mIndex.Find(indexKey);
    if (rval && rval->size == data->numBytes && rval->ready) {
        ++rval->age;
        rval->t_req = WorldTimer;
        rval->t_play = Timer(0);
        rval->Lock();
    }
    return rval;
}

SpeechSampleData *Cache::LoadSample(Module *module, SPCHType_SampleRequestData *data) {
    SpeechSampleData *sample;
    unsigned long long key;
    unsigned int baseoffset;
    SpeechLoadCBData *CBdata;

#if defined(EA_BUILD_A124)
    if (static_cast<float>(data->numBytes) > 15360.0f) {
#elif defined(EA_PLATFORM_PLAYSTATION2)
    if (static_cast<float>(data->numBytes) > 10240.0f) {
#else
    if (static_cast<float>(data->numBytes) > 5529.6f) {
#endif
        return 0;
    }

    sample = MakeSpaceFor(data, true);
    if (!sample) {
        if (SPEECH_CACHE_STATS) {
            DebugPrints();
        }

        return 0;
    }

    key = CreateKey(data->bankNum, data->sampleOffset);
    mIndex.Add(key, sample);

    baseoffset = module->GetBankOffset(data->bankNum);
    sample->dataoffset = baseoffset + data->sampleOffset;

#ifdef EA_BUILD_A124
    CBdata = new SpeechLoadCBData; // la A124 llama a __builtin_new
#else
    // gcdatos4 (medido y REVERTIDO): la receta gcdatos1-C, `new
    // ("SpeechLoadCBData", 0)`, si interna el literal de obj+0x794 (falta
    // 647->631) y LoadSample sigue casada al 100 % en las 5, pero la forma
    // con nombre reordena la emision de .data de la TU (los 32 `_name.*` de
    // DECLARE_CONTAINER_TYPE saltan a la cabeza, seccion 1496->1552 B) y el
    // .data de zSpeech GC cae 100->73,6930 % (matched_data 8716->7220).
    // Probadas la forma escalar y la de array, con y sin comentario: las dos
    // rompen igual. El literal sigue faltando; ver gcdatos4.md.
    CBdata = new SpeechLoadCBData[1];
#endif
    CBdata->data = sample;
    CBdata->object = module;

    AddQueuedFile(sample->GetData(), module->GetFilename(), sample->dataoffset, sample->size, Cache::LoadedSampleDataCB,
                  reinterpret_cast<int>(CBdata), 0);
    return sample;
}

void Cache::LoadedSampleDataCB(int param, int error_status) {
    SpeechLoadCBData *CBdata = reinterpret_cast<SpeechLoadCBData *>(static_cast<unsigned int>(param));
    CBdata->data->ready = true;
    ++CBdata->data->age;
    CBdata->data->t_load = WorldTimer;
    delete CBdata;
}

void *Cache::Alloc(int size, unsigned int key) {
    void *memptr = nullptr;
    if (size <= bLargestMalloc(SpeechMemoryPool)) {
        const char *allocname = "Speech::Cache::Alloc";
        // La A124 pide el bloque con 0x400 (ori 0x400 en su Alloc); SLUS y GameCube, con 0x1000.
#ifdef EA_BUILD_A124
        memptr = bMalloc(size, allocname, 0, (SpeechMemoryPool & 0xf) | 0x400);
#else
        memptr = bMalloc(size, allocname, 0, (SpeechMemoryPool & 0xf) | 0x1000);
#endif
#ifdef EA_BUILD_A124
    } else if (PRINT_SND_IO_DEBUG) { // la A124 mira el aviso de E/S de sonido (declarado en SpeechManager.cpp)
#else
    } else if (SPEECH_CACHE_STATS) {
#endif
        DebugPrintAllocations();
    }
    return memptr;
}

void Cache::Free(void *mem) {
    if (mem) {
        bFree(mem);
    }
}

#ifdef EA_BUILD_A124
void Cache::Defrag() {}

#endif
SpeechSampleData *Cache::MakeSpaceFor(SPCHType_SampleRequestData *data, bool cached) {
    short attempt;
    unsigned int key;
    SpeechSampleData *sample;
    unsigned int highwater_idx;

    key = Manager::mHashMap.GetHash(static_cast<SPCHType_1_EventID>(data->eventSpec.eventID));
    attempt = 0;
    sample = 0;

    highwater_idx = static_cast<unsigned int>(static_cast<float>(mIndex.Capacity()) * 0.98f);
    if (mIndex.Size() <= highwater_idx) {
        sample = SpeechSampleData::Construct(data, key, cached);
    }

    while (!sample && attempt >= 0) {
        switch (attempt) {
        case 0:
            FlushUncached();
            break;
        case 1:
            FlushInactiveSpeakers();
            break;
        case 2:
            FlushLRU();
            break;
        case 3:
            FlushAllUnlocked();
#ifdef EA_BUILD_A124
            Defrag();
#endif
            break;
        default:
            attempt = -1;
            break;
        }

        if (attempt >= 0) {
            ++attempt;
            highwater_idx = static_cast<unsigned int>(static_cast<float>(mIndex.Capacity()) * 0.98f);
            if (mIndex.Size() <= highwater_idx) {
                sample = SpeechSampleData::Construct(data, key, cached);
            }
        }
    }

    return sample;
}

void Cache::TossSample(SpeechSampleData *data) {
#ifdef EA_BUILD_A124
    if (PRINT_SND_IO_DEBUG) { // la A124 mira el aviso de E/S de sonido, aqui y abajo
#else
    if (SPEECH_CACHE_STATS) {
#endif
        Attrib::Gen::speech speech(Manager::mHashMap.GetHash(data->eventID), 0, 0);
    }

    unsigned int index = mIndex.GetNextValidIndex(-1);
    while (mIndex.ValidIndex(index)) {
        SpeechSampleData *sample = mIndex.GetPtrAtIndex(index);
#ifdef EA_PLATFORM_WIN32
        if (sample == data && !sample->lock) {
            SpeechSampleData::Destruct(sample);
#else
        if (sample == data && !data->lock) {
            SpeechSampleData::Destruct(data);
#endif
            mIndex.DeleteIndex(index);
#ifdef EA_BUILD_A124
            if (PRINT_SND_IO_DEBUG) {
#else
            if (SPEECH_CACHE_STATS) {
#endif
                Attrib::Gen::speech speech(Manager::mHashMap.GetHash(data->eventID), 0, 0);
            }
            return;
        }
        index = mIndex.GetNextValidIndex(index);
    }
}

void Cache::FlushUncached() {
    unsigned int index = mIndex.GetNextValidIndex(-1);
    while (mIndex.ValidIndex(index)) {
        SpeechSampleData *sample = mIndex.GetPtrAtIndex(index);
        if (!sample->lock && sample->ready && !sample->cached) {
#ifndef EA_BUILD_A124
            if (PRINT_SPEECH_CACHE_IO) {
                Attrib::Gen::speech speech(Manager::mHashMap.GetHash(sample->eventID), 0, 0);
            }
#endif
            SpeechSampleData::Destruct(sample);
            mIndex.DeleteIndex(index);
        }
        index = mIndex.GetNextValidIndex(index);
    }

    ++flushcount_uncached;
}

void Cache::FlushLRU() {
    bLargestMalloc(SpeechMemoryPool);

    unsigned int prelargest = 0;
    unsigned int index = mIndex.GetNextValidIndex(-1);
    unsigned int postlargest = 0;

    while (mIndex.ValidIndex(index)) {
        SpeechSampleData *sample = mIndex.GetPtrAtIndex(index);
        if (!sample->lock) {
            if (sample->ready && sample->cached && sample->age < 2) {
                SpeechSampleData::Destruct(sample);
                mIndex.DeleteIndex(index);
            }
        }
        index = mIndex.GetNextValidIndex(index);
    }

    bLargestMalloc(SpeechMemoryPool);
    ++flushcount_lru;
}

void Cache::FlushAllUnlocked() {
    bLargestMalloc(SpeechMemoryPool);

    unsigned int prelargest = 0;
    unsigned int index = mIndex.GetNextValidIndex(-1);
    unsigned int postlargest = 0;

    while (mIndex.ValidIndex(index)) {
        SpeechSampleData *sample = mIndex.GetPtrAtIndex(index);
        if (!sample->lock && sample->ready && sample->cached) {
            SpeechSampleData::Destruct(sample);
            mIndex.DeleteIndex(index);
        }

        index = mIndex.GetNextValidIndex(index);
    }

    bLargestMalloc(SpeechMemoryPool);
    ++flushcount_all;
}

void Cache::FlushInactiveSpeakers() {
    bLargestMalloc(SpeechMemoryPool);

    unsigned int prelargest = 0;
    unsigned int index = mIndex.GetNextValidIndex(-1);
    unsigned int postlargest = 0;

    while (mIndex.ValidIndex(index)) {
        SpeechSampleData *sample = mIndex.GetPtrAtIndex(index);
        if (!sample->lock && sample->ready && sample->cached) {
            bool inactive = true;
            for (VoiceIDs::iterator i = mSpeakers->begin(); i != mSpeakers->end(); i++) {
                if ((*i == sample->speakerID) || (sample->speakerID == 0xffff)) {
                    inactive = false;
                }
            }

            if (inactive) {
                SpeechSampleData::Destruct(sample);
                mIndex.DeleteIndex(index);
            }
        }

        index = mIndex.GetNextValidIndex(index);
    }

    bLargestMalloc(SpeechMemoryPool);
    ++flushcount_inactive_spkrs;
}

void Cache::DebugPrintAllocations() {
    bMemoryPrintAllocations(SpeechMemoryPool, 0, 0x7fffffff);

    unsigned int index = mIndex.GetNextValidIndex(-1);
    for (;;) {
        int valid = mIndex.ValidIndex(index);
        if (valid == 0) {
            break;
        }
        SpchSampleMap *map = &mIndex;
        SpeechSampleData *sample = mIndex.GetPtrAtIndex(index);
        Attrib::Gen::speech speech(Manager::mHashMap.GetHash(sample->eventID), 0, 0);
        index = map->GetNextValidIndex(index);
    }
}

void Cache::Validate() {
}

#ifdef EA_BUILD_A124 // solo el prototipo pinta en pantalla el estado de la cache de voz
void Cache::DebugPrints() {
    int poolallocs = 0;
    int poolfree = 0;
    int poollargest_free = 0;
    int dummy = 0;
    const char *dbgstring = "Speech cache data";

    int totalcachedsamples = 0;
    int totalcachedbytes = 0;
    static int mostfreqspkr = 0;
    static int maxage = 0;
    int numlocked = 0;
    static unsigned int maxsize = 0;
    static int avgsamplesz = 0;
    unsigned int blip_size = 0;
    unsigned int blip_count = 0;
    unsigned int index;

    for (index = mIndex.GetNextValidIndex(-1); mIndex.ValidIndex(index); index = mIndex.GetNextValidIndex(index)) {
        SpeechSampleData *sample = mIndex.GetPtrAtIndex(index);
        bool cached = sample->cached;

        if (sample->lock) {
            numlocked++;
        }

        if (cached) {
            totalcachedbytes += sample->size;
            totalcachedsamples++;
            if (sample->size > maxsize) {
                maxsize = sample->size;
            }
        }

        if (sample->age > maxage && sample->speakerID > 10) {
            maxage = sample->age;
            mostfreqspkr = sample->speakerID;
        }

        if (sample->speakerID > 10 && cached) {
            blip_size += sample->size;
            blip_count++;
        }
    }

    avgsamplesz = totalcachedsamples > 0 ? totalcachedbytes / totalcachedsamples : avgsamplesz;
    float pct_cached_blips = static_cast<float>(blip_size) / 307200.0f;
    float pct_samples_blips = totalcachedsamples ? static_cast<float>(blip_count) / static_cast<float>(totalcachedsamples) : 0.0f;

    bMemoryGetStatistics(SpeechMemoryPool, poolallocs, dummy, dummy, dummy, poolfree, poollargest_free, dbgstring);

    if (!UTL::Collections::Singleton<SoundAI>::Get()) {
        return;
    }

    const UMath::Vector2 &coords = UTL::Collections::Singleton<SoundAI>::Get()->GetTune().CacheDisplayCoords();
    int x = static_cast<int>(coords.x);
    int y = static_cast<int>(coords.y);

    ScreenPrintf(x, y, "SPEECH CACHE (%d/%d)", mIndex.Size(), mIndex.Capacity());
    y += 15;
    ScreenPrintf(x, y, "Free: %d", poolfree);
    y += 15;
    ScreenPrintf(x, y, "MaxFreeBlock: %d", poollargest_free);
    y += 15;
    ScreenPrintf(x, y, "Total: %d", poolallocs);
    y += 15;
    ScreenPrintf(x, y, "Flush:%d/%d/%d/%d", flushcount_uncached, flushcount_inactive_spkrs, flushcount_lru, flushcount_all);
    y += 15;
    ScreenPrintf(x, y, "#Bytes: %d", totalcachedbytes);
    y += 15;
    ScreenPrintf(x, y, "#Locked: %d", numlocked);
    y += 15;
    ScreenPrintf(x, y, "Max cache_sz: %d", maxsize);
    y += 15;
    ScreenPrintf(x, y, "Avg cache_sz: %d", avgsamplesz);
    y += 15;
    ScreenPrintf(x, y, "Popular: %d x %d", mostfreqspkr, maxage);
    y += 30;
    ScreenPrintf(x, y, "PctCacheBlips: %.1f", pct_cached_blips * 100.0f);
    y += 15;
    ScreenPrintf(x, y, "PctSampleBlips: %.1f", pct_samples_blips * 100.0f);
    y += 15;
    ScreenPrintf(x, y, "#Blips: %d", blip_count);

    if (mIndex.Size() != poolallocs - 1) {
        DebugPrintAllocations();
    }
}
#else
void Cache::DebugPrints() {}
#endif

} // namespace Speech
