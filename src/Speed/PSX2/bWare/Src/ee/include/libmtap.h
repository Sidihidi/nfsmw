#ifndef _LIBMTAP_H
#define _LIBMTAP_H

// libmtap de Sony (multitap): lo que engancha realcore en input/ps2/ps2_initmtap.cpp.

#ifdef __cplusplus
extern "C" {
#endif

int sceMtapInit(void);
int sceMtapPortOpen(int port);
int sceMtapPortClose(int port);

#ifdef __cplusplus
}
#endif

#endif
