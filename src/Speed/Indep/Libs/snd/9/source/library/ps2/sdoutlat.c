// La latencia de salida del driver de PS2 (C:/packages/snd/9.06.00-ps2/source/library/ps2/sdoutlat.c): no la da.

#include "sndps2.h"

int SNDPLATFORM_outputlatency() {
    return 0;
}
