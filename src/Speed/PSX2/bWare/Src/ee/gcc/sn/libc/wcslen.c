/* wcslen de la libc de SN para PS2 (libc.a(wcslen.obj)): caracter ancho de 16 bits (ver wmemcpy.c), con el bucle
   desenrollado a mano dos veces. */

#include <stddef.h>

size_t
wcslen (const unsigned short *s)
{
  size_t n = 0;

  while (*s != 0)
    {
      s++;
      n++;
      if (*s == 0)
        return n;
      s++;
      n++;
    }
  return n;
}
