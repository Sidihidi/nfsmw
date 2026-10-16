#include "UVectorMath.h"
#include "eetypes.h"

/* Los vf de estas VU0_* van ESCRITOS en el asm, no los reparte el compilador:
 * en el binario de la A124 cada una sale siempre con los mismos (lqc2 vf1 el
 * primer operando, vf2 el segundo, vf3 el destino...) en mas de mil
 * expansiones, y en los stabs no tienen ninguna local en un registro de VU0 (su
 * unica local de 128 bits es `_t0', u_long128 en un registro entero).  La forma
 * con la restriccion "j" y un asm por paso es la de bMath.hpp (bLength...), no
 * la de UMath: aplicada a cuatro de estas pierde 39 funciones (medido). */

// TODO reorder the rest

// Decl: 159
inline float VU0_v3dotprod(const UMath::Vector3 &a, const UMath::Vector3 &b) {
    float result;
    asm __volatile__("lqc2 vf1, %1\n"
                     "lqc2 vf2, %2\n"
                     "vmul.xyz vf3, vf1, vf2\n"
                     "vaddy.x vf3, vf3, vf3y\n"
                     "vaddz.x vf3, vf3, vf3z\n"
                     "qmfc2.ni %0, vf3"
                     : "=r"(result)
                     : "o"(a), "o"(b));
    return result;
}

// Decl: 263
inline void VU0_v3crossprod(const UMath::Vector3 &a, const UMath::Vector3 &b, UMath::Vector3 &dest) {
    asm __volatile__("lqc2 vf1, %1\n"
                     "lqc2 vf2, %2\n"
                     "lqc2 vf3, %0\n"
                     "vopmula ACC, vf1, vf2\n"
                     "vopmsub vf3, vf2, vf1\n"
                     "sqc2 vf3, %0"
                     : "=o"(dest)
                     : "o"(a), "o"(b));
}

// Decl: Carbon: UVectorMathCPU.hpp: 94, GC MW: UVectorMathGC.hpp: 26, PS2 MW: UVectorMath.hpp: 354
inline void VU0_v3unitcrossprod(const UMath::Vector3 &a, const UMath::Vector3 &b, UMath::Vector3 &dest) {
    u_long128 _t0;
    asm __volatile__("lui %3, 0x3f80\n"
                     "lqc2 vf1, %1\n"
                     "lqc2 vf2, %2\n"
                     "lqc2 vf3, %0\n"
                     "vopmula ACC, vf1, vf2\n"
                     "vopmsub vf3, vf2, vf1\n"
                     "qmtc2.ni %3, vf5\n"
                     "vmul.xyz vf4, vf3, vf3\n"
                     "vmulax.x ACC, vf3, vf3x\n"
                     "vmadday.x ACC, vf5, vf4y\n"
                     "vmaddz.x vf4, vf5, vf4z\n"
                     "vrsqrt Q, vf0w, vf4x\n"
                     "vwaitq\n"
                     "vmulq.xyz vf3, vf3, Q\n"
                     "sqc2 vf3, %0"
                     : "=o"(dest)
                     : "o"(a), "o"(b), "r"(_t0));
}

// TODO order
/* El EE tiene raiz cuadrada en la FPU y el original usa `sqrt.s' escalar, no
 * el bloque de VU0 que habia antes (que ademas estaba roto: leia vf2 sin
 * cargarlo).  Medido el 2026-09-23: el `sqrt.s; nop; nop; mov.s' del binario
 * (lineas 1734-1751, 37 veces) NO lo ponen ni el compilador ni el ensamblador
 * (con sqrtf() no sale ningun nop): es asm de EA, con las locales `temp' (el
 * destino del sqrt.s) y `result' (el del mov.s) en los stabs.  Asm de EA
 * aprobado por el usuario (ronda 16): `temp' entra al asm SIN valor, y eso
 * explica que el original ponga cada `temp' en un registro preservado distinto
 * ($f22-$f25 en SphereVsBox, $f21 en SetFulcrum); sin volatile o con `temp'
 * de salida no casa.  Medido en todas las unidades: A124 +14 -0, SLUS +12 -0. */
inline float VU0_sqrt(const float a) {
    float temp;
    float result;
    asm __volatile__("sqrt.s %1, %2\n"
                     "nop\n"
                     "nop\n"
                     "mov.s %0, %1"
                     : "=f"(result)
                     : "f"(temp), "f"(a));
    return result;
}

/* Asm de EA aprobado por el usuario (ronda 46), como VU0_sqrt: en el original
 * (linea 1769) es `rsqrt.s temp, 1.0, a', dos `nop' y `mov.s result, temp',
 * sin la comparacion con cero, con las locales `result' y `temp' de los stabs
 * y `temp' de entrada sin valor (cada sitio la pone en otro registro, de $f1 a
 * $f26, y DrawAmbientShadow la recarga de la pila con lwc1).  El 1.0 lo carga el compilador (lui/mtc1 con la linea
 * del asm).  El texto va entre `.set noreorder' y `.set reorder': al leer
 * `.set noreorder' el ensamblador vacia los retardos pendientes, y por eso el
 * original lleva un `nop' entre el mtc1 del 1.0 y el rsqrt.s cuando van
 * pegados (MakeSegSpaceMatrix) y ninguno cuando delante hay un add.s; los dos
 * `nop' son los que EA tuvo que poner a mano en modo noreorder.  Prueba y
 * formas en C medidas: docs/asm-de-ea.md. */
inline float VU0_rsqrt(const float a) {
    float temp;
    float result;
    asm __volatile__(".set noreorder\n"
                     "rsqrt.s %1, %2, %3\n"
                     "nop\n"
                     "nop\n"
                     "mov.s %0, %1\n"
                     ".set reorder"
                     : "=f"(result)
                     : "f"(temp), "f"(1.0f), "f"(a));
    return result;
}

// Lineas 1471-1492 del original: la resta y la suma de x y z, sin la raiz.
inline float VU0_v3distancesquarexz(const UMath::Vector3 &p1, const UMath::Vector3 &p2) {
    float result;
    u_long128 _t0;
    asm __volatile__("lui %3, 0x3f80\n"
                     "lqc2 vf1, %1\n"
                     "lqc2 vf2, %2\n"
                     "vsub.xyz vf1, vf1, vf2\n"
                     "qmtc2.ni %3, vf3\n"
                     "vmul.xyz vf2, vf1, vf1\n"
                     "vmulax.x ACC, vf1, vf1x\n"
                     "vmaddz.x vf2, vf3, vf2z\n"
                     "qmfc2.ni %0, vf2"
                     : "=r"(result)
                     : "o"(p1), "o"(p2), "r"(_t0));
    return result;
}

