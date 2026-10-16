#include <string.h>

#include "../../../include/common/realmemcard/impl/memcard_interface_impl.h"

namespace Realmc {

// {Shift-JIS, ASCII} de los signos: el espacio y la puntuacion de 0x20-0x2F, 0x3A-0x40, 0x5B-0x60 y 0x7B-0x7E.
unsigned short ascii_special[33][2] = {
    {0x8140, ' '}, {0x8149, '!'}, {0x8168, '"'}, {0x8194, '#'}, {0x8190, '$'}, {0x8193, '%'}, {0x8195, '&'},
    {0x8166, '\''}, {0x8169, '('}, {0x816A, ')'}, {0x8196, '*'}, {0x817B, '+'}, {0x8143, ','}, {0x817C, '-'},
    {0x8144, '.'}, {0x815E, '/'}, {0x8146, ':'}, {0x8147, ';'}, {0x8171, '<'}, {0x8181, '='}, {0x8172, '>'},
    {0x8148, '?'}, {0x8197, '@'}, {0x816D, '['}, {0x818F, '\\'},{0x816E, ']'}, {0x814F, '^'}, {0x8151, '_'},
    {0x8165, '`'}, {0x816F, '{'}, {0x8162, '|'}, {0x8170, '}'}, {0x8150, '~'},
};

// {Shift-JIS, ASCII} del primer caracter de los digitos, las mayusculas y las minusculas.
static unsigned short ascii_table[3][2] = {
    {0x824F, '0'},
    {0x8260, 'A'},
    {0x8281, 'a'},
};

unsigned short Sjis::Ascii2Sjis(unsigned char asciiCode) {
    unsigned short sjisCode = 0;
    unsigned char stmp = 0;
    unsigned char stmp2 = 0;

    if (asciiCode >= 0x20 && asciiCode <= 0x2F) {
        stmp2 = 1;
    } else if (asciiCode >= 0x30 && asciiCode <= 0x39) {
        stmp = 0;
    } else if (asciiCode >= 0x3A && asciiCode <= 0x40) {
        stmp2 = 11;
    } else if (asciiCode >= 0x41 && asciiCode <= 0x5A) {
        stmp = 1;
    } else if (asciiCode >= 0x5B && asciiCode <= 0x60) {
        stmp2 = 37;
    } else if (asciiCode >= 0x61 && asciiCode <= 0x7A) {
        stmp = 2;
    } else if (asciiCode >= 0x7B && asciiCode <= 0x7E) {
        stmp2 = 63;
    } else {
        return 0;
    }

    if (stmp2 != 0) {
        sjisCode = ascii_special[asciiCode - 0x20 - (stmp2 - 1)][0];
    } else {
        sjisCode = ascii_table[stmp][0] + asciiCode - ascii_table[stmp][1];
    }
    return sjisCode;
}

void Sjis::Ascii2SjisString(const unsigned char *asciiStr, unsigned short *sjisStr) {
    int i;
    int len;

    len = strlen(reinterpret_cast<const char *>(asciiStr));
    for (i = 0; i < len; i++) {
        unsigned short sjis = this->Ascii2Sjis(asciiStr[i]);
        sjisStr[i] = (sjis >> 8) | (sjis << 8);
    }
    sjisStr[i] = 0;
}

} // namespace Realmc
