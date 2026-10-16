// Platform_PC.cpp: el Platform.cpp de PC. La mayoria de la API comun de plataforma son funciones vacias en el
// original (el exe las trae como un `ret` suelto); PC_GetOSLanguage si lee el idioma guardado por el arranque.
#include "Speed/Indep/Src/Misc/Platform.h"
#include "Speed/Indep/bWare/Inc/bWare.hpp"
#include "Speed/Indep/bWare/Inc/bMemory.hpp"
#include "Speed/Indep/Libs/realcore/6.24.00/include/common/realcore/system/threads.h"
#include "Speed/Indep/Libs/realcore/6.24.00/include/common/realcore/system/timer.h"

#include "Speed/PC/Src/Win32Min.hpp"

#include "Speed/Indep/Src/Misc/bFile.hpp"
#include "Speed/Indep/Src/Misc/BuildRegion.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// El idioma que fija el arranque de PC (-1 mientras no se ha leido del registro/disco).
// Nombre nuestro: el exe de PC no trae simbolos de esta capa.
extern "C" int __PC_CurrentLanguage;
// La region de la misma entrada de la tabla de idiomas del arranque (0x982D24). Nombre nuestro.
extern "C" int __PC_CurrentBuildRegion;

void EnableInterrupts() {}

void InitDisplaySystem() {}

void ServicePlatform() {}

void FinishedRenderingFEngLayer() {}

int PC_GetOSLanguage() {
    if (__PC_CurrentLanguage == -1) {
        return 0;
    }
    return __PC_CurrentLanguage;
}

// La region de PC (0x6CBB40): America mientras el arranque no ha leido el idioma. BuildRegion.cpp la consulta.
BuildRegion::eBuildRegion BuildRegion::GetBuildRegion() {
    if (__PC_CurrentLanguage == -1) {
        return BUILD_REGION_AMERICA;
    }
    return static_cast<eBuildRegion>(__PC_CurrentBuildRegion);
}

// Vacia en el original (1 byte, un ret). Su contexto real (comprobacion de fugas del CRT en el apagado) no deja
// mas rastro en el exe.
void CheckedLeakDetector() {}

// main (0x666590) sale si ya hay max_instances copias de speed.exe en marcha: cuenta los procesos con ese nombre de
// ejecutable y, mientras sobren, espera un segundo y vuelve a mirar (tres veces) (0x6CBC00).
bool IsTheGameAlreadyRunning(const char *exe_name, int max_instances) {
    PROCESSENTRY32 entry = {0};
    int count = 0;
    int tries = 3;
    HANDLE snapshot = CreateToolhelp32Snapshot(2, 0); // TH32CS_SNAPPROCESS
    while (snapshot != INVALID_HANDLE_VALUE) {
        entry.dwSize = sizeof(entry);
        if (Process32First(snapshot, &entry)) {
            do {
                char name[MAX_PATH];
                char *slash = strrchr(entry.szExeFile, '\\');
                if (slash) {
                    strcpy(name, slash + 1);
                } else {
                    strcpy(name, entry.szExeFile);
                }
                if (_stricmp(exe_name, name) == 0) {
                    count++;
                }
            } while (Process32Next(snapshot, &entry));
        }
        CloseHandle(snapshot);
        tries--;
        if (count <= max_instances || tries < 0) {
            return count > max_instances;
        }
        Sleep(1000);
        count = 0;
        snapshot = CreateToolhelp32Snapshot(2, 0);
    }
    return false;
}

// La busqueda del DVD (nombres nuestros; entre parentesis, la direccion en speed.exe).
extern char PC_DiscDrive;      // (0x8F945C) letra de la unidad con el disco; empieza en 'A'
extern int PC_AnyDriveIsDisc;  // (0x8A9260) acepta cualquier unidad local y no pide el disco
void PC_SetDiscPath(char drive); // (0x6CBA30) la ruta "X:\" del disco
void PC_MountDisc();           // (0x6CB9D0) monta la ruta del disco en el sistema de ficheros

