#include "pbdll.h"

typedef struct _SORT_NODE {
    int next;
    int freq;
    unsigned char value;
} SORT_NODE;

typedef struct _sortnode {
    int next;
    int freq;
    tokenorptr value;
} sortnode;

static void InsertSorted(sortnode *sn, int node, int *startnode) {
    int which = *startnode;
    int prior = which;

    while (which != -1 && sn[node].freq > sn[which].freq) {
        prior = which;
        which = sn[which].next;
    }

    if (which == *startnode) {
        *startnode = node;
        sn[node].next = which;
    } else {
        sn[prior].next = node;
        sn[node].next = which;
    }
}

void VP6_BuildHuffTree(HUFF_NODE *hn, unsigned int *counts, int values) {
    int i;
    sortnode sn[64];
    int sncount = 0;
    int startnode = 0;
    int huffptr = values - 1;

    for (i = 0; i < values; i++) {
        sn[i].value.selector = 1;
        sn[i].value.value = i;

        if (counts[i] == 0) {
            counts[i] = 1;
        }

        sn[i].freq = counts[i];
        sn[i].next = -1;
    }

    sncount = values;

    for (i = 1; i < values; i++) {
        InsertSorted(sn, i, &startnode);
    }

    while (sn[startnode].next != -1) {
        int first;
        int second;
        int sumfreq;

        first = startnode;
        second = sn[first].next;
        sumfreq = sn[first].freq + sn[second].freq;

        huffptr--;

        hn[huffptr].leftunion.left = sn[first].value;
        hn[huffptr].rightunion.right = sn[second].value;
        hn[huffptr].freq = (sn[first].freq << 8) / sumfreq;

        sn[sncount].value.selector = 0;
        sn[sncount].value.value = huffptr;
        sn[sncount].freq = sumfreq;
        sn[sncount].next = -1;

        startnode = sn[second].next;

        InsertSorted(sn, sncount, &startnode);

        sncount++;
    }
}

void VP6_BuildHuffLookupTable(HUFF_NODE *HuffTreeRoot, UINT16 *HuffTable) {
    int i;
    int j;
#if VP6_VERSION >= 106
    int bits;
#endif
    tokenorptr torp;

    for (i = 0; i < 64; i++) {
        j = 0;
#if VP6_VERSION >= 106
        bits = 6;
#endif
        torp.value = 0;
        torp.selector = 0;

        do {
#if VP6_VERSION >= 106
            bits--;
            j++;

            if ((i >> bits) & 1)
#else
            j++;

            if ((i >> (6 - j)) & 1)
#endif
                torp = HuffTreeRoot[torp.value].rightunion.right;
            else
                torp = HuffTreeRoot[torp.value].leftunion.left;
#if VP6_VERSION >= 106
        } while (!torp.selector && bits);
#else
        } while (!torp.selector && j < 6);
#endif

        HuffTable[i] = (torp.value << 1 | torp.selector) | (j << 12);
    }
}

void VP6_CreateCodeArray(HUFF_NODE *hn, int node, unsigned int *codearray,
                         unsigned char *lengtharray, int codevalue, int codelength) {
    if (hn[node].leftunion.left.selector) {
        codearray[hn[node].leftunion.left.value] = codevalue << 1;
        lengtharray[hn[node].leftunion.left.value] = codelength + 1;
    } else {
        VP6_CreateCodeArray(hn, hn[node].leftunion.left.value, codearray, lengtharray,
                            codevalue << 1, codelength + 1);
    }

    if (hn[node].rightunion.right.selector) {
        codearray[hn[node].rightunion.right.value] = (codevalue << 1) + 1;
        lengtharray[hn[node].rightunion.right.value] = codelength + 1;
    } else {
        VP6_CreateCodeArray(hn, hn[node].rightunion.right.value, codearray, lengtharray,
                            (codevalue << 1) + 1, codelength + 1);
    }
}
