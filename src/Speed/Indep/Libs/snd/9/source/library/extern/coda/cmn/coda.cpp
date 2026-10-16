#include <cstddef>
#include <string.h>
#ifdef EA_PLATFORM_WIN32
#include "Speed/PC/Src/Win32Min.hpp" // GetVersionExA
#endif

namespace Snd {

void *(*CODANew)(unsigned int size);
#ifdef EA_PLATFORM_WIN32
// PC (0x9C2540, entre CODANew y CODADelete): lo que la CPU sabe hacer; el bit 2 es SSE (eaxadecf.cpp elige con el
// el decodificador de EA-XA). Nombre nuestro.
int CODACPUFlags;
#endif
void (*CODADelete)(void *pmem);

namespace Coda {

// El tercer parametro va como size_t y no como unsigned int: en GameCube son el
// mismo tipo --y el codigo sale identico-- pero en LP64 memcpy toma unsigned long
// y la inicializacion no compila.
void *(*gpMemCpy)(void *pdst, void const *psrc, size_t size) = memcpy;

} // namespace Coda

void CODASetNew(void *(*pNew)(unsigned int size)) {
    CODANew = pNew;
}

void CODASetDelete(void (*pfn)(void *pmem)) {
    CODADelete = pfn;
}

#ifdef EA_PLATFORM_WIN32
// PC: la deteccion de CPU de CODA (speed.exe 0x825B1C-0x825C0B; docs/asm-de-ea.md). Lo de dentro de los __asm es de EA:
// el bit ID de EFLAGS y la instruccion cpuid, que el C de MSVC 7.1 no puede escribir. Nombres nuestros.

// 0x825B1C: la CPU tiene cpuid si el bit 21 (ID) de EFLAGS se deja cambiar.
int CODAHasCPUID() {
    int result;

    __asm {
        push ecx
        pushfd
        push ebx
        mov ebx, esp
        sub esp, 0x20
        pushfd
        pop eax
        mov ecx, eax
        xor eax, 0x200000
        push eax
        popfd
        pushfd
        pop eax
        xor eax, ecx
        mov result, eax
        mov esp, ebx
        pop ebx
        popfd
        pop ecx
    }
    return result;
}

// 0x825B46: cpuid con la funcion pedida; eax, ebx, ecx y edx en regs[0..3].
__declspec(naked) void CODACPUID(unsigned int *regs, unsigned int function) {
    __asm {
        push ebx
        push esi
        push edi
        push eax
        push ebx
        push ecx
        push edx
        push esi
        push edi
        mov eax, [esp + 0x2C]
        cpuid
        mov esi, [esp + 0x28]
        mov [esi], eax
        mov [esi + 4], ebx
        mov [esi + 8], ecx
        mov [esi + 0xC], edx
        pop edi
        pop esi
        pop edx
        pop ecx
        pop ebx
        pop eax
        pop edi
        pop esi
        pop ebx
        ret
    }
}

// 0x825B6E: 1 siempre; 2 si hay SSE (y el sistema guarda sus registros: Windows 95 no); 4 si hay 3DNow!.
int CODAGetCPUFlags() {
    OSVERSIONINFOA osvi;
    unsigned int regs[4];
    int win95;
    int flags;

    win95 = 0;
    osvi.dwOSVersionInfoSize = sizeof(osvi);
    GetVersionExA(&osvi);
    flags = 1;
    if (osvi.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS && osvi.dwMinorVersion == 0) {
        win95 = 1;
    }
    if (CODAHasCPUID()) {
        CODACPUID(regs, 0);
        if (regs[0] >= 1) {
            CODACPUID(regs, 1);
            if ((regs[3] & 0x2000000) && !win95) {
                flags = 3;
            }
            CODACPUID(regs, 0x80000001);
            if (regs[3] & 0x80000000) {
                flags |= 4;
            }
        }
    }
    return flags;
}

// 0x825BFA y su inicializador estatico (0x88DEEB): la deteccion se hace al arrancar.
struct CODACPUDetect {
    CODACPUDetect() {
        CODACPUFlags = CODAGetCPUFlags();
    }
};

CODACPUDetect gCODACPUDetect;
#endif

} // namespace Snd
