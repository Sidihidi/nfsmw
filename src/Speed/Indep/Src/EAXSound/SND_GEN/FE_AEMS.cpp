
#include "Speed/Indep/Src/EAXSound/SND_GEN/FE_AEMS.h"

// gcdatos16: el systemCrc es por familia y el de la familia FE/PlayFrontEnd cambia por
// version: 0x2D07 en la A124 (bytes de su .data objetivo: 07 2d 29 26 / 07 2d 3d 1e /
// 07 2d bc 5a / 07 2d e5 0d) y 0x3332 en las demas (GOWE69 33 32 26 29 y SLUS
// 32 33 29 26 en las suyas). Solo dato: no toca codigo.
#ifdef EA_BUILD_A124
#define Csis_FE_SYSTEM_CRC 0x2D07
#else
#define Csis_FE_SYSTEM_CRC 0x3332
#endif

namespace Csis {

InterfaceId PlayCommonSampleId = {"PlayCommonSample", Csis_FE_SYSTEM_CRC, 0x2629}; // size: 0x8, Decl: 12
ClassHandle gPlayCommonSampleHandle;                                               // size: 0x10, Decl: 13

InterfaceId PlayFrontEndSampleId = {"PlayFrontEndSample", Csis_FE_SYSTEM_CRC, 0x1E3D}; // size: 0x8, Decl: 18
ClassHandle gPlayFrontEndSampleHandle;                                                 // size: 0x10, Decl: 19

#ifdef EA_BUILD_A124
// gcdatos15: la A124 tiene el bucle de frontend en lugar de PlayFrontEndSample_RS
// (FE_AEMS.h:311). Sus stabs y su .data objetivo (0x5e5f00/0x5e5f08 = zEAXSound2 .data
// +0x1520/+0x1528), tras gPlayFrontEndSampleHandle. Los crc de su InterfaceId en esa
// .data objetivo: systemCrc 0x2d07 (el de toda esta familia en la A124) e
// interfaceCrc 0x5abc.
InterfaceId PlayFrontEndSample_loopId = {"PlayFrontEndSample_loop", Csis_FE_SYSTEM_CRC, 0x5ABC};
ClassHandle gPlayFrontEndSample_loopHandle;
#endif

#ifndef EA_BUILD_A124
// La pareja RS no existe en la A124 (ningun simbolo RS en su .o): alli la sustituye la
// de PlayFrontEndSample_loop, de arriba. Sacarla tambien en A124 deja el inicializador
// dinamico de la TU con las mismas llamadas que el objetivo (_GLOBAL_$I$, medido).
InterfaceId PlayFrontEndSample_RSId = {"PlayFrontEndSample_RS", Csis_FE_SYSTEM_CRC, 0x60E7}; // size: 0x8, Decl: 21
ClassHandle gPlayFrontEndSample_RSHandle;                                                      // size: 0x10, Decl: 22
#endif

InterfaceId FEDriveOnId = {"FEDriveOn", Csis_FE_SYSTEM_CRC, 0x0DE5}; // size: 0x8, Decl: 24
ClassHandle gFEDriveOnHandle;                                       // size: 0x10, Decl: 25

// InterfaceId FX_STREAKId;                  // size: 0x8, Decl: 27
// ClassHandle gFX_STREAKHandle;             // size: 0x10, Decl: 28

}; // namespace Csis
