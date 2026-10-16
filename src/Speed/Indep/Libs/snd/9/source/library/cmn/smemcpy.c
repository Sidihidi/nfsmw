#include "Speed/Indep/Libs/snd/9/source/library/cmn/sndcmn.h"

namespace Snd {
#ifdef EA_PLATFORM_PLAYSTATION2
    // snd 9.06.00-ps2 (cmn/smemcpy.cpp): la copia es ensamblador de EA con las MMI (lq/sq de 128 bits y, si el origen
    // no va alineado a 16, mtsab + qfsrv para desplazar), todo en una linea del mapa del .mdebug; el destino se alinea
    // antes byte a byte.
    void *Util::MemCpy(void *pDst, const void *pSrc, unsigned int bytes) {
        asm __volatile__(".set noreorder\n"
                         "sltiu $3, $6, 32\n"
                         "bnez $3, 8f\n"
                         "andi $2, $4, 0xf\n"
                         "beqz $2, 2f\n"
                         "nop\n"
                         "lui $2, 0xffff\n"
                         "addiu $10, $4, 15\n"
                         "ori $2, $2, 0xfff0\n"
                         "and $10, $10, $2\n"
                         "1:\n"
                         "lbu $8, 0($5)\n"
                         "addiu $5, $5, 1\n"
                         "sb $8, 0($4)\n"
                         "addiu $4, $4, 1\n"
                         "nop\n"
                         "bne $10, $4, 1b\n"
                         "addiu $6, $6, -1\n"
                         "2:\n"
                         "sltiu $3, $6, 32\n"
                         "bnez $3, 8f\n"
                         "nop\n"
                         "andi $2, $5, 0xf\n"
                         "bnez $2, 6f\n"
                         "nop\n"
                         "sltiu $3, $6, 64\n"
                         "bnez $3, 4f\n"
                         "nop\n"
                         "3:\n"
                         "lq $8, 0($5)\n"
                         "lq $9, 16($5)\n"
                         "lq $10, 32($5)\n"
                         "addiu $6, $6, -64\n"
                         "lq $11, 48($5)\n"
                         "sltiu $2, $6, 64\n"
                         "sq $8, 0($4)\n"
                         "addiu $5, $5, 64\n"
                         "sq $9, 16($4)\n"
                         "sq $10, 32($4)\n"
                         "sq $11, 48($4)\n"
                         "beqz $2, 3b\n"
                         "addiu $4, $4, 64\n"
                         "sltiu $2, $6, 32\n"
                         "beqz $2, 4f\n"
                         "nop\n"
                         "sltiu $2, $6, 8\n"
                         "beqz $2, 5f\n"
                         "nop\n"
                         "j 9f\n"
                         "addiu $6, $6, -1\n"
                         "4:\n"
                         "lq $8, 0($5)\n"
                         "addiu $6, $6, -32\n"
                         "sq $8, 0($4)\n"
                         "lq $9, 16($5)\n"
                         "addiu $5, $5, 32\n"
                         "sq $9, 16($4)\n"
                         "addiu $4, $4, 32\n"
                         "sltiu $2, $6, 8\n"
                         "bnez $2, 8f\n"
                         "nop\n"
                         "5:\n"
                         "ld $8, 0($5)\n"
                         "addiu $6, $6, -8\n"
                         "addiu $5, $5, 8\n"
                         "sltiu $2, $6, 8\n"
                         "sd $8, 0($4)\n"
                         "beqz $2, 5b\n"
                         "addiu $4, $4, 8\n"
                         "j 9f\n"
                         "addiu $6, $6, -1\n"
                         "6:\n"
                         "mtsab $2, 0\n"
                         "sltiu $2, $6, 64\n"
                         "bnez $2, 7f\n"
                         "nop\n"
                         "lq $8, 0($5)\n"
                         "10:\n"
                         "lq $9, 16($5)\n"
                         "lq $10, 32($5)\n"
                         "lq $11, 48($5)\n"
                         "addiu $6, $6, -64\n"
                         "lq $12, 64($5)\n"
                         "sltiu $2, $6, 64\n"
                         "qfsrv $13, $9, $8\n"
                         "addiu $5, $5, 64\n"
                         "sq $13, 0($4)\n"
                         "qfsrv $14, $10, $9\n"
                         "sq $14, 16($4)\n"
                         "qfsrv $15, $11, $10\n"
                         "sq $15, 32($4)\n"
                         "qfsrv $7, $12, $11\n"
                         "sq $7, 48($4)\n"
                         "addiu $4, $4, 64\n"
                         "beqz $2, 10b\n"
                         "padduw $8, $12, $0\n"
                         "sltiu $2, $6, 32\n"
                         "bnez $2, 8f\n"
                         "nop\n"
                         "7:\n"
                         "lq $8, 0($5)\n"
                         "lq $9, 16($5)\n"
                         "lq $10, 32($5)\n"
                         "addiu $6, $6, -32\n"
                         "addiu $5, $5, 32\n"
                         "qfsrv $12, $9, $8\n"
                         "sq $12, 0($4)\n"
                         "qfsrv $13, $10, $9\n"
                         "sq $13, 16($4)\n"
                         "addiu $4, $4, 32\n"
                         "8:\n"
                         "addiu $6, $6, -1\n"
                         "9:\n"
                         "li $2, -1\n"
                         "beq $6, $2, 12f\n"
                         "move $3, $2\n"
                         "11:\n"
                         "lbu $8, 0($5)\n"
                         "addiu $6, $6, -1\n"
                         "addiu $5, $5, 1\n"
                         "sb $8, 0($4)\n"
                         "nop\n"
                         "bne $6, $3, 11b\n"
                         "addiu $4, $4, 1\n"
                         "12:\n"
                         ".set reorder"
                         :
                         :
                         : "$2", "$3", "$4", "$5", "$6", "$7", "$8", "$9", "$10", "$11", "$12", "$13", "$14", "$15",
                           "memory");
        return NULL;
    }
#else
    void *Util::MemCpy(void *pDst, const void *pSrc, unsigned int bytes) {
        sndptrint si = (sndptrint)pSrc;
        sndptrint di = (sndptrint)pDst;
        sndptrint difference = si - di;
        if (bytes == 0 || difference == 0) {
            return NULL;
        }

        if ((difference & 1) != 0) {
            char *sb = (char *)pSrc;
            char *db = (char *)pDst;
            char t0, t1, t2, t3;

            while (bytes > 3) {
                bytes -= 4;

                t0 = sb[0];
                t1 = sb[1];
                t2 = sb[2];
                t3 = sb[3];

                sb += 4;
                db[0] = t0;
                db[1] = t1;
                db[2] = t2;
                db[3] = t3;
                db += 4;
            }

            while (bytes != 0) {
                *db = *sb;
                bytes--;
                sb++;
                db++;
            }
        } else if ((difference & 2) != 0) {
            short *ss = (short *)si;
            short *ds = (short *)di;
            short t0, t1, t2, t3;

            if ((si & 1) != 0) {
                ds = (short *)(di + 1);
                ss = (short *)(si + 1);
                bytes--;
                *(char *)pDst = *(char *)pSrc;
            }

            while (bytes > 7) {
                bytes -= 8;

                t0 = ss[0];
                t1 = ss[1];
                t2 = ss[2];
                t3 = ss[3];
                ss += 4;

                ds[0] = t0;
                ds[1] = t1;
                ds[2] = t2;
                ds[3] = t3;
                ds += 4;
            }

            while (bytes > 1) {
                *ds = *ss;
                bytes -= 2;
                ss++;
                ds++;
            }

            if (bytes != 0) {
                *(char *)ds = *(char *)ss;
            }
        } else {
            int t0, t1, t2, t3;

            if ((si & 1) != 0) {
                *(char *)di = *(char *)si;
                si++;
                di++;
                bytes--;
            }

            if ((si & 2) != 0 && bytes > 1) {
                *(short *)di = *(short *)si;
                si += 2;
                di += 2;
                bytes -= 2;
            }

            int *sw = (int *)si;
            int *dw = (int *)di;

            while (bytes > 15) {
                bytes -= 16;

                t0 = sw[0];
                t1 = sw[1];
                t2 = sw[2];
                t3 = sw[3];
                sw += 4;

                dw[0] = t0;
                dw[1] = t1;
                dw[2] = t2;
                dw[3] = t3;
                dw += 4;
            }

            while (bytes > 3) {
                *dw = *sw;
                bytes -= 4;
                sw++;
                dw++;
            }

            if (bytes > 1) {
                *(short *)dw = *(short *)sw;
                sw = (int *)((sndptrint)sw + 2);
                dw = (int *)((sndptrint)dw + 2);
                bytes -= 2;
            }

            if (bytes != 0) {
                *(char *)dw = *(char *)sw;
            }
        }

        return NULL;
    }
#endif
}
