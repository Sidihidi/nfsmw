/* malloc y free de la libc de SN para PS2 (libc.a(malloc.obj)): los de newlib, con el cerrojo de mlock alrededor. */

#include <_ansi.h>
#include <reent.h>
#include <stdlib.h>
#include <malloc.h>

_PTR
malloc (size_t nbytes)
{
  _PTR p;

  __malloc_lock (_REENT);
  p = _malloc_r (_REENT, nbytes);
  __malloc_unlock (_REENT);
  return p;
}

void
free (_PTR aptr)
{
  __malloc_lock (_REENT);
  _free_r (_REENT, aptr);
  __malloc_unlock (_REENT);
}
