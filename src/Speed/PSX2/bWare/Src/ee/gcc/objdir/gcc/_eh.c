/* libgcc.a(_eh.o): el Makefile de gcc compila ../../src/gcc/libgcc2.c con -DL_eh y, solo esta, con -fexceptions
   (LIB2FUNCS_EH). */
#define L_eh
#include "../../src/gcc/libgcc2.c"
