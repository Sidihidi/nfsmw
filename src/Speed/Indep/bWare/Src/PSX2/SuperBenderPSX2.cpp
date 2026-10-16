#include "Speed/Indep/bWare/Inc/bWare.hpp"
#include "Speed/Indep/bWare/Inc/bDebug.hpp"
#include "Speed/Indep/bWare/Inc/bFunk.hpp"
#include "Speed/Indep/bWare/Src/PSX2/FastPacketBuffer.hpp"
#include "Speed/Indep/Libs/realcore/6.24.00/include/common/realcore/system.h"
#include "Speed/Indep/Src/Misc/LZCompress.hpp"

#include <eekernel.h>
#include <sif.h>
#include <sifrpc.h>

// indep/bware/src/psx2/SuperBenderPSX2.cpp de la A124: el monitor de red del depurador SuperBender. Habla con el PC
// por el canal de paquetes rapido del IOP (FastPacketBuffer) y, en modo nucleo, se engancha a los vectores de excepcion
// y a la llamada al sistema 99 del EE. Layouts, nombres, locales y cadenas salen de los stabs y del ejecutable.
// ASM (forense 2026-10-03, docs/analisis/2026-10-03_forense-asm.md §5-6): los ~120 bloques inline
// de este .cpp son de CLASE B — asm cuya existencia en el fuente original esta probada (instrucciones
// no-emisibles: COP0, FPU registro a registro, syscall, break) y el texto es reconstruccion.
// El modulo .s (superbenderasm.obj, 0x488 B, en el NFS.MAP como objeto del build de EA, sin
// stabs) va aparte como transcripcion declarada (clase C); este .cpp solo lo declara extern.
// SuperBender (la herramienta) es de SN Systems; la integracion (Perforce
// d:/p4_apex1666_d1001856/mw/speed/indep/bware/src/psx2/) es de EA. Va en el RETAIL (SLUS
// 0x4363EC reubicado y verificado); exclusivo de PS2 (el gemelo de GC es Codeine).

// El modulo de asm del monitor (0x47cc10 en la A124): los manejadores que se instalan en los vectores y la
// copia de HI/LO.
extern "C" {
void MySyscallExceptionHandler();
void MyExceptionHandler();
void PolitelyServiceMonitor();
void SaveMultiplyRegisters(int *multiply_registers);
void RestoreMultiplyRegisters(int *multiply_registers);
void WritebackDCache();
void InvalidateICache();
}
extern unsigned char *pMonitorStack;

void SendPacketNetMonitor(bFunkPacketHeader *header, const void *data, int data_size);
void ServiceNetMonitor();

int UseRealCommINet = 0;
int UseRealCommILink = 0;
int UseFastPacket = 1;
int UserBreakOnlyInMainThread = 1;
int SoftRebootOnlyInMainThread = 1;
eSuperBenderDebuggerLevel SuperBenderDebuggerLevel = SB_DEBUGGER_LEVEL_FULL;
unsigned int OriginalSyscallHandlerAddress = 0;
int ReceivedPacketError = 0;
int CommandLineArgumentsBuffer[128] = {-1};

int SuperBenderGetCommandLineArgc() {
    if (CommandLineArgumentsBuffer[0] == -1) {
        CommandLineArgumentsBuffer[0] = 0;
    }
    return CommandLineArgumentsBuffer[0];
}

char **SuperBenderGetCommandLineArgv() {
    int argc = SuperBenderGetCommandLineArgc();
    if (argc == 0) {
        return 0;
    }

    char *arg = reinterpret_cast<char *>(&CommandLineArgumentsBuffer[argc + 1]);

    if (CommandLineArgumentsBuffer[1] != reinterpret_cast<int>(arg)) {
        for (int n = 0; n < argc; n++) {
            int len = CommandLineArgumentsBuffer[n + 1];

            CommandLineArgumentsBuffer[n + 1] = reinterpret_cast<int>(arg);
            arg += len;
        }
    }
    return reinterpret_cast<char **>(&CommandLineArgumentsBuffer[1]);
}

static unsigned int MyGetTicker() {
    unsigned int ret;
    asm volatile("mfc0 %0, $9" : "=r"(ret));
    return ret;
}

static void DelayMilliseconds(int ms) {
    for (int n = 0; n < ms; n++) {
        unsigned int tick = bGetTicker();

        while (bGetFixTickerDifference(tick, bGetTicker()) < 0x10000) {
        }
    }
}

unsigned int DisableInterruptsByCause() {
    unsigned int irq_status = 0;

    for (int i = 0; i < 11; i++) {
        irq_status |= iDisableIntc(i) << i;
    }
    return irq_status;
}

void RestoreInterruptsByCause(unsigned int irq_status) {
    for (int i = 0; i < 11; i++) {
        if (irq_status & (1 << i)) {
            iEnableIntc(i);
        }
    }
}

void MyDisableInterrupts() {
    unsigned int sr;

    asm volatile("mfc0 %0, $12" : "=r"(sr));
    if (sr & 0x10000) {
        do {
            asm volatile(".p2align 3");
            asm volatile("di");
            asm volatile("sync.p");
            asm volatile("mfc0 %0, $12" : "=r"(sr));
        } while (sr & 0x10000);
    }
}

void MyEnableInterrupts() {
    asm volatile("ei");
}

// La llamada al sistema 99, la que HookOperatingSystem redirige a TempNetMonitorSyscall: los cuatro argumentos se
// corren un registro y el numero de funcion va en $a0.
static int NetMonitorSyscall(int function_number, int param1, int param2, int param3) {
    int ret;

    asm volatile("addiu $3, $0, 0x63\n"
                 "daddu $8, $0, $7\n"
                 "daddu $7, $0, $6\n"
                 "daddu $6, $0, $5\n"
                 "daddu $5, $0, $4\n"
                 "addiu $4, $0, 0x15\n"
                 "addiu $2, $0, 0\n"
                 "syscall\n"
                 "daddu %0, $0, $2"
                 : "=r"(ret));
    return ret;
}

struct PSX2MonitorVars {
    u_long128 GeneralPurposeRegisters[32]; // offset 0x0, size 0x200
    int TickerStart;                       // offset 0x200, size 0x4
    int TickerEnd;                         // offset 0x204, size 0x4
    int ProgramCounter;                    // offset 0x208, size 0x4
    int BreakCause;                        // offset 0x20C, size 0x4
    int MultiplyRegisters[32];             // offset 0x210, size 0x80
    int Cop0Registers[32];                 // offset 0x290, size 0x80
    float FloatRegisters[32];              // offset 0x310, size 0x80
    float FloatStatusRegister;             // offset 0x390, size 0x4
    float FloatPadding[3];                 // offset 0x394, size 0xC
    char DecompressionBuffer[24576];       // offset 0x3A0, size 0x6000
};

struct IOPMonitorVars {
    int GeneralPurposeRegisters[32]; // offset 0x0, size 0x80
    int ProgramCounter;              // offset 0x80, size 0x4
    int BreakCause;                  // offset 0x84, size 0x4
    int Cop0Registers[32];           // offset 0x88, size 0x80
};

struct SuperBenderIRXStartupInfo {
    FastPacketBuffer *pOtherSide;  // offset 0x0, size 0x4
    unsigned int Unused;           // offset 0x4, size 0x4
    char NetworkSettingsName[40];  // offset 0x8, size 0x28
};

