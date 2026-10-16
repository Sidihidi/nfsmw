/* boolhuff.inl -- el .mdebug de la A124 y el DWARF de GameCube dicen que el criticalpath.c de cada plataforma lo
 * incluye (D:/env/egami/vp6/dev/source/decode/ps2/../cmn/boolhuff.inl en PS2, 1.0.3; gc/../cmn en GameCube, 1.0.6). */

int VP6_DecodeBool(BOOL_CODER *br, int probability) {
    unsigned int bit = 0;
    unsigned int split;
    unsigned int bigsplit;
    unsigned int count = br->count;
    unsigned int range = br->range;
    unsigned int value = br->value;

    split = 1 + (((range - 1) * probability) >> 8);
    bigsplit = split << 24;

    range = split;
    if (value >= bigsplit) {
        range = br->range - split;
        value = value - bigsplit;
        bit = 1;
    }

    if (range >= 128) {
        br->value = value;
        br->range = range;
        return bit;
    }

    do {
        range += range;
        value += value;
        if (--count == 0) {
            count = 8;
            value |= br->buffer[br->pos];
            br->pos++;
        }
    } while (range < 128);

    br->count = count;
    br->value = value;
    br->range = range;
    return bit;
}

int VP6_DecodeBool128(BOOL_CODER *br) {
    unsigned int bit;
    unsigned int split;
    unsigned int bigsplit;
    unsigned int count = br->count;
    unsigned int range = br->range;
    unsigned int value = br->value;

    split = (range + 1) >> 1;
    bigsplit = split << 24;

    if (value >= bigsplit) {
        range = (range - split) << 1;
        value = (value - bigsplit) << 1;
        bit = 1;
    } else {
        range = split << 1;
        value = value << 1;
        bit = 0;
    }

    if (--count == 0) {
        count = 8;
        value |= br->buffer[br->pos];
        br->pos++;
    }

    br->count = count;
    br->value = value;
    br->range = range;
    return bit;
}
