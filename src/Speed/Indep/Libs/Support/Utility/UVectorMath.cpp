
#include "UVectorMath.h"
#include "UMath.h"
#include "UEALibs.hpp"

// El modo `PS` (paired-single) es de SN Systems; el ASM es de EA (forense 2026-10-03:
// los stabs del original declaran los locales del asm, p.ej. t0..t6 en f8-f13 — su
// existencia en el fuente esta probada; el texto es reconstruccion). r77: modo `PS` GC.
#ifdef EA_PLATFORM_GAMECUBE
typedef float sn_ps __attribute__((mode(PS)));

#define V2(v) (*(sn_ps *)&(v))
#endif

// Decl: GC MW: UVectorMath.cpp: 312
// De aqui al `#endif` esta la implementacion de GameCube (paired-singles: modo
// `PS` y __builtin_ps_muls0). PS2 tiene la suya en UVectorMath.hpp, con
// ensamblador VU0, y compilar las dos da 25 redefiniciones. VU0_Atan2 y los dos
// estaticos del final SI son comunes (en PS2 tambien sale de este .cpp) y quedan
// fuera de la guarda.
#ifdef EA_PLATFORM_GAMECUBE
void VU0_v3crossprod(const UMath::Vector3 &a, const UMath::Vector3 &b, UMath::Vector3 &dest) {
    float t0, t1, t2, t3, t4, t5, t6;
    asm volatile (
        "psq_l %1, 0(%8), 0, 0\n\t"
        "lfs %2, 8(%7)\n\t"
        "psq_l %0, 0(%7), 0, 0\n\t"
        "ps_merge10 %6, %1, %1\n\t"
        "ps_mul %4, %1, %2\n\t"
        "lfs %3, 8(%8)\n\t"
        "ps_muls0 %1, %1, %0\n\t"
        "ps_msub %2, %0, %3, %4\n\t"
        "ps_msub %3, %0, %6, %1\n\t"
        "ps_merge11 %0, %2, %2\n\t"
        "ps_merge01 %1, %2, %3\n\t"
        "stfs %0, 0(%9)\n\t"
        "ps_neg %1, %1\n\t"
        "psq_st %1, 4(%9), 0, 0"
        : "=f"(t0), "=f"(t1), "=f"(t2), "=f"(t3), "=f"(t4), "=f"(t5), "=f"(t6)
        : "b"(&a), "b"(&b), "b"(&dest));
}

// Decl: GC MW: UVectorMath.cpp: 333
void VU0_v4crossprodxyz(const UMath::Vector4 &a, const UMath::Vector4 &b, UMath::Vector4 &dest) {
    float t0, t1, t2, t3, t4, t5, t6;
    asm volatile (
        "psq_l %1, 0(%8), 0, 0\n\t"
        "lfs %2, 8(%7)\n\t"
        "psq_l %0, 0(%7), 0, 0\n\t"
        "ps_merge10 %6, %1, %1\n\t"
        "ps_mul %4, %1, %2\n\t"
        "lfs %3, 8(%8)\n\t"
        "ps_muls0 %1, %1, %0\n\t"
        "ps_msub %2, %0, %3, %4\n\t"
        "ps_msub %3, %0, %6, %1\n\t"
        "ps_merge11 %0, %2, %2\n\t"
        "ps_merge01 %1, %2, %3\n\t"
        "stfs %0, 0(%9)\n\t"
        "ps_neg %1, %1\n\t"
        "psq_st %1, 4(%9), 0, 0"
        : "=f"(t0), "=f"(t1), "=f"(t2), "=f"(t3), "=f"(t4), "=f"(t5), "=f"(t6)
        : "b"(&a), "b"(&b), "b"(&dest));
}

// Decl: GC MW: UVectorMath.cpp: 390
float VU0_v3dotprod(const UMath::Vector3 &a, const UMath::Vector3 &b) {
    float t0, t1, t2, t3, result;
    asm (
        "psq_l %1, 0(%5), 0, 0\n\t"
        "psq_l %2, 0(%6), 0, 0\n\t"
        "ps_mul %1, %1, %2\n\t"
        "lfs %4, 8(%5)\n\t"
        "lfs %3, 8(%6)\n\t"
        "ps_sum0 %1, %1, %1, %1\n\t"
        "ps_madd %0, %4, %3, %1"
        : "=f"(result), "=f"(t0), "=f"(t1), "=f"(t2), "=f"(t3)
        : "b"(&a), "b"(&b));
    return result;
}

// Decl: GC MW: UVectorMath.cpp: 407
float VU0_v4dotprod(const UMath::Vector4 &a, const UMath::Vector4 &b) {
    float t0, t1, t2, t3, result;
    asm (
        "psq_l %1, 0(%5), 0, 0\n\t"
        "psq_l %2, 0(%6), 0, 0\n\t"
        "ps_mul %1, %1, %2\n\t"
        "psq_l %4, 8(%5), 0, 0\n\t"
        "psq_l %3, 8(%6), 0, 0\n\t"
        "ps_madd %2, %4, %3, %1\n\t"
        "ps_sum0 %0, %2, %2, %2"
        : "=f"(result), "=f"(t0), "=f"(t1), "=f"(t2), "=f"(t3)
        : "b"(&a), "b"(&b));
    return result;
}

// Decl: GC MW: UVectorMath.cpp: 428
float VU0_v4dotprodxyz(const UMath::Vector4 &a, const UMath::Vector4 &b) {
    float t0, t1, t2, t3, result;
    asm (
        "psq_l %1, 0(%5), 0, 0\n\t"
        "psq_l %2, 0(%6), 0, 0\n\t"
        "ps_mul %1, %1, %2\n\t"
        "lfs %4, 8(%5)\n\t"
        "lfs %3, 8(%6)\n\t"
        "ps_sum0 %1, %1, %1, %1\n\t"
        "ps_madd %0, %4, %3, %1"
        : "=f"(result), "=f"(t0), "=f"(t1), "=f"(t2), "=f"(t3)
        : "b"(&a), "b"(&b));
    return result;
}

// Decl: GC MW: UVectorMath.cpp: 453
void VU0_v3add(const UMath::Vector3 &a, const UMath::Vector3 &b, UMath::Vector3 &result) {
    float t0, t1, t2, t3, t4, t5;
    asm volatile (
        "psq_l %0, 0(%6), 0, 0\n\t"
        "psq_l %1, 0(%7), 0, 0\n\t"
        "lfs %3, 8(%6)\n\t"
        "ps_add %2, %0, %1\n\t"
        "lfs %4, 8(%7)\n\t"
        "psq_st %2, 0(%8), 0, 0\n\t"
        "ps_add %5, %3, %4\n\t"
        "psq_st %5, 8(%8), 1, 0"
        : "=f"(t0), "=f"(t1), "=f"(t2), "=f"(t3), "=f"(t4), "=f"(t5)
        : "b"(&a), "b"(&b), "b"(&result));
}

// Decl: GC MW: UVectorMath.cpp: 468
void VU0_v4add(const UMath::Vector4 &a, const UMath::Vector4 &b, UMath::Vector4 &result) {
    float t0, t1, t2, t3, t4, t5;
    asm volatile (
        "psq_l %0, 0(%6), 0, 0\n\t"
        "psq_l %1, 0(%7), 0, 0\n\t"
        "psq_l %3, 8(%6), 0, 0\n\t"
        "ps_add %2, %0, %1\n\t"
        "psq_l %4, 8(%7), 0, 0\n\t"
        "psq_st %2, 0(%8), 0, 0\n\t"
        "ps_add %5, %3, %4\n\t"
        "psq_st %5, 8(%8), 0, 0"
        : "=f"(t0), "=f"(t1), "=f"(t2), "=f"(t3), "=f"(t4), "=f"(t5)
        : "b"(&a), "b"(&b), "b"(&result));
}

// Decl: GC MW: UVectorMath.cpp: 487
void VU0_v4addxyz(const UMath::Vector4 &a, const UMath::Vector4 &b, UMath::Vector4 &result) {
    float t0, t1, t2, t3, t4, t5;
    asm volatile (
        "psq_l %0, 0(%6), 0, 0\n\t"
        "psq_l %1, 0(%7), 0, 0\n\t"
        "lfs %3, 8(%6)\n\t"
        "ps_add %2, %0, %1\n\t"
        "lfs %4, 8(%7)\n\t"
        "psq_st %2, 0(%8), 0, 0\n\t"
        "ps_add %5, %3, %4\n\t"
        "psq_st %5, 8(%8), 1, 0"
        : "=f"(t0), "=f"(t1), "=f"(t2), "=f"(t3), "=f"(t4), "=f"(t5)
        : "b"(&a), "b"(&b), "b"(&result));
}

