/* criticalpath.c de PlayStation 2 (vp6 1.0.3): D:/env/egami/vp6/dev/source/decode/ps2/criticalpath.c en el
 * .mdebug de la A124. Como el de GameCube, junta los .inl de cmn con los de la plataforma (el filtro, la IDCT y la
 * reconstruccion en MMI). */
#include <string.h>

#include "../cmn/pbdll.h"
#include "../cmn/vputil.h"

#include "../cmn/boolhuff.inl"
#include "../cmn/decodembs.inl"
#include "../cmn/decodemode.inl"
#include "../cmn/decodemv.inl"
#include "../cmn/recon.inl"
#include "ps2filterblock.inl"
#include "ps2idct.inl"
#include "ps2reconstruct.inl"
