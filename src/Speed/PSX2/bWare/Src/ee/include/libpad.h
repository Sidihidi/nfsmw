#ifndef _LIBPAD_H
#define _LIBPAD_H

// libpad de Sony: lo que usan el juego (psx2/src/platform.cpp cierra la biblioteca de mandos antes de lanzar una demo)
// y el mando de realcore (input/ps2/ps2_pad.cpp, ps2_effect.cpp, ps2_interface.cpp).

#include <eetypes.h>

#define scePadStateDiscon 0
#define scePadStateFindPad 1
#define scePadStateFindCTP1 2
#define scePadStateExecCmd 5
#define scePadStateStable 6
#define scePadStateError 7

#define scePadReqStateComplete 0
#define scePadReqStateFaild 1
#define scePadReqStateBusy 2

#define InfoModeCurID 1
#define InfoModeCurExID 2
#define InfoModeCurExOffs 3
#define InfoModeIdTable 4

#define InfoActFunc 1
#define InfoActSub 2
#define InfoActSize 3
#define InfoActCurr 4

#ifdef __cplusplus
extern "C" {
#endif

int scePadInit(int mode);
int scePadEnd(void);
int scePadPortOpen(int port, int slot, u_long128 *addr);
int scePadPortClose(int port, int slot);
int scePadRead(int port, int slot, unsigned char *rdata);
int scePadGetState(int port, int slot);
int scePadGetReqState(int port, int slot);
int scePadInfoAct(int port, int slot, int actno, int term);
int scePadInfoMode(int port, int slot, int term, int offs);
int scePadSetMainMode(int port, int slot, int offs, int lock);
int scePadSetActDirect(int port, int slot, const unsigned char *data);
int scePadSetActAlign(int port, int slot, const unsigned char *data);
int scePadInfoPressMode(int port, int slot);
int scePadEnterPressMode(int port, int slot);

#ifdef __cplusplus
}
#endif

#endif
