#ifndef BOOLHUFF_H
#define BOOLHUFF_H

/* Decodificador booleano (aritmetico binario). El cuerpo de VP6_DecodeBool y
   compania esta en boolhuff.inl, que incluye el criticalpath.c de cada
   plataforma. */
typedef struct {
    unsigned int lowvalue;
    unsigned int range;
    unsigned int value;
    int count;
    unsigned int pos;
    unsigned char *buffer;
    unsigned int MeasureCost;
    unsigned int BitCounter;
} BOOL_CODER;

extern void VP6_StartDecode(BOOL_CODER *br, unsigned char *source);
extern int VP6_DecodeBool(BOOL_CODER *br, int probability);
extern int VP6_DecodeBool128(BOOL_CODER *br);
extern int nDecodeBool(BOOL_CODER *br, int probability);

#endif
