#ifndef PSX2_ONLINE_PLATFORMNETWORKCORE_HPP
#define PSX2_ONLINE_PLATFORMNETWORKCORE_HPP

#ifdef EA_PRAGMA_ONCE_SUPPORTED
#pragma once
#endif

#include <types.h>

// DoWithMainMemoryStack: UIOnlineLogin::DoISPProcessing la llama con NetworkCore::StaticInit.
#include "Speed/Indep/Src/Misc/Main.hpp"

extern "C" {
int32 NetConnStatus(int32 kind, int32 data, void *buffer);
}

class PlatformNetworkCore {
  public:
    PlatformNetworkCore();
    virtual ~PlatformNetworkCore();

    bool IsOnline();
    void ExecNetGui();

  protected:
    bool Init();
    void Destroy();

  private:
    void LoadIRXs();
    void PrepareForExec();
    char *FilenameFromSlus();
};

#endif