struct SuperBenderUDPHelloPacket {
    unsigned int Magic;            // offset 0x0, size 0x4
    unsigned int IPAddress;        // offset 0x4, size 0x4
    char NetworkSettingsName[40];  // offset 0x8, size 0x28
};

struct InitNetMonitorPacket {
    void *RequestExeFlag;    // offset 0x0, size 0x4
    void *MonitorVarAddress; // offset 0x4, size 0x4
    float Version;           // offset 0x8, size 0x4
    int AllowKernelMode;     // offset 0xC, size 0x4
};

struct Breakpoint {
    unsigned int Address; // offset 0x0, size 0x4
    unsigned int Opcode;  // offset 0x4, size 0x4
};

class NetMonitor {
  public:
    NetMonitor();
    virtual ~NetMonitor();
    void *operator new(unsigned int size) { return bMalloc(size, "NetMonitor", 0, BMEMORY_ALIGNMENT(64)); } void operator delete(void *ptr) { bFree(ptr); }
    unsigned int InitNetwork();
    bool InitSBenderIRX(const char *network_settings_name);
    void CloseSBenderIRX(bool sleep);
    void InitMonitor();
    void CloseMonitor(bool launching_executable);
    bool HookOperatingSystem();
    void UnhookOperatingSystem();
    void Service();
    void SendPacket(bFunkPacketHeader *header, const void *data, int data_size);
    bool CanSendPacket() {
        return !InSendPacket && !MonitorVarsSaved && !InServiceMonitor;
    }
    bool IsConnected() {
        return Connected;
    }
    void SetSoftRebootFunction(void (*soft_reboot_function)()) {
        SoftRebootFunction = soft_reboot_function;
    }
    static void HandleSoftReboot();
    void LoadExecutable();
    void HandleSyscallEnterMonitor();
    void HandleSyscallExitMonitor();
    void ServiceMonitorFromSyscall();
    int HandleNetMonitorSyscall(int function_number, int param1, int param2, int param3);
    void HandleException();
    void EnterMonitorFromException();
    void ServiceMonitorLoop();
    void InMonitorLoop();
    int32 GetServiceMonitorCount() {
        return ServiceMonitorCount;
    }
    void ServicePackets();
    void SetConnected();
    void SetDisconnected();

  private:
    void SendCodeinePacket(unsigned char function_num, const void *data, int data_size);
    void BuildReturnPacket(const bFunkPacketHeader *sync_packet, bFunkPacket *packet, const void *data, int data_size);
    void bFunkCallASync(const char *server_name, unsigned char function_num, const void *param_buffer, int param_size);
    void ProcessPacket(const bFunkPacket *packet, bFunkPacket *return_packet);

  public:
    bool IsPacketWaiting();
    void CheckForReceivePacket();
    void EnterMonitor();
    void ExitMonitor();
    bool CanEnterKernelMode();
    bool IsInKernelMode() {
        return SupportsKernelMode;
    }

  private:
    void SetBreakpointTable(unsigned int *breakpoint_table, int num_breakpoints);
    void ActivateBreakpoints(unsigned int program_counter);
    void DeactivateBreakpoints();
    static void MonitorServiceThread(void *arg);

    bool Connected;                                             // offset 0x0, size 0x4
    bool HasREALCOMMIncommingPacket;                            // offset 0x4, size 0x4
    void (*SoftRebootFunction)();                               // offset 0x8, size 0x4
    int MonitorServiceThreadID;                                 // offset 0xC, size 0x4
    void *MonitorServiceThreadStack;                            // offset 0x10, size 0x4
    int MainThreadID;                                           // offset 0x14, size 0x4
    int MainThreadPriority;                                     // offset 0x18, size 0x4
    unsigned int NetMonSifRPCBuffer[32] __attribute__((aligned(64))); // offset 0x40, size 0x80
    sceSifClientData NetMonSifClientData;                       // offset 0xC0, size 0x28
    FastPacketBuffer *pFastPacketBuffer;                        // offset 0xE8, size 0x4
    FastPacketBuffer *pFastPacketBufferOriginal;                // offset 0xEC, size 0x4
    int TotalNumSent;                                           // offset 0xF0, size 0x4
    int TotalNumReceived;                                       // offset 0xF4, size 0x4
    int SendPacketNumber;                                       // offset 0xF8, size 0x4
    int ReceivePacketNumber;                                    // offset 0xFC, size 0x4
    bool InMonitor;                                             // offset 0x100, size 0x4
    bool InServiceMonitor;                                      // offset 0x104, size 0x4
    bool Lingering;                                             // offset 0x108, size 0x4
    bool InSendPacket;                                          // offset 0x10C, size 0x4
    bool MonitorVarsSaved;                                      // offset 0x110, size 0x4
    bool SupportsKernelMode;                                    // offset 0x114, size 0x4
    int32 ServiceMonitorCount;                                  // offset 0x118, size 0x4
    volatile bool UserWantsToBreakToMonitor;                             // offset 0x11C, size 0x4
    volatile bool UserWantsSoftReboot;                                   // offset 0x120, size 0x4
    volatile bool WantToExitMonitor;                                     // offset 0x124, size 0x4
    bool PreventUserBreakToMonitor;                             // offset 0x128, size 0x4
    bool EnteredMonitorFromException;                           // offset 0x12C, size 0x4
    PSX2MonitorVars ThePSX2MonitorVars;                         // offset 0x130, size 0x63A0
    char MonitorStack[8192];                                    // offset 0x64D0, size 0x2000
    unsigned int ExceptionTableAddress;                         // offset 0x84D0, size 0x4
    unsigned int ExceptionTableCopy[32];                        // offset 0x84D4, size 0x80
    unsigned int SyscallTableAddress;                           // offset 0x8554, size 0x4
    unsigned int SyscallTableCopy[128];                         // offset 0x8558, size 0x200
    Breakpoint BreakpointTable[128];                            // offset 0x8758, size 0x400
    int NumBreakpoints;                                         // offset 0x8B58, size 0x4
};

NetMonitor *pNetMonitor = 0;
PSX2MonitorVars *pPSX2MonitorVars = 0;

NetMonitor::NetMonitor() {
    pPSX2MonitorVars = &ThePSX2MonitorVars;

    Connected = false;

    SoftRebootFunction = 0;

    MonitorServiceThreadID = 0;
    MonitorServiceThreadStack = 0;
    MainThreadID = 0;
    MainThreadPriority = 0;

    HasREALCOMMIncommingPacket = false;

    InMonitor = false;
    InServiceMonitor = false;
    InSendPacket = false;
    MonitorVarsSaved = false;
    Lingering = false;
    SupportsKernelMode = false;
    ServiceMonitorCount = 0;

    NumBreakpoints = 0;
    bMemSet(&ThePSX2MonitorVars, 0, sizeof(ThePSX2MonitorVars));

    UserWantsToBreakToMonitor = false;
    UserWantsSoftReboot = false;
    WantToExitMonitor = false;
    PreventUserBreakToMonitor = false;
    EnteredMonitorFromException = false;

    pFastPacketBuffer = 0;
    pFastPacketBufferOriginal = 0;

    TotalNumSent = 0;
    TotalNumReceived = 0;
    SendPacketNumber = 0;
    ReceivePacketNumber = 0;

    bMemSet(MonitorStack, 17, sizeof(MonitorStack));
    pMonitorStack = reinterpret_cast<unsigned char *>(&MonitorStack[sizeof(MonitorStack) - 16]);
}

