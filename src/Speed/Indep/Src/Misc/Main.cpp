#include "BuildRegion.hpp"
#include "Config.h"
#include "DemoDisc.hpp"
#include "GameFlow.hpp"
#include "HandyBreakpoint.hpp"
#include "Hermes.h"
#include "Joylog.hpp"
#include "Platform.h"
#include "QueuedFile.hpp"
#include "ResourceLoader.hpp"
#include "Speed/Indep/Src/Animation/AnimCtrl.hpp"
#include "Speed/Indep/Src/Camera/Camera.hpp"
#include "Speed/Indep/Src/Camera/CameraAI.hpp"
#include "Speed/Indep/Src/Camera/ICE/ICEManager.hpp"
#include "Speed/Indep/Src/EAXSound/EAXSOund.hpp"
#include "Speed/Indep/Src/EAXSound/SoundConn.h"
#include "Speed/Indep/Src/Ecstasy/Ecstasy.hpp"
#include "Speed/Indep/Src/Ecstasy/EmitterSystem.h"
#include "Speed/Indep/Src/Ecstasy/eMath.hpp"
#include "Speed/Indep/Src/Frontend/Database/FEDatabase.hpp"
#include "Speed/Indep/Src/Frontend/FEManager.hpp"
#include "Speed/Indep/Src/Frontend/FEPackageManager.hpp"
#include "Speed/Indep/Src/Frontend/MemoryCard/MemoryCard.hpp"
#include "Speed/Indep/Src/Gameplay/GInfractionManager.h"
#include "Speed/Indep/Src/Gameplay/GManager.h"
#include "Speed/Indep/Src/Gameplay/GRaceDatabase.h"
#include "Speed/Indep/Src/Input/IOModule.h"
#include "Speed/Indep/Src/Interfaces/Simables/IVehicle.h"
#include "Speed/Indep/Src/Main/Scheduler.h"
#include "Speed/Indep/Src/Misc/Profiler.hpp"
#include "Speed/Indep/Src/Physics/PhysicsInfo.hpp"
#include "Speed/Indep/Src/Physics/SmokeableInfo.hpp"
#include "Speed/Indep/Src/Render/RenderConn.h"
#include "Speed/Indep/Src/Sim/Simulation.h"
#include "Speed/Indep/Src/World/CarLoader.hpp"
#include "Speed/Indep/Src/World/CarRender.hpp"
#include "Speed/Indep/Src/World/DebugVehicleSelection.h"
#include "Speed/Indep/Src/World/DebugWorld.h"
#include "Speed/Indep/Src/World/EventManager.hpp"
#include "Speed/Indep/Src/World/OnlineManager.hpp"
#include "Speed/Indep/Src/World/Scenery.hpp"
#include "Speed/Indep/Src/World/SpaceNode.hpp"
#include "Speed/Indep/Src/World/TimeOfDay.hpp"
#include "Speed/Indep/Src/World/TrackPath.hpp"
#include "Speed/Indep/Src/World/TrackStreamer.hpp"
#include "Speed/Indep/Src/World/VehiclePartDamage.h"
#include "Speed/Indep/Src/World/WWorld.h"
#include "Speed/Indep/Src/World/World.hpp"
#include "Speed/Indep/Src/World/WorldConn.h"
#include "Speed/Indep/bWare/Inc/bMath.hpp"
#include "Speed/Indep/bWare/Inc/bMemory.hpp"
#include "Speed/Indep/bWare/Inc/bWare.hpp"
#include "Stomper.hpp"
#include "bFile.hpp"
#include "Speed/Indep/Src/Misc/bFile.hpp"
#include "Speed/Indep/bWare/Inc/bFunk.hpp"
#ifdef EA_BUILD_A124
#include "Speed/Indep/Src/Debug/Common/DebugDraw.h"
#include "Speed/Indep/Src/Frontend/NFSDebugMenu.h"
#include "Speed/Indep/Src/Misc/TestHooks/debugtrap.h"
#include "Speed/Indep/Src/Misc/TestHooks/JuiceHooks/JuiceHooks.h"
#include "Speed/Indep/Src/Misc/Timer.hpp"
#include "Speed/Indep/Src/Online/BuddyCore.hpp"
#include "Speed/Indep/Src/Online/ConditionSimulator.hpp"
#include "Speed/Indep/Src/World/WDbgCollisionDraw.h"
#include "Speed/Indep/Src/Online/NetworkCore.hpp"
#include <sifdev.h>
#endif

