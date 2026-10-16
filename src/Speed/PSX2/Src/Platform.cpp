#include "Speed/Indep/Src/Frontend/Localization/Localize.hpp"
#include "Speed/Indep/Src/Misc/BuildRegion.hpp"
#include "Speed/Indep/Src/Misc/Timer.hpp"
#include "Speed/Indep/Src/Ecstasy/EcstasyE.hpp"
#include "Speed/Indep/Src/World/OnlineManager.hpp"
#include "Speed/Indep/bWare/Inc/Strings.hpp"
#include "Speed/Indep/bWare/Inc/bCodeineFile.hpp"
#include "Speed/Indep/bWare/Inc/bDebug.hpp"
#include "Speed/Indep/bWare/Inc/bFunk.hpp"
#include "Speed/Indep/bWare/Inc/bMemory.hpp"
#include "Speed/Indep/bWare/Inc/bPrintf.hpp"
#include "Speed/Indep/Libs/realcore/6.24.00/include/common/realcore/file.h"
#include "Speed/Indep/Libs/realcore/6.24.00/include/common/realcore/system/systask.h"
#include "Speed/Indep/Libs/realcore/6.24.00/include/common/realcore/system/threads.h"
#include "Speed/Indep/Libs/realcore/6.24.00/include/common/realcore/system/timer.h"
#include "Speed/Indep/Libs/snd/9/source/library/ps2/sndps2.h"
#include "Speed/PSX2/Src/Ecstasy/EcstasyE.hpp"
#include "Speed/PSX2/Src/Logitech/liblgdev.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <eekernel.h>
#include <eeregs.h>
#include <libcdvd.h>
#include <libdma.h>
#include <libgraph.h>
#include <libpad.h>
#include <libscf.h>
#include <sifcmd.h>
#include <sifdev.h>

// total size: 0x8
struct PS2LangsToOurLangs {
    int PS2Language;        // offset 0x0, size 0x4
    eLanguages OurLanguage; // offset 0x4, size 0x4
};

int Timer0InterruptCnt1;
int g_timeHandler = -1;
int DoMonitorServiceInVBlank;
int StuckRealLoopCounter;
int StuckFrameCounter;
#ifndef EA_BUILD_A124
// Solo SLUS: MyCDVD_watchdogtimeout la trata como una bandeja abierta. Nadie mas la lee ni la escribe (se cambia desde
// el depurador), asi que el nombre no sale de ningun sitio y es provisional.
bool ForceDiscTrayOpen;
#endif
bool SndPlatformInitHasBeenCalled;
bool AlreadyCalledClosePlatform;

PS2LangsToOurLangs LangConversionTable[8] = {
    {SCE_ENGLISH_LANGUAGE, eLANGUAGE_ENGLISH}, {SCE_FRENCH_LANGUAGE, eLANGUAGE_FRENCH},
    {SCE_GERMAN_LANGUAGE, eLANGUAGE_GERMAN},   {SCE_ITALIAN_LANGUAGE, eLANGUAGE_ITALIAN},
    {SCE_SPANISH_LANGUAGE, eLANGUAGE_SPANISH}, {SCE_DUTCH_LANGUAGE, eLANGUAGE_DUTCH},
    {SCE_JAPANESE_LANGUAGE, eLANGUAGE_ENGLISH}, {SCE_PORTUGUESE_LANGUAGE, eLANGUAGE_ENGLISH},
};

// El float a entero de libgcc (fptosi/fptoui de fp-bit) es lento: al arrancar se copian encima estas dos rutinas, que
// convierten con la FPU. Son asm de EA: no tienen procedimiento en el .mdebug de la A124 (una funcion de C lo tiene) y
// HackPatchFloatToInt las copia como datos, 16 bytes cada una.
extern "C" {
extern unsigned long _HackFpToSi[2];
extern unsigned long _HackFpToUi[2];
extern unsigned long fptosi[2];
extern unsigned long fptoui[2];

void HackPatchFloatToInt();
}

asm(".section .text\n"
    "\t.set noreorder\n"
    "\t.align 3\n"
    "\t.globl _HackFpToSi\n"
    "\t.type _HackFpToSi, @function\n"
    "_HackFpToSi:\n"
    "\tcvt.w.s $f12, $f12\n"
    "\tmfc1 $2, $f12\n"
    "\tjr $31\n"
    "\tnop\n"
    "\t.size _HackFpToSi, . - _HackFpToSi\n"
    "\t.align 3\n"
    "\t.globl _HackFpToUi\n"
    "\t.type _HackFpToUi, @function\n"
    "_HackFpToUi:\n"
    "\tcvt.w.s $f12, $f12\n"
    "\tmfc1 $2, $f12\n"
    "\tjr $31\n"
    "\tnop\n"
    "\t.size _HackFpToUi, . - _HackFpToUi\n"
    "\t.set reorder\n"
    "\t.previous\n");

void HackPatchFloatToInt() {
    memcpy(fptosi, _HackFpToSi, sizeof(fptosi));
    memcpy(fptoui, _HackFpToUi, sizeof(fptoui));
    FlushCache(0);
}

