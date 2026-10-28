#include "../../../include/common/realmemcard/memcard_interface.h"
#if defined(EA_PLATFORM_WIN32) || defined(EA_PLATFORM_XENON)
#include "../../../include/common/realmemcard/impl/memcard_interface_impl.h"
#endif

namespace Realmc {

static EA::Allocator::IAllocator *gAllocator = nullptr;

void SetMemAllocator(EA::Allocator::IAllocator *allocator) {
    gAllocator = allocator;
}

void *AllocateMemSize(const char *pBlockName, int size, int align, int, int) {
    return gAllocator->Alloc(
        size,
        EA::TagValuePair(EA::Allocator::ATT_NAME, pBlockName) +
            EA::TagValuePair(EA::Allocator::ATT_ALIGNMENT, align));
}

void FreeMemSize(void *pBlock, int size) {
    gAllocator->Free(pBlock, size);
}

} // namespace Realmc

#if defined(EA_PLATFORM_WIN32) || defined(EA_PLATFORM_XENON)
namespace RealmcIface {

void LoadInfo::operator delete[](void *ptr) {
    Realmc::gAllocator->Free(ptr, 0);
}

} // namespace RealmcIface
#endif
