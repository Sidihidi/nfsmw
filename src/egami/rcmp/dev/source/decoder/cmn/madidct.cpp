extern "C" {

#define MULT(a, b) ((int)((((long long)(a) * (b)) + 32768) >> 16))

int idctinput[64];

int idctprescale[64] = {
    0x00002000, 0x00001712, 0x0000187E, 0x00001B37, 0x00002000, 0x000028BA, 0x00003B21, 0x000073FC,
    0x00001712, 0x000010A2, 0x000011A8, 0x0000139F, 0x00001712, 0x00001D5D, 0x00002AA1, 0x0000539F,
    0x0000187E, 0x000011A8, 0x000012BF, 0x000014D4, 0x0000187E, 0x00001F2C, 0x00002D41, 0x000058C5,
    0x00001B37, 0x0000139F, 0x000014D4, 0x00001725, 0x00001B37, 0x000022A3, 0x00003249, 0x000062A3,
    0x00002000, 0x00001712, 0x0000187E, 0x00001B37, 0x00002000, 0x000028BA, 0x00003B21, 0x000073FC,
    0x000028BA, 0x00001D5D, 0x00001F2C, 0x000022A3, 0x000028BA, 0x000033D6, 0x00004B42, 0x0000939F,
    0x00003B21, 0x00002AA1, 0x00002D41, 0x00003249, 0x00003B21, 0x00004B42, 0x00006D41, 0x0000D650,
    0x000073FC, 0x0000539F, 0x000058C5, 0x000062A3, 0x000073FC, 0x0000939F, 0x0000D650, 0x0001A463,
};

static int work[64];

static void IdctColumn(int *src, int *dest) {
    int t1, t2, t3, t4, t5, t6, t7, t8, t9;

    if ((src[1] | src[2] | src[3] | src[4] | src[5] | src[6] | src[7]) == 0) {
        dest[0] = src[0];
        dest[8] = src[0];
        dest[16] = src[0];
        dest[24] = src[0];
        dest[32] = src[0];
        dest[40] = src[0];
        dest[48] = src[0];
        dest[56] = src[0];
    } else {
        t1 = src[5] - src[3];
        t2 = src[1] + src[7];
        t3 = src[1] - src[7];
        t4 = src[5] + src[3];

        t5 = t2 - t4;
        t4 = t4 + t2;

        t2 = MULT(t1 + t3, 25080);
        t1 = MULT(t1, 35468) + t2;
        t5 = MULT(t5, 46341);
        t3 = MULT(t3, 85627) - t2;

        t4 = t4 + t3;
        t3 = t3 + t5;
        t5 = t5 + t1;

        t6 = src[0] + src[4];
        t7 = src[0] - src[4];
        t8 = MULT(src[2] - src[6], 46341);
        t9 = src[2] + src[6] + t8;

        t2 = t6 - t9;
        t6 = t6 + t9;
        t9 = t7 - t8;
        t7 = t7 + t8;

        dest[0] = t6 + t4;
        dest[8] = t7 + t3;
        dest[16] = t9 + t5;
        dest[24] = t2 + t1;
        dest[32] = t2 - t1;
        dest[40] = t9 - t5;
        dest[48] = t7 - t3;
        dest[56] = t6 - t4;
    }
}

static void IdctRow(int *src, int *dest) {
    int t1, t2, t3, t4, t5, t6, t7, t8, t9;

    t1 = src[5] - src[3];
    t2 = src[1] + src[7];
    t3 = src[1] - src[7];
    t4 = src[5] + src[3];

    t5 = t2 - t4;
    t4 = t4 + t2;

    t2 = MULT(t1 + t3, 25080);
    t1 = MULT(t1, 35468) + t2;
    t5 = MULT(t5, 46341);
    t3 = MULT(t3, 85627) - t2;

    t4 = t4 + t3;
    t3 = t3 + t5;
    t5 = t5 + t1;

    t6 = src[0] + src[4];
    t7 = src[0] - src[4];
    t8 = MULT(src[2] - src[6], 46341);
    t9 = src[2] + src[6] + t8;

    t2 = t6 - t9;
    t6 = t6 + t9;
    t9 = t7 - t8;
    t7 = t7 + t8;

    dest[0] = t6 + t4;
    dest[1] = t7 + t3;
    dest[2] = t9 + t5;
    dest[3] = t2 + t1;
    dest[4] = t2 - t1;
    dest[5] = t9 - t5;
    dest[6] = t7 - t3;
    dest[7] = t6 - t4;
}

void idctcompute(int *dest, int stride) {
    IdctColumn(&idctinput[0], &work[0]);
    IdctColumn(&idctinput[8], &work[1]);
    IdctColumn(&idctinput[16], &work[2]);
    IdctColumn(&idctinput[24], &work[3]);
    IdctColumn(&idctinput[32], &work[4]);
    IdctColumn(&idctinput[40], &work[5]);
    IdctColumn(&idctinput[48], &work[6]);
    IdctColumn(&idctinput[56], &work[7]);

    IdctRow(&work[0], dest);
    IdctRow(&work[8], dest + stride);
    IdctRow(&work[16], dest + stride * 2);
    IdctRow(&work[24], dest + stride * 3);
    IdctRow(&work[32], dest + stride * 4);
    IdctRow(&work[40], dest + stride * 5);
    IdctRow(&work[48], dest + stride * 6);
    IdctRow(&work[56], dest + stride * 7);
}

} // extern "C"