// Decl: GC MW: UVectorMath.cpp: 510
void VU0_v3sub(const UMath::Vector3 &a, const UMath::Vector3 &b, UMath::Vector3 &result) {
    float t0, t1, t2, t3, t4, t5;
    asm volatile (
        "psq_l %0, 0(%6), 0, 0\n\t"
        "psq_l %1, 0(%7), 0, 0\n\t"
        "lfs %3, 8(%6)\n\t"
        "ps_sub %2, %0, %1\n\t"
        "lfs %4, 8(%7)\n\t"
        "psq_st %2, 0(%8), 0, 0\n\t"
        "ps_sub %5, %3, %4\n\t"
        "psq_st %5, 8(%8), 1, 0"
        : "=f"(t0), "=f"(t1), "=f"(t2), "=f"(t3), "=f"(t4), "=f"(t5)
        : "b"(&a), "b"(&b), "b"(&result));
}

// Decl: GC MW: UVectorMath.cpp: 525
void VU0_v4sub(const UMath::Vector4 &a, const UMath::Vector4 &b, UMath::Vector4 &result) {
    float t0, t1, t2, t3, t4, t5;
    asm volatile (
        "psq_l %0, 0(%6), 0, 0\n\t"
        "psq_l %1, 0(%7), 0, 0\n\t"
        "psq_l %3, 8(%6), 0, 0\n\t"
        "ps_sub %2, %0, %1\n\t"
        "psq_l %4, 8(%7), 0, 0\n\t"
        "psq_st %2, 0(%8), 0, 0\n\t"
        "ps_sub %5, %3, %4\n\t"
        "psq_st %5, 8(%8), 0, 0"
        : "=f"(t0), "=f"(t1), "=f"(t2), "=f"(t3), "=f"(t4), "=f"(t5)
        : "b"(&a), "b"(&b), "b"(&result));
}

// Decl: GC MW: UVectorMath.cpp: 544
void VU0_v4subxyz(const UMath::Vector4 &a, const UMath::Vector4 &b, UMath::Vector4 &result) {
    float t0, t1, t2, t3, t4, t5;
    asm volatile (
        "psq_l %0, 0(%6), 0, 0\n\t"
        "psq_l %1, 0(%7), 0, 0\n\t"
        "lfs %3, 8(%6)\n\t"
        "ps_sub %2, %0, %1\n\t"
        "lfs %4, 8(%7)\n\t"
        "psq_st %2, 0(%8), 0, 0\n\t"
        "ps_sub %5, %3, %4\n\t"
        "psq_st %5, 8(%8), 1, 0"
        : "=f"(t0), "=f"(t1), "=f"(t2), "=f"(t3), "=f"(t4), "=f"(t5)
        : "b"(&a), "b"(&b), "b"(&result));
}

// Decl: GC MW: UVectorMath.cpp: 567
void VU0_v3scale(const UMath::Vector3 &a, const float scaleby, UMath::Vector3 &result) {
    float t0, t1;
    asm volatile (
        "psq_l %0, 0(%2), 0, 0\n\t"
        "lfs %1, 8(%2)\n\t"
        "ps_muls0 %0, %0, %4\n\t"
        "ps_muls0 %1, %1, %4\n\t"
        "psq_st %0, 0(%3), 0, 0\n\t"
        "psq_st %1, 8(%3), 1, 0"
        : "=&f"(t0), "=&f"(t1)
        : "b"(&a), "b"(&result), "f"(scaleby));
}

// Decl: GC MW: UVectorMath.cpp: 580
void VU0_v3scale(const UMath::Vector3 &a, const UMath::Vector3 &b, UMath::Vector3 &result) {
    float t0, t1, t2, t3;
    asm volatile (
        "psq_l %0, 0(%4), 0, 0\n\t"
        "psq_l %1, 0(%5), 0, 0\n\t"
        "lfs %2, 8(%4)\n\t"
        "lfs %3, 8(%5)\n\t"
        "ps_mul %0, %0, %1\n\t"
        "ps_muls0 %2, %2, %3\n\t"
        "psq_st %0, 0(%6), 0, 0\n\t"
        "psq_st %2, 8(%6), 1, 0"
        : "=f"(t0), "=f"(t1), "=f"(t2), "=f"(t3)
        : "b"(&a), "b"(&b), "b"(&result));
}

// Decl: GC MW: UVectorMath.cpp: 599
void VU0_v4scale(const UMath::Vector4 &a, const float scaleby, UMath::Vector4 &result) {
    sn_ps zw = __builtin_ps_muls0(V2(a.z), (sn_ps)scaleby);
    sn_ps xy = __builtin_ps_muls0(V2(a.x), (sn_ps)scaleby);
    V2(result.x) = xy;
    V2(result.z) = zw;
}

// Decl: GC MW: UVectorMath.cpp: 615
void VU0_v4scalexyz(const UMath::Vector4 &a, const float scaleby, UMath::Vector4 &result) {
    float t0, t1;
    asm volatile (
        "psq_l %0, 0(%2), 0, 0\n\t"
        "lfs %1, 8(%2)\n\t"
        "ps_muls0 %0, %0, %4\n\t"
        "ps_muls0 %1, %1, %4\n\t"
        "psq_st %0, 0(%3), 0, 0\n\t"
        "psq_st %1, 8(%3), 1, 0"
        : "=&f"(t0), "=&f"(t1)
        : "b"(&a), "b"(&result), "f"(scaleby));
}

// Decl: GC MW: UVectorMath.cpp: 634
void VU0_v4scalexyz(const UMath::Vector4 &a, const UMath::Vector4 &b, UMath::Vector4 &result) {
    float t0, t1, t2, t3;
    asm volatile (
        "psq_l %0, 0(%4), 0, 0\n\t"
        "psq_l %1, 0(%5), 0, 0\n\t"
        "lfs %2, 8(%4)\n\t"
        "lfs %3, 8(%5)\n\t"
        "ps_mul %0, %0, %1\n\t"
        "ps_muls0 %2, %2, %3\n\t"
        "psq_st %0, 0(%6), 0, 0\n\t"
        "psq_st %2, 8(%6), 1, 0"
        : "=f"(t0), "=f"(t1), "=f"(t2), "=f"(t3)
        : "b"(&a), "b"(&b), "b"(&result));
}

// Decl: GC MW: UVectorMath.cpp: 676
void VU0_v3addscale(const UMath::Vector3 &a, const UMath::Vector3 &b, const float scaleby, UMath::Vector3 &result) {
    float t0, t1, t2, t3;
    asm volatile (
        "psq_l %0, 0(%4), 0, 0\n\t"
        "lfs %1, 8(%4)\n\t"
        "psq_l %2, 0(%5), 0, 0\n\t"
        "lfs %3, 8(%5)\n\t"
        "ps_add %0, %0, %2\n\t"
        "ps_add %1, %1, %3\n\t"
        "ps_muls0 %0, %0, %7\n\t"
        "ps_muls0 %1, %1, %7\n\t"
        "psq_st %0, 0(%6), 0, 0\n\t"
        "psq_st %1, 8(%6), 1, 0"
        : "=&f"(t0), "=&f"(t1), "=&f"(t2), "=&f"(t3)
        : "b"(&a), "b"(&b), "b"(&result), "f"(scaleby));
}

// Decl: GC MW: UVectorMath.cpp: 693
void VU0_v4addscale(const UMath::Vector4 &a, const UMath::Vector4 &b, const float scaleby, UMath::Vector4 &result) {
    float t0, t1, t2, t3;
    asm volatile (
        "psq_l %0, 0(%4), 0, 0\n\t"
        "psq_l %1, 8(%4), 0, 0\n\t"
        "psq_l %2, 0(%5), 0, 0\n\t"
        "psq_l %3, 8(%5), 0, 0\n\t"
        "ps_add %0, %0, %2\n\t"
        "ps_add %1, %1, %3\n\t"
        "ps_muls0 %0, %0, %7\n\t"
        "ps_muls0 %1, %1, %7\n\t"
        "psq_st %0, 0(%6), 0, 0\n\t"
        "psq_st %1, 8(%6), 0, 0"
        : "=&f"(t0), "=&f"(t1), "=&f"(t2), "=&f"(t3)
        : "b"(&a), "b"(&b), "b"(&result), "f"(scaleby));
}

