#include "boolhuff.h"

void VP6_StartDecode(BOOL_CODER *br, unsigned char *source) {
    br->lowvalue = 0;
    br->range = 255;
    br->count = 8;
    br->buffer = source;
    br->pos = 0;
    br->value = (source[0] << 24) + (source[1] << 16) + (source[2] << 8) + source[3];
    br->pos += 4;
}