void abort() {
    bBreak();
    while (1) {
    }
}

void RemoveTimerInterrupt() {
    if (g_timeHandler != -1) {
        RemoveIntcHandler(INTC_TIM0, g_timeHandler);
    }
}

void EnableInterrupts() {
    EIntr();
}

int PSX2VBlank(int param) {
    LastFrameCounterTick = bGetTicker();

    if (DoMonitorServiceInVBlank) {
        bMonitorService();
    }

    eVblCallback();

    FrameCounter++;
    ExitHandler();
    return 0;
}

void RegisterPSX2VBlankHandler() {
    sceGsSyncVCallback(PSX2VBlank);
}

void UnregisterPSX2VBlankHandler() {
    sceGsSyncVCallback(0);
}

// La tarea de realcore que vigila el lector, cambiada por la nuestra: si hay que reiniciarlo (se abrio la bandeja),
// espera a que la cierren antes de reiniciar el lector y vuelve a registrar el apagado.
void MyCDVD_watchdogtimeout(void *, int) {
#ifdef EA_BUILD_A124
    if (CDVD_ResetNeeded()) {
        if (TheOnlineManager.IsOnlineRace()) {
            TheOnlineManager.NotifyDiscEjected();
        }
#else
    if (CDVD_ResetNeeded() || ForceDiscTrayOpen) {
#endif
        bool tray_open = sceCdStatus() == SCECdStatShellOpen;

#ifdef EA_BUILD_A124
        if (tray_open) {
            while (sceCdStatus() == SCECdStatShellOpen) {
#else
        if (tray_open || ForceDiscTrayOpen) {
            while (sceCdStatus() == SCECdStatShellOpen || ForceDiscTrayOpen) {
#endif
                THREAD_yield(1);
            }
        }

        CDVD_resettask();
        ReregisterPowerOffHandler();
    }
}

void InitRealFile() {
    FILESYSOPTS fso;

    fso.size = sizeof(fso);
    FILE_getopts(&fso);
    fso.allocator = &gMemoryAllocator;

    if (!bIsPlatSuperBender()) {
        fso.DemoCdRoot = "NFS";
    }

    fso.bUseTMFSRootDirAsAppRootDir = true;
    fso.MaxDevices = 6;
    fso.nSearchLocs = 3;
    fso.MaxOpenFiles = 24;
    fso.MaxFileOps = 64;
    fso.nSearchPathLength = 80;
    fso.ThreadStackSize = 0x4000;
    fso.DiscType = 0;

    if (bIsThereACDInTheDrive()) {
        fso.DiscType = bIsThereACDInTheDrive();
        fso.bUseTMFSRootDirAsAppRootDir = false;
    }

    FILE_setopts(&fso);
    FILE_init(0, 0);

    if (bIsPlatSuperBender()) {
        AddCodeineDeviceDriver();
    }

    SYNCTASK_del(CDVD_watchdogtimeout);
    SYNCTASK_add(MyCDVD_watchdogtimeout, 0, 0, 0);
}

void FlushCaches() {
    FlushCache(0);
    FlushCache(2);
}

// Para los canales de DMA y la GIF antes de devolver el control (reinicio de SuperBender o lanzar una demo).
void ClosePlatform() {
    if (AlreadyCalledClosePlatform) {
        return;
    }

    AlreadyCalledClosePlatform = true;

    *D1_CHCR = 5;
    *VIF1_FBRST = 9;
    *GIF_CTRL = 1;

    UnregisterPSX2VBlankHandler();

    if (SndPlatformInitHasBeenCalled) {
        SNDPLATFORM_restore(WARM);
    }
}

void HandleSuperBenderSoftReboot() {
    ClosePlatform();

    const char *executable_name = "cdrom0:\\NFSBOOT.ELF;1";
    if (bIsPlatDTL_T10000()) {
        executable_name = "host0:NFSBOOT.ELF";
    }

    SuperBenderSoftReboot(executable_name);
}

void InitPlatform(int argc, char **argv) {
    bool reboot_iop;
    bool reconnect_super_bender;
    bool super_bender_out_of_date;

    HackPatchFloatToInt();
    bWareInit();

    bMemoryInit();
    bCountFreeMemory(0);

    scePrintf("main() argc = %d\n", argc);
    for (int n = 0; n < argc; n++) {
        scePrintf("    %d : '%s'\n", n, argv[n]);
    }

    super_bender_out_of_date = false;
    if (argc >= 2 && bStrCmp(argv[1], "SBReboot271_V1") == 0) {
        super_bender_out_of_date = true;
        reboot_iop = false;
        reconnect_super_bender = true;
    } else if (argc >= 2 && bStrCmp(argv[1], "SBReboot281") == 0) {
        reboot_iop = false;
        reconnect_super_bender = true;
    } else {
        reboot_iop = true;
        reconnect_super_bender = false;
    }

    if (reboot_iop) {
        RebootIOP();
        LoadStartupIRXModules();
    } else {
        InitializeDVD();
    }

    THREAD_init();

    SetVideoMode(GetBuildRegionVideoMode());
    if (GetVideoMode() == MODE_PAL) {
        TIMER_setvideomode(2);
    } else {
        TIMER_setvideomode(1);
    }
    TIMER_init(100);

    {
        char network_settings_name[32];

        if (reconnect_super_bender) {
            SuperBenderReconnect(SB_DEBUGGER_LEVEL_FULL);
            SuperBenderSetSoftResetFunction(HandleSuperBenderSoftReboot);
        }
    }

    if (super_bender_out_of_date) {
        bBreak();
    }

    if (bIsPlatSuperBender()) {
        bMemoryUpdateTraceInformation();
        bRefreshTweaker();
    } else {
        bMemoryUpdateTraceInformation();
    }

    InitRealFile();

    if (bIsPlatSuperBender()) {
        ReregisterPowerOffHandler();
    }

    LoadRemainingIRXModules();

#ifdef EA_BUILD_A124
    if (!bIsPlatSuperBender()) {
        LoadNetworkIRXModules(false);
    }
#endif

    InitVRAM();
}

void InitDisplaySystem() {
    eSetDisplaySystem(GetVideoMode());
}

// En America y en Europa en ingles no se mira la consola: siempre ingles. Japon va en japones. En las builds de tienda,
// las ediciones europeas de tres idiomas (ingles/italiano/espanol e ingles/frances/aleman) pasan al ingles si la
// consola esta en otro idioma. Esas comparaciones solo usan == y <: la unidad tiene stl_relops.h (UStandard.h), y con
// != o > el original de SLUS pasaria por sus plantillas.
eLanguages PS2_GetOSLanguage() {
    int ps2lang;
    eLanguages lang;

    if (BuildRegion::IsAmerica() || BuildRegion::IsEuropeEng()) {
        return eLANGUAGE_ENGLISH;
    }

    if (BuildRegion::IsJapan()) {
        return eLANGUAGE_JAPANESE;
    }

    ps2lang = sceScfGetLanguage();
    lang = eLANGUAGE_ENGLISH;

    for (unsigned int i = 0; i < 8; i++) {
        if (LangConversionTable[i].PS2Language == ps2lang) {
            lang = LangConversionTable[i].OurLanguage;
            break;
        }
    }

#ifndef EA_BUILD_A124
    if (BuildRegion::IsEuropeEngItalSpan() && !(lang == eLANGUAGE_ENGLISH || lang == eLANGUAGE_ITALIAN || lang == eLANGUAGE_SPANISH)) {
        return eLANGUAGE_ENGLISH;
    }
    if (BuildRegion::IsEuropeEngFrGer() && (lang < eLANGUAGE_ENGLISH || eLANGUAGE_GERMAN < lang)) {
        return eLANGUAGE_ENGLISH;
    }
#endif

    return lang;
}

void ServicePlatform() {}

// Deja la consola como la encuentra un ejecutable nuevo: el disco duro (dev9) apagado y la SIF cerrada.
void ShutdownForDemoLaunch() {
    eWaitUntilRenderingDone();
    ClosePlatform();
    lgDevPrepareForIOPReboot();
    sceDmaReset(1);
    scePadEnd();

    if (sceSifSearchModuleByName("dev9") >= 0) {
        while (sceDevctl("dev9x:", 0x4402, 0, 0, 0, 0) < 0) { // DDIOC_OFF
        }
    }

    sceSifExitCmd();
}

void LaunchDemo(const char *demo_elf_filename) {
    int language_id = GetDemoLanguageID(GetCurrentLanguage());
    if (language_id == -1) {
        language_id = 0;
    }

    char full_elf_filename[80];
    bSPrintf(full_elf_filename, "cdrom0:\\%s;1", demo_elf_filename);

    char str_language[10];
    char str_aspect[10];
    char str_playmode[10];
    char str_to_inactive[10];
    char str_to_gameplay[10];
    char str_mediatype[10];
    char str_mastervolscale[10];
    char str_dirsector[10];
    char str_exitwhy[10];
    char str_unknown1[32];
    char str_unknown2[10];
    char *args[11] = {str_language,  str_aspect,        str_playmode,  str_to_inactive, str_to_gameplay, str_mediatype,
                      str_mastervolscale, str_dirsector, str_exitwhy, str_unknown1,    str_unknown2};

    sprintf(str_language, "%d", language_id);
    sprintf(str_aspect, "%d", 0);
    sprintf(str_playmode, "%d", 0);
    sprintf(str_to_inactive, "%d", 9999);
    sprintf(str_to_gameplay, "%d", 9999);
    sprintf(str_mediatype, "0");
    sprintf(str_mastervolscale, "10");
    sprintf(str_dirsector, "(null)");
    sprintf(str_exitwhy, "(null)");
    sprintf(str_unknown1, "");
    sprintf(str_unknown2, "666");

    ShutdownForDemoLaunch();

    for (int n = 10; n >= 0; n--) {
    }

    LoadExecPS2(full_elf_filename, 11, args);
    bBreak();
}

void LaunchBondDemo() {
    LaunchDemo("FRWL\\FRWL.ELF");
}

void LaunchMarvelDemo() {
    LaunchDemo("MARVEL\\SLUS_000.00");
}
