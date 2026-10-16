#ifndef _LIBCDVD_H
#define _LIBCDVD_H

#include <eetypes.h>

// Lo que usan de libcdvd el juego (psx2/src/rebootiop.cpp y platform.cpp) y el lector de realcore
// (file/ps2/cddvd_device.cpp).

#define SCECdINIT 0x00
#define SCECdINoD 0x01
#define SCECdEXIT 0x05

#define SCECdSpinMax 0
#define SCECdSpinNom 1

#define SCECdSecS2048 0

#define SCECdErNO 0x00

#define SCECdCD 1
#define SCECdDVD 2

#define SCECdNODISC 0x00
#define SCECdDETCT 0x01
#define SCECdDETCTCD 0x02
#define SCECdDETCTDVDS 0x03
#define SCECdDETCTDVDD 0x04
#define SCECdUNKNOWN 0x05
#define SCECdPSCD 0x10
#define SCECdPSCDDA 0x11
#define SCECdPS2CD 0x12
#define SCECdPS2CDDA 0x13
#define SCECdPS2DVD 0x14
#define SCECdCDDA 0xFD
#define SCECdDVDV 0xFE
#define SCECdIllgalMedia 0xFF

#define SCECdStatShellOpen 0x01

typedef struct {
    u_char trycount;
    u_char spindlctrl;
    u_char datapattern;
    u_char pad;
} sceCdRMode;

typedef struct {
    u_int lbn;
    u_int sectors;
    u_int radr;
} sceCdRChain;

#ifdef __cplusplus
extern "C" {
#endif

int sceCdInit(int init_mode);
int sceCdGetDiskType(void);
int sceCdMmode(int media);
int sceCdStatus(void);
int sceCdDiskReady(int mode);
int sceCdRead(u_int lbn, u_int sectors, void *buf, sceCdRMode *mode);
int sceCdReadChain(sceCdRChain *tag, sceCdRMode *mode);
int sceCdSync(int mode);
int sceCdGetError(void);
int sceCdBreak(void);

#ifdef __cplusplus
}
#endif

#endif
