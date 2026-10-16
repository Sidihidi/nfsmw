/* snprintf de la libc de SN para PS2 (libc.a(snprintf.obj)). newlib 1.8.1 no lo trae: es el sprintf.c de newlib
   con el tamano del buffer en vez de INT_MAX, y -1 si la salida no cabe. */

#include <stdio.h>
#include <stdarg.h>
#include <_ansi.h>
#include "../../src/newlib/libc/stdio/local.h"

int
snprintf (char *str, size_t size, _CONST char *fmt, ...)
{
  int ret;
  va_list ap;
  FILE f;

  f._flags = __SWR | __SSTR;
  f._bf._base = f._p = (unsigned char *) str;
  f._bf._size = f._w = size;
  f._data = _REENT;
  va_start (ap, fmt);
  ret = vfprintf (&f, fmt, ap);
  va_end (ap);
  if (ret > size)
    ret = -1;
  *f._p = 0;
  return (ret);
}
