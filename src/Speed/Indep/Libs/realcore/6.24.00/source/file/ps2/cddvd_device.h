#ifndef REALCORE_SOURCE_FILE_PS2_CDDVD_DEVICE_H
#define REALCORE_SOURCE_FILE_PS2_CDDVD_DEVICE_H

// El lector de CD/DVD de PS2 (cdrom0:), sobre libcdvd. Tipos de los stabs de la A124.

#include "Speed/Indep/Libs/realcore/6.24.00/include/common/realcore/file/driver.h"

class Ps2DvdFileDeviceDriver : public RealFile::DeviceDriver {
  public:
    Ps2DvdFileDeviceDriver(const char *pName) : DeviceDriver(pName) {}

    virtual ~Ps2DvdFileDeviceDriver() {}

    virtual bool Init();
    virtual void Restore();
    virtual EAFileHandle Open(const char *name, int oflags, int *pParentFileHandle);
    virtual void Close(EAFileHandle h);
    virtual uint32_t Read(EAFileHandle h, void *buf, unsigned int bufsize, DeviceDriver *ddParent,
                          EAFileHandle ddFileHandle);
    virtual uint64_t Seek(EAFileHandle h, uint64_t offset, int whence, DeviceDriver *ddParent,
                          EAFileHandle ddFileHandle);
    virtual uint64_t Getsize(EAFileHandle h);
    virtual uint64_t QueryLocation(EAFileHandle h);

  private:
    bool CheckReadAheadBuffer();
    void FillReadAheadBuffer();
};

extern Ps2DvdFileDeviceDriver Ps2Dvd_fdd;

#endif
