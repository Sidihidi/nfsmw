#ifndef SND_EXTERN_FTOI_H
#define SND_EXTERN_FTOI_H

#ifdef EA_PRAGMA_ONCE_SUPPORTED
#pragma once
#endif

// El paso de float a entero del driver de PS2 de snd 9.06.00-ps2 (library/ps2/../extern/ftoi.h en el .mdebug de la
// A124): cvt.w.s a $f4 y mfc1, las dos en ftoi.h:232, en las once expansiones del binario (SetVolInternal y
// SNDPLATFORM_setfxlevel). Es asm del original, como el FloatToInt de bMath: el registro es $f4 las once veces (la
// conversion en C, `(int)f`, la hace EE-GCC en $f1) y nada se planifica a traves de ella (sin volatile, el
// planificador sube por encima la carga de numchan del bucle siguiente y las dos funciones dejan de casar).
#ifdef EA_PLATFORM_PLAYSTATION2
static inline int ftoi(float f) { // nombre adivinado: el .mdebug da el fichero y la linea, no la funcion
    int result;

    asm volatile("cvt.w.s $f4, %1\n\tmfc1 %0, $f4" : "=r"(result) : "f"(f) : "$f4");
    return result;
}
#endif

#endif
