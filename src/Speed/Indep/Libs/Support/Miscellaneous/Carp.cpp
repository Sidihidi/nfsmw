#include "Speed/Indep/Libs/Support/Miscellaneous/CARP.h"
#include "Speed/Indep/Libs/Support/Utility/UGroup.hpp"

#include <vector>
#include <algorithm>

struct Query {
    static void ResolveStaticData(const void *staticData, const UGroup *context);
};

struct RegisterEvent {
    static void (*ResolveEvent(unsigned int tag))(void *, const struct UGroup *);
};

namespace CARP {

// total size: 0x8
struct TagResolverNode {
    unsigned int fTag;                                // offset 0x0, size 0x4
    void (*fResolver)(const UData *, const UGroup *); // offset 0x4, size 0x4

    inline TagResolverNode(unsigned int tag = 0, void (*func)(const UData *, const UGroup *) = 0)
        : fTag(tag), fResolver(func) {}

    inline TagResolverNode(const TagResolverNode &trn) {
        fTag = trn.fTag;
        fResolver = trn.fResolver;
    }

    inline TagResolverNode &operator=(const TagResolverNode &trn) {
        fTag = trn.fTag;
        fResolver = trn.fResolver;
        return *this;
    }

    inline bool operator<(const TagResolverNode &trn) const {
        return fTag < trn.fTag;
    }
};

typedef int (*DiagnosticFunc)(const char *message, int flag);

std::vector<TagResolverNode> gDataResolverMap;
// c36an2: el ORDEN del objetivo en .data es gResolversSorted, gHaveInitialized,
// 4 B muertos, gDiagnosticFunc, gResolving, gDeltaAddress (+0x28..+0x40). Estos
// globales tienen enlace externo, asi que GCC 2.9 los emite donde se definen y
// el orden de estas cinco lineas ES el del binario.
unsigned int gResolversSorted = 0;
unsigned int gHaveInitialized = 0;
DiagnosticFunc gDiagnosticFunc = 0;
unsigned int gResolving = 0;
unsigned int gDeltaAddress = 0;

static void EventSeqEngineResolver(const UData *d, const UGroup *context) {
    const EventSeqEngine *e = static_cast<const EventSeqEngine *>(d->GetDataConst());
    new (const_cast<EventSeqEngine *>(e)) TagReference(context);

    const EventSeqSystem *const *systems = e->GetSystems();
    for (unsigned int i = 0; i < e->mNumSystems; i++) {
        new (const_cast<EventSeqSystem **>(&systems[i])) TagReference(context);
    }
}

// Decl: Carp.cpp GC
static void EventSeqSystemResolver(const UData *d, const UGroup *context) {
    const EventSeqSystem *s = static_cast<const EventSeqSystem *>(d->GetDataConst());
    const EventSeqState *const *states = s->GetStates();
    for (unsigned int i = 0; i < s->mNumStates; i++) {
        new (const_cast<EventSeqState **>(&states[i])) TagReference(context);
    }
}

// Decl: Carp.cpp GC
static void EventSeqStateResolver(const UData *d, const UGroup *context) {
    const EventSeqState *s = static_cast<const EventSeqState *>(d->GetDataConst());
    new (const_cast<StimulusFilter **>(&s->mFilter)) TagReference(context);

    const EventSeqResponse *responses = s->GetResponses();
    for (unsigned int i = 0; i < s->mNumStimuli; i++) {
        new (const_cast<EventSeqAction **>(&responses[i].mActionSeq)) TagReference(context);
    }
}

// Decl: Carp.cpp GC
static void EventSeqActionResolver(const UData *d, const UGroup *context) {
    const EventSeqAction *a = static_cast<const EventSeqAction *>(d->GetDataConst());
    const EventList *const *eventlists = a->GetEventListArray();
    for (unsigned int i = 0; i < a->mNumTags + a->mNumTimes; i++) {
        new (const_cast<EventList **>(&eventlists[i])) TagReference(context);
    }
}

// Decl: Carp.cpp GC
static void EventListResolver(const UData *d, const UGroup *context) {
    const EventList *eventList = static_cast<const EventList *>(d->GetDataConst());
    const EventStaticData *event = eventList->Event();
    unsigned int numEvents = eventList->fNumEvents;
    while (numEvents-- > 0) {
        void (*resolver)(void *, const UGroup *) = RegisterEvent::ResolveEvent(event->fEventID);
        if (resolver != 0) {
            resolver(const_cast<char *>(event->StaticData()), context);
        }
        event++;
    }
}

// Decl: Carp.cpp GC
static void StimulusFilterResolver(const UData *d, const UGroup *context) {
    const StimulusFilter *filter = static_cast<const StimulusFilter *>(d->GetDataConst());
    const QueryDesc *queries = filter->GetQueries();
    const char *staticData = filter->GetStaticData();
    for (unsigned int i = 0; i < filter->mNumQueries; i++) {
        if (queries[i].mQueryName != 0) {
            Query::ResolveStaticData(staticData + queries[i].mDataOffset, context);
        }
    }
}

// Decl: Carp.cpp GC
static void CollisionInstanceResolver(const UData *d, const UGroup *context) {
    const CollisionInstance *i = static_cast<const CollisionInstance *>(d->GetDataConst());
    for (unsigned int j = 0; j < d->DataCount(); j++, i++) {
        new (const_cast<WCollisionArticle **>(&i->fCollisionArticle)) TagReference(context);
    }
}

// Decl: Carp.cpp GC
void AddUDataTagResolver(unsigned int dataTag, void (*dtrFunc)(const UData *, const UGroup *)) {
    gDataResolverMap.push_back(TagResolverNode(dataTag, dtrFunc));
}

// Decl: Carp.cpp GC
void ResolveData(const UGroup *g, const UData *d) {
    unsigned int tag;
    if (d->fIndexed) {
        tag = (d->fTag & 0xFFFF0000U) | 0x2020U;
    } else {
        tag = d->fTag;
    }
    TagResolverNode searchNode(tag, 0);
    std::vector<TagResolverNode>::iterator node = std::lower_bound(gDataResolverMap.begin(), gDataResolverMap.end(), searchNode);
    if (node != gDataResolverMap.end() && node->fTag == searchNode.fTag) {
        node->fResolver(d, g);
    }
}

// Decl: Carp.cpp GC
void InitResolvers() {
    if (gHaveInitialized == 0) {
        gDataResolverMap.reserve(0x10);
        AddUDataTagResolver(0x63692020U, CollisionInstanceResolver);
        AddUDataTagResolver(0x65452020U, EventSeqEngineResolver);
        AddUDataTagResolver(0x65532020U, EventSeqSystemResolver);
        AddUDataTagResolver(0x65542020U, EventSeqStateResolver);
        AddUDataTagResolver(0x65412020U, EventSeqActionResolver);
        AddUDataTagResolver(0x73662020U, StimulusFilterResolver);
        AddUDataTagResolver(0x656C2020U, EventListResolver);
        std::sort(gDataResolverMap.begin(), gDataResolverMap.end());
        gHaveInitialized = 1;
        gResolversSorted = 1;
    }
}

// Decl: Carp.cpp GC
void *ResolveTagReferences(const UGroup *g, unsigned int deltaAddress) {
    if (gHaveInitialized == 0) {
        InitResolvers();
    }
    CarpResolver resolver(deltaAddress);
    g->ProcessBreadthFirst(resolver);
    const UData *data = g->DataLocateTag(0x53656374U);
    if (data != g->DataEnd()) {
        return *reinterpret_cast<void **>(const_cast<char *>(static_cast<const char *>(data->GetDataConst()) + 0x7C));
    }
    return 0;
}

// Decl: Carp.cpp GC
TagReference::TagReference(const UGroup *context) {
    if (gDiagnosticFunc != 0 && gDeltaAddress == 0) {
        int rc;
        if (fData != 0) {
            if (fData == 0xFFFFFFFFU) {
                rc = gDiagnosticFunc("Container Reference", 1);
            } else {
                const UData *data = context->DataLocateTag(fData);
                if (data == context->DataEnd()) {
                    rc = gDiagnosticFunc("Invalid TagReference", 1);
                } else {
                    unsigned int tag;
                    if (data->fIndexed) {
                        tag = (data->fTag & 0xFFFF0000U) | 0x2020U;
                    } else {
                        tag = data->fTag;
                    }
                    if (tag == 0x73722020U) {
                        rc = gDiagnosticFunc(static_cast<const char *>(data->GetDataConst()), 1);
                    } else {
                        char str[16];
                        UDataGroupEncodeTag(data->fTag, data->fIndexed, str);
                        rc = gDiagnosticFunc(str, 1);
                    }
                }
            }
        } else {
            rc = gDiagnosticFunc("Null Reference", 0);
        }
        if (rc != 0) {
            return;
        }
    }

    unsigned int ref = fData;
    if (ref == 0) {
        return;
    }
    if (gDeltaAddress != 0) {
        fData = ref + gDeltaAddress;
        return;
    }
    if (ref == 0xFFFFFFFFU) {
        fData = reinterpret_cast<unsigned int>(context);
        return;
    }
    const UData *data = context->DataLocateTag(ref);
    if (data != context->DataEnd()) {
        fData = reinterpret_cast<unsigned int>(data->GetDataConst());
    } else {
        fData = 0;
    }
}

#ifdef EA_BUILD_A124
// Linea 86 del Carp.cpp de la alpha: monta la matriz sin traslacion y la valida. Solo vive la alpha
// (la llama el dibujo de depuracion de colisiones de zWorld2).
bool Trigger::ValidateMatrix() const {
    UMath::Matrix4 m;
    MakeMatrix(m, false);
    return ::ValidateMatrix(m);
}
#endif

#if defined(EA_BUILD_A124) || defined(EA_PLATFORM_XENON)
// Lineas 88-98 del original: la fila 1 es el producto cruzado de las filas 2 y 0 (o el eje Y si el
// trigger no tiene bit 0x1000), calculado en la local n antes de volcar las filas en m. El simbolo
// tambien existe en el milestone de Xbox 360; en GC/SLUS/PC no queda copia.
void Trigger::MakeMatrix(UMath::Matrix4 &m, bool addXLate) const {
    UMath::Vector3 n;
    if ((fFlags & 0x1000) != 0) {
        UMath::Cross(Coord4To3(fMatRow2Length), Coord4To3(fMatRow0Width), n);
    } else {
        n.x = 0.0f;
        n.y = 1.0f;
        n.z = 0.0f;
    }
    m.v0.x = fMatRow0Width.x;
    m.v0.y = fMatRow0Width.y;
    m.v0.z = fMatRow0Width.z;
    m.v0.w = 0.0f;
    m[1][0] = n.x;
    m[1][1] = n.y;
    m[1][2] = n.z;
    m[1][3] = 0.0f;
    m[2][0] = fMatRow2Length.x;
    m[2][1] = fMatRow2Length.y;
    m[2][2] = fMatRow2Length.z;
    m[2][3] = 0.0f;
    if (addXLate) {
        m[3][0] = fPosRadius.x;
        m[3][1] = fPosRadius.y;
        m[3][2] = fPosRadius.z;
        m[3][3] = 1.0f;
    } else {
        m[3][0] = 0.0f;
        m[3][1] = 0.0f;
        m[3][2] = 0.0f;
        m[3][3] = 1.0f;
    }
}
#endif

// Decl: Carp.cpp GC
// `inline` NO es una optimizacion: es la POSICION. En GCC 2.9 una funcion
// inline se emite en `finish_file`, o sea al final de la unidad de traduccion.
// El objetivo tiene este simbolo en los ultimos 200 B de zFoundation, detras de
// UGroup::Processor y justo antes del `_GLOBAL_.I.`; sin `inline` sale aqui, y
// eso empuja 68 funciones. No se puede llevar el cuerpo a la clase de CARP.h
// porque usa gResolving, gDeltaAddress y ResolveData, que viven en este .cpp.
inline bool CarpResolver::StartGroup(const UGroup *group) {
    gResolving = 1;
    gDeltaAddress = mDeltaAddress;
    const UData *endD = group->DataEnd();
    for (const UData *d = group->DataBegin(); d != endD; d++) {
        ResolveData(group, d);
    }
    gResolving = 0;
    mSkipData = 1;
    return true;
}

} // namespace CARP
