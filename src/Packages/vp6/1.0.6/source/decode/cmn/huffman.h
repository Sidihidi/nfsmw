#ifndef HUFFMAN_H
#define HUFFMAN_H

#include "type_aliases.h"

/* Los campos de bits van en orden inverso en big-endian para que el byte en
   memoria sea el mismo en las dos arquitecturas: los stabs de PS2 los dan en un
   orden y el DWARF de GameCube en el contrario. */
typedef struct _tokenorptr {
#ifdef __BIG_ENDIAN__
    unsigned int value : 7;
    unsigned int selector : 1;
#else
    unsigned int selector : 1;
    unsigned int value : 7;
#endif
} tokenorptr;

typedef struct _huffnode {
    union {
        char l;
        tokenorptr left;
    } leftunion;
    union {
        char r;
        tokenorptr right;
    } rightunion;
    unsigned char freq;
} HUFF_NODE;

typedef struct HUFF_TABLE_NODE {
#ifdef __BIG_ENDIAN__
    unsigned short length : 4;
    unsigned short unused : 6;
    unsigned short value : 5;
    unsigned short flag : 1;
#else
    unsigned short flag : 1;
    unsigned short value : 5;
    unsigned short unused : 6;
    unsigned short length : 4;
#endif
} HUFF_TABLE_NODE;

extern void VP6_BuildHuffTree(HUFF_NODE *hn, unsigned int *counts, int values);
extern void VP6_BuildHuffLookupTable(HUFF_NODE *HuffTreeRoot, UINT16 *HuffTable);
extern void VP6_CreateCodeArray(HUFF_NODE *hn, int node, unsigned int *codearray, unsigned char *lengtharray,
                                int codevalue, int codelength);

#endif
