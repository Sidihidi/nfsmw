#include <cstddef>

#include "../extern/endian.h"

namespace Snd {

// total size: 0x8
struct XAFSTATE {
    float sample1; // offset 0x0
    float sample2; // offset 0x4
};

// total size: 0x14
struct MXAPACKETF {
    int numframes;        // offset 0x0
    float sample1;        // offset 0x4
    float sample2;        // offset 0x8
    unsigned char *psrc;  // offset 0xC
    float *pdst;          // offset 0x10
};

// total size: 0x90
struct EAXAVARSF {
    int residual;          // offset 0x0
    int numsamples;        // offset 0x4
    int sampledatasize;    // offset 0x8
    float *presidue;       // offset 0xC
    float xablockprev[2];  // offset 0x10
    float xablock[30];     // offset 0x18
};

// total size: 0xA8
struct CEAXABLKDecf {
    static void *operator new(size_t size);
    static void operator delete(void *ptr);

    CEAXABLKDecf();
    ~CEAXABLKDecf() {}
    int Feed(void *pSampleData, int sampleDataSize, int numSamples);
    int Decode(float **pDstBuf, int numSamples);
    XAFSTATE GetState();
    void SetState(XAFSTATE *pxas);

    void (*decodexa)(MXAPACKETF *); // offset 0x0
    EAXAVARSF xav;                  // offset 0x4
    MXAPACKETF xap;                 // offset 0x94
};

extern void *(*CODANew)(unsigned int size);
extern void (*CODADelete)(void *pmem);
#ifdef EA_PLATFORM_WIN32
// PC: lo que la CPU sabe hacer (coda.cpp); el bit 2 es SSE
extern int CODACPUFlags;
#endif

float xafilterf[2][4] = {
    {0.0f, 0.9375f, 1.796875f, 1.53125f},
    {0.0f, 0.0f, -0.8125f, -0.859375f},
};

float xatablef[16][16] = {
    {0.0f, 4096.0f, 8192.0f, 12288.0f, 16384.0f, 20480.0f, 24576.0f, 28672.0f, -32768.0f, -28672.0f, -24576.0f, -20480.0f, -16384.0f, -12288.0f,
     -8192.0f, -4096.0f},
    {0.0f, 2048.0f, 4096.0f, 6144.0f, 8192.0f, 10240.0f, 12288.0f, 14336.0f, -16384.0f, -14336.0f, -12288.0f, -10240.0f, -8192.0f, -6144.0f, -4096.0f,
     -2048.0f},
    {0.0f, 1024.0f, 2048.0f, 3072.0f, 4096.0f, 5120.0f, 6144.0f, 7168.0f, -8192.0f, -7168.0f, -6144.0f, -5120.0f, -4096.0f, -3072.0f, -2048.0f,
     -1024.0f},
    {0.0f, 512.0f, 1024.0f, 1536.0f, 2048.0f, 2560.0f, 3072.0f, 3584.0f, -4096.0f, -3584.0f, -3072.0f, -2560.0f, -2048.0f, -1536.0f, -1024.0f,
     -512.0f},
    {0.0f, 256.0f, 512.0f, 768.0f, 1024.0f, 1280.0f, 1536.0f, 1792.0f, -2048.0f, -1792.0f, -1536.0f, -1280.0f, -1024.0f, -768.0f, -512.0f, -256.0f},
    {0.0f, 128.0f, 256.0f, 384.0f, 512.0f, 640.0f, 768.0f, 896.0f, -1024.0f, -896.0f, -768.0f, -640.0f, -512.0f, -384.0f, -256.0f, -128.0f},
    {0.0f, 64.0f, 128.0f, 192.0f, 256.0f, 320.0f, 384.0f, 448.0f, -512.0f, -448.0f, -384.0f, -320.0f, -256.0f, -192.0f, -128.0f, -64.0f},
    {0.0f, 32.0f, 64.0f, 96.0f, 128.0f, 160.0f, 192.0f, 224.0f, -256.0f, -224.0f, -192.0f, -160.0f, -128.0f, -96.0f, -64.0f, -32.0f},
    {0.0f, 16.0f, 32.0f, 48.0f, 64.0f, 80.0f, 96.0f, 112.0f, -128.0f, -112.0f, -96.0f, -80.0f, -64.0f, -48.0f, -32.0f, -16.0f},
    {0.0f, 8.0f, 16.0f, 24.0f, 32.0f, 40.0f, 48.0f, 56.0f, -64.0f, -56.0f, -48.0f, -40.0f, -32.0f, -24.0f, -16.0f, -8.0f},
    {0.0f, 4.0f, 8.0f, 12.0f, 16.0f, 20.0f, 24.0f, 28.0f, -32.0f, -28.0f, -24.0f, -20.0f, -16.0f, -12.0f, -8.0f, -4.0f},
    {0.0f, 2.0f, 4.0f, 6.0f, 8.0f, 10.0f, 12.0f, 14.0f, -16.0f, -14.0f, -12.0f, -10.0f, -8.0f, -6.0f, -4.0f, -2.0f},
    {0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, -8.0f, -7.0f, -6.0f, -5.0f, -4.0f, -3.0f, -2.0f, -1.0f},
    {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f},
    {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f},
    {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f},
};

// En PS2 el simbolo no lleva decoracion (tabla de simbolos del NFS.ELF de la A124): enlace de C; en GameCube, de C++.
#ifdef EA_PLATFORM_PLAYSTATION2
extern "C" {
#endif
void process_raw_block(MXAPACKETF *pxap) {
    unsigned int i;
    short temp;

    if (*pxap->psrc == 0xEE) {
        pxap->psrc++;
        PutUB(temp, *(short *)pxap->psrc);
        pxap->sample1 = temp;
        pxap->psrc += 2;
        PutUB(temp, *(short *)pxap->psrc);
        pxap->sample2 = temp;
        pxap->psrc += 2;
        for (i = 0; i < 28; i++) {
            PutUB(temp, *(short *)pxap->psrc);
            *pxap->pdst = temp;
            pxap->pdst++;
            pxap->psrc += 2;
        }
    }
}
#ifdef EA_PLATFORM_PLAYSTATION2
}
#endif

#ifdef EA_PLATFORM_WIN32
// PC: asm SSE de EA (speed.exe 0x828CE9, 245 B; docs/asm-de-ea.md). Guarda todos los registros (tambien alrededor de
// process_raw_block, con pushad/pushfd), lleva sample1/sample2 en xmm2/xmm3 y cuenta los 14 pares de -14 a 0 en ecx.
// MSVC 7.1 no genera SSE escalar; el C de abajo (el de GameCube) es el que compilan las demas versiones y los ports.
__declspec(naked) void decodexac(MXAPACKETF *xap) {
    __asm {
        push eax
        push ebx
        push ecx
        push edx
        push esi
        push edi
        push ebp
        mov esi, [esp + 0x20]
        mov edi, [esi + 0xC]
        mov ebp, [esi + 0x10]
        movss xmm2, [esi + 4]
        movss xmm3, [esi + 8]
        movss xmm4, xmm2
    nextblock:
        cmp dword ptr [esi], 0
        jle done
        sub dword ptr [esi], 0x1C
        movzx eax, byte ptr [edi]
        cmp eax, 0xEE
        jne decodeblock
        pushad
        pushfd
        push esi
        call process_raw_block
        add esp, 4
        popfd
        popad
        movss xmm2, [esi + 4]
        movss xmm3, [esi + 8]
        movss xmm4, xmm2
        mov edi, [esi + 0xC]
        mov ebp, [esi + 0x10]
        jmp nextblock
    decodeblock:
        movzx eax, byte ptr [edi]
        shr eax, 4
        movzx ebx, byte ptr [edi]
        and ebx, 0xF
        shl ebx, 6
        add edi, 1
        xor edx, edx
        mov ecx, -14
        add edi, 14
        add ebp, 0x70
    decodepair:
        mulss xmm4, xafilterf[eax * 4]
        mov dl, [edi + ecx]
        shr dl, 4
        mulss xmm3, xafilterf[eax * 4 + 0x10]
        addss xmm4, xatablef[ebx + edx * 4]
        mulss xmm2, xafilterf[eax * 4 + 0x10]
        addss xmm4, xmm3
        movss [ebp + ecx * 8], xmm4
        movss xmm3, xmm4
        mov dl, [edi + ecx]
        and dl, 0xF
        mulss xmm4, xafilterf[eax * 4]
        addss xmm4, xatablef[ebx + edx * 4]
        addss xmm4, xmm2
        movss xmm2, xmm4
        movss [ebp + ecx * 8 + 4], xmm4
        inc ecx
        jl decodepair
        movss [esi + 4], xmm2
        movss [esi + 8], xmm3
        mov [esi + 0x10], ebp
        mov [esi + 0xC], edi
        jmp nextblock
    done:
        pop ebp
        pop edi
        pop esi
        pop edx
        pop ecx
        pop ebx
        pop eax
        ret
    }
}
// PC: la gemela x87 de decodexac (speed.exe 0x828DDE, 245 B; docs/asm-de-ea.md), para las CPU sin SSE: el constructor
// elige una u otra. No es naked: el marco y ebx/esi/edi los pone el compilador; el resto, el asm.
void decodexax87(MXAPACKETF *xap) {
    __asm {
        push eax
        push ebx
        push ecx
        push edx
        push esi
        push edi
        mov esi, xap
        mov ebx, [esi + 0x10]
        mov edx, [esi + 4]
        mov ecx, [esi + 8]
        mov [ebx - 4], edx
        mov [ebx - 8], ecx
        mov edi, [esi + 0xC]
        xor edx, edx
    nextblock:
        cmp dword ptr [esi], 0
        jle done
        sub dword ptr [esi], 0x1C
        xor eax, eax
        mov al, [edi]
        cmp al, 0xEE
        jne decodeblock
        pushad
        pushfd
        push esi
        call process_raw_block
        add esp, 4
        popfd
        popad
        mov esi, xap
        mov ebx, [esi + 0x10]
        mov edx, [esi + 4]
        mov ecx, [esi + 8]
        mov [ebx - 4], edx
        mov [ebx - 8], ecx
        mov edi, [esi + 0xC]
        xor edx, edx
        jmp nextblock
    decodeblock:
        mov al, [edi]
        shr eax, 4
        fld xafilterf[eax * 4 + 0x10]
        fld xafilterf[eax * 4]
        mov al, [edi]
        mov ecx, -14
        mov dl, [edi + 1]
        shr dl, 4
        and eax, 0xF
        add ebx, 0x70
        shl eax, 6
        add edi, 15
    decodepair:
        fld dword ptr [ebx + ecx * 8 - 4]
        fmul st, st(1)
        fld dword ptr [ebx + ecx * 8 - 8]
        fmul st, st(3)
        fxch st(1)
        fadd xatablef[eax + edx * 4]
        faddp st(1), st
        mov dl, [edi + ecx]
        and dl, 0xF
        fstp dword ptr [ebx + ecx * 8]
        fld dword ptr [ebx + ecx * 8]
        fmul st, st(1)
        fld dword ptr [ebx + ecx * 8 - 4]
        fmul st, st(3)
        fxch st(1)
        fadd xatablef[eax + edx * 4]
        faddp st(1), st
        mov dl, [edi + ecx + 1]
        shr dl, 4
        fstp dword ptr [ebx + ecx * 8 + 4]
        inc ecx
        jl decodepair
        fstp st(0)
        fstp st(0)
        push edx
        mov edx, [ebx - 4]
        mov [esi + 4], edx
        mov edx, [ebx - 8]
        mov [esi + 8], edx
        pop edx
        mov [esi + 0x10], ebx
        mov [esi + 0xC], edi
        jmp nextblock
    done:
        pop edi
        pop esi
        pop edx
        pop ecx
        pop ebx
        pop eax
    }
}
#else
void decodexac(MXAPACKETF *xap) {
    int i;
    int filt;
    int shift;
    unsigned char b;
    unsigned char *ps;
    float f0;
    float f1;
    // NOTA ronda 18 (aplicado en eaxadecf_ps2.s, solo PS2): en PS2 el bucle esta desenrollado 14 veces con
    // adda.s/madda.s/madd.s ($f10/$f11 de estado, $f0/$f1 de constantes; bytes psrc[1..14], stores
    // pdst[0..108]), tres etiquetas locales con nombre en el symtab (decodeblock/decodeps2normal/done) y
    // marco a mano (addi sp, lq ra). ee-gcc no tiene patron para madda: es asm de EA por compilador, no
    // por forma (el C pierde 840 B por PS2; -ffast-math medido neutro).

    ps = xap->psrc;
    xap->pdst[-2] = xap->sample2;
    xap->pdst[-1] = xap->sample1;
    while (xap->numframes > 0) {
        if (*ps == 0xEE) {
            process_raw_block(xap);
            xap->numframes -= 28;
        } else {
            filt = *xap->psrc >> 4;
            xap->numframes -= 28;
            f0 = xafilterf[0][filt];
            f1 = xafilterf[1][filt];
            shift = *xap->psrc & 0xF;
            xap->psrc++;
            for (i = 0; i < 14; i++) {
                b = xap->psrc[i];
                xap->pdst[0] = xatablef[shift][b >> 4] + xap->pdst[-1] * f0 + xap->pdst[-2] * f1;
                xap->pdst[1] = xatablef[shift][b & 0xF] + xap->pdst[0] * f0 + xap->pdst[-1] * f1;
                xap->pdst += 2;
            }
            xap->psrc += 14;
            xap->sample2 = xap->pdst[-2];
            xap->sample1 = xap->pdst[-1];
        }
        ps = xap->psrc;
    }
}
#endif

void *CEAXABLKDecf::operator new(size_t size) {
    return CODANew(size);
}

void CEAXABLKDecf::operator delete(void *ptr) {
    CODADelete(ptr);
}

CEAXABLKDecf::CEAXABLKDecf() {
    xav.sampledatasize = 0;
    xav.numsamples = 0;
    xav.residual = 0;
    xap.sample1 = 0.0f;
    xap.sample2 = 0.0f;
#ifdef EA_PLATFORM_WIN32
    // PC (0x828ED3): SSE si la CPU lo tiene (CODACPUFlags, coda.cpp); si no, x87
    if (CODACPUFlags & 2) {
        decodexa = decodexac;
    } else {
        decodexa = decodexax87;
    }
#endif
}

int CEAXABLKDecf::Feed(void *pSampleData, int sampleDataSize, int numSamples) {
    if (pSampleData == NULL) {
        return -1;
    }
    if (xav.numsamples != 0) {
        return -1;
    }
    xav.numsamples = numSamples;
    xav.sampledatasize = sampleDataSize;
    xap.psrc = (unsigned char *)pSampleData;
    return 0;
}

int CEAXABLKDecf::Decode(float **pDstBuf, int numSamples) {
    int retVal;
    int framesleft;
    int i;
    int tmpframecnt;
    int maxsamples;
    float *ptmpDst;

    retVal = 0;
    xap.pdst = *pDstBuf;
    if (xav.numsamples == 0) {
        return 0;
    }
    maxsamples = numSamples < xav.numsamples ? numSamples : xav.numsamples;
    if (xav.residual != 0) {
        if (xav.residual > maxsamples) {
            tmpframecnt = maxsamples;
        } else {
            tmpframecnt = xav.residual;
        }
        for (i = 0; i < tmpframecnt; i++) {
            *xap.pdst = *xav.presidue;
            xap.pdst++;
            xav.presidue++;
        }
        xav.numsamples -= tmpframecnt;
        xav.residual -= tmpframecnt;
        maxsamples -= tmpframecnt;
        retVal += tmpframecnt;
    }
    i = (maxsamples / 28) * 28;
    xap.numframes = i;
    framesleft = maxsamples - i;
    if (i > 0) {
#ifdef EA_PLATFORM_WIN32
        decodexa(&xap);
#else
        decodexac(&xap);
#endif
        xav.numsamples -= i;
        retVal += i;
    }
    if (framesleft > 0) {
        ptmpDst = xap.pdst;
        xap.pdst = &xav.xablock[2];
        xap.numframes = framesleft;
#ifdef EA_PLATFORM_WIN32
        decodexa(&xap);
#else
        decodexac(&xap);
#endif
        xav.numsamples -= framesleft;
        xav.presidue = xap.pdst + xap.numframes;
        xav.residual = -xap.numframes;
        xap.pdst -= 28;
        for (i = 0; i < framesleft; i++) {
            ptmpDst[i] = xap.pdst[i];
        }
        retVal += framesleft;
    }
    if (xav.numsamples <= 0) {
        xav.residual = 0;
    }
    return retVal;
}

XAFSTATE CEAXABLKDecf::GetState() {
    XAFSTATE xtmp;

    xtmp.sample1 = xap.sample1;
    xtmp.sample2 = xap.sample2;
    return xtmp;
}

void CEAXABLKDecf::SetState(XAFSTATE *pxas) {
    xap.sample1 = pxas->sample1;
    xap.sample2 = pxas->sample2;
}

} // namespace Snd
