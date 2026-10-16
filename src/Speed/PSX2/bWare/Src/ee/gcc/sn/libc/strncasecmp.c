/* strncasecmp de la libc de SN para PS2 (libc.a(strncasecmp.obj)): el de newlib 1.8.1, con la funcion _tolower en
   vez de la macro tolower (como su strcasecmp, que comparte con GameCube: src/libc/strcasecmp.c). */

#include <string.h>
#include <ctype.h>

int 
_DEFUN (strncasecmp, (s1, s2, n),
	_CONST char *s1 _AND
	_CONST char *s2 _AND
	size_t n)
{
  if (n == 0)
    return 0;

  while (n-- != 0 && _tolower(*s1) == _tolower(*s2))
    {
      if (n == 0 || *s1 == '\0' || *s2 == '\0')
	break;
      s1++;
      s2++;
    }

  return _tolower(*(unsigned char *) s1) - _tolower(*(unsigned char *) s2);
}