/* Los nueve de aqui abajo estaban VACIOS o devolviendo 0.0f, que es la trampa
 * de `bMin'/`bMax' y `bSqrt' otra vez: no fallan al compilar, devuelven basura
 * y GCC se lleva por delante todo lo que cuelga de ellos.  De estos nueve
 * dependen 184 funciones de PS2 que cubren 200 kB.  Cada forma sale del
 * catalogo de bloques VU0 del propio binario (scripts/vu0bloques.py), con su
 * numero de apariciones, y coincide instruccion a instruccion con la gemela de
 * `Vector3' que ya estaba escrita. */

// xyzw: escribe las cuatro componentes, asi que NO hace falta cargar el
// destino.  82 apariciones la variante xyz, que si lo carga.
inline void VU0_v4scaleadd(const UMath::Vector4 &a, const float scaleby, const UMath::Vector4 &b, UMath::Vector4 &result) {
    asm __volatile__("lqc2 vf1, %1\n"
                     "lqc2 vf2, %3\n"
                     "qmtc2.ni %2, vf4\n"
                     "vmulx.xyzw vf3, vf1, vf4x\n"
                     "vadd.xyzw vf3, vf2, vf3\n"
                     "sqc2 vf3, %0"
                     : "=o"(result)
                     : "o"(a), "r"(scaleby), "o"(b));
}

inline void VU0_v4scaleaddxyz(const UMath::Vector4 &a, const float scaleby, const UMath::Vector4 &b, UMath::Vector4 &result) {
    asm __volatile__("lqc2 vf1, %1\n"
                     "lqc2 vf2, %3\n"
                     "lqc2 vf3, %0\n"
                     "qmtc2.ni %2, vf4\n"
                     "vmulx.xyz vf3, vf1, vf4x\n"
                     "vadd.xyz vf3, vf2, vf3\n"
                     "sqc2 vf3, %0"
                     : "=o"(result)
                     : "o"(a), "r"(scaleby), "o"(b));
}

// El `lui 0x3f80' mete un 1,0f en vf3 para que los tres `vmadd' sumen las
// componentes sin multiplicarlas otra vez.  Es el mismo truco de VU0_v3length.
inline float VU0_v4lengthsquare(const UMath::Vector4 &a) {
    u_long128 _t0;
    float result;
    asm __volatile__("lui %2, 0x3f80\n"
                     "lqc2 vf1, %1\n"
                     "vmul.xyzw vf2, vf1, vf1\n"
                     "qmtc2.ni %2, vf3\n"
                     "vmulax.x ACC, vf1, vf1x\n"
                     "vmadday.x ACC, vf3, vf2y\n"
                     "vmaddaz.x ACC, vf3, vf2z\n"
                     "vmaddw.x vf2, vf3, vf2w\n"
                     "qmfc2.ni %0, vf2"
                     : "=r"(result)
                     : "o"(a), "r"(_t0));
    return result;
}

inline float VU0_v4lengthsquarexyz(const UMath::Vector4 &a) {
    u_long128 _t0;
    float result;
    asm __volatile__("lui %2, 0x3f80\n"
                     "lqc2 vf1, %1\n"
                     "vmul.xyz vf2, vf1, vf1\n"
                     "qmtc2.ni %2, vf3\n"
                     "vmulax.x ACC, vf1, vf1x\n"
                     "vmadday.x ACC, vf3, vf2y\n"
                     "vmaddz.x vf2, vf3, vf2z\n"
                     "qmfc2.ni %0, vf2"
                     : "=r"(result)
                     : "o"(a), "r"(_t0));
    return result;
}

inline void VU0_v4subxyz(const UMath::Vector4 &a, const UMath::Vector4 &b, UMath::Vector4 &result) {
    asm __volatile__("lqc2 vf1, %1\n"
                     "lqc2 vf2, %2\n"
                     "lqc2 vf3, %0\n"
                     "vsub.xyz vf3, vf1, vf2\n"
                     "sqc2 vf3, %0"
                     : "=o"(result)
                     : "o"(a), "o"(b));
}

inline float VU0_v4dotprodxyz(const UMath::Vector4 &a, const UMath::Vector4 &b) {
    float result;
    asm __volatile__("lqc2 vf1, %1\n"
                     "lqc2 vf2, %2\n"
                     "vmul.xyz vf3, vf1, vf2\n"
                     "vaddy.x vf3, vf3, vf3y\n"
                     "vaddz.x vf3, vf3, vf3z\n"
                     "qmfc2.ni %0, vf3"
                     : "=r"(result)
                     : "o"(a), "o"(b));
    return result;
}

/* La misma forma que VU0_v3dotprod mas `vaddw.x', que es la cuarta componente.
 * Di por hecho que no aparecia en el binario porque la busque en el catalogo
 * de bloques y no salia: venia fundida dentro de una racha mas larga.  Quien
 * la delato fue el histograma de SphereVsBox, donde faltaban exactamente
 * `vmul.xyzw', `vaddy.x', `vaddz.x' y `vaddw.x' y sobraba una llamada a
 * VU0_v4dotprod. Verificada despues en zDynamics 0x6150. */
inline float VU0_v4dotprod(const UMath::Vector4 &a, const UMath::Vector4 &b) {
    float result;
    asm __volatile__("lqc2 vf1, %1\n"
                     "lqc2 vf2, %2\n"
                     "vmul.xyzw vf3, vf1, vf2\n"
                     "vaddy.x vf3, vf3, vf3y\n"
                     "vaddz.x vf3, vf3, vf3z\n"
                     "vaddw.x vf3, vf3, vf3w\n"
                     "qmfc2.ni %0, vf3"
                     : "=r"(result)
                     : "o"(a), "o"(b));
    return result;
}

inline void VU0_v4scalexyz(const UMath::Vector4 &a, const float scaleby, UMath::Vector4 &result) {
    asm __volatile__("lqc2 vf1, %1\n"
                     "lqc2 vf3, %0\n"
                     "qmtc2.ni %2, vf2\n"
                     "vmulx.xyz vf3, vf1, vf2x\n"
                     "sqc2 vf3, %0"
                     : "=o"(result)
                     : "o"(a), "r"(scaleby));
}

