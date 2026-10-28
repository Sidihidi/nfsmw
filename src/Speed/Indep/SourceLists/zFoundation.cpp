// Prefijo comun de las unidades de juego del original (como zFEng.cpp): en la A124 los 402 primeros tipos de
// los stabs son los mismos en todas, y la .rodata empieza por las cadenas de bWare.hpp.
#include "Speed/Indep/bWare/Inc/bWare.hpp"
#include "Speed/Indep/Libs/Support/Utility/UStandard.h"
#include "Speed/Indep/Libs/Support/Utility/UCOM.h"
#include "Speed/Indep/Libs/Support/Utility/UCollections.h"
const UCrc32 UCrc32::kNull; // la definicion va AQUI, no en UGroup.cpp: ee-gcc 2.9 nombra los fragmentos _GLOBAL_$I$/$D$ con el primer .globl del ensamblado (el original: _6UCrc32$kNull), y ngccc de GC los nombra con la primera funcion (Evaluate__11UBezierLite, el del original de GC). Los dos nombres salen buenos a la vez y el orden de los inicializadores dinamicos no se mueve: kFloatScaleUp/Down ya se han registrado en el prefijo (UStandard.h incluye UVectorMath.h) y UBezierLite.cpp/UEALibs.cpp no aportan ninguno. Ver docs/analisis/2026-10-25_zfound91.md
#include "Speed/Indep/Libs/Support/Utility/UBezierLite.cpp"

#include "Speed/Indep/Libs/Support/Utility/UEALibs.cpp"

#include "Speed/Indep/Libs/Support/Utility/UGroup.cpp"

#include "Speed/Indep/Libs/Support/Utility/UMath.cpp"

#include "Speed/Indep/Libs/Support/Utility/FastMem.cpp"

#include "Speed/Indep/Libs/Support/Utility/UStringToNumber.cpp"

#include "Speed/Indep/Libs/Support/Utility/UVectorMath.cpp"

#include "Speed/Indep/Libs/Support/Utility/UFoundationBody.cpp"

#include "Speed/Indep/Libs/Support/Utility/USpline.cpp"

#include "Speed/Indep/Libs/Support/Utility/UCOM.cpp"

#include "Speed/Indep/Libs/Support/Utility/UCollections.cpp"

#include "Speed/Indep/Libs/Support/Miscellaneous/Carp.cpp"

#include "Speed/Indep/Libs/Support/Miscellaneous/StringHash.cpp"

#include "Speed/Indep/Libs/Support/Miscellaneous/ExpressionEvaluator.cpp"

#include "Speed/Indep/Libs/Support/Miscellaneous/StringRegistry.cpp"
