#ifndef CODEC_COMMON_H
#define CODEC_COMMON_H

#include "type_aliases.h"

/* Version del paquete. GameCube enlaza la 1.0.6 y PS2 la 1.0.3 (el NFS.MAP de
   la A124 las saca de mw\speed\indep\libs\vp6\1.0.3\lib\ps2-sn\libvp6decodez.a);
   configure.py compila las unidades de PS2 con -DVP6_VERSION=103. */
#ifndef VP6_VERSION
#define VP6_VERSION 106
#endif

typedef UINT8 YUV_BUFFER_ENTRY;
typedef UINT8 *YUV_BUFFER_ENTRY_PTR;

typedef struct CONFIG_TYPE {
    UINT32 VideoFrameWidth;
    UINT32 VideoFrameHeight;
    INT32 YStride;
    INT32 UVStride;
    UINT32 HFragPixels;
    UINT32 VFragPixels;
    UINT32 HScale;
    UINT32 HRatio;
    UINT32 VScale;
    UINT32 VRatio;
    UINT32 ScalingMode;
    UINT32 Interlaced;
    UINT32 ExpandedFrameWidth;
    UINT32 ExpandedFrameHeight;
} CONFIG_TYPE;

typedef struct {
    INT16 x;
    INT16 y;
} MOTION_VECTOR;

typedef INT16 Q_LIST_ENTRY;

/* Puntos de entrada que cada plataforma implementa en sus *systemdependant.c */
extern void PostProcMachineSpecificConfig(UINT32 Version);
extern unsigned int VP6_GetProcessorFrequency(void);
extern void VP6_DMachineSpecificConfig(void);
extern void fillidctconstants(void);
extern void UtilMachineSpecificConfig(void);

#endif
