#ifndef MISC_TEST_HOOKS_JUICE_HOOKS_H
#define MISC_TEST_HOOKS_JUICE_HOOKS_H

#ifdef EA_PRAGMA_ONCE_SUPPORTED
#pragma once
#endif

#include <types.h>
#include "Speed/Indep/Libs/Support/Utility/UCOM.h"
#include "Speed/Indep/Src/Input/Action.h"
#include "Speed/Indep/Src/Input/ActionQueue.h"
#include "Speed/Indep/Src/Frontend/Database/FEDatabase.hpp"
// ICountdown vive en IFengHud.h. Esta cabecera la redefinia con los metodos
// como virtuales NO puros, cosa que cambia que vtables emite GCC, y ademas
// rompia en PS2 toda unidad que incluyera las dos.
#include "Speed/Indep/Src/Interfaces/IFengHud.h"
// MoviePlayer tambien: aqui habia un `struct MoviePlayer` de juguete con un
// solo metodo, cuando la de verdad es una `class` de MoviePlayer.hpp.
#include "Speed/Indep/Src/Frontend/MoviePlayer/MoviePlayer.hpp"

class IPlayer;
enum FormationType;
struct FEPackage;

struct ProtoAriesRefT;

extern bool gJuiceEnabled;

// MWExtension.cpp: arranque de Juice y la salida de texto que se le pasa a bSetUserPutStringFunction.
void InitJuice();
bool JuicePutStringFunction(int terminal_channel, const char *s);