NetMonitor::~NetMonitor() {}

void NetMonitor::InitMonitor() {
    SetConnected();

    ThreadParam tparam;

    bMemSet(&tparam, 0, sizeof(tparam));
    MainThreadID = GetThreadId();
    ReferThreadStatus(MainThreadID, &tparam);
    MainThreadPriority = tparam.currentPriority;

    bMemSet(&tparam, 0, sizeof(tparam));
    tparam.stackSize = 1024;
    tparam.gpReg = &_gp;
    tparam.entry = MonitorServiceThread;
    MonitorServiceThreadStack = bMalloc(tparam.stackSize, "NetMonitor::MonitorServiceThreadStack", 0, 0);
    tparam.stack = MonitorServiceThreadStack;
    tparam.initPriority = 1;
    MonitorServiceThreadID = CreateThread(&tparam);
    StartThread(MonitorServiceThreadID, 0);

    bCodeineVersion = 1.0f;
    bFunkCallSync("CODEINE", 82, 0, 0, &bCodeineVersion, 2016);

    if (SuperBenderDebuggerLevel >= SB_DEBUGGER_LEVEL_FULL && bCodeineVersion > 1.0f && bIsPlatDTL_T10000()) {
        bFunkCallSync("CODEINE", 53, 0, 0, 0, 2016);
    }

    static int dummy;
    InitNetMonitorPacket packet = {&dummy, &ThePSX2MonitorVars, 2.42f, SuperBenderDebuggerLevel >= SB_DEBUGGER_LEVEL_MID};
    bFunkCallSync("CODEINE", 81, &packet, sizeof(packet), 0, 2016);

    if (UseFastPacket) {
        if (SuperBenderDebuggerLevel >= SB_DEBUGGER_LEVEL_FULL && HookOperatingSystem()) {
            SupportsKernelMode = true;
            bFunkCallSync("CODEINE", 54, 0, 0, 0, 2016);
            WritebackDCache();
            InvalidateICache();
        }
    }
}

void NetMonitor::CloseMonitor(bool launching_executable) {
    DeactivateBreakpoints();
    WritebackDCache();
    InvalidateICache();

    bFunkCallSync("CODEINE", 85, 0, 0, 0, 2016);

    int do_soft_reset = 1;
    if (launching_executable) {
        do_soft_reset = 2;
    }
    bFunkCallASync("CODEINE", 84, &do_soft_reset, sizeof(do_soft_reset));

    if (MonitorServiceThreadID) {
        TerminateThread(MonitorServiceThreadID);
        MonitorServiceThreadID = 0;
    }

    SetDisconnected();

    DelayMilliseconds(20);

    if (UseFastPacket) {
        if (IsInKernelMode()) {
            UnhookOperatingSystem();
        }
    }

    pPSX2MonitorVars = 0;
}

unsigned int NetMonitor::InitNetwork() {
    int i;
    sceSifClientData *cd = &NetMonSifClientData;

    while (1) {
        if (sceSifBindRpc(cd, 0x1265421, 0) < 0) {
            scePrintf("bind error\n");
        }
        if (cd->serve != 0) {
            break;
        }
        i = 0x10000;
        while (i--) {
        }
    }

    NetMonSifRPCBuffer[0] = 0;
    int ret = sceSifCallRpc(cd, 4, 0, NetMonSifRPCBuffer, 128, NetMonSifRPCBuffer, 128, 0, 0);
    if (ret < 0) {
        scePrintf("call sceSifCallRpc() failed.  ret = %d\n", ret);
        return 0;
    }
    return NetMonSifRPCBuffer[0];
}

bool NetMonitor::InitSBenderIRX(const char *network_settings_name) {
    pFastPacketBufferOriginal =
        static_cast<FastPacketBuffer *>(bMalloc(sizeof(FastPacketBuffer), "FastPacketBuffer", 0, BMEMORY_ALIGNMENT(64)));
    pFastPacketBuffer = reinterpret_cast<FastPacketBuffer *>(reinterpret_cast<unsigned int>(pFastPacketBufferOriginal) | 0x20000000);
    FlushCache(0);

    pFastPacketBuffer->Reset();

    pFastPacketBuffer->GetFunctionInfo()->sceSifSetDma = isceSifSetDma;
    pFastPacketBuffer->GetFunctionInfo()->sceSifDmaStat = isceSifDmaStat;

    SuperBenderIRXStartupInfo *info = reinterpret_cast<SuperBenderIRXStartupInfo *>(NetMonSifRPCBuffer);

    bMemSet(info, 0, sizeof(SuperBenderIRXStartupInfo));
    info->pOtherSide = pFastPacketBuffer;
    info->Unused = 0;
    if (network_settings_name) {
        bStrCpy(info->NetworkSettingsName, network_settings_name);
    }

    int ret = sceSifCallRpc(&NetMonSifClientData, network_settings_name ? 0 : 3, 0, NetMonSifRPCBuffer, 128, NetMonSifRPCBuffer, 128, 0, 0);
    if (ret < 0) {
        scePrintf("call sceSifMCallRpc() failed.  ret = %d\n", ret);
    }

    FastPacketBuffer *other_side = reinterpret_cast<FastPacketBuffer *>(NetMonSifRPCBuffer[0]);
    if (!other_side) {
        return false;
    }

    pFastPacketBuffer->Start(other_side);
    return true;
}

void NetMonitor::CloseSBenderIRX(bool sleep) {
    int function_id;

    if (sleep) {
        function_id = 2;
    } else {
        function_id = 1;
    }

    int ret = sceSifCallRpc(&NetMonSifClientData, function_id, 0, NetMonSifRPCBuffer, 0, NetMonSifRPCBuffer, 0, 0, 0);
    if (ret < 0) {
        scePrintf("call sceSifCallRpc() failed.  ret = %d\n", ret);
    }

    pFastPacketBuffer->Stop();

    if (sleep) {
        bFree(pFastPacketBufferOriginal);
    }
    pFastPacketBuffer = 0;
    pFastPacketBufferOriginal = 0;
}

extern "C" int TempNetMonitorSyscall(int register_number, int function_number, int param1, int param2, int param3) {
    return pNetMonitor->HandleNetMonitorSyscall(function_number, param1, param2, param3);
}