// Decl: GC MW: UVectorMath.cpp: 715
void VU0_v4addscalexyz(const UMath::Vector4 &a, const UMath::Vector4 &b, const float scaleby, UMath::Vector4 &result) {
    float t0, t1, t2, t3;
    asm volatile (
        "psq_l %0, 0(%4), 0, 0\n\t"
        "lfs %1, 8(%4)\n\t"
        "psq_l %2, 0(%5), 0, 0\n\t"
        "lfs %3, 8(%5)\n\t"
        "ps_add %0, %0, %2\n\t"
        "ps_add %1, %1, %3\n\t"
        "ps_muls0 %0, %0, %7\n\t"
        "ps_muls0 %1, %1, %7\n\t"
        "psq_st %0, 0(%6), 0, 0\n\t"
        "psq_st %1, 8(%6), 1, 0"
        : "=&f"(t0), "=&f"(t1), "=&f"(t2), "=&f"(t3)
        : "b"(&a), "b"(&b), "b"(&result), "f"(scaleby));
}

// Decl: GC MW: UVectorMath.cpp: 742
void VU0_v3scaleadd(const UMath::Vector3 &a, const float scaleby, const UMath::Vector3 &b, UMath::Vector3 &result) {
    float t0, t1, t2, t3;
    asm volatile (
        "psq_l %0, 0(%4), 0, 0\n\t"
        "lfs %1, 8(%4)\n\t"
        "psq_l %2, 0(%5), 0, 0\n\t"
        "ps_muls0 %0, %0, %7\n\t"
        "ps_muls0 %1, %1, %7\n\t"
        "lfs %3, 8(%5)\n\t"
        "ps_add %0, %0, %2\n\t"
        "ps_add %1, %1, %3\n\t"
        "psq_st %0, 0(%6), 0, 0\n\t"
        "psq_st %1, 8(%6), 1, 0"
        : "=&f"(t0), "=&f"(t1), "=&f"(t2), "=&f"(t3)
        : "b"(&a), "b"(&b), "b"(&result), "f"(scaleby));
}

// Decl: GC MW: UVectorMath.cpp: 759
void VU0_v4scaleaddxyz(const UMath::Vector4 &a, const float scaleby, const UMath::Vector4 &b, UMath::Vector4 &result) {
    float t0, t1, t2, t3;
    asm volatile (
        "psq_l %0, 0(%4), 0, 0\n\t"
        "lfs %1, 8(%4)\n\t"
        "psq_l %2, 0(%5), 0, 0\n\t"
        "ps_muls0 %0, %0, %7\n\t"
        "ps_muls0 %1, %1, %7\n\t"
        "lfs %3, 8(%5)\n\t"
        "ps_add %0, %0, %2\n\t"
        "ps_add %1, %1, %3\n\t"
        "psq_st %0, 0(%6), 0, 0\n\t"
        "psq_st %1, 8(%6), 1, 0"
        : "=&f"(t0), "=&f"(t1), "=&f"(t2), "=&f"(t3)
        : "b"(&a), "b"(&b), "b"(&result), "f"(scaleby));
}

// Decl: GC MW: UVectorMath.cpp: 781
void VU0_v4scaleadd(const UMath::Vector4 &a, const float scaleby, const UMath::Vector4 &b, UMath::Vector4 &result) {
    float t0, t1, t2, t3;
    asm volatile (
        "psq_l %0, 0(%4), 0, 0\n\t"
        "psq_l %1, 8(%4), 0, 0\n\t"
        "psq_l %2, 0(%5), 0, 0\n\t"
        "ps_muls0 %0, %0, %7\n\t"
        "ps_muls0 %1, %1, %7\n\t"
        "psq_l %3, 8(%5), 0, 0\n\t"
        "ps_add %0, %0, %2\n\t"
        "ps_add %1, %1, %3\n\t"
        "psq_st %0, 0(%6), 0, 0\n\t"
        "psq_st %1, 8(%6), 0, 0"
        : "=&f"(t0), "=&f"(t1), "=&f"(t2), "=&f"(t3)
        : "b"(&a), "b"(&b), "b"(&result), "f"(scaleby));
}

// Decl: GC MW: UVectorMath.cpp: 807
float VU0_v3distancesquare(const UMath::Vector3 &p1, const UMath::Vector3 &p2) {
    float result, t1, t2, t3, t4, t5, t6, t7;
    asm (
        "psq_l %1, 0(%8), 0, 0\n\t"
        "psq_l %2, 0(%9), 0, 0\n\t"
        "lfs %3, 8(%8)\n\t"
        "lfs %4, 8(%9)\n\t"
        "ps_sub %5, %1, %2\n\t"
        "ps_sub %6, %3, %4\n\t"
        "ps_mul %5, %5, %5\n\t"
        "ps_sum0 %5, %5, %5, %5\n\t"
        "ps_madd %0, %6, %6, %5"
        : "=f"(result), "=f"(t1), "=f"(t2), "=f"(t3), "=f"(t4), "=f"(t5), "=f"(t6), "=f"(t7)
        : "b"(&p1), "b"(&p2));
    return result;
}

// Decl: GC MW: UVectorMath.cpp: 851
float VU0_v4distancesquarexyz(const UMath::Vector4 &p1, const UMath::Vector4 &p2) {
    float result, t1, t2, t3, t4, t5, t6, t7;
    asm (
        "psq_l %1, 0(%8), 0, 0\n\t"
        "psq_l %2, 0(%9), 0, 0\n\t"
        "lfs %3, 8(%8)\n\t"
        "lfs %4, 8(%9)\n\t"
        "ps_sub %5, %1, %2\n\t"
        "ps_sub %6, %3, %4\n\t"
        "ps_mul %5, %5, %5\n\t"
        "ps_sum0 %5, %5, %5, %5\n\t"
        "ps_madd %0, %6, %6, %5"
        : "=f"(result), "=f"(t1), "=f"(t2), "=f"(t3), "=f"(t4), "=f"(t5), "=f"(t6), "=f"(t7)
        : "b"(&p1), "b"(&p2));
    return result;
}

// Decl: GC MW: UVectorMath.cpp: 893
float VU0_v3distancesquarexz(const UMath::Vector3 &p1, const UMath::Vector3 &p2) {
    float result, t1, t2, t3, t4, t5, t6, t7;
    asm (
        "psq_l %1, 0(%8), 0, 0\n\t"
        "psq_l %2, 0(%9), 0, 0\n\t"
        "lfs %3, 8(%8)\n\t"
        "lfs %4, 8(%9)\n\t"
        "ps_sub %5, %1, %2\n\t"
        "ps_sub %6, %3, %4\n\t"
        "ps_mul %5, %5, %5\n\t"
        "ps_madd %0, %6, %6, %5"
        : "=f"(result), "=f"(t1), "=f"(t2), "=f"(t3), "=f"(t4), "=f"(t5), "=f"(t6), "=f"(t7)
        : "b"(&p1), "b"(&p2));
    return result;
}

// Decl: GC MW: UVectorMath.cpp: 925
float VU0_v3lengthsquare(const UMath::Vector3 &a) {
    float result, t1, t2, t3, t4;
    asm (
        "psq_l %1, 0(%5), 0, 0\n\t"
        "ps_mul %3, %1, %1\n\t"
        "lfs %2, 8(%5)\n\t"
        "ps_sum0 %3, %3, %3, %3\n\t"
        "ps_madd %0, %2, %2, %3"
        : "=f"(result), "=f"(t1), "=f"(t2), "=f"(t3), "=f"(t4)
        : "b"(&a));
    return result;
}

// Decl: GC MW: UVectorMath.cpp: 948
float VU0_v4lengthsquare(const UMath::Vector4 &a) {
    float result, t1, t2, t3, t4;
    asm (
        "psq_l %1, 0(%5), 0, 0\n\t"
        "ps_mul %3, %1, %1\n\t"
        "psq_l %2, 8(%5), 0, 0\n\t"
        "ps_madd %3, %2, %2, %3\n\t"
        "ps_sum0 %0, %3, %3, %3"
        : "=f"(result), "=f"(t1), "=f"(t2), "=f"(t3), "=f"(t4)
        : "b"(&a));
    return result;
}

