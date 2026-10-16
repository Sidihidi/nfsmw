#ifdef EA_PLATFORM_PLAYSTATION2
// snd 9.06.00-ps2: las muestras que no caben en los bloques de 16 (o de punteros sin alinear).
// asm de EA (ronda 18): el original convierte de cuatro en cuatro con $f0-$f3 fijos, el addi de
// pdst antes de las stores (-16/-12/-8/-4) y etiqueta local con nombre en el symtab de la A124
// (SNDI_s16tofunaligned_convert4); la misma familia que el FloatToInt de EAGL4 (registro fijo).
void SNDI_s16tofunaligned(int samples, short *psrc, float *pdst) {
    int i;
    int remainder;
    float *pdstend;

    pdstend = pdst + samples;
    remainder = samples & 3;
    for (i = 0; i < remainder; i++) {
        *pdst++ = *psrc++;
    }
    if (samples < 4) {
        return;
    }
    asm __volatile__(".set noreorder\n"
                     "move $4, %0\n"
                     "move $5, %1\n"
                     "move $6, %2\n"
                     "2:\n"
                     "lh $8, 0x0($4)\n"
                     "lh $9, 0x2($4)\n"
                     "lh $10, 0x4($4)\n"
                     "lh $11, 0x6($4)\n"
                     "mtc1 $8, $f0\n"
                     "mtc1 $9, $f1\n"
                     "mtc1 $10, $f2\n"
                     "mtc1 $11, $f3\n"
                     "addi $5, $5, 0x10\n"
                     "cvt.s.w $f0, $f0\n"
                     "cvt.s.w $f1, $f1\n"
                     "cvt.s.w $f2, $f2\n"
                     "cvt.s.w $f3, $f3\n"
                     "swc1 $f0, -0x10($5)\n"
                     "swc1 $f1, -0xC($5)\n"
                     "swc1 $f2, -0x8($5)\n"
                     "swc1 $f3, -0x4($5)\n"
                     "bne $5, $6, 2b\n"
                     "addi $4, $4, 0x8\n"
                     ".set reorder"
                     :
                     : "r"(psrc), "r"(pdst), "r"(pdstend)
                     : "$4", "$5", "$6", "$8", "$9", "$10", "$11", "$f0", "$f1", "$f2", "$f3",
                       "memory");
}
#endif

#ifdef __cplusplus
extern "C" {
#endif

void decode16x87(int frames, short *psrc, float *pdst) {
#ifdef EA_PLATFORM_PLAYSTATION2
    int unalignedsamples;

    if (((unsigned int)psrc & 7) != 0 || ((unsigned int)pdst & 0xF) != 0) {
        SNDI_s16tofunaligned(frames, psrc, pdst);
        return;
    }

    unalignedsamples = frames & 0xF;
    if (unalignedsamples != 0) {
        frames &= 0x7FF0;
        SNDI_s16tofunaligned(unalignedsamples, psrc + frames, pdst + frames);
        if (frames <= 0) {
            return;
        }
    }

    asm __volatile__(".set noreorder\n"
                     "move $4, %0\n"
                     "move $5, %1\n"
                     "move $6, %2\n"
                     "1:\n"
                     "ld $8, 0x0($5)\n"
                     "ld $9, 0x8($5)\n"
                     "ld $10, 0x10($5)\n"
                     "ld $11, 0x18($5)\n"
                     "pextlh $8, $8, $0\n"
                     "pextlh $9, $9, $0\n"
                     "pextlh $10, $10, $0\n"
                     "pextlh $11, $11, $0\n"
                     "psraw $8, $8, 16\n"
                     "psraw $9, $9, 16\n"
                     "psraw $10, $10, 16\n"
                     "psraw $11, $11, 16\n"
                     "qmtc2.ni $8, vf1\n"
                     "qmtc2.ni $9, vf2\n"
                     "qmtc2.ni $10, vf3\n"
                     "qmtc2.ni $11, vf4\n"
                     "vitof0.xyzw vf1, vf1\n"
                     "vitof0.xyzw vf2, vf2\n"
                     "vitof0.xyzw vf3, vf3\n"
                     "vitof0.xyzw vf4, vf4\n"
                     "sqc2 vf1, 0x0($6)\n"
                     "sqc2 vf2, 0x10($6)\n"
                     "addiu $4, $4, -0x10\n"
                     "sqc2 vf3, 0x20($6)\n"
                     "addiu $5, $5, 0x20\n"
                     "sqc2 vf4, 0x30($6)\n"
                     "bgtzl $4, 1b\n"
                     "addiu $6, $6, 0x40\n"
                     ".set reorder"
                     :
                     : "r"(frames), "r"(psrc), "r"(pdst)
                     : "$4", "$5", "$6", "$8", "$9", "$10", "$11", "memory");
#else
    int i;

    for (i = 0; i < frames; i++) {
        pdst[i] = (float)psrc[i];
    }
#endif
}

#ifdef __cplusplus
}
#endif