int ExitTheGameFlag = false;
int frames_elapsed;
int loop_ticker;
static int32 last_frame_count = 0;
// Los escribe el manejador de errores de disco de GameCube (Platform_G.cpp los
// declara extern) y los lee FEGameInterface; en el .data objetivo de zMisc van
// aqui, entre last_frame_count y CurrentLoopCounter, a cero los dos.
int g_discErrorNumber = 0;
int g_discErrorOccured = 0;
int32 CurrentLoopCounter = 0;
uint32 TimeDifferenceInMicroseconds = 0;
float TimeDifferenceInMiliseconds = 0.0f;
float TimeDifferenceInSeconds = 0.0f;
float MicrosecondsToMiliseconds = 0.001f;
float MilisecondsToSeconds = 1000.0f;

// zFoundation
extern void (*UFoundation_AssertMessage)(const char *, ...);

////

#ifdef EA_PLATFORM_PLAYSTATION2
#endif

class RaceStarter {
  public:
    static void StartSkipFERace();
};

#ifdef EA_BUILD_A124
// La memoria libre del IOP que MainLoop saca por pantalla o por la TTY; las activa el menu de depuracion.
bool gbShowFreeIOPMem = false;
bool gbLogFreeIOPMemToTTY = false;
bool gbShowFreeIOPMemInKb = true;
#endif

FastMem gFastMem;

extern bool twkDumpProfileMarks;
////

void FastMemEmergencyInitialization(unsigned int &bytes, const char *&name, unsigned int &expansionsize, unsigned int &trackingsize) {
    // speed.exe 0x64A56C y la de Xbox 360 (0x823DAF58) piden mas que GameCube y las dos PS2.
#if defined(EA_PLATFORM_XENON)
    bytes = 0x25f400;
#elif defined(EA_PLATFORM_WIN32)
    bytes = 0x21f400;
#else
    bytes = 0x11f400;
#endif
    name = "gFastMem";
    expansionsize = 0x2000;
    trackingsize = 0x200;
}

#ifdef EA_PLATFORM_PLAYSTATION2
extern "C" int bGetStackPointer(); // bmiscasm.s

int StackOnScratchPad = 1;
int MainMemoryStackPos = 0;

int DoWithMainMemoryStack(void *function, int arg1, int arg2) {
    if ((unsigned int)(bGetStackPointer() - 0x70000000) >= 0x4000) {
        return ((int (*)(int, int))function)(arg1, arg2);
    }
    return bDoWithStack(function, (void *)MainMemoryStackPos, arg1, arg2);
}
#endif

void SeedRandomNumber() {
    int seed;
    if (Joylog::IsReplaying()) {
        seed = Joylog::GetData(32, JOYLOG_CHANNEL_RANDOM);
    } else {
        seed = bGetTicker() + 0x12345678;
        Joylog::AddData(seed, 32, JOYLOG_CHANNEL_RANDOM);
    }
    bSetRandomSeed(seed, &bDefaultSeed);
}

void InitBigFiles() {
    if (bFileExists("NFS\\ZDIR.BIN")) {
        bInitDisculatorDriver("NFS\\ZDIR.BIN", "NFS\\ZZDATA");
    }
}

static void Main_MyAssert(const char *format, ...) {}

int InitializeEverythingTicks = 0;

void InitScreenPrintf();
void DebugMenuInit();