// Decl: GC MW: UVectorMath.cpp: 966
float VU0_v4lengthsquarexyz(const UMath::Vector4 &a) {
    float result, t1, t2, t3, t4;
    asm (
        "psq_l %1, 0(%5), 0, 0\n\t"
        "ps_mul %3, %1, %1\n\t"
        "lfs %2, 8(%5)\n\t"
        "ps_sum0 %3, %3, %3, %3\n\t"
        "ps_madd %0, %2, %2, %3"
        : "=f"(result), "=f"(t1), "=f"(t2), "=f"(t3), "=f"(t4)
        : "b"(&a));
    return result;
}

// Decl: GC MW: UVectorMath.cpp: 948
float VU0_sqrt(const float a) {
    // La forma sale del DWARF del original (mw_dwarfdump.nothpp): el parametro
    // `a` vive en f12 y la funcion tiene UN bloque anonimo cuya unica local es
    // `guess`, en f1, que es el destino del frsqrte. El `volatile` del asm es lo
    // que mantiene el frsqrte delante de los dos `lis` del pool: sin el, el
    // planificador sube uno de los `lis` por encima y el orden r9/r11 se cambia.
    if (a != 0.0f) {
        float guess;
        asm volatile ("frsqrte %0, %1" : "=f"(guess) : "f"(a));
        return (a * guess + 1.0f / guess) * 0.5f;
    }
    return a;
}

#endif // EA_PLATFORM_GAMECUBE

#if defined(EA_PLATFORM_GAMECUBE) || defined(EA_PLATFORM_XENON)
// En C en GameCube y Xbox 360; en PS2 es asm de la VU0 (UVectorMath.hpp) y en PC va en linea
// (UVectorMathCPU.hpp).
// Decl: GC MW: UVectorMath.cpp: 970
float VU0_rsqrt(const float a) {
    float temp = VU0_sqrt(a);
    float zero = 0.0f;
    if (temp == zero)
        return zero;
    return 1.0f / temp;
}
#endif

#ifdef EA_PLATFORM_GAMECUBE
// Decl: GC MW: UVectorMath.cpp: 987
void VU0_MATRIX4_vect3mult(const UMath::Vector3 &v, const UMath::Matrix4 &m, UMath::Vector3 &result) {
    float t0, t1, t2, t3, t4, t5, t6;
    asm volatile (
        "psq_l %6, 0(%7), 0, 0\n\t"
        "psq_l %2, 0(%8), 0, 0\n\t"
        "psq_l %3, 8(%8), 0, 0\n\t"
        "psq_l %4, 16(%8), 0, 0\n\t"
        "ps_muls0 %0, %2, %6\n\t"
        "psq_l %5, 24(%8), 0, 0\n\t"
        "ps_muls0 %1, %3, %6\n\t"
        "psq_l %3, 8(%7), 1, 0\n\t"
        "ps_madds1 %0, %4, %6, %0\n\t"
        "psq_l %2, 32(%8), 0, 0\n\t"
        "ps_madds1 %1, %5, %6, %1\n\t"
        "psq_l %6, 40(%8), 0, 0\n\t"
        "ps_madds0 %0, %2, %3, %0\n\t"
        "ps_madds0 %1, %6, %3, %1\n\t"
        "psq_l %2, 48(%8), 0, 0\n\t"
        "psq_l %3, 56(%8), 0, 0\n\t"
        "ps_add %0, %0, %2\n\t"
        "ps_add %1, %1, %3\n\t"
        "psq_st %0, 0(%9), 0, 0\n\t"
        "psq_st %1, 8(%9), 1, 0"
        : "=f"(t0), "=f"(t1), "=f"(t2), "=f"(t3), "=f"(t4), "=f"(t5), "=f"(t6)
        : "b"(&v), "b"(&m), "b"(&result));
}

// Decl: GC MW: UVectorMath.cpp: 1029
void VU0_MATRIX4_vect4mult(const UMath::Vector4 &v, const UMath::Matrix4 &m, UMath::Vector4 &result) {
    float t0, t1, t2, t3, t4, t5, t6;
    asm volatile (
        "psq_l %6, 0(%7), 0, 0\n\t"
        "psq_l %2, 0(%8), 0, 0\n\t"
        "psq_l %3, 8(%8), 0, 0\n\t"
        "psq_l %4, 16(%8), 0, 0\n\t"
        "ps_muls0 %0, %2, %6\n\t"
        "psq_l %5, 24(%8), 0, 0\n\t"
        "ps_muls0 %1, %3, %6\n\t"
        "psq_l %3, 8(%7), 0, 0\n\t"
        "ps_madds1 %0, %4, %6, %0\n\t"
        "psq_l %2, 32(%8), 0, 0\n\t"
        "ps_madds1 %1, %5, %6, %1\n\t"
        "psq_l %6, 40(%8), 0, 0\n\t"
        "ps_madds0 %0, %2, %3, %0\n\t"
        "ps_madds0 %1, %6, %3, %1\n\t"
        "psq_l %4, 48(%8), 0, 0\n\t"
        "psq_l %5, 56(%8), 0, 0\n\t"
        "ps_madds1 %0, %4, %3, %0\n\t"
        "ps_madds1 %1, %5, %3, %1\n\t"
        "psq_st %0, 0(%9), 0, 0\n\t"
        "psq_st %1, 8(%9), 0, 0"
        : "=f"(t0), "=f"(t1), "=f"(t2), "=f"(t3), "=f"(t4), "=f"(t5), "=f"(t6)
        : "b"(&v), "b"(&m), "b"(&result));
}

// Decl: GC MW: UVectorMath.cpp: 1060
void VU0_MATRIX4_vect4mult(const UMath::Vector4 *v, const UMath::Matrix4 &m, UMath::Vector4 *result, int count) {
    int i;
    for (i = 0; i < count; i++) {
        VU0_MATRIX4_vect4mult(*v, m, *result);
        v++;
        result++;
    }
}

// Decl: GC MW: UVectorMath.cpp: 1070
void VU0_MATRIX3x4_vect3mult(const UMath::Vector3 &v, const UMath::Matrix4 &m, UMath::Vector3 &result) {
    float t0, t1, t2, t3, t4, t5, t6;
    asm volatile (
        "psq_l %6, 0(%7), 0, 0\n\t"
        "psq_l %2, 0(%8), 0, 0\n\t"
        "psq_l %3, 8(%8), 0, 0\n\t"
        "psq_l %4, 16(%8), 0, 0\n\t"
        "ps_muls0 %0, %2, %6\n\t"
        "psq_l %5, 24(%8), 0, 0\n\t"
        "ps_muls0 %1, %3, %6\n\t"
        "psq_l %3, 8(%7), 1, 0\n\t"
        "ps_madds1 %0, %4, %6, %0\n\t"
        "psq_l %2, 32(%8), 0, 0\n\t"
        "ps_madds1 %1, %5, %6, %1\n\t"
        "psq_l %6, 40(%8), 0, 0\n\t"
        "ps_madds0 %0, %2, %3, %0\n\t"
        "ps_madds0 %1, %6, %3, %1\n\t"
        "psq_st %0, 0(%9), 0, 0\n\t"
        "psq_st %1, 8(%9), 1, 0"
        : "=f"(t0), "=f"(t1), "=f"(t2), "=f"(t3), "=f"(t4), "=f"(t5), "=f"(t6)
        : "b"(&v), "b"(&m), "b"(&result));
}

// Decl: GC MW: UVectorMath.cpp: 1097
void VU0_MATRIX3x4_vect4mult(const UMath::Vector4 &v, const UMath::Matrix4 &m, UMath::Vector4 &result) {
    float t0, t1, t2, t3, t4, t5, t6;
    asm volatile (
        "psq_l %6, 0(%7), 0, 0\n\t"
        "psq_l %2, 0(%8), 0, 0\n\t"
        "psq_l %3, 8(%8), 0, 0\n\t"
        "psq_l %4, 16(%8), 0, 0\n\t"
        "ps_muls0 %0, %2, %6\n\t"
        "psq_l %5, 24(%8), 0, 0\n\t"
        "ps_muls0 %1, %3, %6\n\t"
        "psq_l %3, 8(%7), 1, 0\n\t"
        "ps_madds1 %0, %4, %6, %0\n\t"
        "psq_l %2, 32(%8), 0, 0\n\t"
        "ps_madds1 %1, %5, %6, %1\n\t"
        "psq_l %6, 40(%8), 0, 0\n\t"
        "ps_madds0 %0, %2, %3, %0\n\t"
        "ps_madds0 %1, %6, %3, %1\n\t"
        "psq_st %0, 0(%9), 0, 0\n\t"
        "psq_st %1, 8(%9), 1, 0"
        : "=f"(t0), "=f"(t1), "=f"(t2), "=f"(t3), "=f"(t4), "=f"(t5), "=f"(t6)
        : "b"(&v), "b"(&m), "b"(&result));
}