// Producto componente a componente en xyz; la misma forma que VU0_v3scale de
// dos vectores (linea 1002 del original).  UVectorMath.h la declaraba y en PS2
// no tenia cuerpo.
inline void VU0_v4scalexyz(const UMath::Vector4 &a, const UMath::Vector4 &b, UMath::Vector4 &result) {
    asm __volatile__("lqc2 vf3, %0\n"
                     "lqc2 vf1, %1\n"
                     "lqc2 vf2, %2\n"
                     "vmul.xyz vf3, vf1, vf2\n"
                     "sqc2 vf3, %0"
                     : "=o"(result)
                     : "o"(a), "o"(b));
}

// Un solo bloque, no una resta seguida de una longitud: el original nunca
// guarda la diferencia (33 apariciones).
inline float VU0_v4distancesquarexyz(const UMath::Vector4 &p1, const UMath::Vector4 &p2) {
    u_long128 _t0;
    float result;
    asm __volatile__("lui %3, 0x3f80\n"
                     "lqc2 vf1, %1\n"
                     "lqc2 vf2, %2\n"
                     "vsub.xyz vf1, vf1, vf2\n"
                     "qmtc2.ni %3, vf3\n"
                     "vmul.xyz vf2, vf1, vf1\n"
                     "vmulax.x ACC, vf1, vf1x\n"
                     "vmadday.x ACC, vf3, vf2y\n"
                     "vmaddz.x vf2, vf3, vf2z\n"
                     "qmfc2.ni %0, vf2"
                     : "=r"(result)
                     : "o"(p1), "o"(p2), "r"(_t0));
    return result;
}

inline void VU0_v4addxyz(const UMath::Vector4 &a, const UMath::Vector4 &b, UMath::Vector4 &result) {
    asm __volatile__("lqc2 vf1, %1\n"
                     "lqc2 vf2, %2\n"
                     "lqc2 vf3, %0\n"
                     "vadd.xyz vf3, vf1, vf2\n"
                     "sqc2 vf3, %0"
                     : "=o"(result)
                     : "o"(a), "o"(b));
}

/* Las siete de aqui abajo no estaban vacias: NO ESTABAN.  `UMath.h' las llama,
 * `UVectorMath.h' las declara y el unico cuerpo que existe esta dentro del
 * `#ifdef EA_PLATFORM_GAMECUBE' de `UVectorMath.cpp', escrito con los
 * paired-singles de Gekko.  En PS2 salian como `jal' a un simbolo indefinido, y
 * no se ve porque de PS2 comparamos objetos y no enlazamos ejecutable.
 *
 * Con `.xyzw' no se carga el destino, que se escribe entero; con `.xyz' si,
 * para conservar la w.  Esa es toda la diferencia entre las parejas. */
inline void VU0_v4add(const UMath::Vector4 &a, const UMath::Vector4 &b, UMath::Vector4 &result) {
    asm __volatile__("lqc2 vf1, %1\n"
                     "lqc2 vf2, %2\n"
                     "vadd.xyzw vf3, vf1, vf2\n"
                     "sqc2 vf3, %0"
                     : "=o"(result)
                     : "o"(a), "o"(b));
}

inline void VU0_v4sub(const UMath::Vector4 &a, const UMath::Vector4 &b, UMath::Vector4 &result) {
    asm __volatile__("lqc2 vf1, %1\n"
                     "lqc2 vf2, %2\n"
                     "vsub.xyzw vf3, vf1, vf2\n"
                     "sqc2 vf3, %0"
                     : "=o"(result)
                     : "o"(a), "o"(b));
}

// (a + b) * escala: primero la suma y luego la escala, como en GameCube
// (UVectorMath.cpp) y como la usa UMath::Mean con 0,5f.  Lineas 1036-1103.
inline void VU0_v4addscale(const UMath::Vector4 &a, const UMath::Vector4 &b, const float scaleby, UMath::Vector4 &result) {
    asm __volatile__("lqc2 vf1, %1\n"
                     "lqc2 vf2, %2\n"
                     "qmtc2.ni %3, vf4\n"
                     "vadd.xyzw vf3, vf1, vf2\n"
                     "vmulx.xyzw vf3, vf3, vf4x\n"
                     "sqc2 vf3, %0"
                     : "=o"(result)
                     : "o"(a), "o"(b), "r"(scaleby));
}

inline void VU0_v4addscalexyz(const UMath::Vector4 &a, const UMath::Vector4 &b, const float scaleby, UMath::Vector4 &result) {
    asm __volatile__("lqc2 vf1, %1\n"
                     "lqc2 vf2, %2\n"
                     "lqc2 vf3, %0\n"
                     "qmtc2.ni %3, vf4\n"
                     "vadd.xyz vf3, vf1, vf2\n"
                     "vmulx.xyz vf3, vf3, vf4x\n"
                     "sqc2 vf3, %0"
                     : "=o"(result)
                     : "o"(a), "o"(b), "r"(scaleby));
}

inline void VU0_v3addscale(const UMath::Vector3 &a, const UMath::Vector3 &b, const float scaleby, UMath::Vector3 &result) {
    asm __volatile__("lqc2 vf1, %1\n"
                     "lqc2 vf2, %2\n"
                     "lqc2 vf3, %0\n"
                     "qmtc2.ni %3, vf4\n"
                     "vadd.xyz vf3, vf1, vf2\n"
                     "vmulx.xyz vf3, vf3, vf4x\n"
                     "sqc2 vf3, %0"
                     : "=o"(result)
                     : "o"(a), "o"(b), "r"(scaleby));
}

inline void VU0_v4crossprodxyz(const UMath::Vector4 &a, const UMath::Vector4 &b, UMath::Vector4 &dest) {
    asm __volatile__("lqc2 vf1, %1\n"
                     "lqc2 vf2, %2\n"
                     "lqc2 vf3, %0\n"
                     "vopmula ACC, vf1, vf2\n"
                     "vopmsub vf3, vf2, vf1\n"
                     "sqc2 vf3, %0"
                     : "=o"(dest)
                     : "o"(a), "o"(b));
}

// Las cuatro filas de la matriz salen de la misma base (un puntero con
// desplazamiento); el vector y el resultado son operandos de memoria, como en
// VU0_MATRIX4_vect3mult: el original pliega su desplazamiento (lqc2 vf1,
// 144(sp) / sqc2 vf6, 192(a0), linea 1891).
inline void VU0_MATRIX4_vect4mult(const UMath::Vector4 &v, const UMath::Matrix4 &m, UMath::Vector4 &result) {
    asm __volatile__("lqc2 vf1, %1\n"
                     "lqc2 vf2, 0x0(%2)\n"
                     "lqc2 vf3, 0x10(%2)\n"
                     "lqc2 vf4, 0x20(%2)\n"
                     "lqc2 vf5, 0x30(%2)\n"
                     "vmulax.xyzw ACC, vf2, vf1x\n"
                     "vmadday.xyzw ACC, vf3, vf1y\n"
                     "vmaddaz.xyzw ACC, vf4, vf1z\n"
                     "vmaddw.xyzw vf6, vf5, vf1w\n"
                     "sqc2 vf6, %0"
                     : "=o"(result)
                     : "o"(v), "r"(&m));
}

