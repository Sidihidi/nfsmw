#ifndef PC_WIN32MIN_HPP
#define PC_WIN32MIN_HPP

// Lo minimo de la API de Win32 (kernel32, user32 y winmm) que usan las fuentes de PC, declarado a mano para no copiar cabeceras de
// Microsoft al arbol (como D3D9Min.hpp). El original incluia <windows.h>: el exe llama a todas por la tabla de
// importaciones (`call [__imp_X]`), de ahi el dllimport. WINBASEAPI y WINAPI se pueden definir antes, como en
// <windows.h>, para compilar estas fuentes en otra plataforma.

#ifndef EA_PLATFORM_WIN32
#error "Win32Min.hpp solo es de PC"
#endif

#ifndef WINBASEAPI
#define WINBASEAPI __declspec(dllimport)
#endif
#ifndef WINAPI
#define WINAPI __stdcall
#endif

typedef int BOOL;
typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned long DWORD;
typedef void *HANDLE;
typedef char *LPSTR;
typedef const char *LPCSTR;

typedef union _LARGE_INTEGER {
    struct {
        DWORD LowPart;
        long HighPart;
    };
    __int64 QuadPart;
} LARGE_INTEGER;

typedef struct _STARTUPINFOA {
    DWORD cb;
    LPSTR lpReserved;
    LPSTR lpDesktop;
    LPSTR lpTitle;
    DWORD dwX;
    DWORD dwY;
    DWORD dwXSize;
    DWORD dwYSize;
    DWORD dwXCountChars;
    DWORD dwYCountChars;
    DWORD dwFillAttribute;
    DWORD dwFlags;
    WORD wShowWindow;
    WORD cbReserved2;
    BYTE *lpReserved2;
    HANDLE hStdInput;
    HANDLE hStdOutput;
    HANDLE hStdError;
} STARTUPINFOA;

typedef struct _PROCESS_INFORMATION {
    HANDLE hProcess;
    HANDLE hThread;
    DWORD dwProcessId;
    DWORD dwThreadId;
} PROCESS_INFORMATION;

typedef struct _SECURITY_ATTRIBUTES SECURITY_ATTRIBUTES;
typedef struct HWND__ *HWND;
typedef long HRESULT;

typedef struct _FILETIME {
    DWORD dwLowDateTime;
    DWORD dwHighDateTime;
} FILETIME;

#ifndef MAX_PATH
#define MAX_PATH 260
#endif

// 0x140 B (MemoryCardImp::BootupCheckDone de PC lo lleva en la pila detras de su ruta de 0x100).
typedef struct _WIN32_FIND_DATAA {
    DWORD dwFileAttributes;
    FILETIME ftCreationTime;
    FILETIME ftLastAccessTime;
    FILETIME ftLastWriteTime;
    DWORD nFileSizeHigh;
    DWORD nFileSizeLow;
    DWORD dwReserved0;
    DWORD dwReserved1;
    char cFileName[MAX_PATH];
    char cAlternateFileName[14];
} WIN32_FIND_DATAA;
typedef DWORD(WINAPI *LPTHREAD_START_ROUTINE)(void *lpThreadParameter);
// tlhelp32.h
typedef struct tagPROCESSENTRY32 {
    DWORD dwSize;
    DWORD cntUsage;
    DWORD th32ProcessID;
    unsigned long th32DefaultHeapID;
    DWORD th32ModuleID;
    DWORD cntThreads;
    DWORD th32ParentProcessID;
    long pcPriClassBase;
    DWORD dwFlags;
    char szExeFile[MAX_PATH];
} PROCESSENTRY32;

#ifndef FALSE
#define FALSE 0
#endif
#ifndef INFINITE
#define INFINITE 0xFFFFFFFF
#endif
#ifndef THREAD_PRIORITY_TIME_CRITICAL
#define THREAD_PRIORITY_TIME_CRITICAL 15
#endif
#ifndef WINUSERAPI
#define WINUSERAPI __declspec(dllimport)
#endif
#ifndef INVALID_HANDLE_VALUE
#define INVALID_HANDLE_VALUE ((HANDLE)(long)-1)
#endif
// shlobj.h / shfolder.h
#ifndef CSIDL_PERSONAL
#define CSIDL_PERSONAL 0x0005
#endif
#ifndef CSIDL_FLAG_CREATE
#define CSIDL_FLAG_CREATE 0x8000
#endif
#ifndef CSIDL_LOCAL_APPDATA
#define CSIDL_LOCAL_APPDATA 0x001c
#endif
#ifndef SHFOLDERAPI
#define SHFOLDERAPI __declspec(dllimport) HRESULT __stdcall
#endif
#ifndef WINMMAPI
#define WINMMAPI __declspec(dllimport)
#endif

