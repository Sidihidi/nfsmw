#ifndef REALCORE_SOURCE_FILE_PS2_HOST0_DEVICE_H
#define REALCORE_SOURCE_FILE_PS2_HOST0_DEVICE_H

// El dispositivo host0: de PS2, los ficheros del PC de desarrollo a traves del IOP (sifdev).

#pragma interface

#include "Speed/Indep/Libs/realcore/6.24.00/include/common/realcore/file/driver.h"

class Ps2HostFileDeviceDriver : public RealFile::DeviceDriver {
  public:
    Ps2HostFileDeviceDriver(const char *pName) : DeviceDriver(pName) {}

    virtual ~Ps2HostFileDeviceDriver() {}

    virtual EAFileHandle Open(const char *name, int oflags, int *pParentFileHandle);
    virtual void Close(EAFileHandle h);
    virtual uint32_t Read(EAFileHandle h, void *buf, unsigned int bufsize, DeviceDriver *ddParent,
                          EAFileHandle ddFileHandle);
    virtual uint32 Write(EAFileHandle h, const void *buf, unsigned int bufsize, DeviceDriver *ddParent,
                         EAFileHandle ddFileHandle);
    virtual uint64_t Seek(EAFileHandle h, uint64_t offset, int whence, DeviceDriver *ddParent,
                          EAFileHandle ddFileHandle);
    virtual uint64_t Getsize(EAFileHandle h);
    virtual bool Remove(const char *name);
};

extern Ps2HostFileDeviceDriver Ps2Host_fdd;

#endif
