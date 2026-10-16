#include "duck_mem.h"

void *duck_malloc(size_t blocksize, dmemType foo) {
    void *temp = Vp6::Alloc(blocksize);
    return temp;
}

void duck_free(void *old_blk) {
    Vp6::Free(old_blk);
}
