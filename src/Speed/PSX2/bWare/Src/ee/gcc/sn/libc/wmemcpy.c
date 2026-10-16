/* wmemcpy de la libc de SN para PS2 (libc.a(wmemcpy.obj)). Su caracter ancho es de 16 bits (el binario copia
   n * 2 bytes) y el <stddef.h> de esta toolchain declara wchar_t como int: va escrito unsigned short. */

#include <string.h>

unsigned short *
wmemcpy (unsigned short *d, const unsigned short *s, size_t n)
{
  memcpy (d, s, n * sizeof (unsigned short));
  return d;
}
