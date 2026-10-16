#ifdef __cplusplus
extern "C" {
#endif

void mixc(int frames, float gain, void *psrcp, void *pdstp) {
#ifdef EA_PLATFORM_PLAYSTATION2
    // snd 9.06.00-ps2: la mezcla con ganancia por la VU0, de 16 en 16 muestras (lqc2/vmulx/vadd/sqc2 en el binario)
    asm __volatile__(".set noreorder\n"
                     "move $4, %0\n"
                     "qmtc2.ni %3, vf1\n"
                     "sll $4, $4, 2\n"
                     "move $6, %1\n"
                     "move $7, %2\n"
                     "add $4, $4, $6\n"
                     "1:\n"
                     "lqc2 vf2, 0x0($6)\n"
                     "lqc2 vf3, 0x10($6)\n"
                     "lqc2 vf4, 0x20($6)\n"
                     "lqc2 vf5, 0x30($6)\n"
                     "lqc2 vf6, 0x0($7)\n"
                     "lqc2 vf7, 0x10($7)\n"
                     "lqc2 vf8, 0x20($7)\n"
                     "lqc2 vf9, 0x30($7)\n"
                     "vmulx.xyzw vf2, vf2, vf1x\n"
                     "vmulx.xyzw vf3, vf3, vf1x\n"
                     "vmulx.xyzw vf4, vf4, vf1x\n"
                     "vmulx.xyzw vf5, vf5, vf1x\n"
                     "vadd.xyzw vf6, vf6, vf2\n"
                     "vadd.xyzw vf7, vf7, vf3\n"
                     "addi $6, $6, 0x40\n"
                     "vadd.xyzw vf8, vf8, vf4\n"
                     "vadd.xyzw vf9, vf9, vf5\n"
                     "sqc2 vf6, 0x0($7)\n"
                     "sqc2 vf7, 0x10($7)\n"
                     "sqc2 vf8, 0x20($7)\n"
                     "sqc2 vf9, 0x30($7)\n"
                     "bne $4, $6, 1b\n"
                     "addi $7, $7, 0x40\n"
                     ".set reorder"
                     :
                     : "r"(frames), "r"(psrcp), "r"(pdstp), "r"(gain)
                     : "$4", "$6", "$7", "memory");
#else
    float *psrc = (float *)psrcp;
    float *pdst = (float *)pdstp;
    int i;

    for (i = 0; i < frames; i++) {
        pdst[i] = psrc[i] * gain + pdst[i];
    }
#endif
}

#ifdef __cplusplus
}
#endif
