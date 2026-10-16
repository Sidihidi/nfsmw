/* _mbtowc_r de la libc de SN para PS2 (libc.a(mbtowc_r.obj)): el mbtowc_r.c de newlib 1.8.1 sin cambios, con dos
   cosas que pone el entorno de SN y no la fuente: wchar_t de 16 bits (el binario guarda con sh) y el prototipo de
   strlen de <string.h> (la comparacion de la longitud del locale es sin signo). */

#define __WCHAR_TYPE__ unsigned short
#include <string.h>
#include "../../src/newlib/libc/stdlib/mbtowc_r.c"
