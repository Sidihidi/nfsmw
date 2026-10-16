#ifndef DUCK_MEM_H
#define DUCK_MEM_H

#include <stddef.h>

typedef enum tmemtype {
    DMEM_GENERAL = 0,
    DMEM_TEMP,
    DMEM_CDBUFF,
    DMEM_FRAMEBUFF
} dmemType;

#ifdef __cplusplus
extern "C" {
#endif

void *duck_memmove(void *dst, const void *src, size_t length);
void *duck_malloc(size_t blocksize, dmemType foo);
void duck_free(void *old_blk);
void *duck_memset(void *dest, int c, size_t count);
void *duck_calloc(size_t n, size_t blocksize, dmemType foo);
void *duck_memcpy(void *dest, const void *source, size_t length);
int duck_strcmp(const char *one, const char *two);

#ifdef __cplusplus
}

namespace EA {
namespace Allocator {
class IAllocator;
}
}

/* allocator.cpp: la memoria del codec pasa por el asignador que ponga el juego */
namespace Vp6 {
void SetAllocator(EA::Allocator::IAllocator *allocator);
void *Alloc(int size);
void Free(void *ptr);
}
#endif

#endif
