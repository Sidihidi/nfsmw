/* wmemchr de la libc de SN para PS2 (libc.a(wmemchr.obj)): caracter ancho de 16 bits (ver wmemcpy.c), con el bucle
   desenrollado a mano cuatro veces. Tal cual el binario: el primer bloque de cuatro se mira siempre y el resto
   empieza en i + 1. */

#include <stddef.h>

unsigned short *
wmemchr (const unsigned short *s, unsigned short c, size_t n)
{
  size_t i;

  for (i = 0; i <= n - 4; i += 4)
    {
      if (s[i] == c)
        return (unsigned short *) &s[i];
      if (s[i + 1] == c)
        return (unsigned short *) &s[i + 1];
      if (s[i + 2] == c)
        return (unsigned short *) &s[i + 2];
      if (s[i + 3] == c)
        return (unsigned short *) &s[i + 3];
    }
  for (i++; i <= n; i++)
    if (s[i] == c)
      return (unsigned short *) &s[i];
  return 0;
}
