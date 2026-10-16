/* wcscpy de la libc de SN para PS2 (libc.a(wcscpy.obj)): la misma que la de GameCube (src/libc/wcscpy.c) con el
   caracter ancho de 16 bits de esta libc (ver wmemcpy.c). newlib 1.8.1 no tiene <wchar.h>. */

#include <string.h>

size_t wcslen (const unsigned short *s);

unsigned short *
wcscpy (unsigned short *dst, const unsigned short *src)
{
  size_t len = wcslen (src);

  memcpy (dst, src, len * sizeof (unsigned short));
  dst[len] = 0;
  return dst;
}