namespace Juice {

// Los botones del mando de Juice (stabs de la A124); JuicePad::SetButton e IsButtonPressed los reciben con este tipo.
enum eJuiceButtonNames {
    JUICEPAD_DPADUP = 1,
    JUICEPAD_DPADDOWN = 2,
    JUICEPAD_DPADLEFT = 3,
    JUICEPAD_DPADRIGHT = 4,
    JUICEPAD_BUTTON1 = 5,
    JUICEPAD_BUTTON2 = 6,
    JUICEPAD_BUTTON3 = 7,
    JUICEPAD_BUTTON4 = 8,
    JUICEPAD_BUTTONL1 = 9,
    JUICEPAD_BUTTONL2 = 10,
    JUICEPAD_BUTTONL3 = 11,
    JUICEPAD_BUTTONR1 = 12,
    JUICEPAD_BUTTONR2 = 13,
    JUICEPAD_BUTTONR3 = 14,
    JUICEPAD_BUTTONSTART = 15,
    JUICEPAD_BUTTONSELECT = 16,
    JUICEPAD_LEFTANALOG_UPDOWN = 17,
    JUICEPAD_LEFTANALOG_LEFTRIGHT = 18,
    JUICEPAD_RIGHTANALOG_UPDOWN = 19,
    JUICEPAD_RIGHTANALOG_LEFTRIGHT = 20,
    LAST_BUTTON = 21
};

struct tJuicePad {
    unsigned int mButtons;
    float mButtonVal[32];
};

class IExtension {
public:
    static IExtension *sCurrentExtension;
    IExtension();
    ~IExtension() {}
    virtual char *GetTitleName();
    virtual char *GetBuildName();
    virtual char *GetChangeList();
    virtual char *GetConsoleTarget();
    virtual char *GetXboxPath();
    virtual char *GetXboxExeName();
    virtual char *GetServerIP();
    virtual char *GetConsoleName();
    virtual int GetServerPort();
    virtual int GetReadyToReset();
    virtual void FileSyncUpdate();
    virtual void GamePadToJuicePad(void *gameInput, tJuicePad *juiceInput);
    virtual void JuicePadToGamePad(tJuicePad *juiceInput, void *gameInput);
    virtual int HasFileLoaded(const void *fileHandle);
    virtual char *GetBuildDate();
    virtual unsigned int GetInitializationTimeOut();
    virtual unsigned int GetHeartBeatTimeOut();
    virtual char *GetBackupServerIP();
    virtual int GetBackupServerPort();
    virtual int NetworkInitRetries();
    virtual int InitialConnectionRetries();
    virtual int WaitBetweenInitialConnectionRretries();
    virtual int WaitBetweenNetworkInitRetries();
    virtual void ThreadYield(int dur);
    virtual char *GetScreenName();
    virtual char *GetPopupName();
    virtual char *GetPopupCheck();
    virtual char *GetCursorPos();
    virtual float GetGameTime();
    virtual int IsOkToConnect();
    virtual void InstantiateScriptExt();
    virtual void ResetGamePad();
    virtual int HangTimeoutLength();
    virtual void UpdatePad(void *input);
    virtual void JuiceScreenshot(char *fileName);
    virtual int NumberOfRepeatedReplayEntries(int channel);
    virtual int GetJuiceThreadPriority();
    virtual int GetJuiceUpdateProcessor();
    virtual void PauseGame();
    virtual void UnPauseGame();
};

class INetwork {
public:
    ~INetwork();
    INetwork();
    virtual int Connect(const char *ip, int port);
    virtual int IsConnected();
    virtual int Recv(char *buffer, int *size, int flags);
    virtual int PeekHdr(char *buffer);
    virtual int Send(int length, int channel, char *buffer);
    virtual int IsEnabled();
    virtual int Initialize();
    virtual int Disconnect();
    virtual void MaintainConn();
    virtual void TearDown();
    virtual void FreeEverything();
    virtual char *GetMac();
    virtual char *GetLocalIpAddress();
};

class JuicePad {
public:
    static JuicePad *Instance();
    void SetButton(eJuiceButtonNames button, float value);
    int IsButtonPressed(eJuiceButtonNames button);
    int DidInputHappen();
    tJuicePad *GetMasterPad();
    void UpdateJuicePad();
    void ResetButtons();
};

class JuiceDirtyNet : public INetwork {
private:
    static JuiceDirtyNet *mInstance;
    ProtoAriesRefT *mAries;

public:
    static JuiceDirtyNet *Instance();
    JuiceDirtyNet();
    virtual ~JuiceDirtyNet();
    virtual int Connect(const char *ip, int port);
    virtual int IsConnected();
    virtual int Recv(char *buffer, int *size, int flags);
    virtual int PeekHdr(char *buffer);
    virtual int Send(int length, int channel, char *buffer);
    virtual int IsEnabled();
    virtual int Initialize();
    virtual int Disconnect();
    virtual void MaintainConn();
    virtual void TearDown();
    virtual void FreeEverything();
    virtual char *GetMac();
    virtual char *GetLocalIpAddress();
};

struct GameHook {
    static GameHook *(*Instance)();
    int mEnabled;
    int mFrame;
    void LogText(const char *text);
    int IsConnected();
    void DisableJuice();
    void Initialize();
    void InitTrapHandler();
    void FrameUpdate();
    void GameEvent(const char *world, const char *event, const char *outcome, int fpsAsInt, float fps,
                   const char *zoneName, const char *imageName, const char *playerPosition);
    void LogStat(int stat, const char *statType, const char *raceType, int value);
    void AssetHit(const char *assetName, const char *assetValue);
    void UpdatePad(void *pPad);
    int ReplayData(int &data, int channel, const char *name, int flags);
    float ReplayData(float &data, int channel, const char *name, int flags);
    int Replay_IsReplaying();
    int Replay_IsCapturing();
#ifdef MILESTONE_BUILD
    void AdvanceCycle(int cycle);
#endif
    void AssertLog(int, const char *, const char *, const char *, const char *);
#ifdef EA_PLATFORM_XENON
    unsigned int CalculateUsedBytes(unsigned int iData); // Xbox 360 0x8288BDF0 (1.5.2a: Juice.cpp)
    // Xbox 360 0x8288B908: MainLoop la llama en cada vuelta con el valor que InitializeEverything guarda en 0x8296D17C
    // (lo devuelve 0x8288B9B0 a partir del id del hilo principal, "Main Execution"); probablemente una muestra del
    // perfilador de hilos de Juice. Ningun binario da el nombre (el Juice de PS2 no la tiene): nombre neutro.
    void XenonUnknown8288B908(int value);
    // Xbox 360 0x8288BA18, 0x8288B900, 0x8288B9B0 y 0x8288BA08: InitializeEverything las llama con Juice conectado
    // (65000; sin argumentos; el id del hilo principal, que devuelve el valor de 0x8296D17C; el id y "Main
    // Execution"): probablemente el alta del perfilador de hilos de Juice. Sin nombre en ningun binario: neutros.
    void XenonUnknown8288BA18(int value);
    void XenonUnknown8288B900();
    int XenonUnknown8288B9B0(unsigned int threadId);
    void XenonUnknown8288BA08(unsigned int threadId, const char *name);
#endif
    int GetFrame() {
        return mFrame;
    }
    unsigned int GetCurrentSystemTime();
    unsigned int GetTimeElapsed(unsigned int &startTime);
};

// El gestor de repeticiones de Juice (biblioteca aparte): el juego solo lo para cuando el joylog se desincroniza.
#ifdef EA_PLATFORM_XENON
enum ReplayChannel {
};
#endif
class ReplayManager {
  public:
    static ReplayManager *Instance();
    void StopReplaying();
#ifdef EA_PLATFORM_XENON
    // Xbox 360 (Joylog::IsReplaying): devuelven un byte (los llamantes lo recortan); en la 1.5.2a de la A124 son
    // int. IsReplaying lee el estado de segmento (+0x10) y m_pSettings[estado]; IsReplayEnabled, m_bActive (+0xC).
    bool IsReplaying();
    bool IsReplayEnabled();
    // Joylog::AddData/GetData de Xbox 360 (0x823EAB18, 0x823EA9B0); por forma con la 1.5.2a (JuiceReplayManager.cpp).
    bool CheckPlaybackMask(ReplayChannel eChannel);
    bool CheckRecordMask(ReplayChannel eChannel);
    bool CheckOverWriteMask(ReplayChannel eChannel);
    int GetData(int iSize, ReplayChannel eChannel);
    void AddData(int iData, int iSize, ReplayChannel eChannel);
#endif
};

struct PadConfigManager {
    static PadConfigManager *Instance();
    bool IsFENavCapturing();
    void CaptureKeyAndState(unsigned int buttonMask);
};

namespace Scripting {

struct VarArgs {
    int mNextType;
    int mNumberOfArgs;

