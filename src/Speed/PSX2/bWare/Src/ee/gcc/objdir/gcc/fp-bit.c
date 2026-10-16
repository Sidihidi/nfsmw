#define FLOAT
#ifdef __MIPSEL__
#define FLOAT_BIT_ORDER_MISMATCH
#endif
#define US_SOFTWARE_GOFAST
#define NO_DENORMALS
#include "../../src/gcc/config/fp-bit.c"
