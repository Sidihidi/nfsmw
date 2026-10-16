#include "./sndcmn.h"

int SNDMEM_gethighwater() {
    SNDI_CHECKINSTALLED();
    return (sndgs.mm->heapsize - sndgs.mm->lowmark) * 100 / sndgs.mm->heapsize;
}