// The array version that UMath::RotateTranslate(a, m, r, count) expands in the PS2 prototype (DebugDraw Box, Circle and
// CylinderSection, RigidBody::DoInstanceCollision2d): one VU0 loop whose pointer and counter steps (`daddiu', `addi')
// are written in the asm block, with the matrix rows kept in vf2-vf4 and the next vector loaded behind the store.
inline void VU0_MATRIX4_vect4mult(const UMath::Vector4 *v, const UMath::Matrix4 &m, UMath::Vector4 *result, int count) {
    asm __volatile__("lqc2 vf1, 0x0(%1)\n"
                     "lqc2 vf2, 0x0(%3)\n"
                     "lqc2 vf3, 0x10(%3)\n"
                     "lqc2 vf4, 0x20(%3)\n"
                     ".set noreorder\n"
                     "1:\n"
                     "vmulax.xyzw ACC, vf2, vf1x\n"
                     "lqc2 vf5, 0x30(%3)\n"
                     "vmadday.xyzw ACC, vf3, vf1y\n"
                     "daddiu %1, %1, 0x10\n"
                     "vmaddaz.xyzw ACC, vf4, vf1z\n"
                     "addi %0, %0, -1\n"
                     "vmaddw.xyzw vf6, vf5, vf1w\n"
                     "lqc2 vf1, 0x0(%1)\n"
                     "sqc2 vf6, 0x0(%2)\n"
                     "bnez %0, 1b\n"
                     "daddiu %2, %2, 0x10\n"
                     ".set reorder"
                     : "=r"(count), "=r"(v), "=r"(result)
                     : "r"(&m), "0"(count), "1"(v), "2"(result));
}

// Decl: Carbon: UVectorMathCPU.hpp: 129, GC MW: UVectorMathGC.hpp: 220, PS2 MW: UVectorMath.hpp: 621
inline void VU0_v3unit(const UMath::Vector3 &a, UMath::Vector3 &result) {
    u_long128 _t0;
    asm __volatile__("lui %2, 0x3f80\n"
                     "lqc2 vf1, %1\n"
                     "lqc2 vf2, %0\n"
                     "vmul.xyz vf2, vf1, vf1\n"
                     "qmtc2.ni %2, vf3\n"
                     "vmulax.x ACC, vf1, vf1x\n"
                     "vmadday.x ACC, vf3, vf2y\n"
                     "vmaddz.x vf2, vf3, vf2z\n"
                     "vrsqrt Q, vf0w, vf2x\n"
                     "vwaitq\n"
                     "vmulq.xyz vf2, vf1, Q\n"
                     "sqc2 vf2, %0"
                     : "=o"(result)
                     : "o"(a), "r"(_t0));
}

// Decl: Carbon: UVectorMathCPU.hpp: 146, GC MW: UVectorMathGC.hpp: 236, PS2 MW: UVectorMath.hpp: 677
/* Un solo bloque, no `VU0_rsqrt(lengthsquare)' seguido de una escala: el
 * original nunca sale de VU0 a mitad (27 apariciones), y componerlo dejaba
 * ademas una llamada a `VU0_rsqrt', que esta fuera de linea.  Es el cuerpo de
 * `VU0_v3unit' tal cual. */
inline void VU0_v4unitxyz(const UMath::Vector4 &a, UMath::Vector4 &result) {
    u_long128 _t0;
    asm __volatile__("lui %2, 0x3f80\n"
                     "lqc2 vf1, %1\n"
                     "lqc2 vf2, %0\n"
                     "vmul.xyz vf2, vf1, vf1\n"
                     "qmtc2.ni %2, vf3\n"
                     "vmulax.x ACC, vf1, vf1x\n"
                     "vmadday.x ACC, vf3, vf2y\n"
                     "vmaddz.x vf2, vf3, vf2z\n"
                     "vrsqrt Q, vf0w, vf2x\n"
                     "vwaitq\n"
                     "vmulq.xyz vf2, vf1, Q\n"
                     "sqc2 vf2, %0"
                     : "=o"(result)
                     : "o"(a), "r"(_t0));
}

// Decl: 706
inline void VU0_v3add(const UMath::Vector3 &a, const UMath::Vector3 &b, UMath::Vector3 &result) {
    asm __volatile__("lqc2 vf1, %1\n"
                     "lqc2 vf2, %2\n"
                     "lqc2 vf3, %0\n"
                     "vadd.xyz vf3, vf1, vf2\n"
                     "sqc2 vf3, %0"
                     : "=o"(result)
                     : "o"(a), "o"(b));
}

// Decl: 789
inline void VU0_v3sub(const UMath::Vector3 &a, const UMath::Vector3 &b, UMath::Vector3 &result) {
    asm __volatile__("lqc2 vf1, %1\n"
                     "lqc2 vf2, %2\n"
                     "lqc2 vf3, %0\n"
                     "vsub.xyz vf3, vf1, vf2\n"
                     "sqc2 vf3, %0"
                     : "=o"(result)
                     : "o"(a), "o"(b));
}

// Decl: Carbon: UVectorMathCPU.hpp: 333, GC MW: UVectorMathGC.hpp: 255, PS2 MW: UVectorMath.hpp: 842
// 52 apariciones, y en ninguna guarda la diferencia: no es una resta seguida
// de una longitud, es un bloque.
inline float VU0_v4distancexyz(const UMath::Vector4 &p1, const UMath::Vector4 &p2) {
    u_long128 _t0;
    float result;
    asm __volatile__("lui %3, 0x3f80\n"
                     "lqc2 vf1, %1\n"
                     "lqc2 vf2, %2\n"
                     "vsub.xyz vf1, vf1, vf2\n"
                     "qmtc2.ni %3, vf3\n"
                     "vmul.xyz vf2, vf1, vf1\n"
                     "vmulax.x ACC, vf1, vf1x\n"
                     "vmadday.x ACC, vf3, vf2y\n"
                     "vmaddz.x vf2, vf3, vf2z\n"
                     "vsqrt Q, vf2x\n"
                     "vwaitq\n"
                     "vaddq.x vf3, vf0, Q\n"
                     "qmfc2.ni %0, vf3"
                     : "=r"(result)
                     : "o"(p1), "o"(p2), "r"(_t0));
    return result;
}

