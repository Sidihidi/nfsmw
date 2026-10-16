#include <math.h>
#include <vector>
#include <algorithm>

#include "Speed/Indep/Libs/Support/Miscellaneous/CARP.h"
#include "Speed/Indep/Libs/Support/Miscellaneous/StringHash.h"

static CARP::ExprValType sinf(CARP::ExprValType v) {
    CARP::ExprValType result;

    result.f = sinf(v.f);
    return result;
}

static CARP::ExprValType cosf(CARP::ExprValType v) {
    CARP::ExprValType result;

    result.f = cosf(v.f);
    return result;
}

static CARP::ExprValType tanf(CARP::ExprValType v) {
    CARP::ExprValType result;

    result.f = tanf(v.f);
    return result;
}

static CARP::ExprValType asinf(CARP::ExprValType v) {
    CARP::ExprValType result;

    result.f = asinf(v.f);
    return result;
}

static CARP::ExprValType acosf(CARP::ExprValType v) {
    CARP::ExprValType result;

    result.f = acosf(v.f);
    return result;
}

static CARP::ExprValType atanf(CARP::ExprValType v) {
    CARP::ExprValType result;

    result.f = atanf(v.f);
    return result;
}

static CARP::ExprValType atan2f(CARP::ExprValType v1, CARP::ExprValType v2) {
    CARP::ExprValType result;

    result.f = atan2f(v1.f, v2.f);
    return result;
}

static CARP::ExprValType sinhf(CARP::ExprValType v) {
    CARP::ExprValType result;

    result.f = sinhf(v.f);
    return result;
}

static CARP::ExprValType coshf(CARP::ExprValType v) {
    CARP::ExprValType result;

    result.f = coshf(v.f);
    return result;
}

static CARP::ExprValType tanhf(CARP::ExprValType v) {
    CARP::ExprValType result;

    result.f = tanhf(v.f);
    return result;
}

static CARP::ExprValType expf(CARP::ExprValType v) {
    CARP::ExprValType result;

    result.f = expf(v.f);
    return result;
}

static CARP::ExprValType logf(CARP::ExprValType v) {
    CARP::ExprValType result;

    result.f = logf(v.f);
    return result;
}

static CARP::ExprValType log10f(CARP::ExprValType v) {
    CARP::ExprValType result;

    result.f = log10f(v.f);
    return result;
}

static CARP::ExprValType fmodf(CARP::ExprValType v1, CARP::ExprValType v2) {
    CARP::ExprValType result;

    result.f = fmodf(v1.f, v2.f);
    return result;
}

static CARP::ExprValType powf(CARP::ExprValType v1, CARP::ExprValType v2) {
    CARP::ExprValType result;

    result.f = powf(v1.f, v2.f);
    return result;
}

static CARP::ExprValType sqrtf(CARP::ExprValType v) {
    CARP::ExprValType result;

    result.f = sqrtf(v.f);
    return result;
}

static CARP::ExprValType ceilf(CARP::ExprValType v) {
    CARP::ExprValType result;

    result.f = ceilf(v.f);
    return result;
}

static CARP::ExprValType fabsf(CARP::ExprValType v) {
    CARP::ExprValType result;

    result.f = fabsf(v.f);
    return result;
}

static CARP::ExprValType floorf(CARP::ExprValType v) {
    CARP::ExprValType result;

    result.f = floorf(v.f);
    return result;
}

static CARP::ExprValType roundf(CARP::ExprValType v) {
    CARP::ExprValType result;

    result.f = floorf(v.f + ((v.f < 0.0f) ? -0.5f : 0.5f));
    return result;
}

static CARP::ExprValType hypotf(CARP::ExprValType x, CARP::ExprValType y) {
    CARP::ExprValType result;

    result.f = sqrtf(x.f * x.f + y.f * y.f);
    return result;
}

static CARP::ExprValType hypot3d(CARP::ExprValType x, CARP::ExprValType y, CARP::ExprValType z) {
    CARP::ExprValType result;

    result.f = sqrtf(x.f * x.f + y.f * y.f + z.f * z.f);
    return result;
}

struct FuncDesc {
    unsigned int mNameHash;

    union {
        CARP::ExprValType (*mFunc1)(CARP::ExprValType);
        CARP::ExprValType (*mFunc2)(CARP::ExprValType, CARP::ExprValType);
        CARP::ExprValType (*mFunc3)(CARP::ExprValType, CARP::ExprValType, CARP::ExprValType);
        CARP::ExprValType (*mFunc4)(CARP::ExprValType, CARP::ExprValType, CARP::ExprValType, CARP::ExprValType);
    };

    inline FuncDesc(unsigned int nameHash)
        : mNameHash(nameHash) {}

    inline FuncDesc(const char *name, CARP::ExprValType (*func)(CARP::ExprValType))
        : mNameHash(stringhash32(name)), mFunc1(func) {}

