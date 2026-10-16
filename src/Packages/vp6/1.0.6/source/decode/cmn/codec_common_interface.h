#ifndef CODEC_COMMON_INTERFACE_H
#define CODEC_COMMON_INTERFACE_H

/* La vista del descodificador. La del postproceso (postp.h) es mas antigua y
   no tiene YBufferStart. */
typedef struct {
    int YWidth;
    int YHeight;
    int YStride;
    int UVWidth;
    int UVHeight;
    int UVStride;
    char *YBuffer;
    char *UBuffer;
    char *VBuffer;
    char *YBufferStart;
} YUV_BUFFER_CONFIG;

typedef enum {
    C_SET_KEY_FRAME,
    C_SET_FIXED_Q,
    C_SET_FIRSTPASS_FILE,
    C_SET_EXPERIMENTAL_MIN,
    C_SET_EXPERIMENTAL_MAX = 258,
    C_SET_CHECKPROTECT,
    C_SET_TESTMODE,
    C_SET_INTERNAL_SIZE,
    C_SET_RECOVERY_FRAME,
    C_SET_REFERENCEFRAME,
    C_SET_GOLDENFRAME
} C_SETTING;

typedef enum {
    MAINTAIN_ASPECT_RATIO = 0,
    SCALE_TO_FIT = 1,
    CENTER = 2,
    OTHER = 3
} SCALE_MODE;

#endif