// Decl: 869
inline void VU0_v3scale(const UMath::Vector3 &a, const float scaleby, UMath::Vector3 &result) {
    asm __volatile__("lqc2 vf1, %1\n"
                     "lqc2 vf3, %0\n"
                     "qmtc2.ni %2, vf2\n"
                     "vmulx.xyz vf3, vf1, vf2x\n"
                     "sqc2 vf3, %0"
                     : "=o"(result)
                     : "o"(a), "r"(scaleby));
}

// Decl: 896
// Producto componente a componente: `a' va a vf1 y `b' a vf2 (DoJumpStabilizer y las tres
// Resolve*Collision de RigidBody cargan asi el primer argumento).
inline void VU0_v3scale(const UMath::Vector3 &a, const UMath::Vector3 &b, UMath::Vector3 &result) {
    asm __volatile__("lqc2 vf3, %0\n"
                     "lqc2 vf1, %1\n"
                     "lqc2 vf2, %2\n"
                     "vmul.xyz vf3, vf1, vf2\n"
                     "sqc2 vf3, %0"
                     : "=o"(result)
                     : "o"(a), "o"(b));
}

// Decl: 922
inline void VU0_v4scale(const UMath::Vector4 &a, const float scaleby, UMath::Vector4 &result) {
    asm __volatile__("lqc2 vf1, %1\n"
                     "qmtc2.ni %2, vf2\n"
                     "vmulx vf3, vf1, vf2x\n"
                     "sqc2 vf3, %0"
                     : "=o"(result)
                     : "o"(a), "r"(scaleby));
}

// Decl: Carbon: UVectorMathCPU.hpp: 354, GC MW: UVectorMathGC.hpp: 274, PS2 MW: UVectorMath.hpp: 1014
inline float VU0_v3length(const struct UMath::Vector3 &a) {
    u_long128 _t0;
    float result;
    asm __volatile__("lui %2, 0x3f80\n"
                     "lqc2 vf1, %1\n"
                     "vmul.xyz vf2, vf1, vf1\n"
                     "qmtc2.ni %2, vf3\n"
                     "vmulax.x ACC, vf1, vf1x\n"
                     "vmadday.x ACC, vf3, vf2y\n"
                     "vmaddz.x vf2, vf3, vf2z\n"
                     "vsqrt Q, vf2x\n"
                     "vwaitq\n"
                     "vaddq.x vf3, vf0, Q\n"
                     "qmfc2.ni %0, vf3"
                     : "=r"(result)
                     : "o"(a), "r"(_t0));
    return result;
}

// Decl: Carbon: UVectorMathCPU.hpp: 538, GC MW: UVectorMathGC.hpp: 125, PS2 MW: UVectorMath.hpp: 2125
// a x b + a*b.w + b*a.w, y en w a.w*b.w - a.b; `a' (el segundo) va a vf1.
inline void VU0_qmul(const RQUAT &b, const RQUAT &a, RQUAT &dest) {
    asm __volatile__("lqc2 vf1, %2\n"
                     "lqc2 vf2, %1\n"
                     "vmul.xyzw vf3, vf1, vf2\n"
                     "vsubx.w vf3, vf3, vf3x\n"
                     "vsuby.w vf3, vf3, vf3y\n"
                     "vsubz.w vf3, vf3, vf3z\n"
                     "vopmula ACC, vf1, vf2\n"
                     "vopmsub vf3, vf2, vf1\n"
                     "vmulw.xyz vf4, vf1, vf2w\n"
                     "vadd.x vf3, vf3, vf4\n"
                     "vadd.y vf3, vf3, vf4\n"
                     "vadd.z vf3, vf3, vf4\n"
                     "vmulw.xyz vf4, vf2, vf1w\n"
                     "vadd.x vf3, vf3, vf4\n"
                     "vadd.y vf3, vf3, vf4\n"
                     "vadd.z vf3, vf3, vf4\n"
                     "sqc2 vf3, %0"
                     : "=o"(dest)
                     : "o"(b), "o"(a));
}

// Decl: 1136
inline void VU0_v3scaleadd(const UMath::Vector3 &a, const float scaleby, const UMath::Vector3 &b, UMath::Vector3 &result) {
    asm __volatile__("lqc2 vf1, %1\n"
                     "lqc2 vf2, %3\n"
                     "lqc2 vf3, %0\n"
                     "qmtc2.ni %2, vf4\n"
                     "vmulx.xyz vf3, vf1, vf4x\n"
                     "vadd.xyz vf3, vf2, vf3\n"
                     "sqc2 vf3, %0"
                     : "=o"(result)
                     : "o"(a), "r"(scaleby), "o"(b));
}

// Decl: Carbon: UVectorMathCPU.hpp: 139, GC MW: UVectorMathGC.hpp: 228, PS2 MW: UVectorMath.hpp: 1203
// Igual que VU0_v4unitxyz pero sobre las cuatro componentes: con `.xyzw' no
// hace falta cargar el destino, que se escribe entero.
inline void VU0_v4unit(const UMath::Vector4 &a, UMath::Vector4 &result) {
    u_long128 _t0;
    asm __volatile__("lui %2, 0x3f80\n"
                     "lqc2 vf1, %1\n"
                     "vmul.xyzw vf2, vf1, vf1\n"
                     "qmtc2.ni %2, vf3\n"
                     "vmulax.x ACC, vf1, vf1x\n"
                     "vmadday.x ACC, vf3, vf2y\n"
                     "vmaddaz.x ACC, vf3, vf2z\n"
                     "vmaddw.x vf2, vf3, vf2w\n"
                     "vrsqrt Q, vf0w, vf2x\n"
                     "vwaitq\n"
                     "vmulq.xyzw vf2, vf1, Q\n"
                     "sqc2 vf2, %0"
                     : "=o"(result)
                     : "o"(a), "r"(_t0));
}

// Decl: Carbon: UVectorMathCPU.hpp: 293, GC MW: UVectorMathGC.hpp: 62, PS2 MW: UVectorMath.hpp: 1223
inline void VU0_v3negate(UMath::Vector3 &result) {
    asm __volatile__("lqc2 vf1, %0\n"
                     "vsub.xyz vf1, vf0, vf1\n"
                     "sqc2 vf1, %0"
                     : "=o"(result));
}

