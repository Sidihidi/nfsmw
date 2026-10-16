// realcore 6.24.00: el dispositivo host0: de PS2 (source/file/ps2/host0_device.cpp en el .mdebug de la A124). Locales
// de sus stabs; el orden, de su mapa de lineas. El gemelo de GameCube es file/gc/hd_device.cpp.
#pragma implementation
#include "host0_device.h"

#include "Speed/Indep/Libs/realcore/6.24.00/include/common/realcore/file.h"

#include <sifdev.h>
#include <stdio.h>
#include <string.h>

extern FILESYSOPTS gFileSysOpts;

EAFileHandle Ps2HostFileDeviceDriver::Open(const char *name, int oflags, int *pParentFileHandle) {
    int hFile;
    int OpenFlags;
    char hostnamebuf[256];
    char *c;

    if (oflags & 1) {
        OpenFlags = SCE_RDONLY;
    } else {
        OpenFlags = SCE_RDWR;
    }
    if (oflags & 2) {
        OpenFlags |= SCE_CREAT;
    }
    if (oflags & 4) {
        OpenFlags |= SCE_TRUNC;
    }
    if (strncmp(name, "host0:", 6) != 0) {
        if (name[0] == '/' || name[0] == '\\') {
            sprintf(hostnamebuf, "%s%s", this->mDeviceName, name);
        } else {
            sprintf(hostnamebuf, "%s%s", this->mDeviceName, name);
        }
        name = hostnamebuf;
    }
    for (c = const_cast<char *>(name); *c != '\0'; c++) {
        if (*c == '\\') {
            *c = '/';
        }
    }
    if (gFileSysOpts.bUseTMFSRootDirAsAppRootDir == true) {
        c = strchr(name, ':');
        if (c[1] == '/') {
            for (c++; *c != '\0'; c++) {
                *c = c[1];
            }
        }
    }
    hFile = sceOpen(name, OpenFlags);
    if (hFile < 0) {
        hFile = -1;
    }
    return hFile;
}

void Ps2HostFileDeviceDriver::Close(EAFileHandle h) {
    sceClose(h);
}

uint32_t Ps2HostFileDeviceDriver::Read(EAFileHandle h, void *buf, unsigned int bufsize, DeviceDriver *ddParent,
                                       EAFileHandle ddFileHandle) {
    return sceRead(h, buf, bufsize);
}

uint32 Ps2HostFileDeviceDriver::Write(EAFileHandle h, const void *buf, unsigned int bufsize, DeviceDriver *ddParent,
                                      EAFileHandle ddFileHandle) {
    return sceWrite(h, buf, bufsize);
}

uint64_t Ps2HostFileDeviceDriver::Seek(EAFileHandle h, uint64_t offset, int whence, DeviceDriver *ddParent,
                                       EAFileHandle ddFileHandle) {
    int where;

    switch (whence) {
    case 0:
        where = SCE_SEEK_SET;
        break;
    case 1:
        where = SCE_SEEK_CUR;
        break;
    case 2:
        where = SCE_SEEK_END;
        break;
    default:
        where = SCE_SEEK_SET;
        break;
    }
    return sceLseek(h, offset, where);
}

uint64_t Ps2HostFileDeviceDriver::Getsize(EAFileHandle h) {
    int curpos = sceLseek(h, 0, SCE_SEEK_CUR);
    int size = sceLseek(h, 0, SCE_SEEK_END);

    sceLseek(h, curpos, SCE_SEEK_SET);
    return size;
}

bool Ps2HostFileDeviceDriver::Remove(const char *name) {
    return sceRemove(name) == 0;
}

Ps2HostFileDeviceDriver Ps2Host_fdd("host0:");
