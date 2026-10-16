# Runtime de GCC y newlib de PS2

Lo que el juego de PS2 enlaza de la toolchain (segun el NFS.MAP de la A124):

- `src/gcc/` y `objdir/gcc/`: libgcc y el runtime de C++ (tinfo, exception) de GCC 2.9-ee-991111 (Cygnus/Sony).
  Licencia: GNU GPL con la excepcion de enlace de libgcc (ver la cabecera de cada fichero).
- `src/newlib/`: newlib 1.8.1 de la toolchain de Sony (libm y libc). Licencias de cada fichero: BSD/Sun (fdlibm),
  dominio publico (mallocr) y las de newlib (ver la cabecera de cada fichero).
- `src/newlib/libc/machine/r5900/`: las funciones de cadena de la libc de SN (memchr, memcmp, memcpy, memmove,
  memset, strcat, strchr, strcmp, strcpy, strlen, strncat, strncmp, strncpy). Son el asm de Cygnus de newlib 1.8.1
  para el R5900 (Jeff Johnston, 1999; MMI y quadword), copiado sin cambios de `ee/newlib/libc/machine/r5900/` de
  la toolchain: el original es el mismo instruccion a instruccion. Licencia: el aviso de Cygnus de cada fichero
  (uso, copia y distribucion libres conservando el aviso). Se preprocesan con `ee-gcc -E` y se ensamblan con el
  Ps2EeAs de SN (`tools/ee_as_S.py`). setjmp.S no esta: el del juego es de otra version.
- `sn/libc/`: lo que SN Systems anadio o retoco en su libc (snprintf, malloc/mlock, strlwr, strncasecmp, wcscpy,
  wmemcpy, wsetup, wcslen, wmemchr y el printf ancho vfwprintf/vfwiprintf), escrito a partir del binario al estilo
  de newlib, y mbtowc_r.c, que incluye el de newlib tal cual con el wchar_t de 16 bits de SN. El printf de char de
  la libc de SN es el de GameCube: `src/libc/vfprintf.c` (con itoa.c) y `vfprintf_1.c`, comunes a las dos.

Los ficheros de `src/` son copias sin cambios de las fuentes publicas de GCC y newlib de esa toolchain; cada uno
conserva su aviso de copyright y licencia. `objdir/` reproduce lo que genera la build de gcc (tconfig.h, tm.h,
fp-bit.c/dp-bit.c y envoltorios `#define L_x` + `#include libgcc2.c`; `_eh.c` se compila con -fexceptions, como
LIB2FUNCS_EH en el Makefile de gcc).

SN compilo su libc con las funciones internas de GCC (sin -fno-builtin: vfprintf copia los 16 B de PAD con ldl/sdl)
y con wchar_t de 16 bits (`-fshort-wchar` para las L"..."; sus cabeceras lo declaran unsigned short, y las de newlib
de esta toolchain, int: los fuentes que lo usan definen `__WCHAR_TYPE__` antes de <stddef.h>).

La libc de SN (`libc_sn`, `libc_sn_newlib` y `libc_sn_r5900` en configure.py) se ensambla con una replica del Ps2EeAs
que la ensamblo en origen: `scripts/replica_snas.py` copia la toolchain `PS2/ee-gcc2.95.3-136` a
`PS2/ee-gcc2.95.3-136-snas` y cambia en `ee/bin/Ps2EeAs.exe` las dos constantes con las que alarga los bucles cortos
(a 6 instrucciones, como el original, y no a 7), con el SHA-1 comprobado antes y despues. El C pasa por `ee-gcc -S`
y ese Ps2EeAs (`tools/ee_gcc_pp.py --as`).