// Decl: Carbon: UVectorMathCPU.hpp: 300, GC MW: UVectorMathGC.hpp: 69, PS2 MW: UVectorMath.hpp: 1243
// El mismo bloque que VU0_v3negate: 0 - v en xyz, la w se queda.
inline void VU0_v4negatexyz(UMath::Vector4 &result) {
    asm __volatile__("lqc2 vf1, %0\n"
                     "vsub.xyz vf1, vf0, vf1\n"
                     "sqc2 vf1, %0"
                     : "=o"(result));
}

// Decl: Carbon: UVectorMathCPU.hpp: 324, GC MW: UVectorMathGC.hpp: 243, PS2 MW: UVectorMath.hpp: 1323
inline float VU0_v3distance(const UMath::Vector3 &p1, const UMath::Vector3 &p2) {
    float result;
    u_long128 _t0;
    asm __volatile__("lui %3, 0x3f80\n"
                     "lqc2 vf1, %1\n"
                     "lqc2 vf2, %2\n"
                     "vsub.xyz vf1, vf1, vf2\n"
                     "qmtc2.ni %3, vf3\n"
                     "vmul.xyz vf2, vf1, vf1\n"
                     "vmulax.x ACC, vf1, vf1x\n"
                     "vmadday.x ACC, vf3, vf2y\n"
                     "vmaddz.x vf2, vf3, vf2z\n"
                     "vsqrt Q, vf2x\n"
                     "vwaitq\n"
                     "vaddq.x vf3, vf0, Q\n"
                     "qmfc2.ni %0, vf3"
                     : "=r"(result)
                     : "o"(p1), "o"(p2), "r"(_t0));
    return result;
}

// Decl: 1351
inline float VU0_v3distancesquare(const UMath::Vector3 &p1, const UMath::Vector3 &p2) {
    u_long128 _t0;
    float result;
    asm __volatile__("lui %3, 0x3f80\n"
                     "lqc2 vf1, %1\n"
                     "lqc2 vf2, %2\n"
                     "vsub.xyz vf1, vf1, vf2\n"
                     "qmtc2.ni %3, vf3\n"
                     "vmul.xyz vf2, vf1, vf1\n"
                     "vmulax.x ACC, vf1, vf1x\n"
                     "vmadday.x ACC, vf3, vf2y\n"
                     "vmaddz.x vf2, vf3, vf2z\n"
                     "qmfc2.ni %0, vf2\n"
                     : "=r"(result)
                     : "o"(p1), "o"(p2), "r"(_t0));
    return result;
}

// Decl: Carbon: UVectorMathCPU.hpp: 342, GC MW: UVectorMathGC.hpp: 263, PS2 MW: UVectorMath.hpp: 1437
inline float VU0_v3distancexz(const UMath::Vector3 &p1, const UMath::Vector3 &p2) {
    float result;
    u_long128 _t0;
    asm __volatile__("lui %3, 0x3f80\n"
                     "lqc2 vf1, %2\n"
                     "lqc2 vf2, %1\n"
                     "vsub.xyz vf1, vf1, vf2\n"
                     "qmtc2.ni %3, vf3\n"
                     "vmul.xyz vf2, vf1, vf1\n"
                     "vmulax.x ACC, vf1, vf1x\n"
                     "vmaddz.x vf2, vf3, vf2z\n"
                     "vsqrt Q, vf2x\n"
                     "vwaitq\n"
                     "vaddq.x vf3, vf0, Q\n"
                     "qmfc2.ni %0, vf3"
                     : "=r"(result)
                     : "o"(p1), "o"(p2), "r"(_t0));
    return result;
}

// Decl: 1545
inline float VU0_v3lengthsquare(const UMath::Vector3 &a) {
    u_long128 _t0;
    float result;
    asm __volatile__("lui %2, 0x3f80\n"
                     "lqc2 vf1, %1\n"
                     "vmul.xyz vf2, vf1, vf1\n"
                     "qmtc2.ni %2, vf3\n"
                     "vmulax.x ACC, vf1, vf1x\n"
                     "vmadday.x ACC, vf3, vf2y\n"
                     "vmaddz.x vf2, vf3, vf2z\n"
                     "qmfc2.ni %0, vf2"
                     : "=r"(result)
                     : "o"(a), "r"(_t0));
    return result;
}
inline float VU0_v4distancexz(const UMath::Vector4 &p1, const UMath::Vector4 &p2) {
    return 0.0f; // TODO
}

// Decl: Carbon: UVectorMathCPU.hpp: 359, GC MW: UVectorMathGC.hpp: 279, PS2 MW: UVectorMath.hpp: 1571
inline float VU0_v3lengthxz(const UMath::Vector3 &a) {
    u_long128 _t0;
    float result;
    asm __volatile__("lui %2, 0x3f80\n"
                     "lqc2 vf1, %1\n"
                     "vmul.xz vf2, vf1, vf1\n"
                     "qmtc2.ni %2, vf3\n"
                     "vmulax.x ACC, vf1, vf1x\n"
                     "vmaddz.x vf2, vf3, vf2z\n"
                     "vsqrt Q, vf2x\n"
                     "vwaitq\n"
                     "vaddq.x vf3, vf0, Q\n"
                     "qmfc2.ni %0, vf3"
                     : "=r"(result)
                     : "o"(a), "r"(_t0));
    return result;
}

// UMath.h la declara y en PS2 no tenia cuerpo: salia como una llamada.  Eleva al
// cuadrado las cuatro componentes (vmul sin mascara) y suma solo x y z.
inline float VU0_v4lengthxz(const UMath::Vector4 &a) {
    u_long128 _t0;
    float result;
    asm __volatile__("lui %2, 0x3f80\n"
                     "lqc2 vf1, %1\n"
                     "vmul.xyzw vf2, vf1, vf1\n"
                     "qmtc2.ni %2, vf3\n"
                     "vmulax.x ACC, vf1, vf1x\n"
                     "vmaddz.x vf2, vf3, vf2z\n"
                     "vsqrt Q, vf2x\n"
                     "vwaitq\n"
                     "vaddq.x vf3, vf0, Q\n"
                     "qmfc2.ni %0, vf3"
                     : "=r"(result)
                     : "o"(a), "r"(_t0));
    return result;
}