bool NetMonitor::HookOperatingSystem() {
    int i;

    FlushCache(0);

    ExceptionTableAddress = 0;
    for (i = 0; i < 128; i += 4) {
        unsigned int opcode = pFastPacketBuffer->ReadMemory(0x80000180 + i);

        if ((opcode & 0xffff0000) == 0x3c1a0000) ExceptionTableAddress = opcode << 16;
        if ((opcode & 0xffff0000) == 0x8f5a0000) ExceptionTableAddress += opcode & 0xffff;
        if (opcode == 0x03400008) break;
    }
    if (i == 128) {
        return false;
    }

    for (i = 0; i < 32; i++) {
        ExceptionTableCopy[i] = pFastPacketBuffer->ReadMemory(ExceptionTableAddress + i * 4);
    }

    SyscallTableAddress = 0;
    for (i = 0; i < 256; i += 4) {
        unsigned int syscall_handler = ExceptionTableCopy[8];
        unsigned int opcode = pFastPacketBuffer->ReadMemory(syscall_handler + i);

        if ((opcode & 0xffff0000) == 0x3c1a0000) SyscallTableAddress = opcode << 16;
        if ((opcode & 0xffff0000) == 0x8f5a0000) SyscallTableAddress += opcode & 0xffff;
        if (opcode == 0x0340f809) break;
    }
    if (i == 256) {
        return false;
    }

    for (i = 0; i < 128; i++) {
        SyscallTableCopy[i] = pFastPacketBuffer->ReadMemory(SyscallTableAddress + i * 4);
    }

    unsigned int original_cop0_handler = pFastPacketBuffer->ReadMemory(SyscallTableAddress + 99 * 4);
    pFastPacketBuffer->WriteMemory(SyscallTableAddress + 99 * 4, reinterpret_cast<unsigned int>(TempNetMonitorSyscall));

    int ret = NetMonitorSyscall(258, 0x1245323, 0, 0);
    if (ret != 0x1245324) {
        return false;
    }

    OriginalSyscallHandlerAddress = NetMonitorSyscall(259, ExceptionTableAddress + 8 * 4, 0, 0);
    NetMonitorSyscall(260, ExceptionTableAddress + 8 * 4, reinterpret_cast<int>(MySyscallExceptionHandler), 0);
    NetMonitorSyscall(260, SyscallTableAddress + 99 * 4, original_cop0_handler, 0);

    if (NetMonitorSyscall(258, 0x1245323, 0, 0) != ret) {
        return false;
    }

    NetMonitorSyscall(260, ExceptionTableAddress + 0 * 4, reinterpret_cast<int>(MyExceptionHandler), 0);
    NetMonitorSyscall(260, ExceptionTableAddress + 1 * 4, reinterpret_cast<int>(MyExceptionHandler), 0);
    NetMonitorSyscall(260, ExceptionTableAddress + 2 * 4, reinterpret_cast<int>(MyExceptionHandler), 0);
    NetMonitorSyscall(260, ExceptionTableAddress + 3 * 4, reinterpret_cast<int>(MyExceptionHandler), 0);
    NetMonitorSyscall(260, ExceptionTableAddress + 4 * 4, reinterpret_cast<int>(MyExceptionHandler), 0);
    NetMonitorSyscall(260, ExceptionTableAddress + 5 * 4, reinterpret_cast<int>(MyExceptionHandler), 0);
    NetMonitorSyscall(260, ExceptionTableAddress + 6 * 4, reinterpret_cast<int>(MyExceptionHandler), 0);
    NetMonitorSyscall(260, ExceptionTableAddress + 7 * 4, reinterpret_cast<int>(MyExceptionHandler), 0);

    NetMonitorSyscall(260, ExceptionTableAddress + 9 * 4, reinterpret_cast<int>(MyExceptionHandler), 0);
    NetMonitorSyscall(260, ExceptionTableAddress + 10 * 4, reinterpret_cast<int>(MyExceptionHandler), 0);
    NetMonitorSyscall(260, ExceptionTableAddress + 11 * 4, reinterpret_cast<int>(MyExceptionHandler), 0);
    NetMonitorSyscall(260, ExceptionTableAddress + 12 * 4, reinterpret_cast<int>(MyExceptionHandler), 0);
    NetMonitorSyscall(260, ExceptionTableAddress + 13 * 4, reinterpret_cast<int>(MyExceptionHandler), 0);

    return true;
}

void NetMonitor::UnhookOperatingSystem() {
    NetMonitorSyscall(260, ExceptionTableAddress + 0 * 4, ExceptionTableCopy[0], 0);
    NetMonitorSyscall(260, ExceptionTableAddress + 1 * 4, ExceptionTableCopy[1], 0);
    NetMonitorSyscall(260, ExceptionTableAddress + 2 * 4, ExceptionTableCopy[2], 0);
    NetMonitorSyscall(260, ExceptionTableAddress + 3 * 4, ExceptionTableCopy[3], 0);
    NetMonitorSyscall(260, ExceptionTableAddress + 4 * 4, ExceptionTableCopy[4], 0);
    NetMonitorSyscall(260, ExceptionTableAddress + 5 * 4, ExceptionTableCopy[5], 0);
    NetMonitorSyscall(260, ExceptionTableAddress + 6 * 4, ExceptionTableCopy[6], 0);
    NetMonitorSyscall(260, ExceptionTableAddress + 7 * 4, ExceptionTableCopy[7], 0);

    NetMonitorSyscall(260, ExceptionTableAddress + 9 * 4, ExceptionTableCopy[9], 0);
    NetMonitorSyscall(260, ExceptionTableAddress + 10 * 4, ExceptionTableCopy[10], 0);
    NetMonitorSyscall(260, ExceptionTableAddress + 11 * 4, ExceptionTableCopy[11], 0);
    NetMonitorSyscall(260, ExceptionTableAddress + 12 * 4, ExceptionTableCopy[12], 0);
    NetMonitorSyscall(260, ExceptionTableAddress + 13 * 4, ExceptionTableCopy[13], 0);

    NetMonitorSyscall(260, ExceptionTableAddress + 8 * 4, OriginalSyscallHandlerAddress, 0);
}

void ServiceMonitorFromSyscallBridge() {
    pNetMonitor->ServiceMonitorFromSyscall();
}

void NetMonitor::ServiceMonitorFromSyscall() {
    ServiceMonitorLoop();

    NetMonitorSyscall(257, 0, 0, 0);
}

void EnterMonitorFromExceptionBridge() {
    pNetMonitor->EnterMonitorFromException();
}

void NetMonitor::HandleSyscallEnterMonitor() {
    asm volatile("mfc0 %0, $12" : "=r"(ThePSX2MonitorVars.Cop0Registers[12]));
    asm volatile("mfc0 %0, $14" : "=r"(ThePSX2MonitorVars.ProgramCounter));
    ThePSX2MonitorVars.ProgramCounter += 4;

    ThePSX2MonitorVars.BreakCause = 4096;

    MonitorVarsSaved = true;

    asm volatile("mtc0 %0, $14" : : "r"(ServiceMonitorFromSyscallBridge));
    asm volatile("sync.p");
}

void NetMonitor::HandleSyscallExitMonitor() {
    MonitorVarsSaved = false;

    asm volatile("mtc0 %0, $12" : : "r"(ThePSX2MonitorVars.Cop0Registers[12]));
    asm volatile("mtc0 %0, $14" : : "r"(ThePSX2MonitorVars.ProgramCounter));

    asm volatile("sync.p");
}

void NetMonitor::HandleException() {
    int cause;

    asm volatile("mfc0 %0, $13" : "=r"(cause));
    ThePSX2MonitorVars.BreakCause = (cause & 0x7c) >> 2;

    asm volatile("mfc0 %0, $14" : "=r"(ThePSX2MonitorVars.ProgramCounter));
    asm volatile("mfc0 %0, $12" : "=r"(ThePSX2MonitorVars.Cop0Registers[12]));

    MonitorVarsSaved = true;

    asm volatile("mtc0 %0, $14" : : "r"(EnterMonitorFromExceptionBridge));
    asm volatile("sync.p");
}

void NetMonitor::EnterMonitorFromException() {
    unsigned int irq_status = DisableInterruptsByCause();

    MyEnableInterrupts();

    EnteredMonitorFromException = true;

    InMonitorLoop();

    EnteredMonitorFromException = false;

    MyDisableInterrupts();
    RestoreInterruptsByCause(irq_status);

    NetMonitorSyscall(257, 0, 0, 0);
}

