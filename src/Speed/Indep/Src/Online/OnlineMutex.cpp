#include "NetworkMutex.hpp"
#if defined(EA_PLATFORM_WIN32) && ONLINE_SUPPORT
#include "Speed/PC/Src/Win32Min.hpp" // InterlockedIncrement, InterlockedDecrement
#elif defined(EA_PLATFORM_XENON) && ONLINE_SUPPORT
#define NOMINMAX // sin el min/max de windef.h: rompen std::max en VecHashMap64.h
#include <xtl.h> // InterlockedIncrement, InterlockedDecrement (lwarx/stwcx. en el EGM, 0x82573FBC)
#endif

#include "Speed/Indep/bWare/Inc/Strings.hpp"

class OnlineMutex : public NetworkMutex {
  public:
    OnlineMutex(const char *mutexname);
    ~OnlineMutex() {}

    void Lock(const char *logtext);
    void Unlock(const char *logtext);
    bool IsLocked() const;
    void ShowLogText();
    void HideLogText();

  private:
    /* 0x04 */ int32 lockCount;
    /* 0x08 */ bool logTextEnabled;
    /* 0x0c */ char mutexName[64];
};

OnlineMutex::OnlineMutex(const char *mutexname)
    : lockCount(0), //
      logTextEnabled(false) {
    mutexName[0] = '\0';
    if (mutexname) {
        bStrNCpy(mutexName, mutexname, 63);
    }
}

// PC (speed.exe, p. ej. LobbyUsers::ClearUserStats 0x791F10): el contador va con InterlockedIncrement e
// InterlockedDecrement de kernel32 (`call [__imp_...]` sobre lobbyMutex + 0x1C), dentro del mutex de RealSystem.
void OnlineMutex::Lock(const char *logtext) {
    NetworkMutex::Lock();
#if (defined(EA_PLATFORM_WIN32) || defined(EA_PLATFORM_XENON)) && ONLINE_SUPPORT
    InterlockedIncrement(reinterpret_cast<volatile long *>(&lockCount));
#else
    volatile int32 &count = lockCount;
    count++;
#endif
}

void OnlineMutex::Unlock(const char *logtext) {
#if (defined(EA_PLATFORM_WIN32) || defined(EA_PLATFORM_XENON)) && ONLINE_SUPPORT
    InterlockedDecrement(reinterpret_cast<volatile long *>(&lockCount));
#else
    volatile int32 &count = lockCount;
    count--;
#endif
    NetworkMutex::Unlock();
}

bool OnlineMutex::IsLocked() const { return lockCount > 0; }
