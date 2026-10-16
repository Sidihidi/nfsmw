#include "../cmn/pbdll.h"
#include "../cmn/vputil.h"

void FilterBlock1dBil_GC(unsigned char *SrcPtr, unsigned short *OutputPtr,
                         unsigned int SrcPixelsPerLine, unsigned int PixelStep, int *Filter);
void FilterBlock2dBil_GC(unsigned char *SrcPtr, unsigned short *OutputPtr,
                         unsigned int SrcPixelsPerLine, int *HFilter, int *VFilter);

static float f128 __attribute__((aligned(8))) = 128.0f;
static float f64 = 64.0f;

static float idctconstants[8] = {
    8.0f,          0.980785251f, 0.923879504f, 0.831469595f,
    0.707106769f,  0.555570245f, 0.382683426f, 0.195090324f,
};

#include "../cmn/boolhuff.inl"
#include "../cmn/decodembs.inl"
#include "../cmn/decodemode.inl"
#include "../cmn/decodemv.inl"
#include "../cmn/recon.inl"

void ScalarReconIntra_GC(short *TmpDataBuffer, unsigned char *ReconPtr, unsigned short *ChangePtr,
                         unsigned int LineStep) {
    asm("	li 9,f128@sda21
	li 0,0x8
	psq_l 4,0x0(9),1,0
	mtctr 0
	ps_merge00 4,4,4
intra_loop:
	psq_l 0,0x0(5),0,5
	psq_l 1,0x4(5),0,5
	psq_l 2,0x8(5),0,5
	psq_l 3,0xc(5),0,5
	addi 5,5,0x10
	ps_add 0,0,4
	ps_add 1,1,4
	ps_add 2,2,4
	ps_add 3,3,4
	psq_st 0,0x0(4),0,2
	psq_st 1,0x2(4),0,2
	psq_st 2,0x4(4),0,2
	psq_st 3,0x6(4),0,2
	add 4,4,6
	bdnz intra_loop
");
}

void ScalarReconInter_GC(short *TmpDataBuffer, unsigned char *ReconPtr, unsigned char *RefPtr,
                         short *ChangePtr, unsigned int LineStep) {
    asm("	li 0,0x8
	mtctr 0
inter_loop:
	psq_l 0,0x0(5),0,2
	psq_l 1,0x2(5),0,2
	psq_l 2,0x4(5),0,2
	psq_l 3,0x6(5),0,2
	add 5,5,7
	psq_l 4,0x0(6),0,5
	psq_l 5,0x4(6),0,5
	psq_l 6,0x8(6),0,5
	psq_l 7,0xc(6),0,5
	addi 6,6,0x10
	ps_add 0,0,4
	ps_add 1,1,5
	ps_add 2,2,6
	ps_add 3,3,7
	psq_st 0,0x0(4),0,2
	psq_st 1,0x2(4),0,2
	psq_st 2,0x4(4),0,2
	psq_st 3,0x6(4),0,2
	add 4,4,7
	bdnz inter_loop
");
}

void ReconBlock_GC(short *SrcBlock, short *ReconRefPtr, unsigned char *DestBlock,
                   unsigned int LineStep) {
    asm("	li 0,0x8
	mtctr 0
block_loop:
	psq_l 0,0x0(3),0,5
	psq_l 1,0x4(3),0,5
	psq_l 2,0x8(3),0,5
	psq_l 3,0xc(3),0,5
	addi 3,3,0x10
	psq_l 4,0x0(4),0,5
	psq_l 5,0x4(4),0,5
	psq_l 6,0x8(4),0,5
	psq_l 7,0xc(4),0,5
	addi 4,4,0x10
	ps_add 0,0,4
	ps_add 1,1,5
	ps_add 2,2,6
	ps_add 3,3,7
	psq_st 0,0x0(5),0,2
	psq_st 1,0x2(5),0,2
	psq_st 2,0x4(5),0,2
	psq_st 3,0x6(5),0,2
	add 5,5,6
	bdnz block_loop
");
}

void IDct64_GC(short *InputData, short *QuantMatrix, short *OutputData) {
    asm("	stwu 1,-0x98(1)
	stfd 14,0x8(1)
	stfd 15,0x10(1)
	stfd 16,0x18(1)
	stfd 17,0x20(1)
	stfd 18,0x28(1)
	stfd 19,0x30(1)
	stfd 20,0x38(1)
	stfd 21,0x40(1)
	stfd 22,0x48(1)
	stfd 23,0x50(1)
	stfd 24,0x58(1)
	stfd 25,0x60(1)
	stfd 26,0x68(1)
	stfd 27,0x70(1)
	stfd 28,0x78(1)
	stfd 29,0x80(1)
	stfd 30,0x88(1)
	stfd 31,0x90(1)
	lis 11,idctconstants@ha
	addi 11,11,idctconstants@l
	mflr 6
	psq_l 0,0x0(11),1,0
	psq_l 1,0x4(11),1,0
	psq_l 2,0x8(11),1,0
	psq_l 3,0xc(11),1,0
	psq_l 4,0x10(11),1,0
	psq_l 5,0x14(11),1,0
	psq_l 6,0x18(11),1,0
	psq_l 7,0x1c(11),1,0
	ps_merge00 0,0,0
	ps_merge00 1,1,1
	ps_merge00 2,2,2
	ps_merge00 3,3,3
	ps_merge00 4,4,4
	ps_merge00 5,5,5
	ps_merge00 6,6,6
	ps_merge00 7,7,7
	psq_l 16,0x0(3),1,5
	psq_l 17,0x2(3),1,5
	psq_l 18,0x4(3),1,5
	psq_l 19,0x8(3),1,5
	psq_l 20,0x0(4),1,5
	psq_l 21,0x2(4),1,5
	psq_l 22,0x4(4),1,5
	psq_l 23,0x8(4),1,5
	ps_merge00 16,16,18
	ps_merge00 17,17,19
	ps_merge00 20,20,22
	ps_merge00 21,21,23
	ps_mul 8,16,20
	ps_mul 9,17,21
	psq_l 16,0xa(3),1,5
	psq_l 17,0xc(3),1,5
	psq_l 18,0xe(3),1,5
	psq_l 19,0x1a(3),1,5
	psq_l 20,0xa(4),1,5
	psq_l 21,0xc(4),1,5
	psq_l 22,0xe(4),1,5
	psq_l 23,0x1a(4),1,5
	ps_merge00 16,16,18
	ps_merge00 17,17,19
	ps_merge00 20,20,22
	ps_merge00 21,21,23
	ps_mul 10,16,20
	ps_mul 11,17,21
	psq_l 16,0x1c(3),1,5
	psq_l 17,0x1e(3),1,5
	psq_l 18,0x20(3),1,5
	psq_l 19,0x34(3),1,5
	psq_l 20,0x1c(4),1,5
	psq_l 21,0x1e(4),1,5
	psq_l 22,0x20(4),1,5
	psq_l 23,0x34(4),1,5
	ps_merge00 16,16,18
	ps_merge00 17,17,19
	ps_merge00 20,20,22
	ps_merge00 21,21,23
	ps_mul 12,16,20
	ps_mul 13,17,21
	psq_l 16,0x36(3),1,5
	psq_l 17,0x38(3),1,5
	psq_l 18,0x3a(3),1,5
	psq_l 19,0x54(3),1,5
	psq_l 20,0x36(4),1,5
	psq_l 21,0x38(4),1,5
	psq_l 22,0x3a(4),1,5
	psq_l 23,0x54(4),1,5
	ps_merge00 16,16,18
	ps_merge00 17,17,19
	ps_merge00 20,20,22
	ps_merge00 21,21,23
	ps_mul 14,16,20
	ps_mul 15,17,21
	bl idctcolumn64
	b idct64_resume
idctcolumn64:
	ps_mul 16,1,9
	ps_mul 17,3,11
	ps_mul 18,7,9
	ps_mul 19,3,13
	ps_madd 16,7,15,16
	ps_madd 17,5,13,17
	ps_nmsub 18,1,15,18
	ps_nmsub 19,5,11,19
	ps_sub 20,16,17
	ps_add 17,16,17
	ps_sub 21,18,19
	ps_add 19,19,19
	ps_mul 16,4,20
	ps_mul 18,4,21
	ps_add 20,8,12
	ps_mul 21,2,10
	ps_sub 22,8,12
	ps_mul 23,6,10
	ps_mul 20,4,20
	ps_madd 21,6,14,21
	ps_mul 22,4,22
	ps_nmsub 23,2,14,23
	ps_sub 24,20,21
	ps_add 21,20,21
	ps_add 25,22,16
	ps_sub 26,18,23
	ps_sub 22,22,16
	ps_add 23,18,23
	ps_add 8,21,17
	ps_sub 15,21,17
	ps_add 9,25,23
	ps_sub 10,25,23
	ps_add 11,24,19
	ps_sub 12,24,19
	ps_add 13,22,26
	ps_sub 14,22,26
	blr
idct64_resume:
	ps_merge00 16,8,9
	ps_merge00 17,10,11
	ps_merge00 18,12,13
	ps_merge00 19,14,15
	ps_merge11 20,8,9
	ps_merge11 21,10,11
	ps_merge11 22,12,13
	ps_merge11 23,14,15
	psq_st 16,0x0(5),0,5
	psq_st 17,0x4(5),0,5
	psq_st 18,0x8(5),0,5
	psq_st 19,0xc(5),0,5
	psq_st 20,0x10(5),0,5
	psq_st 21,0x14(5),0,5
	psq_st 22,0x18(5),0,5
	psq_st 23,0x1c(5),0,5
	psq_l 16,0x6(3),1,5
	psq_l 17,0x10(3),1,5
	psq_l 18,0x12(3),1,5
	psq_l 19,0x16(3),1,5
	psq_l 20,0x6(4),1,5
	psq_l 21,0x10(4),1,5
	psq_l 22,0x12(4),1,5
	psq_l 23,0x16(4),1,5
	ps_merge00 16,16,18
	ps_merge00 17,17,19
	ps_merge00 20,20,22
	ps_merge00 21,21,23
	ps_mul 8,16,20
	ps_mul 9,17,21
	psq_l 16,0x18(3),1,5
	psq_l 17,0x22(3),1,5
	psq_l 18,0x24(3),1,5
	psq_l 19,0x30(3),1,5
	psq_l 20,0x18(4),1,5
	psq_l 21,0x22(4),1,5
	psq_l 22,0x24(4),1,5
	psq_l 23,0x30(4),1,5
	ps_merge00 16,16,18
	ps_merge00 17,17,19
	ps_merge00 20,20,22
	ps_merge00 21,21,23
	ps_mul 10,16,20
	ps_mul 11,17,21
	psq_l 16,0x32(3),1,5
	psq_l 17,0x3c(3),1,5
	psq_l 18,0x3e(3),1,5
	psq_l 19,0x50(3),1,5
	psq_l 20,0x32(4),1,5
	psq_l 21,0x3c(4),1,5
	psq_l 22,0x3e(4),1,5
	psq_l 23,0x50(4),1,5
	ps_merge00 16,16,18
	ps_merge00 17,17,19
	ps_merge00 20,20,22
	ps_merge00 21,21,23
	ps_mul 12,16,20
	ps_mul 13,17,21
	psq_l 16,0x52(3),1,5
	psq_l 17,0x56(3),1,5
	psq_l 18,0x58(3),1,5
	psq_l 19,0x6a(3),1,5
	psq_l 20,0x52(4),1,5
	psq_l 21,0x56(4),1,5
	psq_l 22,0x58(4),1,5
	psq_l 23,0x6a(4),1,5
	ps_merge00 16,16,18
	ps_merge00 17,17,19
	ps_merge00 20,20,22
	ps_merge00 21,21,23
	ps_mul 14,16,20
	ps_mul 15,17,21
	bl idctcolumn64
	ps_merge00 16,8,9
	ps_merge00 17,10,11
	ps_merge00 18,12,13
	ps_merge00 19,14,15
	ps_merge11 20,8,9
	ps_merge11 21,10,11
	ps_merge11 22,12,13
	ps_merge11 23,14,15
	psq_st 16,0x20(5),0,5
	psq_st 17,0x24(5),0,5
	psq_st 18,0x28(5),0,5
	psq_st 19,0x2c(5),0,5
	psq_st 20,0x30(5),0,5
	psq_st 21,0x34(5),0,5
	psq_st 22,0x38(5),0,5
	psq_st 23,0x3c(5),0,5
	psq_l 16,0x14(3),1,5
	psq_l 17,0x26(3),1,5
	psq_l 18,0x28(3),1,5
	psq_l 19,0x2c(3),1,5
	psq_l 20,0x14(4),1,5
	psq_l 21,0x26(4),1,5
	psq_l 22,0x28(4),1,5
	psq_l 23,0x2c(4),1,5
	ps_merge00 16,16,18
	ps_merge00 17,17,19
	ps_merge00 20,20,22
	ps_merge00 21,21,23
	ps_mul 8,16,20
	ps_mul 9,17,21
	psq_l 16,0x2e(3),1,5
	psq_l 17,0x40(3),1,5
	psq_l 18,0x42(3),1,5
	psq_l 19,0x4c(3),1,5
	psq_l 20,0x2e(4),1,5
	psq_l 21,0x40(4),1,5
	psq_l 22,0x42(4),1,5
	psq_l 23,0x4c(4),1,5
	ps_merge00 16,16,18
	ps_merge00 17,17,19
	ps_merge00 20,20,22
	ps_merge00 21,21,23
	ps_mul 10,16,20
	ps_mul 11,17,21
	psq_l 16,0x4e(3),1,5
	psq_l 17,0x5a(3),1,5
	psq_l 18,0x5c(3),1,5
	psq_l 19,0x66(3),1,5
	psq_l 20,0x4e(4),1,5
	psq_l 21,0x5a(4),1,5
	psq_l 22,0x5c(4),1,5
	psq_l 23,0x66(4),1,5
	ps_merge00 16,16,18
	ps_merge00 17,17,19
	ps_merge00 20,20,22
	ps_merge00 21,21,23
	ps_mul 12,16,20
	ps_mul 13,17,21
	psq_l 16,0x68(3),1,5
	psq_l 17,0x6c(3),1,5
	psq_l 18,0x6e(3),1,5
	psq_l 19,0x78(3),1,5
	psq_l 20,0x68(4),1,5
	psq_l 21,0x6c(4),1,5
	psq_l 22,0x6e(4),1,5
	psq_l 23,0x78(4),1,5
	ps_merge00 16,16,18
	ps_merge00 17,17,19
	ps_merge00 20,20,22
	ps_merge00 21,21,23
	ps_mul 14,16,20
	ps_mul 15,17,21
	bl idctcolumn64
	ps_merge00 16,8,9
	ps_merge00 17,10,11
	ps_merge00 18,12,13
	ps_merge00 19,14,15
	ps_merge11 20,8,9
	ps_merge11 21,10,11
	ps_merge11 22,12,13
	ps_merge11 23,14,15
	psq_st 16,0x40(5),0,5
	psq_st 17,0x44(5),0,5
	psq_st 18,0x48(5),0,5
	psq_st 19,0x4c(5),0,5
	psq_st 20,0x50(5),0,5
	psq_st 21,0x54(5),0,5
	psq_st 22,0x58(5),0,5
	psq_st 23,0x5c(5),0,5
	psq_l 16,0x2a(3),1,5
	psq_l 17,0x44(3),1,5
	psq_l 18,0x46(3),1,5
	psq_l 19,0x48(3),1,5
	psq_l 20,0x2a(4),1,5
	psq_l 21,0x44(4),1,5
	psq_l 22,0x46(4),1,5
	psq_l 23,0x48(4),1,5
	ps_merge00 16,16,18
	ps_merge00 17,17,19
	ps_merge00 20,20,22
	ps_merge00 21,21,23
	ps_mul 8,16,20
	ps_mul 9,17,21
	psq_l 16,0x4a(3),1,5
	psq_l 17,0x5e(3),1,5
	psq_l 18,0x60(3),1,5
	psq_l 19,0x62(3),1,5
	psq_l 20,0x4a(4),1,5
	psq_l 21,0x5e(4),1,5
	psq_l 22,0x60(4),1,5
	psq_l 23,0x62(4),1,5
	ps_merge00 16,16,18
	ps_merge00 17,17,19
	ps_merge00 20,20,22
	ps_merge00 21,21,23
	ps_mul 10,16,20
	ps_mul 11,17,21
	psq_l 16,0x64(3),1,5
	psq_l 17,0x70(3),1,5
	psq_l 18,0x72(3),1,5
	psq_l 19,0x74(3),1,5
	psq_l 20,0x64(4),1,5
	psq_l 21,0x70(4),1,5
	psq_l 22,0x72(4),1,5
	psq_l 23,0x74(4),1,5
	ps_merge00 16,16,18
	ps_merge00 17,17,19
	ps_merge00 20,20,22
	ps_merge00 21,21,23
	ps_mul 12,16,20
	ps_mul 13,17,21
	psq_l 16,0x76(3),1,5
	psq_l 17,0x7a(3),1,5
	psq_l 18,0x7c(3),1,5
	psq_l 19,0x7e(3),1,5
	psq_l 20,0x76(4),1,5
	psq_l 21,0x7a(4),1,5
	psq_l 22,0x7c(4),1,5
	psq_l 23,0x7e(4),1,5
	ps_merge00 16,16,18
	ps_merge00 17,17,19
	ps_merge00 20,20,22
	ps_merge00 21,21,23
	ps_mul 14,16,20
	ps_mul 15,17,21
	bl idctcolumn64
	ps_merge00 16,8,9
	ps_merge00 17,10,11
	ps_merge00 18,12,13
	ps_merge00 19,14,15
	ps_merge11 20,8,9
	ps_merge11 21,10,11
	ps_merge11 22,12,13
	ps_merge11 23,14,15
	psq_st 16,0x60(5),0,5
	psq_st 17,0x64(5),0,5
	psq_st 18,0x68(5),0,5
	psq_st 19,0x6c(5),0,5
	psq_st 20,0x70(5),0,5
	psq_st 21,0x74(5),0,5
	psq_st 22,0x78(5),0,5
	psq_st 23,0x7c(5),0,5
	li 0,0x4
	mtctr 0
idct64_loop:
	psq_l 8,0x0(5),0,5
	psq_l 9,0x10(5),0,5
	psq_l 10,0x20(5),0,5
	psq_l 11,0x30(5),0,5
	psq_l 12,0x40(5),0,5
	psq_l 13,0x50(5),0,5
	psq_l 14,0x60(5),0,5
	psq_l 15,0x70(5),0,5
	bl idctcolumn64
	ps_add 8,8,0
	ps_add 9,9,0
	ps_add 10,10,0
	ps_add 11,11,0
	ps_add 12,12,0
	ps_add 13,13,0
	ps_add 14,14,0
	ps_add 15,15,0
	psq_st 8,0x0(5),0,5
	psq_st 9,0x10(5),0,5
	psq_st 10,0x20(5),0,5
	psq_st 11,0x30(5),0,5
	psq_st 12,0x40(5),0,5
	psq_st 13,0x50(5),0,5
	psq_st 14,0x60(5),0,5
	psq_st 15,0x70(5),0,5
	addi 5,5,0x4
	bdnz idct64_loop
	li 0,0x10
	subi 5,5,0x10
	mtctr 0
idct64_scale:
	lha 7,0x0(5)
	lha 8,0x2(5)
	lha 9,0x4(5)
	lha 10,0x6(5)
	srawi 7,7,4
	srawi 8,8,4
	srawi 9,9,4
	srawi 10,10,4
	sth 7,0x0(5)
	sth 8,0x2(5)
	sth 9,0x4(5)
	sth 10,0x6(5)
	addi 5,5,0x8
	bdnz idct64_scale
	mtlr 6
	lfd 14,0x8(1)
	lfd 15,0x10(1)
	lfd 16,0x18(1)
	lfd 17,0x20(1)
	lfd 18,0x28(1)
	lfd 19,0x30(1)
	lfd 20,0x38(1)
	lfd 21,0x40(1)
	lfd 22,0x48(1)
	lfd 23,0x50(1)
	lfd 24,0x58(1)
	lfd 25,0x60(1)
	lfd 26,0x68(1)
	lfd 27,0x70(1)
	lfd 28,0x78(1)
	lfd 29,0x80(1)
	lfd 30,0x88(1)
	lfd 31,0x90(1)
	addi 1,1,0x98
");
}

void IDct1_GC(short *InputData, short *QuantMatrix, short *OutputData) {
    unsigned int *dest = (unsigned int *)OutputData;
    unsigned int out;
    int i;

    out = (InputData[0] * QuantMatrix[0] + 15) >> 5;
    out = (out << 16) | (out & 0xFFFF);

    for (i = 0; i < 8; i++) {
        dest[0] = out;
        dest[1] = out;
        dest[2] = out;
        dest[3] = out;
        dest += 4;
    }
}

void UnpackBlock_GC(unsigned char *ReconPtr, short *ReconRefPtr, unsigned int ReconPixelsPerLine) {
    asm("	li 0,0x8
	mtctr 0
unpack_loop:
	psq_l 0,0x0(3),0,2
	psq_l 1,0x2(3),0,2
	psq_l 2,0x4(3),0,2
	psq_l 3,0x6(3),0,2
	add 3,3,5
	psq_st 0,0x0(4),0,5
	psq_st 1,0x4(4),0,5
	psq_st 2,0x8(4),0,5
	psq_st 3,0xc(4),0,5
	addi 4,4,0x10
	bdnz unpack_loop
");
}

void FilterBlock1dBil_GC(unsigned char *SrcPtr, unsigned short *OutputPtr,
                         unsigned int SrcPixelsPerLine, unsigned int PixelStep, int *Filter) {
    asm("	li 9,f64@sda21
	li 0,0x8
	psq_l 8,0x3(7),1,2
	psq_l 9,0x7(7),1,2
	psq_l 10,0x0(9),1,0
	mtctr 0
	add 6,3,6
	ps_merge00 10,10,10
filt1d_loop:
	psq_l 0,0x0(3),0,2
	psq_l 1,0x2(3),0,2
	psq_l 2,0x4(3),0,2
	psq_l 3,0x6(3),0,2
	add 3,3,5
	psq_l 4,0x0(6),0,2
	psq_l 5,0x2(6),0,2
	psq_l 6,0x4(6),0,2
	psq_l 7,0x6(6),0,2
	add 6,6,5
	ps_madds0 0,0,8,10
	ps_madds0 1,1,8,10
	ps_madds0 2,2,8,10
	ps_madds0 3,3,8,10
	ps_madds0 0,4,9,0
	ps_madds0 1,5,9,1
	ps_madds0 2,6,9,2
	ps_madds0 3,7,9,3
	psq_st 0,0x0(4),0,6
	psq_st 1,0x4(4),0,6
	psq_st 2,0x8(4),0,6
	psq_st 3,0xc(4),0,6
	addi 4,4,0x10
	bdnz filt1d_loop
");
}

void FilterBlock2dBil_GC(unsigned char *SrcPtr, unsigned short *OutputPtr,
                         unsigned int SrcPixelsPerLine, int *HFilter, int *VFilter) {
    static unsigned short FData[72];
    asm("	lis 9,%0@ha
	li 11,f64@sda21
	addi 9,9,%0@l
	li 0,0x9
	psq_l 8,0x3(6),1,2
	psq_l 9,0x7(6),1,2
	psq_l 10,0x0(11),1,0
	mtctr 0
	addi 6,3,0x1
	ps_merge00 10,10,10
filt2d_loop1:
	psq_l 0,0x0(3),0,2
	psq_l 1,0x2(3),0,2
	psq_l 2,0x4(3),0,2
	psq_l 3,0x6(3),0,2
	add 3,3,5
	psq_l 4,0x0(6),0,2
	psq_l 5,0x2(6),0,2
	psq_l 6,0x4(6),0,2
	psq_l 7,0x6(6),0,2
	add 6,6,5
	ps_madds0 0,0,8,10
	ps_madds0 1,1,8,10
	ps_madds0 2,2,8,10
	ps_madds0 3,3,8,10
	ps_madds0 0,4,9,0
	ps_madds0 1,5,9,1
	ps_madds0 2,6,9,2
	ps_madds0 3,7,9,3
	psq_st 0,0x0(9),0,6
	psq_st 1,0x4(9),0,6
	psq_st 2,0x8(9),0,6
	psq_st 3,0xc(9),0,6
	addi 9,9,0x10
	bdnz filt2d_loop1
	li 0,0x8
	subi 3,9,0x90
	psq_l 8,0x3(7),1,2
	psq_l 9,0x7(7),1,2
	mtctr 0
	addi 6,3,0x10
filt2d_loop2:
	psq_l 0,0x0(3),0,3
	psq_l 1,0x4(3),0,3
	psq_l 2,0x8(3),0,3
	psq_l 3,0xc(3),0,3
	addi 3,3,0x10
	psq_l 4,0x0(6),0,3
	psq_l 5,0x4(6),0,3
	psq_l 6,0x8(6),0,3
	psq_l 7,0xc(6),0,3
	addi 6,6,0x10
	ps_madds0 0,0,8,10
	ps_madds0 1,1,8,10
	ps_madds0 2,2,8,10
	ps_madds0 3,3,8,10
	ps_madds0 0,4,9,0
	ps_madds0 1,5,9,1
	ps_madds0 2,6,9,2
	ps_madds0 3,7,9,3
	psq_st 0,0x0(4),0,6
	psq_st 1,0x4(4),0,6
	psq_st 2,0x8(4),0,6
	psq_st 3,0xc(4),0,6
	addi 4,4,0x10
	bdnz filt2d_loop2
" : : "s"(FData));
}

void FilterBlock_GC(unsigned char *ReconPtr1, unsigned char *ReconPtr2, unsigned short *ReconRefPtr,
                    unsigned int PixelsPerLine, int ModX, int ModY, int UseBicubic) {
    int diff = ReconPtr2 - ReconPtr1;

    if (diff < 0) {
        unsigned char *temp = ReconPtr1;
        ReconPtr1 = ReconPtr2;
        ReconPtr2 = temp;
        diff = ReconPtr2 - ReconPtr1;
    }

    if (!UseBicubic) {
        if (diff == 1) {
            FilterBlock1dBil_GC(ReconPtr1, ReconRefPtr, PixelsPerLine, 1, BilinearFilters[ModX]);
        } else if (diff == PixelsPerLine) {
            FilterBlock1dBil_GC(ReconPtr1, ReconRefPtr, PixelsPerLine, PixelsPerLine,
                                BilinearFilters[ModY]);
        } else if (diff == PixelsPerLine - 1) {
            FilterBlock2dBil_GC(ReconPtr1 - 1, ReconRefPtr, PixelsPerLine, BilinearFilters[ModX],
                                BilinearFilters[ModY]);
        } else {
            FilterBlock2dBil_GC(ReconPtr1, ReconRefPtr, PixelsPerLine, BilinearFilters[ModX],
                                BilinearFilters[ModY]);
        }
    } else {
        if (diff == 1) {
            FilterBlock1d(ReconPtr1, ReconRefPtr, PixelsPerLine, 1, 8, 8, BicubicFilters[ModX]);
        } else if (diff == PixelsPerLine) {
            FilterBlock1d(ReconPtr1, ReconRefPtr, PixelsPerLine, PixelsPerLine, 8, 8,
                          BicubicFilters[ModY]);
        } else if (diff == PixelsPerLine - 1) {
            FilterBlock2d(ReconPtr1 - 1, ReconRefPtr, PixelsPerLine, BicubicFilters[ModX],
                          BicubicFilters[ModY]);
        } else {
            FilterBlock2d(ReconPtr1, ReconRefPtr, PixelsPerLine, BicubicFilters[ModX],
                          BicubicFilters[ModY]);
        }
    }
}