void InitializeEverything(int argc, char **argv) {
    ResetCapturedLoadingTimes();
    CaptureLoadingTime("InitializeEverything");
    // ProfileManager::Init(); // TODO

    UFoundation_AssertMessage = Main_MyAssert;
    bReserveMemoryPool(7);
    bReserveMemoryPool(8);
    bMemoryCreatePersistentPool(0x1000);
    bInitTicker(6e+04f);
#ifndef EA_BUILD_A124
    InitializeEverythingTicks = bGetTicker();
#endif
    bInitSharedStringPool(0x8000);
#if defined(EA_PLATFORM_PLAYSTATION2) || defined(EA_PLATFORM_WIN32) // PC: InitPlatform(argc, argv) (speed.exe 0x6CB400)
    InitPlatform(argc, argv);
#else
    InitPlatform();
#endif
    FirstBreakpoint();
    bMathTimingTest();
#ifdef EA_BUILD_A124
    // La A124 crea el procesador de ordenes de depuracion (debugtrap.h).
    pCommandProc = new cCommandProcessor;
#endif
    BuildRegion::IsAmerica();
#ifdef EA_PLATFORM_PLAYSTATION2
    InitBigFiles();
#endif
    TheDemoDiscManager.Init(argc, argv);
#ifdef EA_PLATFORM_PLAYSTATION2
    bPListInit(0x1400);
#else
    bPListInit(0x1800);
#endif
    InitConfig();
#ifdef EA_BUILD_A124
    // La A124 arranca Juice y, si queda activo, el joylog lo graba la repeticion de Juice.
    InitJuice();
    if (!gJuiceEnabled)
#endif
    InitJoylog();
    SeedRandomNumber();
    InitQueuedFiles();
    emEventManagerInit();
    eMathInit();
    eInitEngine();
    InitDisplaySystem();
    EmitterSystem::Init();
    InitResourceLoader();
    InitializeSoundDriver();
    FEPackageManager::Init();
    LoadGlobalAChunks();
    FEManager::Init();
    BootLoadingScreen();
#ifndef EA_PLATFORM_PLAYSTATION2
    InitBigFiles();
    InitMemoryCard();
#endif
    LoadGlobalChunks();
    InitFrontendDatabase();

    TheOnlineManager.Initialize(argc, argv);
#ifdef EA_BUILD_A124
    // Solo la A124 (SLUS no lee m_simCondition): GetSimCondition es inline de conditionsimulator.hpp.
    if (ConditionSimulator::GetSimCondition() == SIMCONDITION_NTSCINEUROPE) {
        SetVideoMode(MODE_NTSC);
    }
#endif

    IOModule::GetIOModule().Initialize();
    FEManager::InitInput();
    IOModule::GetIOModule().EnableUpdating(true);
#ifdef EA_BUILD_A124
    DebugDraw::Get();
#endif
    EventManager::Init();
    Hermes::System::Init();
    Scheduler::Init(0.016666668f); // scaf-data r71: bloque +0x7A4
    WWorld::Init();
    EventSequencer::Init(0.0f); // scaf-data r71: bloque +0x7A8
#ifdef MILESTONE_BUILD
    InitScreenPrintf();
    DebugMenuInit();
#endif
    UnloadFrontEndVault();
    GManager::Init("GLOBAL\\GAMEPLAY.BIN");
    GRaceDatabase::Init();
    GManager::Get().InitializeRaceStreaming();
    GInfractionManager::Init();
    SmokeableSpawner::Init();
    InitCarRender();
    InitStandardModels();
    InitCarLoader();
    InitVehicleDamage();
    DebugVehicleSelection::Init();
    DebugWorld::Init();
#ifdef EA_BUILD_A124
    WDbgCollisionDraw::Get();
#endif
    InitStomper();
    Physics::Info::Init();
    InitAnimCtrls();
    InitSpaceNodes();
    TheICEManager.Init();
    FastMem::Lock();

#ifdef EA_BUILD_A124
    // Sin servidor de Juice se avisa en pantalla y se apaga; con el, la salida de texto va a Juice.
    if (!Juice::GameHook::Instance()->IsConnected()) {
        int lastPrintfState = DoScreenPrintf;
        DoScreenPrintf = 1;
        ScreenPrintf(80, -200, 10.0f, 0xffaa2222, "Juice Disabled");
        ScreenPrintf(80, -180, 10.0f, 0xffaa2222, "%s", Juice::MWExtension::GetJuiceBuildName());
        DoScreenPrintf = lastPrintfState;
        gJuiceEnabled = 0;
    } else {
        bSetUserPutStringFunction(JuicePutStringFunction);
    }
    Juice::MWCommands::Instance();
    JuiceStatsDB::Instance();
#endif

    PrintCapturedLoadingTime("InitializeEverything", "InitializeEverything");
}

void WriteFreekerBaseAddressBeacon() {}

void DisplayDebugScreenPrints() {}