int NetMonitor::HandleNetMonitorSyscall(int function_number, int param1, int param2, int param3) {
    unsigned int epc;
    int ret;

    asm volatile("mfc0 %0, $14" : "=r"(epc));

    ret = 0;
    if (function_number == 261) {
        ProcessPacket(reinterpret_cast<const bFunkPacket *>(param1), reinterpret_cast<bFunkPacket *>(param2));
    } else if (function_number == 258) {
        ret = param1 + 1;
    } else if (function_number == 260) {
        *reinterpret_cast<int *>(param1) = param2;
    } else if (function_number == 259) {
        ret = *reinterpret_cast<int *>(param1);
    }

    epc += 4;
    asm volatile("mtc0 %0, $14" : : "r"(epc));
    asm volatile("sync.p");

    return ret;
}

extern "C" int HandleNetMonitorSyscallBridge(NetMonitor *net_monitor, int function_number, int param1, int param2, int param3) {
    return net_monitor->HandleNetMonitorSyscall(function_number, param1, param2, param3);
}

extern "C" void HandleExceptionBridge(NetMonitor *net_monitor) {
    net_monitor->HandleException();
}

extern "C" void HandleSyscallEnterMonitorBridge(NetMonitor *net_monitor) {
    net_monitor->HandleSyscallEnterMonitor();
}

extern "C" void HandleSyscallExitMonitorBridge(NetMonitor *net_monitor) {
    net_monitor->HandleSyscallExitMonitor();
}

void NetMonitor::MonitorServiceThread(void *arg) {
    int num_awakens = 0;
    int32 service_count = -1;

    while (pNetMonitor && pNetMonitor->MonitorServiceThreadID) {
        if (pNetMonitor->ServiceMonitorCount == service_count || pNetMonitor->UserWantsToBreakToMonitor || pNetMonitor->UserWantsSoftReboot) {
            num_awakens++;
            if (num_awakens >= 4) {
                if (pNetMonitor->UserWantsSoftReboot) {
                    // Lo que hubiera aqui no esta en el ejecutable; solo queda la lectura del volatile.
                }
                pNetMonitor->Service();
                num_awakens = 0;
            }
        }

        service_count = pNetMonitor->ServiceMonitorCount;
        THREAD_yield(500);
    }
}

void NetMonitor::Service() {
    if (UserWantsSoftReboot) {
        int thread_id = GetThreadId();

        if (thread_id == MainThreadID || thread_id == MonitorServiceThreadID) {
            UserWantsSoftReboot = false;
            HandleSoftReboot();
        }
    }

    if (MonitorVarsSaved || InServiceMonitor) {
        return;
    }

    InServiceMonitor = true;

    PreventUserBreakToMonitor = false;
    if (UserBreakOnlyInMainThread && MainThreadID && MonitorServiceThreadID) {
        int thread_id = GetThreadId();

        if (thread_id != MainThreadID && thread_id != MonitorServiceThreadID) {
            PreventUserBreakToMonitor = true;
        }
    }

    bEnableInterrupts();

    ServiceMonitorCount++;

    if (IsPacketWaiting() || (UserWantsToBreakToMonitor && !PreventUserBreakToMonitor)) {
        bDisableInterrupts();
        unsigned int irq_status = DisableInterruptsByCause();
        bEnableInterrupts();

        if (IsInKernelMode()) {
            NetMonitorSyscall(256, 0, 0, 0);
        } else {
            MonitorVarsSaved = true;
            PolitelyServiceMonitor();
            MonitorVarsSaved = false;
        }

        bDisableInterrupts();
        RestoreInterruptsByCause(irq_status);
        bEnableInterrupts();
    }

    InServiceMonitor = false;
}

void NetMonitor::ServiceMonitorLoop() {
    Lingering = false;

    for (;;) {
        if (!IsPacketWaiting() && !Lingering) {
            return;
        }

        CheckForReceivePacket();

        if (UserWantsToBreakToMonitor && !PreventUserBreakToMonitor) {
            UserWantsToBreakToMonitor = false;
            ThePSX2MonitorVars.BreakCause = 4097;
            InMonitorLoop();
            return;
        }
    }
}

extern "C" void ServiceMonitorLoopBridge(NetMonitor *net_monitor) {
    net_monitor->ServiceMonitorLoop();
}

void NetMonitor::InMonitorLoop() {
    InMonitor = true;

    ServiceMonitorCount++;

    EnterMonitor();

    while (!WantToExitMonitor) {
        CheckForReceivePacket();
    }
    WantToExitMonitor = false;

    ExitMonitor();

    InMonitor = false;
}

void NetMonitor::SetConnected() {
    if (!IsConnected()) {
        Connected = true;

        bSetupMonitorFunctionHooks(SendPacketNetMonitor, ServiceNetMonitor);

        bSuperBenderConnected = true;
    }
}

void NetMonitor::SetDisconnected() {
    if (IsConnected()) {
        Connected = false;

        bSetupMonitorFunctionHooks(0, 0);

        bSuperBenderConnected = false;
    }
}

void NetMonitor::SendPacket(bFunkPacketHeader *header, const void *data, int data_size) {
    if (!IsConnected()) {
        return;
    }

    InSendPacket = true;

    int total_size = header->GetTotalSize();
    bFunkPacket *packet;

    bEnableInterrupts();
    do {
        packet = static_cast<bFunkPacket *>(pFastPacketBuffer->GetSendPacket(total_size));
    } while (!packet);
    bDisableInterrupts();

    TotalNumSent += total_size;

    header->SourceServer = 0xf8f6e9a7;
    header->PacketID = SendPacketNumber++;

    if (UseRealCommINet || UseRealCommILink) {
    } else if (UseFastPacket) {
        header->Checksum = bFunkPacketHeader::CalculateChecksum(data, data_size);

        bMemCpy(packet, header, sizeof(bFunkPacketHeader));
        if (data_size) {
            bMemCpy(packet->GetData(), data, data_size);
        }
        pFastPacketBuffer->AdvanceSendPacket();
    }

    InSendPacket = false;

    bEnableInterrupts();
}

void NetMonitor::bFunkCallASync(const char *server_name, unsigned char function_num, const void *param_buffer, int param_size) {
    if (param_size <= 2016) {
        bFunkPacketHeader header;

        bMemSet(&header, 0, sizeof(header));
        header.SetDataSize(param_size);
        header.FunctionNum = function_num;
        header.Type = 0;
        header.DestServer = bStringHash(server_name);

        SendPacket(&header, param_buffer, param_size);
    }
}

void NetMonitor::SendCodeinePacket(unsigned char function_num, const void *data, int data_size) {
    bFunkCallASync("CODEINE", function_num, data, data_size);
}

void NetMonitor::BuildReturnPacket(const bFunkPacketHeader *sync_packet, bFunkPacket *packet, const void *data, int data_size) {
    packet->InitReturnPacketHeader(sync_packet, data_size);

    if (data_size) {
        bMemCpy(packet->GetData(), data, data_size);
    }
}

