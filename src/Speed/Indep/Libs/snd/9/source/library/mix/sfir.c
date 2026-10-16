#include <snd/sfilter.h>

/* calcFIRCoeffs: la normalizacion del final es `if (halfLen & 1) sum = sum + sum; else sum = sum + sum -
 * coef[halfLen];` (la suma de los nueve coeficientes simetricos) y el ultimo bucle recorre hasta halfLen sin volver a
 * escribirlo. Con esa forma casa en GameCube y en las dos PS2 sin ningun asm: los dos `asm` de cero bytes, el
 * `halfLen = 4;` repetido y la historia de la veda que habia aqui eran sintomas de la condicion que faltaba (en PS2 el
 * original la deja sin plegar: `daddu v0, zero, zero` en las dos ramas y un `beqz v0`). */
float SNDI_sin(float x);
float SNDI_cos(float x);

// total size: 0x3C (sfir8.c)
// En PS2 (snd 9.06.00-ps2) mide 0x100: los stabs de la A124 le ponen delante los coeficientes para la VU0,
// alineados a 16 (por eso la `fir` de LPFSTATE/HPFSTATE/BPFSTATE cae en 0x20), y detras resetVU0coef.
typedef struct SNDFIRSTATE {
#ifdef EA_PLATFORM_PLAYSTATION2
    float VU0coef[12][4] __attribute__((aligned(16))); // offset 0x0, size 0xC0
#endif
    float history[8]; // offset 0x0, size 0x20 (PS2: 0xC0)
    float coef[5];    // offset 0x20, size 0x14 (PS2: 0xE0)
    float cutLow;     // offset 0x34 (PS2: 0xF4)
    float cutHigh;    // offset 0x38 (PS2: 0xF8)
#ifdef EA_PLATFORM_PLAYSTATION2
    signed char resetVU0coef; // offset 0xFC
    char pad[3];              // offset 0xFD
#endif
} SNDFIRSTATE;

void calcFIRCoeffs(SNDFIRSTATE *pfir, int filtType) {
    float sum;
    float tmpFloat;
    int halfLen;
    int cnt;
    int fir_coef_var;

#ifdef EA_PLATFORM_PLAYSTATION2
    pfir->resetVU0coef = 1;
#endif
    halfLen = 4;
    if ((filtType == 3 || filtType == 4) && pfir->cutLow > 0.8f) {
        pfir->cutLow = 0.8f;
    }
    switch (filtType) {
    case 2:
        pfir->coef[halfLen] = pfir->cutHigh;
        for (cnt = 1; cnt <= halfLen; cnt++) {
            tmpFloat = cnt * 3.1415927f;
            pfir->coef[halfLen - cnt] = SNDI_sin(pfir->cutHigh * tmpFloat) / tmpFloat;
        }
        break;
    case 3:
        pfir->coef[halfLen] = pfir->cutLow;
        for (cnt = 1; cnt <= halfLen; cnt++) {
            tmpFloat = cnt * 3.1415927f;
            pfir->coef[halfLen - cnt] = SNDI_sin(pfir->cutLow * tmpFloat) / tmpFloat;
        }
        break;
    case 4:
        pfir->coef[halfLen] = pfir->cutHigh - pfir->cutLow;
        for (cnt = 1; cnt <= halfLen; cnt++) {
            tmpFloat = cnt * 3.1415927f;
            pfir->coef[halfLen - cnt] =
                2.0f * SNDI_sin((pfir->cutHigh - pfir->cutLow) * 0.5f * tmpFloat) *
                SNDI_cos((pfir->cutHigh + pfir->cutLow) * 0.5f * tmpFloat) / tmpFloat;
        }
        break;
    }
    tmpFloat = 0.7853982f;
    sum = 0.0f;
    for (cnt = 0; cnt <= halfLen; cnt++) {
        if (filtType == 2 || filtType == 4) {
            pfir->coef[cnt] *= 0.54f - SNDI_cos(tmpFloat * cnt) * 0.46f;
            sum += pfir->coef[cnt];
        } else if (filtType == 3) {
            pfir->coef[cnt] *= SNDI_cos(tmpFloat * cnt) * 0.46f - 0.54f;
            if ((cnt & 1) == 0) {
                sum += pfir->coef[cnt];
            } else {
                sum -= pfir->coef[cnt];
            }
        }
    }
    if (filtType == 3) {
        pfir->coef[halfLen] += 1.0f;
        sum += 1.0f;
    }
    if (halfLen & 1) {
        sum = sum + sum;
    } else {
        sum = sum + sum - pfir->coef[halfLen];
    }
    if (sum < 0.0f) {
        sum = -sum;
    }
    for (cnt = 0; cnt <= halfLen; cnt++) {
        pfir->coef[cnt] /= sum;
    }
}