void VerifyJoylogChecksum() {
    if (!Joylog::IsCapturing() && !Joylog::IsReplaying()) {
        return;
    }
#ifndef EA_PLATFORM_PLAYSTATION2
    if (Joylog::IsCapturing()) {
        Joylog::AddData(bDefaultSeed, 32, JOYLOG_CHANNEL_RANDOM);
    } else if (Joylog::IsReplaying()) {
        bDefaultSeed = Joylog::GetData(32, JOYLOG_CHANNEL_RANDOM);
    }
#endif
    uint16 world_checksum = 0;
    {
        const IVehicle::List &vehicles = IVehicle::GetList(VEHICLE_ALL);
        for (IVehicle::List::const_iterator iter = vehicles.begin(); iter != vehicles.end(); iter++) {
            IVehicle *vehicle = *iter;
            const UMath::Vector3 &position = vehicle->GetPosition();
            world_checksum += *reinterpret_cast<const uint32 *>(&position.x);
            world_checksum += *reinterpret_cast<const uint32 *>(&position.y);
            world_checksum += *reinterpret_cast<const uint32 *>(&position.z);
        }
    }
    uint16 menu_checksum = FEngGetActiveScreensChecksum();
    uint16 random_seed_checksum = bDefaultSeed & 0xffff;
    if (TheGameFlowManager.IsInGame()) {
        random_seed_checksum = 0;
    }
    uint16 real_loop_counter_checksum = RealLoopCounter;
    uint16 current_checksum;
    int checksum_error = 0;
    if (Joylog::IsReplaying()) {
        uint16 prev_world_checksum = Joylog::GetData(16, JOYLOG_CHANNEL_CHECKSUM);

        if (world_checksum != prev_world_checksum) {
            checksum_error = 1;
        }
        if (menu_checksum != Joylog::GetData(16, JOYLOG_CHANNEL_CHECKSUM)) {
            checksum_error++;
        }
        if (random_seed_checksum != Joylog::GetData(16, JOYLOG_CHANNEL_CHECKSUM)) {
            checksum_error++;
        }
        if (real_loop_counter_checksum != Joylog::GetData(16, JOYLOG_CHANNEL_CHECKSUM)) {
            checksum_error++;
        }
    } else {
        Joylog::AddData(world_checksum, 16, JOYLOG_CHANNEL_CHECKSUM);
        Joylog::AddData(menu_checksum, 16, JOYLOG_CHANNEL_CHECKSUM);
        Joylog::AddData(random_seed_checksum, 16, JOYLOG_CHANNEL_CHECKSUM);
        Joylog::AddData(real_loop_counter_checksum, 16, JOYLOG_CHANNEL_CHECKSUM);
    }
    if (checksum_error) {
#ifdef EA_BUILD_A124
        // La A124 lleva los ganchos de Juice: con Juice activo el error se registra alli y para su gestor de
        // repeticiones.
        if (!gJuiceEnabled)
#endif
        {
            bBreak();
            Joylog::StopReplaying();
        }
#ifdef EA_BUILD_A124
        else {
            Juice::GameHook::Instance()->AssertLog(0, "", "Main.cpp", "checksum_error && StopOnJoylogChecksumError",
                                                  "Joylog Checksum Error has occured");
            Juice::ReplayManager::Instance()->StopReplaying();
        }
#endif
    }
}

int TweakerPauseCamera = 0;

static float Main_AnimateFrame(float real_dT) {
    ProfileNode profile_node;
    static float camera_dt = 0.0f;
    if (real_dT <= 0.0f) {
        if (TheGameFlowManager.IsInFrontend()) {
            const float fe_dt_fix = 0.01f;
            CameraAI::Update(fe_dt_fix);
            Camera::UpdateAll(fe_dt_fix);
        }
        return 0.0f;
    }
    float game_dT = 0.0f;
    if (TweakerPauseCamera == 0) {
        camera_dt += real_dT;
    }
    static uint32 last_sim_tick_animated = 0;
    static uint32 last_render_frame_animated = 0;
    if (Sim::Exists() && Sim::GetState() == Sim::STATE_ACTIVE && last_sim_tick_animated != Sim::GetTick() &&
        last_render_frame_animated != eGetFrameCounter()) {
        game_dT = Sim::GetFrameTimeElapsed();
        if (game_dT <= 0.0f) {
            return 0.0f;
        }
        last_sim_tick_animated = Sim::GetTick();
        last_render_frame_animated = eGetFrameCounter();

        profile_node.Begin("WorldConn::UpdateServices()", 0);
        WorldConn::UpdateServices(game_dT);

        profile_node.Begin("SoundConn::UpdateServices()", 0);
        SoundConn::UpdateServices(game_dT);

        profile_node.Begin("RenderConn::UpdateServices()", 0);
        RenderConn::UpdateServices(game_dT);
    }
    profile_node.Begin("CameraUpdate", 0);
    CameraAI::Update(camera_dt);
    Camera::UpdateAll(camera_dt);
    camera_dt = 0.0f;
    return game_dT;
}