// Decl: Carbon: UVectorMathCPU.hpp: 364, GC MW: UVectorMathGC.hpp: 284, PS2 MW: UVectorMath.hpp: TODO
inline float VU0_v4length(const UMath::Vector4 &a) {
    u_long128 _t0;
    float result;
    asm __volatile__("lui %2, 0x3f80\n"
                     "lqc2 vf1, %1\n"
                     "vmul.xyzw vf2, vf1, vf1\n"
                     "qmtc2.ni %2, vf3\n"
                     "vmulax.x ACC, vf1, vf1x\n"
                     "vmadday.x ACC, vf3, vf2y\n"
                     "vmaddaz.x ACC, vf3, vf2z\n"
                     "vmaddw.x vf2, vf3, vf2w\n"
                     "vsqrt Q, vf2x\n"
                     "vwaitq\n"
                     "vaddq.x vf3, vf0, Q\n"
                     "qmfc2.ni %0, vf3"
                     : "=r"(result)
                     : "o"(a), "r"(_t0));
    return result;
}

// Decl: Carbon: UVectorMathCPU.hpp: 374, GC MW: UVectorMathGC.hpp: 294, PS2 MW: UVectorMath.hpp: 1653
// El bloque mas repetido de todo el juego: 123 apariciones entre esta y la
// gemela de Vector3.  Componerlo con VU0_sqrt obliga a sacar el valor de VU0 y
// volverlo a meter (qmfc2, mtc1, swc1) y el original no sale.
inline float VU0_v4lengthxyz(const UMath::Vector4 &a) {
    u_long128 _t0;
    float result;
    asm __volatile__("lui %2, 0x3f80\n"
                     "lqc2 vf1, %1\n"
                     "vmul.xyz vf2, vf1, vf1\n"
                     "qmtc2.ni %2, vf3\n"
                     "vmulax.x ACC, vf1, vf1x\n"
                     "vmadday.x ACC, vf3, vf2y\n"
                     "vmaddz.x vf2, vf3, vf2z\n"
                     "vsqrt Q, vf2x\n"
                     "vwaitq\n"
                     "vaddq.x vf3, vf0, Q\n"
                     "qmfc2.ni %0, vf3"
                     : "=r"(result)
                     : "o"(a), "r"(_t0));
    return result;
}

// Decl: 1812
inline void VU0_MATRIX4_vect3mult(const UMath::Vector3 &v, const UMath::Matrix4 &m, UMath::Vector3 &result) {
    asm __volatile__("lqc2 vf1, %1\n"
                     "lqc2 vf2, 0x0(%2)\n"
                     "lqc2 vf3, 0x10(%2)\n"
                     "lqc2 vf4, 0x20(%2)\n"
                     "lqc2 vf5, 0x30(%2)\n"
                     // El DESTINO, no la entrada: `vmaddw.xyz' solo escribe
                     // xyz y la w tiene que venir de donde se va a guardar.
                     // Estaba puesto %1 y el original carga 0xe0(sp), que es
                     // el mismo sitio del `sqc2' de abajo.
                     "lqc2 vf6, %0\n"
                     "vmulax.xyz ACC, vf2, vf1x\n"
                     "vmadday.xyz ACC, vf3, vf1y\n"
                     "vmaddaz.xyz ACC, vf4, vf1z\n"
                     "vmaddw.xyz vf6, vf5, vf0w\n"
                     "sqc2 vf6, %0"
                     : "=o"(result)
                     : "o"(v), "r"(&m));
}

// Decl: 1968
inline void VU0_MATRIX3x4_vect3mult(const UMath::Vector3 &v, const UMath::Matrix4 &m, UMath::Vector3 &result) {
    asm __volatile__("lqc2 vf1, %1\n"
                     "lqc2 vf2, 0x0(%2)\n"
                     "lqc2 vf3, 0x10(%2)\n"
                     "lqc2 vf4, 0x20(%2)\n"
                     "lqc2 vf5, %0\n"
                     "vmulax.xyz ACC, vf2, vf1x\n"
                     "vmadday.xyz ACC, vf3, vf1y\n"
                     "vmaddz.xyz vf5, vf4, vf1z\n"
                     "sqc2 vf5, %0"
                     : "=o"(result)
                     : "o"(v), "r"(&m));
}

// La de Vector3 con Vector4 (lineas 1973-2003 del original); sin cuerpo en PS2
// salia como una llamada.
inline void VU0_MATRIX3x4_vect4mult(const UMath::Vector4 &v, const UMath::Matrix4 &m, UMath::Vector4 &result) {
    asm __volatile__("lqc2 vf1, %1\n"
                     "lqc2 vf2, 0x0(%2)\n"
                     "lqc2 vf3, 0x10(%2)\n"
                     "lqc2 vf4, 0x20(%2)\n"
                     "lqc2 vf5, %0\n"
                     "vmulax.xyz ACC, vf2, vf1x\n"
                     "vmadday.xyz ACC, vf3, vf1y\n"
                     "vmaddz.xyz vf5, vf4, vf1z\n"
                     "sqc2 vf5, %0"
                     : "=o"(result)
                     : "o"(v), "r"(&m));
}

// Decl: Carbon: UVectorMathCPU.hpp: 506, GC MW: UVectorMathGC.hpp: 92, PS2 MW: UVectorMath.hpp: 2061
// Con MMI: carga las cuatro filas antes de guardar ninguna, asi que vale
// tambien cuando `m' y `result' son la misma matriz.
inline void VU0_MATRIX4_transpose(const UMath::Matrix4 &m, UMath::Matrix4 &result) {
    u_long128 _t0;
    u_long128 _t1;
    u_long128 _t2;
    u_long128 _t3;
    u_long128 _t4;
    u_long128 _t5;
    u_long128 _t6;
    u_long128 _t7;
    asm __volatile__("lq %0, 0x0(%8)\n"
                     "lq %1, 0x10(%8)\n"
                     "lq %2, 0x20(%8)\n"
                     "lq %3, 0x30(%8)\n"
                     "pextlw %4, %1, %0\n"
                     "pextuw %5, %1, %0\n"
                     "pextlw %6, %3, %2\n"
                     "pextuw %7, %3, %2\n"
                     "pcpyld %0, %6, %4\n"
                     "pcpyud %1, %4, %6\n"
                     "pcpyld %2, %7, %5\n"
                     "pcpyud %3, %5, %7\n"
                     "sq %0, 0x0(%9)\n"
                     "sq %1, 0x10(%9)\n"
                     "sq %2, 0x20(%9)\n"
                     "sq %3, 0x30(%9)"
                     :
                     : "r"(_t0), "r"(_t1), "r"(_t2), "r"(_t3), "r"(_t4), "r"(_t5), "r"(_t6), "r"(_t7), "r"(&m),
                       "r"(&result)
                     : "memory");
}

