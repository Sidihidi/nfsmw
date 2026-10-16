#include <stdlib.h>

#include <Allocator/iallocator.h>

#include "duck_mem.h"

namespace Vp6 {

static EA::Allocator::IAllocator *gAllocator = NULL;

void SetAllocator(EA::Allocator::IAllocator *allocator) {
    gAllocator = allocator;
}

void *Alloc(int size) {
    if (gAllocator) {
        return gAllocator->Alloc(size, NULLALLOCTVP);
    }
    return malloc(size);
}

void Free(void *ptr) {
    if (gAllocator) {
        gAllocator->Free(ptr, 0);
    } else {
        free(ptr);
    }
}

} // namespace Vp6
