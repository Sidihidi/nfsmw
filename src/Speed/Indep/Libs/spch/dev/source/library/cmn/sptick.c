#include "spch/spch.h"

extern SPCHType_ExtVecs gExtVecs;

int SPCHEXT_gettick(void) {
    int result = 0;
    if (gExtVecs.spchGetTick != 0) {
        result = gExtVecs.spchGetTick();
    }
    return result;
}
