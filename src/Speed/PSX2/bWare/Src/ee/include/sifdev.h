#ifndef _SIFDEV_H
#define _SIFDEV_H

// Solo lo que usa el juego de sifdev (psx2/src/rebootiop.cpp): ficheros del IOP, reinicio del IOP y carga de
// modulos; la memoria libre del IOP que MainLoop de la A124 (misc/Main.cpp) muestra con el menu de depuracion; y la
// busqueda de un modulo cargado con que realmemcard comprueba los suyos (ps2_interface.cpp); y el control de un
// dispositivo con que psx2/src/platform.cpp para dev9 antes de lanzar una demo. Y el resto de la E/S de ficheros que usa
// el dispositivo host0: de realcore (file/ps2/host0_device.cpp).

#define SCE_RDONLY 0x0001
#define SCE_WRONLY 0x0002
#define SCE_RDWR 0x0003
#define SCE_CREAT 0x0200
#define SCE_TRUNC 0x0400

#define SCE_SEEK_SET 0
#define SCE_SEEK_CUR 1
#define SCE_SEEK_END 2

#ifdef __cplusplus
extern "C" {
#endif

int sceOpen(const char *filename, int flag, ...);
int sceClose(int fd);
int sceRead(int fd, void *buf, int nbyte);
int sceWrite(int fd, const void *buf, int nbyte);
int sceLseek(int fd, int offset, int where);
int sceRemove(const char *filename);
int sceFsReset(void);

int sceSifRebootIop(const char *img);
int sceSifSyncIop(void);

int sceSifLoadModule(const char *filename, int args, const char *argp);
int sceSifLoadModuleBuffer(void *addr, int args, const char *argp);
int sceSifSearchModuleByName(const char *modulename);

int sceDevctl(const char *devname, int cmd, const void *arg, unsigned int arglen, void *bufp, unsigned int buflen);

int sceSifInitIopHeap(void);
void *sceSifAllocSysMemory(int type, int size, void *addr);
int sceSifFreeSysMemory(void *addr);
unsigned int sceSifQueryMemSize(void);
unsigned int sceSifQueryMaxFreeMemSize(void);
unsigned int sceSifQueryTotalFreeMemSize(void);

#ifdef __cplusplus
}
#endif

#endif