void NetMonitor::CheckForReceivePacket() {
    if (UseRealCommINet || UseRealCommILink) {
    } else if (UseFastPacket) {
        bFunkPacket *packet = static_cast<bFunkPacket *>(pFastPacketBuffer->GetReceivePacket(0));

        if (packet) {
            bFunkPacket return_packet;

            if (IsInKernelMode()) {
                NetMonitorSyscall(261, reinterpret_cast<int>(packet), reinterpret_cast<int>(&return_packet), 0);
            } else {
                ProcessPacket(packet, &return_packet);
            }
            pFastPacketBuffer->AdvanceReceivePacket();

            if (return_packet.GetTotalSize() > 0) {
                SendPacket(&return_packet, return_packet.GetData(), return_packet.GetDataSize());
            }
        }
    }
}

void NetMonitor::ProcessPacket(const bFunkPacket *packet, bFunkPacket *return_packet) {
    int pos;

    return_packet->SetTotalSize(0);

    TotalNumReceived += packet->GetTotalSize();

    if (packet->PacketID != ReceivePacketNumber) {
        ReceivedPacketError++;
    }
    ReceivePacketNumber++;

    if (packet->Checksum != packet->CalculateChecksum()) {
        ReceivedPacketError++;
        scePrintf("NetMonitor::ProcessPacket()  Received checksum 0x%08x, calculated 0x%08x\n", packet->Checksum,
                  packet->CalculateChecksum());
    }

    pos = 0;
    if (packet->Type == 0) {
        int function_number = packet->FunctionNum;

        if (function_number == 4) {
            bMemCpy(reinterpret_cast<void *>(*reinterpret_cast<const unsigned int *>(packet->GetData())), packet->GetData(4),
                    packet->GetDataSize() - 4);
            pos = packet->GetDataSize();
        } else if (function_number == 5) {
            *reinterpret_cast<unsigned short *>(*reinterpret_cast<const unsigned int *>(packet->GetData())) =
                *reinterpret_cast<const unsigned short *>(packet->GetData(4));
            pos = 6;
        } else if (function_number == 6) {
            *reinterpret_cast<unsigned int *>(*reinterpret_cast<const unsigned int *>(packet->GetData())) =
                *reinterpret_cast<const unsigned int *>(packet->GetData(4));
            pos = 8;
        } else if (function_number == 12) {
            pos = 12;
            LZDecompress(reinterpret_cast<uint8 *>(*reinterpret_cast<const unsigned int *>(packet->GetData())),
                         reinterpret_cast<uint8 *>(*reinterpret_cast<const unsigned int *>(packet->GetData(4))));
        } else if (function_number == 3) {
            if (!InMonitor) {
                UserWantsToBreakToMonitor = true;
            }
        } else if (function_number == 14) {
        } else if (function_number == 15) {
        } else if (function_number == 23) {
            if (!InMonitor) {
                DeactivateBreakpoints();
            }
            SetBreakpointTable(reinterpret_cast<unsigned int *>(packet->GetData()),
                               packet->GetDataSize() / 4);
            pos = packet->GetDataSize();
            if (!InMonitor) {
                ActivateBreakpoints(0);
            }
        } else {
            scePrintf("WARNING:  ProcessPacket received unknown ASYNC function_number = %d\n", function_number);
        }
    } else if (packet->Type == 1) {
        int function_number = packet->FunctionNum;

        if (function_number == 7) {
            pos = 8;
            BuildReturnPacket(packet, return_packet, reinterpret_cast<const void *>(*reinterpret_cast<const unsigned int *>(packet->GetData())),
                              *reinterpret_cast<const int *>(packet->GetData(4)));
        } else if (function_number == 8) {
            unsigned short value = *reinterpret_cast<unsigned short *>(*reinterpret_cast<const unsigned int *>(packet->GetData()));
            pos = 4;
            BuildReturnPacket(packet, return_packet, &value, sizeof(value));
        } else if (function_number == 9) {
            unsigned int value = *reinterpret_cast<unsigned int *>(*reinterpret_cast<const unsigned int *>(packet->GetData()));
            pos = 4;
            BuildReturnPacket(packet, return_packet, &value, sizeof(value));
        } else if (function_number == 24) {
            unsigned char buffer[2016];
            pos = 4;
            int size = *reinterpret_cast<const int *>(packet->GetData());
            int num = (packet->GetDataSize() - 4) / sizeof(unsigned int);

            for (int n = 0; n < num; n++) {
                bMemCpy(&buffer[n * size], reinterpret_cast<const void *>(*reinterpret_cast<const unsigned int *>(packet->GetData(pos))), size);
                pos += 4;
            }
            BuildReturnPacket(packet, return_packet, buffer, num * size);
        } else if (function_number == 10) {
            BuildReturnPacket(packet, return_packet, 0, 0);
            if (InMonitor) {
                WantToExitMonitor = true;
            }
        } else if (function_number == 25) {
        } else if (function_number == 26) {
            if (SoftRebootOnlyInMainThread && PreventUserBreakToMonitor && !EnteredMonitorFromException) {
                UserWantsSoftReboot = true;
            } else {
                ThePSX2MonitorVars.ProgramCounter = reinterpret_cast<int>(&NetMonitor::HandleSoftReboot);
            }
            BuildReturnPacket(packet, return_packet, 0, 0);
            Lingering = false;
            InServiceMonitor = false;
        } else if (function_number == 27) {
            int in_monitor = InMonitor;
            BuildReturnPacket(packet, return_packet, &in_monitor, sizeof(in_monitor));
        } else {
            scePrintf("WARNING:  ProcessPacket received unknown SYNC function_number = %d\n", function_number);
        }
    } else if (packet->Type == 2) {
        bMemCpy(reinterpret_cast<void *>(packet->ReturnBufferAddress), packet->GetData(), packet->GetDataSize());
        pos = packet->GetDataSize();
        *reinterpret_cast<int *>(packet->ReturnCodeAddress) = packet->ReturnCode;
    }

    if (packet->Checksum != packet->CalculateChecksum()) {
        scePrintf("NetMonitor::ProcessPacket()  packet has got gibbled.  Received checksum 0x%08x, calculated 0x%08x\n", packet->Checksum,
                  packet->CalculateChecksum());
    }
    if (pos != packet->GetDataSize()) {
        scePrintf("WARNING:  ProcessPacket did not use up all of it's bytes.  pos = %d, size = %d\n", pos,
                  packet->GetDataSize());
    }
}

bool NetMonitor::IsPacketWaiting() {
    if (UseFastPacket) {
        return pFastPacketBuffer->CanReceivePacket();
    }
    return HasREALCOMMIncommingPacket;
}

void NetMonitor::SetBreakpointTable(unsigned int *breakpoint_table, int num_breakpoints) {
    NumBreakpoints = num_breakpoints;

    for (int n = 0; n < NumBreakpoints; n++) {
        unsigned int address = breakpoint_table[n];

        BreakpointTable[n].Address = address;
        BreakpointTable[n].Opcode = 0;
    }
}

void NetMonitor::ActivateBreakpoints(unsigned int program_counter) {
    for (int n = 0; n < NumBreakpoints; n++) {
        unsigned int *address = reinterpret_cast<unsigned int *>(BreakpointTable[n].Address);

        BreakpointTable[n].Opcode = *address;
        if (reinterpret_cast<unsigned int>(address) != program_counter) {
            if (IsInKernelMode()) {
                *address = 0x0000000d;
            }
        }
    }
}

void NetMonitor::DeactivateBreakpoints() {
    for (int n = NumBreakpoints - 1; n >= 0; n--) {
        unsigned int *address = reinterpret_cast<unsigned int *>(BreakpointTable[n].Address);

        *address = BreakpointTable[n].Opcode;
    }
}