#endif // EA_PLATFORM_GAMECUBE

#ifndef EA_PLATFORM_PLAYSTATION2
// Las dos son C sobre las demas VU0_*: valen tal cual fuera de PS2.
// Decl: GC MW: UVectorMath.cpp: 3078
void VU0_v3quatrotate(const UMath::Vector4 &q, const UMath::Vector3 &v, UMath::Vector3 &result) {
    UMath::Vector3 temp0, temp1, temp2;
    VU0_v3crossprod((const UMath::Vector3 &)q, v, temp1);
    VU0_v3scaleadd(v, q.w, temp1, temp1);
    VU0_v3crossprod(temp1, (const UMath::Vector3 &)q, temp2);
    VU0_v3scale((const UMath::Vector3 &)q, VU0_v3dotprod((const UMath::Vector3 &)q, v), temp0);
    VU0_v3scaleadd(temp1, q.w, temp0, temp0);
    VU0_v3sub(temp0, temp2, result);
}

#ifdef EA_PLATFORM_XENON
// Solo en Xbox 360 (NFS.exe 0x82342550, entre las dos de Vector3); en GameCube y PS2 el enlazador la quita.
void VU0_v4quatrotate(const UMath::Vector4 &q, const UMath::Vector4 &v, UMath::Vector4 &result) {
    UMath::Vector4 temp0, temp1, temp2;
    VU0_v4crossprodxyz(q, v, temp1);
    VU0_v4scaleaddxyz(v, q.w, temp1, temp1);
    VU0_v4crossprodxyz(temp1, q, temp2);
    VU0_v4scalexyz(q, VU0_v4dotprodxyz(q, v), temp0);
    VU0_v4scaleaddxyz(temp1, q.w, temp0, temp0);
    VU0_v4subxyz(temp0, temp2, result);
}
#endif

// Decl: GC MW: UVectorMath.cpp: 3112
void VU0_v3quatrotate_xlate(const UMath::Vector4 &q, const UMath::Vector3 &v, const UMath::Vector3 &trans, UMath::Vector3 &result) {
    UMath::Vector3 temp0, temp1, temp2;
    VU0_v3crossprod((const UMath::Vector3 &)q, v, temp1);
    VU0_v3scaleadd(v, q.w, temp1, temp1);
    VU0_v3crossprod(temp1, (const UMath::Vector3 &)q, temp2);
    VU0_v3scale((const UMath::Vector3 &)q, VU0_v3dotprod((const UMath::Vector3 &)q, v), temp0);
    VU0_v3scaleadd(temp1, q.w, temp0, temp0);
    VU0_v3sub(temp0, temp2, result);
    VU0_v3add(result, trans, result);
}

#ifdef EA_PLATFORM_XENON
// Solo en Xbox 360 (NFS.exe 0x82342778).
void VU0_v4quatrotate_xlate(const UMath::Vector4 &q, const UMath::Vector4 &v, const UMath::Vector4 &trans, UMath::Vector4 &result) {
    UMath::Vector4 temp0, temp1, temp2;
    VU0_v4crossprodxyz(q, v, temp1);
    VU0_v4scaleaddxyz(v, q.w, temp1, temp1);
    VU0_v4crossprodxyz(temp1, q, temp2);
    VU0_v4scalexyz(q, VU0_v4dotprodxyz(q, v), temp0);
    VU0_v4scaleaddxyz(temp1, q.w, temp0, temp0);
    VU0_v4subxyz(temp0, temp2, result);
    VU0_v4addxyz(result, trans, result);
}
#endif

#endif // EA_PLATFORM_PLAYSTATION2

#ifdef EA_PLATFORM_XENON
// Solo en Xbox 360 (NFS.exe 0x82342948, delante de la de y); en GameCube y PS2 no llega al ejecutable.
void VU0_MATRIX4setxrot(UMath::Matrix4 &dest, const float xangle) {
    float xcos = VU0_Cos(xangle);
    float xsin = VU0_Sin(xangle);
    for (int i = 0; i < 16; i++) {
        *((int *)&dest.GetElements()[i]) = 0;
    }
    dest.GetElements()[5] = dest.GetElements()[10] = xcos;
    dest.GetElements()[6] = xsin;
    dest.GetElements()[9] = -xsin;
    *((int *)&dest.GetElements()[0]) = PS2_FLOAT_ONE_AS_INT;
    *((int *)&dest.GetElements()[15]) = PS2_FLOAT_ONE_AS_INT;
}
#endif

// En todas las plataformas (la A124 la tiene en las lineas 4043-4057 de su UVectorMath.cpp; la de GameCube, en la
// 4028): con VU0_Cos y VU0_Sin en vueltas no depende de ninguna.
// Decl: GC MW: UVectorMath.cpp: 4028
void VU0_MATRIX4setyrot(UMath::Matrix4 &dest, const float yangle) {
    float ycos = VU0_Cos(yangle);
    float ysin = VU0_Sin(yangle);
    for (int i = 0; i < 16; i++) {
        *((int *)&dest.GetElements()[i]) = 0;
    }
    dest.GetElements()[10] = dest.GetElements()[0] = ycos;
    dest.GetElements()[2] = -ysin;
    dest.GetElements()[8] = ysin;
    *((int *)&dest.GetElements()[5]) = PS2_FLOAT_ONE_AS_INT;
    *((int *)&dest.GetElements()[15]) = PS2_FLOAT_ONE_AS_INT;
}

#ifndef EA_PLATFORM_GAMECUBE
// Lineas 4072-4088 del original de la A124, justo detras de la de y. El DWARF de GameCube tambien la tiene, pero sin
// direccion (su enlazador la quita porque nadie la llama); alli no se compila para no mover nada del DOL.
void VU0_MATRIX4setzrot(UMath::Matrix4 &dest, const float zangle) {
    float zcos = VU0_Cos(zangle);
    float zsin = VU0_Sin(zangle);
    for (int i = 0; i < 16; i++) {
        *((int *)&dest.GetElements()[i]) = 0;
    }
    dest.GetElements()[0] = dest.GetElements()[5] = zcos;
    dest.GetElements()[1] = zsin;
    dest.GetElements()[4] = -zsin;
    *((int *)&dest.GetElements()[10]) = PS2_FLOAT_ONE_AS_INT;
    *((int *)&dest.GetElements()[15]) = PS2_FLOAT_ONE_AS_INT;
}
#endif

#ifndef EA_PLATFORM_PLAYSTATION2
// Decl: GC MW: UVectorMath.cpp: 4088
void VU0_m4toquat(const UMath::Matrix4 &mat, UMath::Vector4 &result) {
    // La forma sale del DWARF del original (mw_dwarfdump.nothpp), que en esta
    // funcion solo declara DOS locales: `float s` en f11 y `int i` en r9. No hay
    // ni `t`, ni `tr`, ni `d`, ni `m00/m11/m22`: son subexpresiones que CSE
    // comparte. Y la clave del reparto es que `s` es UNA SOLA variable que
    // primero guarda la traza y luego se reutiliza para la raiz -- por eso f11
    // lleva los dos valores en el objetivo.
    const float *e = mat.GetElements();
    float s = e[0] + e[5] + e[10];
    int i;
    if (s > 0.0f) {
        s = VU0_sqrt(s + 1.0f);
        result.w = s * 0.5f;
        s = 0.5f / s;
        result.x = (e[6] - e[9]) * s;
        result.y = (e[8] - e[2]) * s;
        result.z = (e[1] - e[4]) * s;
    } else {
        if (e[5] > e[0])
            i = 5;
        else
            i = 0;
        if (e[10] > e[i]) {
            s = VU0_sqrt((e[10] - (e[0] + e[5])) + 1.0f);
            result.z = s * 0.5f;
            if (s != 0.0f)
                s = 0.5f / s;
            result.w = (e[1] - e[4]) * s;
            result.x = (e[8] + e[2]) * s;
            result.y = (e[9] + e[6]) * s;
        } else if (i != 0) {
            s = VU0_sqrt((e[5] - (e[10] + e[0])) + 1.0f);
            result.y = s * 0.5f;
            if (s != 0.0f)
                s = 0.5f / s;
            result.w = (e[8] - e[2]) * s;
            result.z = (e[6] + e[9]) * s;
            result.x = (e[4] + e[1]) * s;
        } else {
            s = VU0_sqrt((e[0] - (e[5] + e[10])) + 1.0f);
            result.x = s * 0.5f;
            if (s != 0.0f)
                s = 0.5f / s;
            result.w = (e[6] - e[9]) * s;
            result.y = (e[1] + e[4]) * s;
            result.z = (e[2] + e[8]) * s;
        }
    }
}

