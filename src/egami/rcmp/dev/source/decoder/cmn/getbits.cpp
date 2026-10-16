// rcmp: lectura de bits del flujo MPEG-2 (decoder/cmn/getbits.cpp en el .mdebug de la A124), el getbits.c del
// MPEG Software Simulation Group que EA redujo a un puntero de lectura sobre el bloque en memoria.
#include "../ps2/rcmp_ps2_sony_codec.h"

void Initialize_Buffer(unsigned char *bitstream) {
    MPDld->Rdptr = bitstream;
    MPDld->Incnt = 0;
    MPDld->Bfr = 0;
    Flush_Buffer(0);
}

int Show_Bits(int N) {
    return MPDld->Bfr >> (32 - N);
}

int Get_Bits1() {
    return Get_Bits(1);
}

void Flush_Buffer(int N) {
    int Incnt;

    MPDld->Bfr <<= N;
    Incnt = MPDld->Incnt - N;
    if (Incnt <= 24) {
        do {
            MPDld->Bfr |= *MPDld->Rdptr++ << (24 - Incnt);
            Incnt += 8;
        } while (Incnt <= 24);
    }
    MPDld->Incnt = Incnt;
}

int Get_Bits(int N) {
    int Val;

    Val = Show_Bits(N);
    Flush_Buffer(N);
    return Val;
}

void Flush_Buffer32() {
    int Incnt;

    MPDld->Bfr = 0;
    Incnt = MPDld->Incnt - 32;
    while (Incnt <= 24) {
        MPDld->Bfr |= *MPDld->Rdptr++ << (24 - Incnt);
        Incnt += 8;
    }
    MPDld->Incnt = Incnt;
}

int Get_Bits32() {
    int l;

    l = Show_Bits(32);
    Flush_Buffer32();
    return l;
}