void NetMonitor::HandleSoftReboot() {
    if (pNetMonitor->SoftRebootFunction) {
        if (GetThreadId() == pNetMonitor->MonitorServiceThreadID) {
            ChangeThreadPriority(pNetMonitor->MonitorServiceThreadID, pNetMonitor->MainThreadPriority - 1);
        }
        pNetMonitor->SoftRebootFunction();
    }
    asm("break 0, 1");
}

void NetMonitor::EnterMonitor() {
    PSX2MonitorVars *monitor_vars = &ThePSX2MonitorVars;
    unsigned int cop0_reg;
    float *float_ptr;

    monitor_vars->TickerEnd = MyGetTicker();

    SaveMultiplyRegisters(monitor_vars->MultiplyRegisters);

    DeactivateBreakpoints();

    WritebackDCache();
    InvalidateICache();

    asm volatile("mfc0 %0, $0" : "=r"(cop0_reg)); monitor_vars->Cop0Registers[0] = cop0_reg;
    asm volatile("mfc0 %0, $1" : "=r"(cop0_reg)); monitor_vars->Cop0Registers[1] = cop0_reg;
    asm volatile("mfc0 %0, $2" : "=r"(cop0_reg)); monitor_vars->Cop0Registers[2] = cop0_reg;
    asm volatile("mfc0 %0, $3" : "=r"(cop0_reg)); monitor_vars->Cop0Registers[3] = cop0_reg;
    asm volatile("mfc0 %0, $4" : "=r"(cop0_reg)); monitor_vars->Cop0Registers[4] = cop0_reg;
    asm volatile("mfc0 %0, $5" : "=r"(cop0_reg)); monitor_vars->Cop0Registers[5] = cop0_reg;
    asm volatile("mfc0 %0, $6" : "=r"(cop0_reg)); monitor_vars->Cop0Registers[6] = cop0_reg;
    asm volatile("mfc0 %0, $7" : "=r"(cop0_reg)); monitor_vars->Cop0Registers[7] = cop0_reg;
    asm volatile("mfc0 %0, $8" : "=r"(cop0_reg)); monitor_vars->Cop0Registers[8] = cop0_reg;
    asm volatile("mfc0 %0, $9" : "=r"(cop0_reg)); monitor_vars->Cop0Registers[9] = cop0_reg;
    asm volatile("mfc0 %0, $10" : "=r"(cop0_reg)); monitor_vars->Cop0Registers[10] = cop0_reg;
    asm volatile("mfc0 %0, $11" : "=r"(cop0_reg)); monitor_vars->Cop0Registers[11] = cop0_reg;

    asm volatile("mfc0 %0, $13" : "=r"(cop0_reg)); monitor_vars->Cop0Registers[13] = cop0_reg;
    asm volatile("mfc0 %0, $14" : "=r"(cop0_reg)); monitor_vars->Cop0Registers[14] = cop0_reg;
    asm volatile("mfc0 %0, $15" : "=r"(cop0_reg)); monitor_vars->Cop0Registers[15] = cop0_reg;
    asm volatile("mfc0 %0, $16" : "=r"(cop0_reg)); monitor_vars->Cop0Registers[16] = cop0_reg;
    asm volatile("mfc0 %0, $17" : "=r"(cop0_reg)); monitor_vars->Cop0Registers[17] = cop0_reg;
    asm volatile("mfc0 %0, $18" : "=r"(cop0_reg)); monitor_vars->Cop0Registers[18] = cop0_reg;
    asm volatile("mfc0 %0, $19" : "=r"(cop0_reg)); monitor_vars->Cop0Registers[19] = cop0_reg;
    asm volatile("mfc0 %0, $20" : "=r"(cop0_reg)); monitor_vars->Cop0Registers[20] = cop0_reg;
    asm volatile("mfc0 %0, $21" : "=r"(cop0_reg)); monitor_vars->Cop0Registers[21] = cop0_reg;
    asm volatile("mfc0 %0, $22" : "=r"(cop0_reg)); monitor_vars->Cop0Registers[22] = cop0_reg;
    asm volatile("mfc0 %0, $23" : "=r"(cop0_reg)); monitor_vars->Cop0Registers[23] = cop0_reg;
    asm volatile("mfbpc %0" : "=r"(cop0_reg)); monitor_vars->Cop0Registers[24] = cop0_reg;
    asm volatile("mfps %0, 0" : "=r"(cop0_reg)); monitor_vars->Cop0Registers[25] = cop0_reg;
    asm volatile("mfc0 %0, $26" : "=r"(cop0_reg)); monitor_vars->Cop0Registers[26] = cop0_reg;
    asm volatile("mfc0 %0, $27" : "=r"(cop0_reg)); monitor_vars->Cop0Registers[27] = cop0_reg;
    asm volatile("mfc0 %0, $28" : "=r"(cop0_reg)); monitor_vars->Cop0Registers[28] = cop0_reg;
    asm volatile("mfc0 %0, $29" : "=r"(cop0_reg)); monitor_vars->Cop0Registers[29] = cop0_reg;
    asm volatile("mfc0 %0, $30" : "=r"(cop0_reg)); monitor_vars->Cop0Registers[30] = cop0_reg;
    asm volatile("mfc0 %0, $31" : "=r"(cop0_reg)); monitor_vars->Cop0Registers[31] = cop0_reg;

    float_ptr = monitor_vars->FloatRegisters;

    asm volatile("swc1 $f0, 0(%0)" : : "r"(float_ptr));
    asm volatile("swc1 $f1, 4(%0)" : : "r"(float_ptr));
    asm volatile("swc1 $f2, 8(%0)" : : "r"(float_ptr));
    asm volatile("swc1 $f3, 12(%0)" : : "r"(float_ptr));
    asm volatile("swc1 $f4, 16(%0)" : : "r"(float_ptr));
    asm volatile("swc1 $f5, 20(%0)" : : "r"(float_ptr));
    asm volatile("swc1 $f6, 24(%0)" : : "r"(float_ptr));
    asm volatile("swc1 $f7, 28(%0)" : : "r"(float_ptr));
    asm volatile("swc1 $f8, 32(%0)" : : "r"(float_ptr));
    asm volatile("swc1 $f9, 36(%0)" : : "r"(float_ptr));
    asm volatile("swc1 $f10, 40(%0)" : : "r"(float_ptr));
    asm volatile("swc1 $f11, 44(%0)" : : "r"(float_ptr));
    asm volatile("swc1 $f12, 48(%0)" : : "r"(float_ptr));
    asm volatile("swc1 $f13, 52(%0)" : : "r"(float_ptr));
    asm volatile("swc1 $f14, 56(%0)" : : "r"(float_ptr));
    asm volatile("swc1 $f15, 60(%0)" : : "r"(float_ptr));
    asm volatile("swc1 $f16, 64(%0)" : : "r"(float_ptr));
    asm volatile("swc1 $f17, 68(%0)" : : "r"(float_ptr));
    asm volatile("swc1 $f18, 72(%0)" : : "r"(float_ptr));
    asm volatile("swc1 $f19, 76(%0)" : : "r"(float_ptr));
    asm volatile("swc1 $f20, 80(%0)" : : "r"(float_ptr));
    asm volatile("swc1 $f21, 84(%0)" : : "r"(float_ptr));
    asm volatile("swc1 $f22, 88(%0)" : : "r"(float_ptr));
    asm volatile("swc1 $f23, 92(%0)" : : "r"(float_ptr));
    asm volatile("swc1 $f24, 96(%0)" : : "r"(float_ptr));
    asm volatile("swc1 $f25, 100(%0)" : : "r"(float_ptr));
    asm volatile("swc1 $f26, 104(%0)" : : "r"(float_ptr));
    asm volatile("swc1 $f27, 108(%0)" : : "r"(float_ptr));
    asm volatile("swc1 $f28, 112(%0)" : : "r"(float_ptr));
    asm volatile("swc1 $f29, 116(%0)" : : "r"(float_ptr));
    asm volatile("swc1 $f30, 120(%0)" : : "r"(float_ptr));
    asm volatile("swc1 $f31, 124(%0)" : : "r"(float_ptr));

    if (monitor_vars->BreakCause == 9) {
        monitor_vars->BreakCause = 4098;

        if (*reinterpret_cast<unsigned int *>(monitor_vars->ProgramCounter) == 0x0000004d) {
            monitor_vars->ProgramCounter += 4;
        }
    }

    struct exception_packet {
        int ProgramCounter;
        int BreakCause;
    } packet;

    packet.ProgramCounter = monitor_vars->ProgramCounter;
    packet.BreakCause = monitor_vars->BreakCause;
    SendCodeinePacket(4, &packet, sizeof(packet));
}