// GetVersionExA (la deteccion de CPU de CODA, coda.cpp)
#define VER_PLATFORM_WIN32_WINDOWS 1
typedef struct _OSVERSIONINFOA {
    DWORD dwOSVersionInfoSize;
    DWORD dwMajorVersion;
    DWORD dwMinorVersion;
    DWORD dwBuildNumber;
    DWORD dwPlatformId;
    char szCSDVersion[128];
} OSVERSIONINFOA;

extern "C" {
WINBASEAPI BOOL WINAPI QueryPerformanceCounter(LARGE_INTEGER *lpPerformanceCount);
WINBASEAPI BOOL WINAPI QueryPerformanceFrequency(LARGE_INTEGER *lpFrequency);
WINBASEAPI void WINAPI OutputDebugStringA(LPCSTR lpOutputString);
WINBASEAPI BOOL WINAPI IsBadReadPtr(const void *lp, unsigned int ucb);
WINBASEAPI BOOL WINAPI CreateProcessA(LPCSTR lpApplicationName, LPSTR lpCommandLine,
                                      SECURITY_ATTRIBUTES *lpProcessAttributes, SECURITY_ATTRIBUTES *lpThreadAttributes,
                                      BOOL bInheritHandles, DWORD dwCreationFlags, void *lpEnvironment,
                                      LPCSTR lpCurrentDirectory, STARTUPINFOA *lpStartupInfo,
                                      PROCESS_INFORMATION *lpProcessInformation);
WINBASEAPI DWORD WINAPI GetLastError();
WINBASEAPI HANDLE WINAPI CreateMutexA(SECURITY_ATTRIBUTES *lpMutexAttributes, BOOL bInitialOwner, LPCSTR lpName);
WINBASEAPI BOOL WINAPI ReleaseMutex(HANDLE hMutex);
WINBASEAPI DWORD WINAPI WaitForSingleObject(HANDLE hHandle, DWORD dwMilliseconds);
WINBASEAPI HANDLE WINAPI CreateThread(SECURITY_ATTRIBUTES *lpThreadAttributes, unsigned long dwStackSize,
                                      LPTHREAD_START_ROUTINE lpStartAddress, void *lpParameter, DWORD dwCreationFlags,
                                      DWORD *lpThreadId);
WINBASEAPI BOOL WINAPI SetThreadPriority(HANDLE hThread, int nPriority);
WINBASEAPI void WINAPI Sleep(DWORD dwMilliseconds);
WINBASEAPI BOOL WINAPI CloseHandle(HANDLE hObject);
WINBASEAPI DWORD WINAPI GetLogicalDrives();
WINBASEAPI unsigned int WINAPI SetErrorMode(unsigned int uMode);
WINBASEAPI unsigned int WINAPI GetDriveTypeA(LPCSTR lpRootPathName);
// tlhelp32 (kernel32, sin dllimport en tlhelp32.h: el exe las llama por su salto)
HANDLE WINAPI CreateToolhelp32Snapshot(DWORD dwFlags, DWORD th32ProcessID);
BOOL WINAPI Process32First(HANDLE hSnapshot, PROCESSENTRY32 *lppe);
BOOL WINAPI Process32Next(HANDLE hSnapshot, PROCESSENTRY32 *lppe);
WINBASEAPI BOOL WINAPI CreateDirectoryA(LPCSTR lpPathName, SECURITY_ATTRIBUTES *lpSecurityAttributes);
WINBASEAPI HANDLE WINAPI FindFirstFileA(LPCSTR lpFileName, WIN32_FIND_DATAA *lpFindFileData);
WINBASEAPI BOOL WINAPI FindNextFileA(HANDLE hFindFile, WIN32_FIND_DATAA *lpFindFileData);
WINBASEAPI BOOL WINAPI FindClose(HANDLE hFindFile);
WINBASEAPI BOOL WINAPI GetVersionExA(OSVERSIONINFOA *lpVersionInformation);
// shfolder (SHFOLDER.dll)
SHFOLDERAPI SHGetFolderPathA(HWND hwnd, int csidl, HANDLE hToken, DWORD dwFlags, LPSTR pszPath);
// user32
WINUSERAPI HWND WINAPI GetForegroundWindow();
WINUSERAPI HWND WINAPI GetDesktopWindow();
WINUSERAPI int WINAPI MessageBoxA(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, unsigned int uType);
// winmm (mmsystem.h)
WINMMAPI DWORD WINAPI timeGetTime();
WINMMAPI unsigned int WINAPI timeBeginPeriod(unsigned int uPeriod);
WINMMAPI unsigned int WINAPI timeEndPeriod(unsigned int uPeriod);
}

#endif