// Decl: GC MW: UVectorMath.cpp: 4896
void VU0_Matrix4ToEuler(const UMath::Matrix4 &m, UMath::Vector3 &e) {
    float sy = m[0][2];
    if (sy < -1.0f)
        sy = -1.0f;
    if (sy > 1.0f)
        sy = 1.0f;
    float cy = sqrtf(1.0f - sy * sy);
    if ((m[0][0] < 0.0f) && (m[2][2] < 0.0f))
        cy = -cy;
    float z = 0.0f;
    float abs_cy = cy;
    if (cy < z)
        abs_cy = -cy;
    if (abs_cy > 0.001f) {
        e.x = UMath::Atan2a(m[1][2] / cy, m[2][2] / cy);
        e.y = -UMath::Atan2a(sy, cy);
        e.z = UMath::Atan2a(m[0][1] / cy, m[0][0] / cy);
    } else {
        e.x = UMath::Atan2a(-m[2][1], m[1][1]);
        e.y = -rasin(sy);
        e.z = z;
    }
}

#endif // EA_PLATFORM_PLAYSTATION2

#ifdef EA_PLATFORM_PLAYSTATION2
// Lineas 3388-3452 del original. En PS2 no van en linea: los llamadores hacen `jal VU0_Sin__Ff'. El angulo va en
// vueltas (1.0f es la vuelta entera), como en las demas plataformas: se lleva a [-0.5, 0.5], se pasa a radianes y el
// seno sale de un polinomio impar de grado 9 en la VU0 (x + c3 x^3 + c5 x^5 + c7 x^7 + c9 x^9), con los coeficientes
// (-0.16580, 0.0082720, -0.00019711, 0.0000025872) cargados con lui/addiu entre las instrucciones de la VU0 y los
// registros vf escritos, como el resto de la familia VU0_* de UMath. Los stabs dan las locales: rangle, en el
// registro del parametro, y _t0, _t1 y _t2, de 128 bits, para los coeficientes.
float VU0_Sin(float angle) {
    float rangle = angle;

    if (rangle > 1.0f) {
        rangle -= VU0_FloatToInt(rangle);
    } else if (rangle < 0.0f) {
        rangle -= VU0_FloatToInt(rangle) - 1;
    }

    if (rangle < -0.5f) {
        rangle += 1.0f;
    } else if (rangle > 0.5f) {
        rangle -= 1.0f;
    }

    rangle *= UMath::TWOPI;
    u_long128 _t0;
    u_long128 _t1;
    u_long128 _t2;
    float result;

    asm __volatile__("mfc1 $2, %1\n"
                     "lui %2, 0xbe2a\n"
                     "qmtc2.ni $2, vf20\n"
                     "vmulx.x vf3, vf20, vf20x\n"
                     "lui %3, 0x3c08\n"
                     "addiu %2, %2, 0xaaa4\n"
                     "addiu %3, %3, 0x873e\n"
                     "vmulx.x vf1, vf3, vf20x\n"
                     "qmtc2.ni %2, vf11\n"
                     "lui %4, 0xb94f\n"
                     "qmtc2.ni %3, vf12\n"
                     "vmulx.x vf2, vf1, vf3x\n"
                     "addiu %4, %4, 0xb21f\n"
                     "vmulaw.x ACC, vf20, vf0w\n"
                     "lui %2, 0x362e\n"
                     "vmaddax.x ACC, vf1, vf11x\n"
                     "addiu %2, %2, 0x9c14\n"
                     "vmulx.x vf1, vf2, vf3x\n"
                     "qmtc2.ni %4, vf13\n"
                     "vmaddax.x ACC, vf2, vf12x\n"
                     "qmtc2.ni %2, vf14\n"
                     "vmulx.x vf5, vf1, vf3x\n"
                     "vmadda.x ACC, vf1, vf13\n"
                     "vmadd.x vf20, vf5, vf14\n"
                     "qmfc2.ni $2, vf20\n"
                     "mtc1 $2, %0"
                     : "=f"(result)
                     : "f"(rangle), "r"(_t0), "r"(_t1), "r"(_t2)
                     : "$2");
    return result;
}

// Lineas 3455-3526 del original: el mismo polinomio, un cuarto de vuelta mas alla.
float VU0_Cos(float angle) {
    float rangle = angle;

    if (rangle > 1.0f) {
        rangle -= VU0_FloatToInt(rangle);
    } else if (rangle < 0.0f) {
        rangle -= VU0_FloatToInt(rangle) - 1;
    }

    rangle += 0.25f;

    if (rangle < -0.5f) {
        rangle += 1.0f;
    } else if (rangle > 0.5f) {
        rangle -= 1.0f;
    }

    rangle *= UMath::TWOPI;
    u_long128 _t0;
    u_long128 _t1;
    u_long128 _t2;
    float result;

    asm __volatile__("mfc1 $2, %1\n"
                     "lui %2, 0xbe2a\n"
                     "qmtc2.ni $2, vf20\n"
                     "vmulx.x vf3, vf20, vf20x\n"
                     "lui %3, 0x3c08\n"
                     "addiu %2, %2, 0xaaa4\n"
                     "addiu %3, %3, 0x873e\n"
                     "vmulx.x vf1, vf3, vf20x\n"
                     "qmtc2.ni %2, vf11\n"
                     "lui %4, 0xb94f\n"
                     "qmtc2.ni %3, vf12\n"
                     "vmulx.x vf2, vf1, vf3x\n"
                     "addiu %4, %4, 0xb21f\n"
                     "vmulaw.x ACC, vf20, vf0w\n"
                     "lui %2, 0x362e\n"
                     "vmaddax.x ACC, vf1, vf11x\n"
                     "addiu %2, %2, 0x9c14\n"
                     "vmulx.x vf1, vf2, vf3x\n"
                     "qmtc2.ni %4, vf13\n"
                     "vmaddax.x ACC, vf2, vf12x\n"
                     "qmtc2.ni %2, vf14\n"
                     "vmulx.x vf5, vf1, vf3x\n"
                     "vmadda.x ACC, vf1, vf13\n"
                     "vmadd.x vf20, vf5, vf14\n"
                     "qmfc2.ni $2, vf20\n"
                     "mtc1 $2, %0"
                     : "=f"(result)
                     : "f"(rangle), "r"(_t0), "r"(_t1), "r"(_t2)
                     : "$2");
    return result;
}

// Linea 3595 del original (el cuerpo es un solo asm; la llave de cierre, 3599). Interpolacion lineal de los dos
// cuaterniones con signo: a*t mas o menos b*(1-t), el signo que deje la norma mayor (compara |a-b|^2 con |a+b|^2),
// y se normaliza con vrsqrt. Los stabs solo dan la local `_t0', de 128 bits en un registro entero (el 1.0f).
void VU0_fastqslerp(const RQUAT &b, const RQUAT &a, RQUAT &result, const float t) {
    u_long128 _t0;
    asm __volatile__("lui %4, 0x3f80\n"
                     "lqc2 vf1, %1\n"
                     "lqc2 vf2, %2\n"
                     "vsub.xyzw vf4, vf1, vf2\n"
                     "qmtc2.ni %3, vf3\n"
                     "vadd.xyzw vf5, vf1, vf2\n"
                     "qmtc2.ni %4, vf6\n"
                     "vsubx.w vf3, vf0, vf3x\n"
                     "vmulx.xyzw vf1, vf1, vf3x\n"
                     "vmulw.xyzw vf2, vf2, vf3w\n"
                     "vmul.xyzw vf7, vf4, vf4\n"
                     "vmulax.x ACC, vf4, vf4x\n"
                     "vmadday.x ACC, vf6, vf7y\n"
                     "vmaddaz.x ACC, vf6, vf7z\n"
                     "vmaddw.x vf7, vf6, vf7w\n"
                     "vmul.xyzw vf8, vf5, vf5\n"
                     "vmulax.x ACC, vf5, vf5x\n"
                     "vmadday.x ACC, vf6, vf8y\n"
                     "vmaddaz.x ACC, vf6, vf8z\n"
                     "vmaddw.x vf8, vf6, vf8w\n"
                     "vsubx.x vf7, vf7, vf8x\n"
                     "qmfc2.ni %4, vf7\n"
                     "dsll32 %4, %4, 0\n"
                     ".set noreorder\n"
                     "nop\n"
                     "blez %4, 1f\n"
                     "nop\n"
                     "b 2f\n"
                     "vsub.xyzw vf1, vf1, vf2\n"
                     "nop\n"
                     "1:\n"
                     "vadd.xyzw vf1, vf1, vf2\n"
                     "nop\n"
                     "2:\n"
                     ".set reorder\n"
                     "vmul.xyzw vf2, vf1, vf1\n"
                     "vmulax.x ACC, vf1, vf1x\n"
                     "vmadday.x ACC, vf6, vf2y\n"
                     "vmaddaz.x ACC, vf6, vf2z\n"
                     "vmaddw.x vf2, vf6, vf2w\n"
                     "vrsqrt Q, vf0w, vf2x\n"
                     "vwaitq\n"
                     "vmulq.xyzw vf1, vf1, Q\n"
                     "sqc2 vf1, %0"
                     : "=o"(result)
                     : "o"(a), "o"(b), "r"(t), "r"(_t0));
}
#endif