// La unidad tiene el disco si en su raiz esta bin.dat (0x6C25A0).
bool PC_DriveHasDisc(char drive) {
    char *path = new char[0x400];
    if (!path) {
        return false;
    }
    char disc_drive = PC_DiscDrive;
    PC_DiscDrive = 0;
    path[0] = drive;
    path[1] = ':';
    path[2] = '\\';
    path[3] = 0;
    strcat(path, "bin.dat");
    bool found = bFileExists(path) != 0;
    PC_DiscDrive = disc_drive;
    delete path;
    return found;
}

// Recorre las 32 unidades logicas y se queda con la primera de CD-ROM (o cualquier local, con PC_AnyDriveIsDisc)
// que tenga el disco (0x6C2620).
char PC_FindDiscDrive() {
    unsigned long bit = 1;
    char drive = 'A';
    unsigned long drives = GetLogicalDrives();
    int found = 0;
    char *path = new char[0x400];
    PC_DiscDrive = 0;
    if (!path) {
        return 0;
    }
    for (int i = 0; i < 32; i++) {
        if (drives & bit) {
            sprintf(path, "%c:\\", drive);
            unsigned int type = GetDriveTypeA(path);
            if ((type == 5 || PC_AnyDriveIsDisc) && type != 4) { // DRIVE_CDROM, DRIVE_REMOTE
                if (PC_DriveHasDisc(drive)) {
                    found = 1;
                    break;
                }
            }
        }
        drive++;
        bit <<= 1;
    }
    delete path;
    if (found) {
        PC_DiscDrive = drive;
        PC_SetDiscPath(drive);
        PC_MountDisc();
        return drive;
    }
    PC_DiscDrive = 0;
    return 0;
}

// Las rutas de la instalacion (nombres nuestros; entre parentesis, la direccion en speed.exe).
extern char PC_DiscPath[];     // (0x93DC08) "X:\" de la unidad con el disco
extern char PC_InstallPath[];  // (0x93DD0C) la carpeta de la instalacion, del registro
extern int PC_PathFlags[4];    // (0x93DE10)
extern bool PC_StreamingInstall; // (0x982C30)
void PC_ReadRegistry();        // (0x6CB680) lee la instalacion del registro
void PC_FileSysInit(void *memory, int size); // (0x7EE0CA) del middleware (realcore)

// Modo de errores, rutas de la instalacion y del disco (0x6CBA70).
void PC_InitPaths() {
    SetErrorMode(1); // SEM_FAILCRITICALERRORS
    PC_InstallPath[0] = 0;
    PC_DiscPath[0] = 0;
    PC_PathFlags[0] = 0;
    PC_PathFlags[1] = 0;
    PC_PathFlags[2] = 0;
    PC_PathFlags[3] = 0;
    PC_ReadRegistry();
    PC_DiscDrive = PC_DiscPath[0];
    PC_MountDisc();
    PC_FileSysInit(0, 0);
    char drive = PC_FindDiscDrive();
    PC_StreamingInstall = false;
    if (drive) {
        PC_DiscPath[0] = PC_DiscDrive;
        PC_DiscPath[1] = ':';
        PC_DiscPath[2] = '\\';
        PC_DiscPath[3] = 0;
    }
}

// El arranque de PC (nombres nuestros; entre parentesis, la direccion en speed.exe).
void PC_InitLibraries();       // (0x7F3A72) del middleware
bool PC_CheckDisc();           // (0x6CBB70) pide el DVD hasta que esta o se cancela
extern int PC_CommandLineS;    // (0x982C34) /s en la linea de ordenes
extern bool PC_CommandLineN;   // (0x8FAD9C) /n la apaga

// Como InitPlatform de GameCube, sin el hardware de la consola y con la linea de ordenes de PC (0x6CB400).
void InitPlatform(int argc, char **argv) {
    bWareInit();
    bMemoryInit();
    PC_InitLibraries();
    THREAD_init();
    TIMER_init(100);
    PC_InitPaths();
    if (!PC_CheckDisc()) {
        exit(0);
    }
    if (argc > 1) {
        if (bStrCmp(argv[1], "/s") == 0) {
            PC_CommandLineS = 1;
        } else if (bStrCmp(argv[1], "/n") == 0) {
            PC_CommandLineN = false;
        }
    }
}
