#ifndef _LIBDEV_H_
#define _LIBDEV_H_

// Solo lo que usa el juego de libdev (psx2/src/ecstasy/EcstasyE.cpp).

#ifdef __cplusplus
extern "C" {
#endif

void sceDevVu0Reset(void);
int sceDevVu1Pause(void);
void sceDevVu1Reset(void);

#ifdef __cplusplus
}
#endif

#endif