#if !defined(EA_PLATFORM_GAMECUBE) && !defined(EA_PLATFORM_PLAYSTATION2)
// Las plataformas sin VU0 ni paired singles (PC, Xbox 360): las VU0_* que UVectorMathCPU.hpp no trae en linea, en C.
// Hacen lo mismo que las de PS2 (UVectorMath.hpp), mascaras de campo incluidas: las `xyz` dejan la w del destino como
// estaba y las demas escriben las cuatro. Todas admiten que el destino sea una de las entradas.
void VU0_v4add(const UMath::Vector4 &a, const UMath::Vector4 &b, UMath::Vector4 &result) {
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    result.z = a.z + b.z;
    result.w = a.w + b.w;
}

void VU0_v4addxyz(const UMath::Vector4 &a, const UMath::Vector4 &b, UMath::Vector4 &result) {
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    result.z = a.z + b.z;
}

void VU0_v4sub(const UMath::Vector4 &a, const UMath::Vector4 &b, UMath::Vector4 &result) {
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    result.z = a.z - b.z;
    result.w = a.w - b.w;
}

void VU0_v4subxyz(const UMath::Vector4 &a, const UMath::Vector4 &b, UMath::Vector4 &result) {
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    result.z = a.z - b.z;
}

void VU0_v4scale(const UMath::Vector4 &a, const float scaleby, UMath::Vector4 &result) {
    result.x = a.x * scaleby;
    result.y = a.y * scaleby;
    result.z = a.z * scaleby;
    result.w = a.w * scaleby;
}

void VU0_v4scalexyz(const UMath::Vector4 &a, const float scaleby, UMath::Vector4 &result) {
    result.x = a.x * scaleby;
    result.y = a.y * scaleby;
    result.z = a.z * scaleby;
}

void VU0_v4scalexyz(const UMath::Vector4 &a, const UMath::Vector4 &b, UMath::Vector4 &result) {
    result.x = a.x * b.x;
    result.y = a.y * b.y;
    result.z = a.z * b.z;
}