    inline FuncDesc(const char *name, CARP::ExprValType (*func)(CARP::ExprValType, CARP::ExprValType))
        : mNameHash(stringhash32(name)), mFunc2(func) {}

    inline FuncDesc(const char *name, CARP::ExprValType (*func)(CARP::ExprValType, CARP::ExprValType, CARP::ExprValType))
        : mNameHash(stringhash32(name)), mFunc3(func) {}

    inline FuncDesc(const char *name, CARP::ExprValType (*func)(CARP::ExprValType, CARP::ExprValType, CARP::ExprValType, CARP::ExprValType))
        : mNameHash(stringhash32(name)), mFunc4(func) {}

    inline FuncDesc(const FuncDesc &src) {
        mNameHash = src.mNameHash;
        mFunc1 = src.mFunc1;
    }

    inline bool operator<(const FuncDesc &rhs) const {
        return mNameHash < rhs.mNameHash;
    }
};

struct FuncMap : public std::vector<FuncDesc> {
    inline FuncMap() {}
};

static FuncMap gFunctions[5];
static bool gInited = 0;

static void InitializeTables() {
    if (gInited) {
        return;
    }

    gFunctions[1].push_back(FuncDesc("sin", sinf));
    gFunctions[1].push_back(FuncDesc("cos", cosf));
    gFunctions[1].push_back(FuncDesc("tan", tanf));
    gFunctions[1].push_back(FuncDesc("asin", asinf));
    gFunctions[1].push_back(FuncDesc("acos", acosf));
    gFunctions[1].push_back(FuncDesc("atan", atanf));
    gFunctions[2].push_back(FuncDesc("atan2", atan2f));
    gFunctions[1].push_back(FuncDesc("sinh", sinhf));
    gFunctions[1].push_back(FuncDesc("cosh", coshf));
    gFunctions[1].push_back(FuncDesc("tanh", tanhf));
    gFunctions[1].push_back(FuncDesc("exp", expf));
    gFunctions[1].push_back(FuncDesc("log", logf));
    gFunctions[1].push_back(FuncDesc("log10", log10f));
    gFunctions[2].push_back(FuncDesc("mod", fmodf));
    gFunctions[2].push_back(FuncDesc("pow", powf));
    gFunctions[1].push_back(FuncDesc("sqrt", sqrtf));
    gFunctions[1].push_back(FuncDesc("ceil", ceilf));
    gFunctions[1].push_back(FuncDesc("abs", fabsf));
    gFunctions[1].push_back(FuncDesc("floor", floorf));
    gFunctions[1].push_back(FuncDesc("round", roundf));
    gFunctions[2].push_back(FuncDesc("hypot", hypotf));
    gFunctions[3].push_back(FuncDesc("hypot3d", hypot3d));

    for (unsigned int i = 0; i < 5; ++i) {
        std::sort(gFunctions[i].begin(), gFunctions[i].end());
    }

    gInited = true;
}

static inline CARP::ExprValType andf(CARP::ExprValType a, CARP::ExprValType b) {
    bool bBool = b.f != 0.0f;
    bool aBool = a.f != 0.0f;
    CARP::ExprValType r;

    r.f = (aBool && bBool) ? 1.0f : 0.0f;

    return r;
}

static inline CARP::ExprValType orf(CARP::ExprValType a, CARP::ExprValType b) {
    bool bBool = b.f != 0.0f;
    bool aBool = a.f != 0.0f;
    CARP::ExprValType r;

    r.f = (aBool || bBool) ? 1.0f : 0.0f;

    return r;
}

static inline CARP::ExprValType func1f(unsigned int f, CARP::ExprValType p1) {
    FuncMap::iterator iter = std::lower_bound(gFunctions[1].begin(), gFunctions[1].end(), FuncDesc(f));

    return iter->mFunc1(p1);
}

static inline CARP::ExprValType func2f(unsigned int f, CARP::ExprValType p1, CARP::ExprValType p2) {
    FuncMap::iterator iter = std::lower_bound(gFunctions[2].begin(), gFunctions[2].end(), FuncDesc(f));

    return iter->mFunc2(p1, p2);
}

// OJO: el objetivo pasa UN SOLO argumento en func3f/func4f (r3=sret, r4=&p1 y
// nada mas); es un bug de copia-pega del original y hay que reproducirlo.
static inline CARP::ExprValType func3f(unsigned int f, CARP::ExprValType p1, CARP::ExprValType p2, CARP::ExprValType p3) {
    FuncMap::iterator iter = std::lower_bound(gFunctions[3].begin(), gFunctions[3].end(), FuncDesc(f));

    return iter->mFunc1(p1);
}

static inline CARP::ExprValType func4f(unsigned int f, CARP::ExprValType p1, CARP::ExprValType p2, CARP::ExprValType p3, CARP::ExprValType p4) {
    FuncMap::iterator iter = std::lower_bound(gFunctions[4].begin(), gFunctions[4].end(), FuncDesc(f));

    return iter->mFunc1(p1);
}