    int GetNumberOfRemainingArgs() const;
    int GetInt(int &result);
    float GetFloat(float &result);
    char *GetString(char *&result);
};

class ICommands {
public:
    ICommands();
    virtual ~ICommands();
    virtual void AutoRegister() const;
    virtual char *GetNamespaceName() const;
    static void SendMethod(int type, const ICommands *obj, const char *methodName, int argSize,
                           const void *method);

    template <class C, class R>
    static void SendMethod(int type, const ICommands *obj, const char *methodName, int argSize,
                           R (C::* const method)(VarArgs &)) {
        R (ICommands::*n)(VarArgs &);
        R (ICommands::*m)(VarArgs &);
        m = static_cast<R (ICommands::*)(VarArgs &)>(method);
        n = m;
        SendMethod(type, obj, methodName, argSize, static_cast<const void *>(&n));
    }
};

}

class MWCommands : public Scripting::ICommands {
private:
    static unsigned int mFEngScreenLoading;

public:
    static MWCommands *Instance();
    MWCommands();
    virtual char *GetNamespaceName() const;
    static void LoadingNewFEngPackage(unsigned int newPkg);
    int TurnDebugTextOn(Scripting::VarArgs &params);
    int TurnDebugTextOff(Scripting::VarArgs &params);
    int IsInFreeRoam(Scripting::VarArgs &params);
    int UnlockAllThings(Scripting::VarArgs &params);
    int InfRaceBreaker(Scripting::VarArgs &params);
    int IsSplitScreen(Scripting::VarArgs &params);
    int PassChallenge(Scripting::VarArgs &params);
    int PassSpeedtrap(Scripting::VarArgs &params);
    int AwardBounty(Scripting::VarArgs &params);
    int GetBountyValue(Scripting::VarArgs &params);
    char *GetTrackID(Scripting::VarArgs &params);
    int IsPursuit(Scripting::VarArgs &params);
    int InFrontEnd(Scripting::VarArgs &params);
    int InDriveMode(Scripting::VarArgs &params);
    int SetHeat(Scripting::VarArgs &params);
    int FinishRaceInPlace(Scripting::VarArgs &params);
    int TeleportToCoords(Scripting::VarArgs &params);
    int IsCarStuck(Scripting::VarArgs &params);
    char *DisplayFMVfilename(Scripting::VarArgs &params);
    int SetFrameRateThreshold(Scripting::VarArgs &params);
    int ScreenLoaded(Scripting::VarArgs &params);
    int TurnPursuitOn(Scripting::VarArgs &params);
    int TurnPursuitOff(Scripting::VarArgs &params);
    int TurnPursuitForeverOn(Scripting::VarArgs &params);
    int TurnPursuitForeverOff(Scripting::VarArgs &params);
    int SpeedBoost(Scripting::VarArgs &params);
    int TurnAutoPilotOn(Scripting::VarArgs &params);
    int TurnAutoPilotOff(Scripting::VarArgs &params);
    int ScatterCops(Scripting::VarArgs &params);
};

}