float VU0_v4dotprod(const UMath::Vector4 &a, const UMath::Vector4 &b) {
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

float VU0_v4dotprodxyz(const UMath::Vector4 &a, const UMath::Vector4 &b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

void VU0_v3addscale(const UMath::Vector3 &a, const UMath::Vector3 &b, const float scaleby, UMath::Vector3 &result) {
    result.x = (a.x + b.x) * scaleby;
    result.y = (a.y + b.y) * scaleby;
    result.z = (a.z + b.z) * scaleby;
}

void VU0_v4addscale(const UMath::Vector4 &a, const UMath::Vector4 &b, const float scaleby, UMath::Vector4 &result) {
    result.x = (a.x + b.x) * scaleby;
    result.y = (a.y + b.y) * scaleby;
    result.z = (a.z + b.z) * scaleby;
    result.w = (a.w + b.w) * scaleby;
}

void VU0_v4addscalexyz(const UMath::Vector4 &a, const UMath::Vector4 &b, const float scaleby, UMath::Vector4 &result) {
    result.x = (a.x + b.x) * scaleby;
    result.y = (a.y + b.y) * scaleby;
    result.z = (a.z + b.z) * scaleby;
}

void VU0_v4scaleadd(const UMath::Vector4 &a, const float scaleby, const UMath::Vector4 &b, UMath::Vector4 &result) {
    result.x = a.x * scaleby + b.x;
    result.y = a.y * scaleby + b.y;
    result.z = a.z * scaleby + b.z;
    result.w = a.w * scaleby + b.w;
}

void VU0_v4scaleaddxyz(const UMath::Vector4 &a, const float scaleby, const UMath::Vector4 &b, UMath::Vector4 &result) {
    result.x = a.x * scaleby + b.x;
    result.y = a.y * scaleby + b.y;
    result.z = a.z * scaleby + b.z;
}

void VU0_v4crossprodxyz(const UMath::Vector4 &a, const UMath::Vector4 &b, UMath::Vector4 &dest) {
    UMath::Vector4 result;
    result.x = a.y * b.z - a.z * b.y;
    result.y = a.z * b.x - a.x * b.z;
    result.z = a.x * b.y - a.y * b.x;
    result.w = dest.w;
    dest = result;
}

float VU0_v4lengthsquare(const UMath::Vector4 &a) {
    return a.x * a.x + a.y * a.y + a.z * a.z + a.w * a.w;
}

float VU0_v4distancesquarexyz(const UMath::Vector4 &p1, const UMath::Vector4 &p2) {
    float dx = p1.x - p2.x;
    float dy = p1.y - p2.y;
    float dz = p1.z - p2.z;

    return dx * dx + dy * dy + dz * dz;
}

// El vector fila por la matriz (v * M), con la fila de la traslacion (w = 1).
void VU0_MATRIX4_vect3mult(const UMath::Vector3 &v, const UMath::Matrix4 &m, UMath::Vector3 &result) {
    float x = v.x * m.v0.x + v.y * m.v1.x + v.z * m.v2.x + m.v3.x;
    float y = v.x * m.v0.y + v.y * m.v1.y + v.z * m.v2.y + m.v3.y;
    float z = v.x * m.v0.z + v.y * m.v1.z + v.z * m.v2.z + m.v3.z;

    result.x = x;
    result.y = y;
    result.z = z;
}

void VU0_MATRIX4_vect4mult(const UMath::Vector4 &v, const UMath::Matrix4 &m, UMath::Vector4 &result) {
    float x = v.x * m.v0.x + v.y * m.v1.x + v.z * m.v2.x + v.w * m.v3.x;
    float y = v.x * m.v0.y + v.y * m.v1.y + v.z * m.v2.y + v.w * m.v3.y;
    float z = v.x * m.v0.z + v.y * m.v1.z + v.z * m.v2.z + v.w * m.v3.z;
    float w = v.x * m.v0.w + v.y * m.v1.w + v.z * m.v2.w + v.w * m.v3.w;

    result.x = x;
    result.y = y;
    result.z = z;
    result.w = w;
}

void VU0_MATRIX4_vect4mult(const UMath::Vector4 *v, const UMath::Matrix4 &m, UMath::Vector4 *result, int count) {
    for (int i = 0; i < count; i++) {
        VU0_MATRIX4_vect4mult(v[i], m, result[i]);
    }
}

// Solo la rotacion (las tres primeras filas) y solo xyz: la w del destino no cambia.
void VU0_MATRIX3x4_vect4mult(const UMath::Vector4 &v, const UMath::Matrix4 &m, UMath::Vector4 &result) {
    float x = v.x * m.v0.x + v.y * m.v1.x + v.z * m.v2.x;
    float y = v.x * m.v0.y + v.y * m.v1.y + v.z * m.v2.y;
    float z = v.x * m.v0.z + v.y * m.v1.z + v.z * m.v2.z;

    result.x = x;
    result.y = y;
    result.z = z;
}

// La interpolacion de PS2 (el asm de la VU0 de mas arriba): a*t mas o menos b*(1-t), con el signo que deja la norma
// mayor (compara |a-b|^2 con |a+b|^2), y normalizada.
void VU0_fastqslerp(const RQUAT &b, const RQUAT &a, RQUAT &result, const float t) {
    float s = 1.0f - t;
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    float dz = a.z - b.z;
    float dw = a.w - b.w;
    float px = a.x + b.x;
    float py = a.y + b.y;
    float pz = a.z + b.z;
    float pw = a.w + b.w;

    if (dx * dx + dy * dy + dz * dz + dw * dw > px * px + py * py + pz * pz + pw * pw) {
        s = -s;
    }

    float x = a.x * t + b.x * s;
    float y = a.y * t + b.y * s;
    float z = a.z * t + b.z * s;
    float w = a.w * t + b.w * s;
    float rlen = VU0_rsqrt(x * x + y * y + z * z + w * w);

    result.x = x * rlen;
    result.y = y * rlen;
    result.z = z * rlen;
    result.w = w * rlen;
}
#endif

#ifndef EA_PLATFORM_GAMECUBE
// Decl: Carbon: UVectorMath.cpp: 1137, GC MW: UVectorMathGC.hpp: 352, PS2 MW: UVectorMath.hpp: TODO
void VU0_ExtractXAxis3FromQuat(const RQUAT &quat, UMath::Vector3 &result) {
    const float scale = 2.0f;
    float yy = scale * (quat.y * quat.y);
    float zz = scale * (quat.z * quat.z);
    float xy = scale * (quat.x * quat.y);
    float xz = scale * (quat.x * quat.z);
    float yw = scale * (quat.y * quat.w);
    float zw = scale * (quat.z * quat.w);

    result.x = 1.0f - (yy + zz);
    result.y = xy + zw;
    result.z = xz - yw;
}

// Decl: Carbon: UVectorMath.cpp: 1169, GC MW: UVectorMathGC.hpp: 386, PS2 MW: UVectorMath.hpp: TODO
void VU0_ExtractYAxis3FromQuat(const RQUAT &quat, UMath::Vector3 &result) {
    const float scale = 2.0f;
    float xx = scale * (quat.x * quat.x);
    float zz = scale * (quat.z * quat.z);
    float xy = scale * (quat.x * quat.y);
    float xw = scale * (quat.x * quat.w);
    float yz = scale * (quat.y * quat.z);
    float zw = scale * (quat.z * quat.w);

    result.x = xy - zw;
    result.y = 1.0f - (xx + zz);
    result.z = yz + xw;
}

// Decl: Carbon: UVectorMath.cpp: 1202, GC MW: UVectorMathGC.hpp: 423, PS2 MW: UVectorMath.hpp: TODO
void VU0_ExtractZAxis3FromQuat(const RQUAT &quat, UMath::Vector3 &result) {
    const float scale = 2.0f;
    float xx = scale * (quat.x * quat.x);
    float yy = scale * (quat.y * quat.y);
    float xz = scale * (quat.x * quat.z);
    float xw = scale * (quat.x * quat.w);
    float yz = scale * (quat.y * quat.z);
    float yw = scale * (quat.y * quat.w);

    result.x = xz + yw;
    result.y = yz - xw;
    result.z = 1.0f - (xx + yy);
}

// Decl: Carbon: UVectorMath.cpp: 1235, GC MW: UVectorMathGC.hpp: 472, PS2 MW: UVectorMath.hpp: TODO
void VU0_quattom4(const RQUAT &quat, UMath::Matrix4 &result) {
    const float scale = 2.0f;
    float xx = quat.x * quat.x * scale;
    float yy = quat.y * quat.y * scale;
    float zz = quat.z * quat.z * scale;

    float xy = quat.x * quat.y * scale;
    float xz = quat.x * quat.z * scale;
    float xw = quat.x * quat.w * scale;

    float yz = quat.y * quat.z * scale;
    float yw = quat.y * quat.w * scale;
    float zw = quat.z * quat.w * scale;

    result[0][0] = 1.0f - (yy + zz);
    result[0][1] = (xy + zw);
    result[0][2] = (xz - yw);
    result[0][3] = 0.0f;

    result[1][0] = (xy - zw);
    result[1][1] = 1.0f - (xx + zz);
    result[1][2] = (yz + xw);
    result[1][3] = 0.0f;

    result[2][0] = (xz + yw);
    result[2][1] = (yz - xw);
    result[2][2] = 1.0f - (xx + yy);
    result[2][3] = 0.0f;

    result[3][0] = 0.0f;
    result[3][1] = 0.0f;
    result[3][2] = 0.0f;
    result[3][3] = 1.0f;
}

// Decl: Carbon: UVectorMath.cpp: 1313, GC MW: UVectorMathGC.hpp: 538, PS2 MW: UVectorMath.hpp: 1891
#ifdef EA_PLATFORM_PLAYSTATION2
// PS2: fuera de linea (UMath::Mult y MatrixMultiplyDefault la llaman con jal) y un solo bloque de la VU0, todo en la
// linea 3246 del original de la A124: las filas de m2 en vf5-vf8, cada fila de m1 repartida por x, y, z, w y las
// cuatro filas del resultado guardadas al final (familia VU0 de UMath, docs/asm-de-ea.md).
void VU0_MATRIX4_mult(const UMath::Matrix4 &m1, const UMath::Matrix4 &m2, UMath::Matrix4 &result) {
    asm __volatile__("lqc2 vf1, 0x0(%0)\n"
                     "lqc2 vf2, 0x10(%0)\n"
                     "lqc2 vf3, 0x20(%0)\n"
                     "lqc2 vf4, 0x30(%0)\n"
                     "lqc2 vf5, 0x0(%1)\n"
                     "lqc2 vf6, 0x10(%1)\n"
                     "lqc2 vf7, 0x20(%1)\n"
                     "lqc2 vf8, 0x30(%1)\n"
                     "vmulax.xyzw ACC, vf5, vf1x\n"
                     "vmadday.xyzw ACC, vf6, vf1y\n"
                     "vmaddaz.xyzw ACC, vf7, vf1z\n"
                     "vmaddw.xyzw vf9, vf8, vf1w\n"
                     "vmulax.xyzw ACC, vf5, vf2x\n"
                     "vmadday.xyzw ACC, vf6, vf2y\n"
                     "vmaddaz.xyzw ACC, vf7, vf2z\n"
                     "vmaddw.xyzw vf10, vf8, vf2w\n"
                     "vmulax.xyzw ACC, vf5, vf3x\n"
                     "vmadday.xyzw ACC, vf6, vf3y\n"
                     "vmaddaz.xyzw ACC, vf7, vf3z\n"
                     "vmaddw.xyzw vf11, vf8, vf3w\n"
                     "vmulax.xyzw ACC, vf5, vf4x\n"
                     "vmadday.xyzw ACC, vf6, vf4y\n"
                     "vmaddaz.xyzw ACC, vf7, vf4z\n"
                     "vmaddw.xyzw vf12, vf8, vf4w\n"
                     "sqc2 vf9, 0x0(%2)\n"
                     "sqc2 vf10, 0x10(%2)\n"
                     "sqc2 vf11, 0x20(%2)\n"
                     "sqc2 vf12, 0x30(%2)"
                     :
                     : "r"(&m1), "r"(&m2), "r"(&result)
                     : "memory");
}
#else
void VU0_MATRIX4_mult(const UMath::Matrix4 &m1, const UMath::Matrix4 &m2, UMath::Matrix4 &result) {
    UMath::Matrix4 temp;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            temp[i][j] = m1[i][0] * m2[0][j] + m1[i][1] * m2[1][j] + m1[i][2] * m2[2][j] + m1[i][3] * m2[3][j];
        }
    }
    result = temp;
}
#endif // EA_PLATFORM_PLAYSTATION2
#endif

// Decl: Carbon: UVectorMath.cpp: 4934, GC MW: UVectorMath.cpp: 4935, PS2 MW: UVectorMath.cpp: 4936
float VU0_Atan2(const float opposite, const float adjacent) {
    float x = VU0_fabs(adjacent) > UMath::Epsilon ? adjacent : 0.0f;
    float y = VU0_fabs(opposite) > UMath::Epsilon ? opposite : 0.0f;

    short bangle = bATan(x, y);
    return bangle / 65536.0f;
}

// Los dos estaticos que el static-init del objetivo inicializa desde este .cpp
// (UVectorMath.h:311 y :350 en el mapa de lineas son los CONSTRUCTORES en
// linea, no el sitio de la definicion).
const UTransform UTransform::fgIdentityTransform(UMath::Matrix4::kIdentity);

const USphere USphere::fgNullSphere(UMath::Vector4::kZero);
