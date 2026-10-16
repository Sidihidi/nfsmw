#include "types.h"

// El simbolo es GLOBAL en el objeto extraido y cae en .rodata +0x138: un
// `const` de ambito de fichero tendria enlace interno y GCC lo diferiria a
// finish_file. La declaracion extern le da enlace externo y lo emite aqui.
extern const u32 FEKeyTypeSize[7];
const u32 FEKeyTypeSize[7] = {4, 4, 4, 8, 12, 16, 16}; // size: 0x1C, address: 0x803EA920, Decl: speed/indep/src/feng/FEKeyTypes.cpp:11

const bool FEKeyInterpValid[7][5] = { // size: 0x23, address: 0xFFFFFFFF, Decl: speed/indep/src/feng/FEKeyTypes.cpp:21
    {1, 0, 0, 0, 0}, {0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}, {1, 0, 0, 0, 1}, {0, 0, 0, 1, 0}, {0, 0, 1, 0, 0}};

// Solo lo usa FETypeLib::CreateObjectScript. En GameCube el enlazador estripa los dos y de sus 28 B deja la cola de
// 4 (`00000001`, lbl_8041D078 en la .data de zFEng); en PS2 estan enteros (A124 0x614C00, SLUS 0x570928).
i32 FEKeyInterpDefault[7] = {0, 1, 1, 1, 1, 1, 1}; // size: 0x1C, Decl: speed/indep/src/feng/FEKeyTypes.cpp:32