enum MWJuicePadButtonType {
    JUICE_UP = 0,
    JUICE_DOWN,
    JUICE_LEFT,
    JUICE_RIGHT,
    JUICE_ACCEPT,
    JUICE_CANCEL,
    JUICE_Y,
    JUICE_X,
    JUICE_L1,
    JUICE_R1,
    JUICE_WHITE,
    JUICE_BLACK,
    JUICE_LSTICKBUTTON,
    JUICE_RSTICKBUTTON,
    JUICE_START,
    JUICE_BACK,
    JUICE_R3,
    JUICE_L3,
    JUICE_LEFTANALOG_LEFT,
    JUICE_LEFTANALOG_RIGHT,
    JUICE_LEFTANALOG_UP,
    JUICE_LEFTANALOG_DOWN,
    JUICE_RIGHTANALOG_LEFT,
    JUICE_RIGHTANALOG_RIGHT,
    JUICE_RIGHTANALOG_UP,
    JUICE_RIGHTANALOG_DOWN,
    JUICE_MAX_BUTTON
};

enum MWJPadReleaseType {
    JUICE_NORMAL = 0,
    JUICE_FORCED
};

struct MWJuicePadState {
    bool buttonState[26];
};

struct MWJuicePad {
private:
    static MWJuicePad *mInstance;
    MWJuicePadState mPadState[2];
    MWJuicePadState mTrackSegments[2];
    bool mIsInBE;
    bool mButtonPressedLastFrame;
    ActionID FEActionMapping[26];
    ActionID BEActionMapping[26];
    ActionQueue *mInputQueue;
    bool mIsLastFrame;
    bool mResetSegmentPresses;

public:
    static MWJuicePad *Instance();
    void Initialize();
    void PressButton(int port, MWJuicePadButtonType buttonType);
    void ReleaseAllButtons(MWJPadReleaseType type);
    void PollInput();
    void SetIsLastFrame(bool val);
    bool IsLastJoyFrame();
    void TrackButtonPress(int actionId, float data, int port);
    void ResetGamePad();
    void ReleaseSegmentPresses();

protected:
    MWJuicePad();

private:
    void SimulateFEButton(int port, int buttonID, float buttonData);
    void SimulateBEButton(int port, int buttonID, float buttonData);
    ActionQueue *GetActionQueue(const char *qName, int port);
    void TrackFEPresses(int actionID, float data, int port);
    void TrackBEPresses(int actionID, float data, int port);
    void ResetTrackedPresses();
};

enum JuicePerRaceStatType {
    JUICE_COP_CAR_SPAWN = 0,
    JUICE_COP_CAR_UNSPAWN,
    JUICE_COP_CHOPPER_SPAWN,
    JUICE_COP_CHOPPER_UNSPAWN,
    JUICE_TRAFFIC_SPAWN,
    JUICE_TRAFFIC_UNSPAWN,
    JUICE_CURRENT_HEAT,
    JUICE_MAX_HEAT,
    JUICE_CURRENT_CAMERA,
    JUICE_CURRENT_RACE,
    JUICE_MAX_PER_RACE_STAT
};

enum JuiceRaceType {
    JUICE_FREE_ROAM = 0,
    JUICE_CIRCUIT,
    JUICE_P2P,
    JUICE_OTHERRACE
};

enum CareerDataType {
    JUICE_REP_EARNED = 0,
    JUICE_CASH_EARNED,
    JUICE_PURSUIT_LENGTH,
    JUICE_MAX_CAREER_DATA
};

