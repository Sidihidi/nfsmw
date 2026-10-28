#include "StringRegistry.h"

#ifdef EA_PLATFORM_PLAYSTATION2
StringStoreBlock::~StringStoreBlock() {
}

void StringStoreBlockPtr::operator delete(void *mem, unsigned int size) {
    if (mem) {
        gFastMem.Free(mem, size, nullptr);
    }
}

StringStoreBlockPtr::~StringStoreBlockPtr() {
    if (mData->RmvRef()) {
        delete mData;
    }
}
#endif

// Decl: StringRegistry.cpp GC
StringRegistry::StringRegistry() {
}

#ifdef EA_PLATFORM_PLAYSTATION2
StringRegistry::~StringRegistry() {
}
#endif

// Decl: StringRegistry.cpp GC
StringPool::StringPool() {
    mStrings = 0;
    mStorage = 0;
}

#ifdef EA_PLATFORM_PLAYSTATION2
StringPool::~StringPool() {
}
#endif

std::vector<StringStoreBlockPtr> StringStoreBlockPtr::sStringStoreBlockPtrCache;

StringRegistry StringRegistry::fgThis;
