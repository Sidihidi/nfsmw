// Base64 (misc/Base64.cpp en el .mdebug de la A124, entre EasterEggs.cpp y Table.cpp). Clase y locales del DWARF de
// GameCube, donde Encode y Decode estan estripadas (Range 0xFFFFFFFF). Las dos PS2 tambien estripan lo muerto: la A124
// conserva Encode (la llama el online) y SLUS solo las tablas, que prueban que compila el fichero; Decode, en ninguna.

#include "Speed/Indep/Src/Misc/Base64.hpp"

const unsigned char Base64::encode_table[64] = {
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
    'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
    'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
    'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/',
};

const unsigned int Base64::decode_table[256] = {
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 62, 255, 255, 255, 63,
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 255, 255, 255, 255, 255, 255,
    255, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 255, 255, 255, 255, 255,
    255, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
};

unsigned long Base64::Encode(const void *source, const unsigned long source_buflen, void *dest,
                             const unsigned long dest_buflen, const bool terminate) {
    const unsigned char *_source;
    unsigned char *_dest;
    unsigned long source_pos;
    unsigned long dest_pos;

    if (!source) {
        return 0;
    }
    _source = static_cast<const unsigned char *>(source);
    _dest = static_cast<unsigned char *>(dest);
    source_pos = 0;
    dest_pos = 0;
    while (source_pos < source_buflen) {
        unsigned char sbyte0 = _source[source_pos];
        unsigned char sbyte1 = source_pos + 1 < source_buflen ? _source[source_pos + 1] : 0;
        unsigned char sbyte2 = source_pos + 2 < source_buflen ? _source[source_pos + 2] : 0;

        source_pos += 3;
        if (_dest && dest_pos + 3 <= dest_buflen) {
            _dest[dest_pos] = encode_table[sbyte0 >> 2];
            _dest[dest_pos + 1] = encode_table[((sbyte0 & 3) << 4) | (sbyte1 >> 4)];
            _dest[dest_pos + 2] =
                source_pos == source_buflen + 2 ? '=' : encode_table[((sbyte1 & 0xF) << 2) | (sbyte2 >> 6)];
            _dest[dest_pos + 3] = source_pos < source_buflen + 1 ? encode_table[sbyte2 & 0x3F] : '=';
        }
        dest_pos += 4;
    }
    if (terminate == true) {
        if (_dest && dest_pos < dest_buflen) {
            _dest[dest_pos] = 0;
        }
        dest_pos++;
    }
    return dest_pos;
}