enum ExpressionOpCode {
    OP_NOP = 0,
    OP_PARAM = 1,
    OP_LOOKUP = 2,
    OP_ADD = 3,
    OP_SUB = 4,
    OP_MUL = 5,
    OP_DIV = 6,
    OP_POW = 7,
    OP_NEG = 8,
    OP_NOT = 9,
    OP_AND = 10,
    OP_OR = 11,
    OP_EQ = 12,
    OP_EQ2 = 13,
    OP_LT = 14,
    OP_LE = 15,
    OP_GT = 16,
    OP_GE = 17,
    OP_SELECT = 18,
    OP_FUNC1 = 19,
    OP_FUNC2 = 20,
    OP_FUNC3 = 21,
    OP_FUNC4 = 22,
};

CARP::ExprValType CARP::ExpressionEvaluator(
    const Expression *expr,
    ExprValType (*lookup)(unsigned int, unsigned int, const void *, const ExprValType *),
    const void *context,
    const ExprValType *values
) {
    // Orden de declaracion tomado LITERAL del volcado DWARF: kMaxEvalStackSize,
    // evalStack, opCodePtr, paramPtr, subindex, top, count.
    const unsigned int kMaxEvalStackSize = 128;
    CARP::ExprValType evalStack[kMaxEvalStackSize];
    const unsigned char *opCodePtr;
    const unsigned int *paramPtr;
    unsigned int subindex = 0;
    CARP::ExprValType *top = evalStack;
    unsigned int count;

    InitializeTables();

    opCodePtr = expr->OpCodes();
    paramPtr = expr->ParamNames();

    count = expr->mNumOpCodes;
    while (count-- > 0) {
        CARP::ExprValType a;
        CARP::ExprValType b;
        CARP::ExprValType c;
        CARP::ExprValType d;

        switch (*opCodePtr++) {
        case OP_NOP:
            break;

        case OP_PARAM:
            (top++)->u = *paramPtr++;
            break;

        case OP_LOOKUP:
            *top++ = lookup(*paramPtr++, subindex, context, values);
            break;

        case OP_ADD:
            b = *--top;
            a = *--top;
            (top++)->f = a.f + b.f;
            break;

        case OP_SUB:
            b = *--top;
            a = *--top;
            (top++)->f = a.f - b.f;
            break;

        case OP_MUL:
            b = *--top;
            a = *--top;
            (top++)->f = a.f * b.f;
            break;

        case OP_DIV:
            b = *--top;
            a = *--top;
            (top++)->f = a.f / b.f;
            break;

        case OP_POW:
            b = *--top;
            a = *--top;
            (top++)->f = powf(a.f, b.f);
            break;

        case OP_NEG:
            a = *--top;
            (top++)->f = -a.f;
            break;

        case OP_NOT:
            a = *--top;
            (top++)->f = (a.f != 0.0f) ? 0.0f : 1.0f;
            break;

        case OP_AND:
            b = *--top;
            a = *--top;
            *top++ = andf(a, b);
            break;

        case OP_OR:
            b = *--top;
            a = *--top;
            *top++ = orf(a, b);
            break;

        case OP_EQ:
            b = *--top;
            a = *--top;
            (top++)->f = (a.f == b.f) ? 1.0f : 0.0f;
            break;

        case OP_EQ2:
            b = *--top;
            a = *--top;
            (top++)->f = (a.f == b.f) ? 1.0f : 0.0f;
            break;

        case OP_LT:
            b = *--top;
            a = *--top;
            (top++)->f = (a.f < b.f) ? 1.0f : 0.0f;
            break;

        case OP_LE:
            b = *--top;
            a = *--top;
            (top++)->f = (a.f <= b.f) ? 1.0f : 0.0f;
            break;

        case OP_GT:
            b = *--top;
            a = *--top;
            (top++)->f = (a.f > b.f) ? 1.0f : 0.0f;
            break;

        case OP_GE:
            b = *--top;
            a = *--top;
            (top++)->f = (a.f >= b.f) ? 1.0f : 0.0f;
            break;

        case OP_SELECT:
            c = *--top;
            b = *--top;
            a = *--top;
            *top++ = (a.f != 0.0f) ? b : c;
            break;

        case OP_FUNC1:
            a = *--top;
            *top++ = func1f(*paramPtr++, a);
            break;

        case OP_FUNC2:
            b = *--top;
            a = *--top;
            *top++ = func2f(*paramPtr++, a, b);
            break;

        case OP_FUNC3:
            c = *--top;
            b = *--top;
            a = *--top;
            *top++ = func3f(*paramPtr++, a, b, c);
            break;

        case OP_FUNC4:
            d = *--top;
            c = *--top;
            b = *--top;
            a = *--top;
            *top++ = func4f(*paramPtr++, a, b, c, d);
            break;
        }
    }

    return *top;
}