#ifndef EA_BUILD_A124
int gFramesToSkip = 0;

void Main_SkipFrame(int numToSkip) {
    gFramesToSkip = bMax(numToSkip, gFramesToSkip);
}
#endif

int RenderTimingStart = 0;
int RenderTimingEnd = 0;
int FrameTimingStartTime = 0;
int FrameTimingEndTime = 0;

extern float HackTime;

void Main_DisplayFrame() {
    ProfileNode profile_node;
    static float timeStep = 0.01666f;
#ifndef EA_BUILD_A124
    if (gFramesToSkip > 0) {
        gFramesToSkip--;
    } else
#endif
    {
        FrameTimingStartTime = FrameTimingEndTime;
        FrameTimingEndTime = bGetTicker();
        RenderTimingStart = bGetTicker();
        profile_node.Begin("eDisplayFrame()", 0); // scaf-data r71: $LC414 solo vivo por DWARF
        eDisplayFrame();
        RenderTimingEnd = bGetTicker();
        HackTime += timeStep;
        DisplayDebugScreenPrints();
        EnableInterrupts();
    }
}

void CheckTweakerTriggers() {
#ifdef EA_BUILD_A124
    // La A124 (una milestone) atiende aqui el volcado de memoria y los ajustes de coches; GameCube y SLUS la
    // tienen vacia.
    ServiceMemoryDump();
    TheCarLoader.ServiceTweakables();
#endif
}

#ifndef EA_BUILD_A124
void MainLoopCheckForFatalDiscError() {}
#endif

#ifdef EA_BUILD_A124
static void DoBuddyCoreProcessing() {
    BuddyCore::instance()->doprocessing();
}
#endif

void MiniMainLoop() {
    static int recursion_checker = 0;
    static int previous_ticks = 0;

    // TODO are these ifdefs right?
#ifdef EA_PLATFORM_PLAYSTATION2
    bMonitorService();
#endif
    bThreadYield(8);
#ifndef EA_BUILD_A124
    Sim::Suspend();
#endif
    float dt = bGetTickerDifference(previous_ticks);
    previous_ticks = bGetTicker();
    PrepareRealTimestep(dt * 0.001f);
    ServiceResourceLoading();
#ifndef EA_BUILD_A124
    ServiceFileStats();
    MainLoopCheckForFatalDiscError();
#endif
    IOModule::GetIOModule().Update();
    FEManager::Get()->Update();
    TheTrackStreamer.ServiceGameState();
    TheTrackStreamer.ServiceNonGameState();
#ifndef EA_BUILD_A124
    if (g_pEAXSound) {
        g_pEAXSound->Update(RealTimeElapsed);
    }
#endif
    eDisplayFrame();
    AdvanceRealTime();
    recursion_checker--;
}

bool Tweak_FullSpeedMode = false; // Decl: 2585
bool twkDumpProfileMarks = false; // Decl: 2586

