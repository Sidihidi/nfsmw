/* strlwr de la libc de SN para PS2 (libc.a(strlwr.obj)): el de newlib, con la funcion _tolower en vez de la macro
   tolower. */

#include <_ansi.h>
#include <string.h>
#include <ctype.h>

char *
strlwr (char *a)
{
  char *ret = a;

  while (*a != '\0')
    {
      if (isupper (*a))
	*a = _tolower (*a);
      ++a;
    }

  return ret;
}