// Decl: Carbon: UVectorMathCPU.hpp: 530, GC MW: UVectorMathGC.hpp: 115, PS2 MW: UVectorMath.hpp: 2086
// El conjugado: 0 - q en xyz, la w se queda.
inline void VU0_qtranspose(const RQUAT &a, RQUAT &result) {
    asm __volatile__("lqc2 vf1, %1\n"
                     "vsub.xyz vf1, vf0, vf1\n"
                     "sqc2 vf1, %0"
                     : "=o"(result)
                     : "o"(a));
}

// Decl: Carbon: UVectorMathCPU.hpp: 584, GC MW: UVectorMathGC.hpp: 169, PS2 MW: UVectorMath.hpp: 2192-2210
// La ultima fila es vf0 (0, 0, 0, 1) y el resto se escribe palabra a palabra,
// con `sd $0' para los ceros que van juntos.
inline void VU0_MATRIX4Init(UMath::Matrix4 &dest, const float xx, const float yy, const float zz) {
    asm __volatile__("sqc2 vf0, 0x30+%0\n"
                     "swc1 %1, 0x0+%0\n"
                     "sw $0, 0x4+%0\n"
                     "sd $0, 0x8+%0\n"
                     "sw $0, 0x10+%0\n"
                     "swc1 %2, 0x14+%0\n"
                     "sd $0, 0x18+%0\n"
                     "sd $0, 0x20+%0\n"
                     "swc1 %3, 0x28+%0\n"
                     "sw $0, 0x2c+%0"
                     : "=o"(dest)
                     : "f"(xx), "f"(yy), "f"(zz));
}

// Decl: Carbon: UVectorMathCPU.hpp: 607, GC MW: UVectorMathGC.hpp: 193, PS2 MW: UVectorMath.hpp: 2234
// Estaba vacia y la copia se perdia: 128 bits por un registro, como
// VU0_MATRIX4Copy.
inline void VU0_v4Copy(const UMath::Vector4 &a, UMath::Vector4 &b) {
    u_long128 _t0;
    asm __volatile__("lq %0, 0x0(%1)\n"
                     "sq %0, %2"
                     :
                     : "r"(_t0), "r"(&a), "o"(b));
}

// Decl: Carbon: UVectorMathCPU.hpp: 612, GC MW: UVectorMathGC.hpp: 198, PS2 MW: UVectorMath.hpp: 2268
// UN operando de memoria para la matriz entera y el desplazamiento de cada
// fila en el texto, que es para lo que sirve "o": en el original las cuatro
// filas salen con la misma base (sq s4, 0x90(s0)); con un operando por fila
// GCC reescribe las tres ultimas con otro registro.  Igual en las dos
// VU0_MATRIX4Init.
inline void VU0_MATRIX4Copy(const UMath::Matrix4 &a, UMath::Matrix4 &b) {
    u_long128 _t0;
    u_long128 _t1;
    asm __volatile__("lq %1, 0x0(%3)\n"
                     "lq %2, 0x10(%3)\n"
                     "sq %1, 0x0+%0\n"
                     "sq %2, 0x10+%0\n"
                     "lq %1, 0x20(%3)\n"
                     "lq %2, 0x30(%3)\n"
                     "sq %1, 0x20+%0\n"
                     "sq %2, 0x30+%0"
                     : "=o"(b)
                     : "r"(_t0), "r"(_t1), "r"(&a));
}

// Decl: Carbon: UVectorMathCPU.hpp: 644, GC MW: UVectorMathGC.hpp: 311, PS2 MW: UVectorMath.hpp: 2466
// v1 + (v2 - v1) * t, con `t' por un registro entero como las demas escalas.
inline void VU0_v3lerp(const UMath::Vector3 &v1, const UMath::Vector3 &v2, const float t, UMath::Vector3 &target) {
    asm __volatile__("lqc2 vf1, %1\n"
                     "lqc2 vf2, %2\n"
                     "lqc2 vf4, %0\n"
                     "vsub.xyz vf2, vf2, vf1\n"
                     "qmtc2.ni %3, vf3\n"
                     "vmulx.xyz vf2, vf2, vf3x\n"
                     "vadd.xyz vf4, vf1, vf2\n"
                     "sqc2 vf4, %0"
                     : "=o"(target)
                     : "o"(v1), "o"(v2), "r"(t));
}

// Lineas 2152-2160 del original: vf0 ya vale (0, 0, 0, 1).
inline void VU0_v4Init(UMath::Vector4 &a) {
    asm __volatile__("sqc2 vf0, %0" : "=o"(a));
}

// Lineas 2164-2188 del original: la identidad con MMI, fila a fila a partir de
// un 1,0f; los `nop' estan en el texto (el ensamblador no los pone).
inline void VU0_MATRIX4Init(UMath::Matrix4 &dest) {
    u_long128 _t0;
    asm __volatile__("paddw %1, $0, $0\n"
                     "nop\n"
                     "lui %1, 0x3f80\n"
                     "sqc2 vf0, 0x30+%0\n"
                     "nop\n"
                     "sq %1, 0x0+%0\n"
                     "pexew %1, %1\n"
                     "nop\n"
                     "sq %1, 0x20+%0\n"
                     "pexcw %1, %1\n"
                     "nop\n"
                     "sq %1, 0x10+%0"
                     : "=o"(dest)
                     : "r"(_t0));
}

// Un solo bloque, como VU0_v3unitcrossprod: el original no guarda el producto
// vectorial para volver a cargarlo (lineas 391-419 del original).
inline void VU0_v4unitcrossprodxyz(const UMath::Vector4 &a, const UMath::Vector4 &b, UMath::Vector4 &dest) {
    u_long128 _t0;
    asm __volatile__("lui %3, 0x3f80\n"
                     "lqc2 vf1, %1\n"
                     "lqc2 vf2, %2\n"
                     "lqc2 vf3, %0\n"
                     "vopmula ACC, vf1, vf2\n"
                     "vopmsub vf3, vf2, vf1\n"
                     "qmtc2.ni %3, vf5\n"
                     "vmul.xyz vf4, vf3, vf3\n"
                     "vmulax.x ACC, vf3, vf3x\n"
                     "vmadday.x ACC, vf5, vf4y\n"
                     "vmaddz.x vf4, vf5, vf4z\n"
                     "vrsqrt Q, vf0w, vf4x\n"
                     "vwaitq\n"
                     "vmulq.xyz vf3, vf3, Q\n"
                     "sqc2 vf3, %0"
                     : "=o"(dest)
                     : "o"(a), "o"(b), "r"(_t0));
}