void MainLoop(
#ifdef EA_PLATFORM_PLAYSTATION2
    bFix hardware_ms_fix
#else
    float hardware_ms
#endif
) {
#ifdef EA_PLATFORM_PLAYSTATION2
    // PS2 (A124 y SLUS) recibe los milisegundos en coma fija 16.16 (MainLoop__Fi) y los pasa a float aqui.
    float hardware_ms = hardware_ms_fix / 65536.0f;
#endif
    int ret = bGetTicker();
    ProfileNode profile_node_entire_mainloop("MainLoop()", 0);
    ProfileNode profile_node;
    MainLoopBreakpoint();
    PrepareRealTimestep(hardware_ms * 0.001f);
    bSyncTaskRun();
    profile_node_entire_mainloop.Begin("IOModule::Update()", 0);
    IOModule::GetIOModule().Update();
#ifdef EA_BUILD_A124
    // La red de la A124 (SLUS no tiene online): con la red en marcha, o en los modos online y LAN del frontend, se
    // atiende con la pila en la memoria principal; en carrera online la simulacion se sincroniza con StartSimFrame.
    if (NetworkCore::Instance().IsInitialized() || FEDatabase->IsOnlineMode() || FEDatabase->IsLanMode()) {
        if (TheOnlineManager.IsOnlineRace() && TheOnlineManager.GetState() == OLS_RACING) {
            TheOnlineManager.StartSimFrame();
        } else {
            DoWithMainMemoryStack((void *)NetworkCore::DoNetworkProcessing, 0, 0);
        }
        DoWithMainMemoryStack((void *)DoBuddyCoreProcessing, 0, 0);
    }
#endif
    profile_node_entire_mainloop.Begin("Scheduler::Run()", 0);
    float scheduler_time_elapsed = Scheduler::Get().Run(Tweak_FullSpeedMode);
    if (TheOnlineManager.IsOnlineRace() && TheOnlineManager.GetState() == OLS_RACING) {
        TheOnlineManager.EndSimFrame();
    }
    Attrib::Database::Get().CollectGarbage();
    profile_node_entire_mainloop.Begin("Main_AnimateFrame()", 0);
    float game_dT = Main_AnimateFrame(scheduler_time_elapsed);
    PrepareWorldTimestep(game_dT);
    last_frame_count = FrameCounter;
#ifdef EA_PLATFORM_PLAYSTATION2
    bMonitorService();
    bFunkService();
#endif
    ServiceJoylog();
    CheckTweakerTriggers();
    profile_node_entire_mainloop.Begin("MainLoop - ServiceGameFlow", 0);
    profile_node.End();
    TheGameFlowManager.Service();
    TheGameFlowManager.CheckForDemoDiscTimeout();
    emProcessAllEvents();
    if (Sim::Exists()) {
        World_Service();
    }
    ServicePlatform();
    ServicePreculler();
    FEManager::Get()->Update();
#ifndef EA_BUILD_A124
    EventManager::RunEvents();
#endif
    // Los Begin van fuera de los if: los stabs de la A124 y el DWARF de GameCube los ponen en el bloque de antes.
    profile_node_entire_mainloop.Begin("EAXSound::Update()", 0);
    if (g_pEAXSound) {
        g_pEAXSound->Update(RealTimeElapsed);
    }
    profile_node_entire_mainloop.Begin("ServiceResourceLoading etc.", 0);
    ServiceResourceLoading();
#ifndef EA_BUILD_A124
    MainLoopCheckForFatalDiscError();
#endif
    RenderConn::UpdateLoading();
    TheTrackStreamer.ServiceNonGameState();
#ifndef EA_BUILD_A124
    HandleTrackStreamerLoadingBar();
#endif
    TickOverTimeOfday();
    profile_node_entire_mainloop.Begin("Main_DisplayFrame()", 0);
    if (!Tweak_FullSpeedMode) {
        Main_DisplayFrame();
    }
    profile_node_entire_mainloop.Begin("MainLoop wind down", 0);
    Sim::StartProfile();
    VerifyJoylogChecksum();
    AdvanceWorldTime();
    MaybePrintUnusedTextures();
    MaybeDoMemoryProfile();
#ifdef EA_BUILD_A124
    // La A124 (milestone) atiende aqui el menu de depuracion, la memoria libre del IOP y los ganchos de Juice.
    if (gDebugMenuIOHandler) {
        gDebugMenuIOHandler->PollInput();
    }
    if (gbLogFreeIOPMemToTTY && TheICEManager.IsEditorOff()) {
        unsigned int IOPMaxFree = sceSifQueryMaxFreeMemSize();
        unsigned int IOPFree = sceSifQueryTotalFreeMemSize();
        unsigned int IOPMem = sceSifQueryMemSize();
        unsigned int IOPUsed = IOPMem - IOPFree;
        // Por la TTY el paso a K se hace sobre las propias variables (del if solo queda la lectura de
        // gbShowFreeIOPMemInKb). Lo que se saca por la TTY no llega al binario (bPrintf no genera codigo en esta
        // build) y su cadena no se conoce, asi que no se escribe.
        if (gbShowFreeIOPMemInKb) {
            IOPMaxFree /= 1024;
            IOPFree /= 1024;
            IOPMem /= 1024;
            IOPUsed /= 1024;
        }
    }
    if (gbShowFreeIOPMem && TheICEManager.IsEditorOff()) {
        unsigned int IOPMaxFree = sceSifQueryMaxFreeMemSize();
        unsigned int IOPFree = sceSifQueryTotalFreeMemSize();
        unsigned int IOPMem = sceSifQueryMemSize();
        unsigned int IOPUsed = IOPMem - IOPFree;
        if (gbShowFreeIOPMemInKb) {
            ScreenPrintf(180, 100, "--- IOP Free ---\n\nTotal: %dk\nUsed: %dk\nFree:  %dk\nBiggest: %dk", IOPMem / 1024,
                         IOPUsed / 1024, IOPFree / 1024, IOPMaxFree / 1024);
        } else {
            ScreenPrintf(180, 100, "--- IOP Free ---\n\nTotal: %d\nUsed: %d\nFree:  %d\nBiggest: %d", IOPMem, IOPUsed,
                         IOPFree, IOPMaxFree);
        }
    }
    JuiceStatsDB::Instance()->UpdateTimers();
    Juice::GameHook::Instance()->FrameUpdate();
#endif
    AdvanceRealTime();
    profile_node_entire_mainloop.End();
    profile_node.End();
#ifdef EA_BUILD_A124
    pCommandProc->setTickCount();
#endif

#ifndef EA_PLATFORM_PLAYSTATION2
    static int PrintFreeMem = 0;
    if (PrintFreeMem != 0) {
        int free_memory = bCountFreeMemory(0);
#ifdef EA_PLATFORM_GAMECUBE
        OSReport("Free memory %dK\n", free_memory / 1024);
#else
#endif
    }
#endif
}

