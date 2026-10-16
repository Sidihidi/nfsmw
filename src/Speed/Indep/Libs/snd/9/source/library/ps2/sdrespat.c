// La direccion de la muestra de un timbre cargado de forma asincrona (C:/packages/snd/9.06.00-ps2/source/library/ps2/
// sdrespat.c): la misma cuenta que el driver de GameCube.

#include "sndps2.h"

int SNDPLATFORM_asyncresolvetimbre(SNDIPATCHHEADER *pph, char *pdata, int *pfirstoffset) {
    if (*pfirstoffset == 0) {
        *pfirstoffset = pph->sampleoffset[0];
    }
    *pph->paudioramaddr = (int)(pdata + (pph->sampleoffset[0] - *pfirstoffset));
    return 0;
}