void NetMonitor::ExitMonitor() {
    PSX2MonitorVars *monitor_vars = &ThePSX2MonitorVars;
    float *float_ptr;

    float_ptr = monitor_vars->FloatRegisters;

    asm volatile("lwc1 $f0, 0(%0)" : : "r"(float_ptr));
    asm volatile("lwc1 $f1, 4(%0)" : : "r"(float_ptr));
    asm volatile("lwc1 $f2, 8(%0)" : : "r"(float_ptr));
    asm volatile("lwc1 $f3, 12(%0)" : : "r"(float_ptr));
    asm volatile("lwc1 $f4, 16(%0)" : : "r"(float_ptr));
    asm volatile("lwc1 $f5, 20(%0)" : : "r"(float_ptr));
    asm volatile("lwc1 $f6, 24(%0)" : : "r"(float_ptr));
    asm volatile("lwc1 $f7, 28(%0)" : : "r"(float_ptr));
    asm volatile("lwc1 $f8, 32(%0)" : : "r"(float_ptr));
    asm volatile("lwc1 $f9, 36(%0)" : : "r"(float_ptr));
    asm volatile("lwc1 $f10, 40(%0)" : : "r"(float_ptr));
    asm volatile("lwc1 $f11, 44(%0)" : : "r"(float_ptr));
    asm volatile("lwc1 $f12, 48(%0)" : : "r"(float_ptr));
    asm volatile("lwc1 $f13, 52(%0)" : : "r"(float_ptr));
    asm volatile("lwc1 $f14, 56(%0)" : : "r"(float_ptr));
    asm volatile("lwc1 $f15, 60(%0)" : : "r"(float_ptr));
    asm volatile("lwc1 $f16, 64(%0)" : : "r"(float_ptr));
    asm volatile("lwc1 $f17, 68(%0)" : : "r"(float_ptr));
    asm volatile("lwc1 $f18, 72(%0)" : : "r"(float_ptr));
    asm volatile("lwc1 $f19, 76(%0)" : : "r"(float_ptr));
    asm volatile("lwc1 $f20, 80(%0)" : : "r"(float_ptr));
    asm volatile("lwc1 $f21, 84(%0)" : : "r"(float_ptr));
    asm volatile("lwc1 $f22, 88(%0)" : : "r"(float_ptr));
    asm volatile("lwc1 $f23, 92(%0)" : : "r"(float_ptr));
    asm volatile("lwc1 $f24, 96(%0)" : : "r"(float_ptr));
    asm volatile("lwc1 $f25, 100(%0)" : : "r"(float_ptr));
    asm volatile("lwc1 $f26, 104(%0)" : : "r"(float_ptr));
    asm volatile("lwc1 $f27, 108(%0)" : : "r"(float_ptr));
    asm volatile("lwc1 $f28, 112(%0)" : : "r"(float_ptr));
    asm volatile("lwc1 $f29, 116(%0)" : : "r"(float_ptr));
    asm volatile("lwc1 $f30, 120(%0)" : : "r"(float_ptr));
    asm volatile("lwc1 $f31, 124(%0)" : : "r"(float_ptr));

    ActivateBreakpoints(monitor_vars->ProgramCounter);

    WritebackDCache();
    InvalidateICache();

    RestoreMultiplyRegisters(monitor_vars->MultiplyRegisters);

    unsigned int tick = MyGetTicker();
    monitor_vars->TickerStart = tick;
    monitor_vars->TickerEnd = tick;
}

unsigned int SuperBenderInitNetwork() {
    pNetMonitor = new NetMonitor;

    return pNetMonitor->InitNetwork();
}

void SuperBenderReconnect(eSuperBenderDebuggerLevel debug_level) {
    SuperBenderInitNetwork();

    SuperBenderDebuggerLevel = debug_level;

    bFigureOutPSX2Platform();

    pNetMonitor->InitSBenderIRX(0);

    pNetMonitor->InitMonitor();
}

void SuperBenderSetSoftResetFunction(void (*soft_reboot_function)()) {
    if (pNetMonitor) pNetMonitor->SetSoftRebootFunction(soft_reboot_function);
}

void SuperBenderSoftReboot(const char *executable_name) {
    pNetMonitor->CloseMonitor(false);

    pNetMonitor->CloseSBenderIRX(false);

    LoadExecPS2(executable_name, 0, 0);

    asm("break 0, 1");
}

void ReregisterPowerOffHandler() {
    static sceSifClientData sif_client_data;
    static int registered_sif_client_data = 0;

    if (!registered_sif_client_data) {
        for (int i = 0; i < 20; i++) {
            if (sceSifBindRpc(&sif_client_data, 0x50011005, 0) < 0) {
                break;
            }
            if (sif_client_data.serve != 0) {
                registered_sif_client_data = 1;
                break;
            }
            THREAD_yield(1);
        }
    }

    if (registered_sif_client_data) {
        sceSifCallRpc(&sif_client_data, 0, 0, 0, 0, 0, 0, 0, 0);
    }
}

int NumSendPacketBytesBeforeSync = 0;
int TotalNumSendPacketBytes = 0;
int MaxSendPacketBytesBeforeSync = 0x8000;
int PrintSendPacketBlocking = 0;

void SendPacketNetMonitor(bFunkPacketHeader *header, const void *data, int data_size) {
    if (pNetMonitor) {
        while (!pNetMonitor->CanSendPacket()) {
            THREAD_yield(1);
        }

        TotalNumSendPacketBytes += data_size;
        NumSendPacketBytesBeforeSync += data_size;

        if (NumSendPacketBytesBeforeSync > MaxSendPacketBytesBeforeSync) {
            static float version = 1.0f;

            NumSendPacketBytesBeforeSync = 0;
            bFunkCallSync("CODEINE", 82, 0, 0, &version, 2016);
        }

        pNetMonitor->SendPacket(header, data, data_size);
    }
}

void ServiceNetMonitor() {
    if (pNetMonitor) pNetMonitor->Service();
}