struct JuiceStatsDB {
private:
    static JuiceStatsDB *mInstance;
    int mPerRaceStatsDB[10];
    int mFrameOfLastInc[10];
    bool mShouldDumpStats;
    int mCareerIntDataDB[15][3];
    float mCareerFloatDataDB[15][3];
    bool mShouldDumpCareerData;
    float mTimeAtHeatLevel[11];
    float mTimeInCameraMode[6];
    float mTimeInRaceType[6];
    unsigned int mCurrentTicker;
#ifdef EA_PLATFORM_XENON
    // X360 (EGM): el resumen de FPS. Nombres y tipos del PDB del prototipo de ProStreet (Sep 16, 2007), que los
    // pone tras mCurrentTicker como aqui (0x220-0x237 en el EGM; mCurrentOptions va en 0x238 y el total es 0x2C4).
    int mRecordState;
    float mTotalFps;
    float mLowestFps;
    int mTotalFrames;
    int mFramesAbove;
    float mSummaryFPS;
#endif
    OptionsSettings mCurrentOptions;

public:
    static JuiceStatsDB *Instance();
    JuiceStatsDB();
    void SubmitStats(JuiceRaceType raceType);
    void SubmitCareerData(const char *profileName);
    void CompareOptions(OptionsSettings *compare);
    void ResetPerRaceStats();
    void IncrementPerRaceStat(JuicePerRaceStatType statType);
    void UpdateTimers();
    void ResetCareerData();
    void SetHeat(int heat);
    void SetCamera(int camID);
#ifdef EA_PLATFORM_XENON
    void SetRace(int raceType);
    void AddFrame(float fps);
    float GetAverageFPS();
    float GetPercOfFramesAbove();
    void ResetFPSData();
#endif
    // Inline en la A124 (L88 de su cabecera; el constructor de UIOptionsScreen lo expande): el parametro va por
    // valor, de ahi las dos copias de 0xC0 bytes (a la pila y de la pila a mCurrentOptions).
    void SetOptions(OptionsSettings options) {
        mCurrentOptions = options;
    }
};

namespace Juice {

class MWExtension : public IExtension {
private:
    int mHasExecutedRPC;
    int mScreenShotHandle;
    static char mFileName[256];
    static char mJuiceBuildName[32];

public:
    static MWExtension *Instance();
    MWExtension();
    void ThreadYield(int dur);
    char *GetTitleName();
    char *GetBuildDate();
    char *GetChangeList();
    char *GetConsoleName();
    int HasFileLoaded(const void *fileHandle);
    int NumberOfRepeatedReplayEntries(int channel);
    int GetReadyToReset();
    void FileSyncUpdate();
    char *GetServerIP();
    int GetServerPort();
    int GetBackupServerPort();
    char *GetBackupServerIP();
    char *GetXboxPath();
    char *GetXboxExeName();
    char *GetConsoleTarget();
    unsigned int GetIntializationTimeOut();
    unsigned int GetHeartBeatTimeOut();
    float GetGameTime();
    int IsOkToConnect();
    void InstantiateScriptExt();
    int NetworkInitRetries();
    int InitialConnectionRetries();
    int WaitBetweenInitialConnectionRretries();
    int WaitBetweenNetworkInitRetries();
    char *GetPopupName();
    char *GetPopupCheck();
    int GetJuiceUpdateProcessor();
    static char *GetImageName();
    static char *GetPlayerPosition();
    static char *GetPlayerPosition(IPlayer *player);
    static char *GetFormationStr(const FormationType &formation);
    static char *GetCurrentRaceType();
    static char *GetCurrentZoneName();
    static void JuiceLowFrameRateLog(const float &fps);
    char *GetScreenName();
    char *GetCursorPos();
    void UpdatePad(void *input);
    void ResetGamePad();
    void GamePadToJuicePad(void *gameInput, tJuicePad *juiceInput);
    void JuicePadToGamePad(tJuicePad *juiceInput, void *gameInput);
    void JuiceScreenshot(char *fileName);
    static char *GetJuiceBuildName();
    int HangTimeoutLength();
    char *GetBuildName();
};

}

#endif
