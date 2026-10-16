/* criticalpath.c de PC (vp6 1.0.6): como el de GameCube y el de PS2, junta los .inl de cmn en una sola
 * unidad (speed.exe 0x806F24-0x80855B: VP6_DecodeBool a ReadHuffTokensPredictA). Las rutinas optimizadas
 * de la plataforma (IDCT/reconstruccion/filtro en MMX y SSE2, seleccionadas por CPUID en los
 * *systemdependant de PC) no tienen fuente en el arbol: aqui solo va el haz de .inl, que es C portable. */
#include <string.h>

#include "../cmn/pbdll.h"
#include "../cmn/vputil.h"

#include "../cmn/boolhuff.inl"
#include "../cmn/decodembs.inl"
#include "../cmn/decodemode.inl"
#include "../cmn/decodemv.inl"
#include "../cmn/recon.inl"