#ifndef EA_PLATFORM_XENON
int main(int argc, char **argv)
#else
int MainThreadFunction(int argc, char **argv)
#endif
{
#ifdef EA_PLATFORM_WIN32
    // PC version wants there to only be one instance at a time (speed.exe 0x666590)
    if (IsTheGameAlreadyRunning("speed.exe", 1)) // Platform_PC.cpp
        _exit(0);
#endif

    InitializeEverything(argc, argv);

    WriteFreekerBaseAddressBeacon();

    if (SkipFE)
        RaceStarter::StartSkipFERace();
    else
        TheGameFlowManager.LoadFrontend();

    frames_elapsed = 1;

    loop_ticker = bGetTicker();
    Scheduler::Get().Synchronize(RealTimer);

    while (!ExitTheGameFlag) {
        const float minumum_time_step = 0.25f;

        uint32 current_tick = bGetTicker();
        float milliseconds = bGetTickerDifference(loop_ticker, current_tick);
        bFix milliseconds_fix;

        if (current_tick != loop_ticker && milliseconds <= 0.0f) {
            loop_ticker = current_tick;
            continue;
        }

        if (milliseconds <= minumum_time_step)
            continue;

        loop_ticker = current_tick;
        if (milliseconds > 3.2e+04f)
            milliseconds = 3.2e+04f;

        milliseconds_fix = bFloatToInt(milliseconds * 65536.0f); // this is used on other platforms

#ifdef EA_PLATFORM_PLAYSTATION2
        // Con StackOnScratchPad 1 o 2 el bucle corre con la pila en el scratchpad del EE (0x70000000-0x70004000); con
        // 2 la simulacion vuelve a la pila de memoria principal y usa el scratchpad como espacio de trabajo, y con 0
        // solo lo usa como espacio de trabajo.
        if (StackOnScratchPad > 0) {
            MainMemoryStackPos = bGetStackPointer();
            if (StackOnScratchPad == 2) {
                Sim::SetWorkspace((void *)0x70000000, (void *)MainMemoryStackPos);
            } else {
                Sim::SetWorkspace(nullptr, nullptr);
            }
            bDoWithStack((void *)MainLoop, (void *)0x70004000, milliseconds_fix, 0);
        } else {
            if (StackOnScratchPad == 0) {
                Sim::SetWorkspace((void *)0x70000000, nullptr);
            }
            MainLoop(milliseconds_fix);
        }
#else
        MainLoop(milliseconds);
#endif

        if (twkDumpProfileMarks)
            twkDumpProfileMarks = false;
    }

    return 0;
}
