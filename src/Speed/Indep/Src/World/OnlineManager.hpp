#ifndef ONLINE_MANAGER_HPP
#define ONLINE_MANAGER_HPP

// TODO check #if ONLINE_PLATFORM instead
#if defined(EA_PLATFORM_GAMECUBE)
#elif defined(EA_PLATFORM_XENON)
#define ONLINE_ENABLED
#elif defined(EA_PLATFORM_PLAYSTATION2)
#define ONLINE_ENABLED
#endif

#include <types.h>

#if ONLINE_SUPPORT
// On PS2 the online subsystem is in (zOnline) and this is the real manager: 0x1704 B in the A124, with its methods
// out of line (zWorld) and called from the other units. GameCube only has the empty stub further down.
#include "Speed/Indep/Libs/Support/Utility/UDefs.h"
#include "Speed/Indep/Libs/Support/Utility/FastMem.h"
#include "Speed/Indep/Libs/Support/Utility/UStandard.h"
#include "Speed/Indep/Libs/Support/Utility/UTypes.h"
#include "Speed/Indep/Src/Debug/Debugable.h"
#include "Speed/Indep/Src/Frontend/Database/RaceDB.hpp"
#include "Speed/Indep/Src/Frontend/Database/VehicleDB.hpp"
#include "Speed/Indep/Src/Interfaces/Simables/ISimable.h"
#include "Speed/Indep/Src/Misc/Timer.hpp"
#include "Speed/Indep/Src/Online/FloatQuantizer.hpp"
#include "Speed/Indep/Src/Online/IntQuantizer.hpp"
#include "Speed/Indep/Src/Online/NetworkMutex.hpp"
#include "Speed/Indep/Tools/AttribSys/Runtime/AttribSys.h"

class SmartBitStream;
struct Client;
struct WorldObject;
class IVehicle;
class RideInfo;
#endif

enum eOnlineState {
    OLS_DISCONNECTED = 0,
    OLS_LOBBY_IN_LOBBY = 1,
    OLS_RACE_DATA_SYNC = 2,
    OLS_RACE_LOAD_TRACK = 3,
    OLS_RACE_START_LINE = 4,
    OLS_RACING = 5,
    OLS_RACE_END = 6,
    NUM_OLS_STATES = 7,
};

#if ONLINE_SUPPORT
enum eOnlineRacerState {
    OPS_DISCONNECTED = 0,
    OPS_CONNECTED = 1,
    OPS_RACING = 2,
    OPS_FINISHED = 3,
    OPS_LOST_CONNECTION = 4,
    OPS_DISCERROR = 5,
    OPS_QUIT = 6,
    NUM_OPS_STATES = 7,
};

enum eOnlineCheatReason {
    CHEAT_NONE = -1,
    CHEAT_FALSESTART = 0,
    CHEAT_TIMER_FUDGE = 1,
    CHEAT_TOOSLOW = 2,
    CHEAT_TOOMUCH_NOS = 3,
    CHEAT_IGNORED_TIMEUP = 4,
    CHEAT_DATA_TAMPER = 5,
    CHEAT_RESERVED = 6,
    CHEAT_TOOMUCH_SPEED = 7,
    CHEAT_TELEPORT = 8,
    CHEAT_TOOMUCH_ACCEL = 9,
    CHEAT_DRIFTSCORE = 10,
    CHEAT_NUMLAPS = 11,
    CHEAT_STYLEMOMENTS = 12,
    CHEAT_CODETAMPER = 13,
    CHEAT_INVALIDRIDE = 14,
    CHEAT_OTHER4 = 15,
    NUM_CHEAT_REASONS = 16,
};

enum eOnlineRaceEndReason {
    OEND_RACE_FINISHED = 0,
    OEND_NO_PLAYERS = 1,
    OEND_DISCONNECTED = 2,
    OEND_REJECTED = 3,
};

enum ePosDataPriority {
    PDP_UNSPECIFIED = -1,
    PDP_CRITICAL = 0,
    PDP_NORMAL = 1,
    PDP_LOW = 2,
    PDP_AESTHETIC = 3,
    NUM_POSDATA_PRIORITIES = 4,
    PDP_INFLIGHT = 5,
};

enum ePosDataPriorityMask {
    PDP_MASK_NONE = 0,
    PDP_MASK_CRITICAL = 1,
    PDP_MASK_NORMAL = 2,
    PDP_MASK_LOW = 4,
    PDP_MASK_AESTHETIC = 8,
    PDP_MASK_INFLIGHT = 32,
    PDP_MASK_ALL = 47,
};

// total size: 0x1160 (the empty Debugable base takes a byte: the state array starts at 0x10)
struct ExtrapolatedCar : Debugable {
    struct State;

    ExtrapolatedCar(Attrib::Key cartype);
    ~ExtrapolatedCar();

    bool IsStateListEmpty() const;
    void ExportSimable(ISimable *simable);
    void SaveToStream(SmartBitStream &data, ePosDataPriorityMask priority_mask);
    void ExportStream(SmartBitStream &data, ePosDataPriorityMask priority_mask);
    void ImportStream(SmartBitStream &data, ePosDataPriorityMask priority_mask);
    void ImportSimable(ISimable *simable, float t, float simtime);
    // Inline tambien en el prototipo (ExtrapolatedCar.hpp:50 segun su mapa de lineas): no existe
    // como funcion en ninguna version.
    bool WantsDriverAI() const {
        return mUseDriverAI;
    }
    void ExtractExtrapolatedPosition(UMath::Vector3 &position) const;
    void ExtractExtrapolatedDirection(UMath::Vector3 &direction) const;
    void Reset(ISimable *simable);
    float GetLastTime() const {
        return mSaved.GetTime();
    }
    float GetLatency() const {
        return mUpdateTime * 0.001f - GetLastTime();
    }
    float GetPercentReceived() const;
    void IncreaseRepositionCount() {
        mRepositionCount = (mRepositionCount + 1) & 3;
    }
    void Pause();
    void UnPause();
    ISimable *SpawnVehicle() {
        return mLast->SpawnVehicle(mCarType);
    }
    bool HasHeadset() const;
    bool IsAbleToSee(ExtrapolatedCar &target);
    Attrib::Key &GetCarType() {
        return mCarType;
    }

    // total size: 0x80. Va detras de los metodos: la A124 la define despues (ExtrapolatedCar.hpp:85 frente
    // a :54), y por eso GetLastTime llama a State::GetTime en vez de expandirla.
    struct ALIGNVEC State {
      private:
        UMath::Vector3 mPosition;           // offset 0x0
        float mSteering;                    // offset 0xC
        UMath::Vector3 mLinearVelocity;     // offset 0x10
        float mGas;                         // offset 0x1C
        UMath::Vector3 mLinearAcceleration; // offset 0x20
        float mBrake;                       // offset 0x2C
        UMath::Vector4 mRotation;           // offset 0x30
        UMath::Vector4 mAngularVelocity;    // offset 0x40
        UMath::Vector4 mAngularAcceleration; // offset 0x50
        float mHandBrake;                   // offset 0x60
        float mBlendRate;                   // offset 0x64
        float mBlend;                       // offset 0x68
        float mTime;                        // offset 0x6C
        bool mInFlight;                     // offset 0x70
        bool mNOS;                          // offset 0x74
        uint8 mGear;                        // offset 0x78
        uint8 mPriority;                    // offset 0x79

      public:
        friend struct ExtrapolatedCar;
        State();
        void Import(const ISimable *simable, float simtime);
        void Export(SmartBitStream &data, ePosDataPriorityMask priority_mask, uint8 repositioncount);
        uint8 Import(float time, SmartBitStream &data, ePosDataPriorityMask priority_mask);
        void Extrapolate(float simtime);
        bool Blend(State &blended, float t);
        bool IsBlending() const;
        void Export(ISimable *simable) const;
        void SetOnGround(IVehicle *vehicle);
        bool IsValidPosition();
        float SquaredDistanceTo(State &target) const;
        float GetTime() const {
            return mTime;
        }
        UMath::Vector3 &GetPosition();
        void ExtractDirection(UMath::Vector3 &direction) const;
        ISimable *SpawnVehicle(Attrib::Key cartype);
        bool HasPriority(ePosDataPriorityMask mask) const {
            return (mPriority & mask) != 0;
        }
    };

  private:
    struct CopMap;

    State mStateArray[32];            // offset 0x10
    State mSaved;                     // offset 0x1010
    State mBlended;                   // offset 0x1090
    Attrib::Key mCarType;             // offset 0x1110
    CopMap *mCops;                    // offset 0x1114
    ALIGNVEC UMath::Vector3 mLastPosition; // offset 0x1120
    State *mLast;                     // offset 0x112C
#if defined(EA_PLATFORM_WIN32) || defined(EA_PLATFORM_XENON)
    // PC (speed.exe): dos palabras mas antes de mHead (el ctor 0x74D510 pone 0 en +0xF80 y 1 en +0xF84; mHead en
    // +0xF88, mMutex en +0xF90, mActive en +0xF9C). Nombres y tipos de Carbon PS3 (DWARF de la Alpha 56), que las
    // tiene en ese mismo sitio, detras de mLast.
    float mLastPDPAesthetic;
    PhysicsMode mCurrentMode;
#endif
    int mHead;                        // offset 0x1130
    int mTail;                        // offset 0x1134
    NetworkMutex *mMutex;             // offset 0x1138
    float mCollisionTime;             // offset 0x113C
    uint32 mUpdateTime;               // offset 0x1140
    bool mActive;                     // offset 0x1144
    uint8 mRepositionCount;           // offset 0x1148
    bool mHasHeadset;                 // offset 0x114C
    bool mPaused;                     // offset 0x1150
    bool mUseDriverAI;                // offset 0x1154

    static int Next(int c) {
#ifdef EA_PLATFORM_WIN32
        int n = c + 1 >= 32 ? 0 : c + 1; // PC (speed.exe 0x75E2DA): cmp 0x20 + setge
#else
        int n = c + 1 > 31 ? 0 : c + 1;
#endif
        return n;
    }
};

// total size: 0x10 (los stabs de la A124: un map con su propio operador new y delete)
struct ExtrapolatedCar::CopMap : public UTL::Std::map<HSIMABLE, ExtrapolatedCar *, _type_map> {
    USE_FASTALLOC(CopMap)
};

// total size: 0x1430
struct OnlineRacer : ExtrapolatedCar {
    friend class OnlineManager;

    Attrib::Key CarTypeKey;               // offset 0x1160
    bool IsCustomCar;                     // offset 0x1164
    FECustomizationRecord CarCustomization; // offset 0x1168

    OnlineRacer(int8 driver_number, bool is_server, const char *persona);
    static void *operator new(size_t size) {
        return bMalloc(size, "OnlineRacer", 0, 8);
    }
    static void operator delete(void *mem) {
        bFree(mem);
    }
    ~OnlineRacer();

    int8 GetDriverNumber() const {
        return DriverNumber;
    }
    eOnlineRacerState GetState() {
        return State;
    }
    int8 GetPlayerID() const {
        return PlayerID;
    }
    void SetPlayerID(int8 player_id) {
        PlayerID = player_id;
    }
    char *GetPersona() {
        return Persona;
    }
    int GetRaceScore() const {
        return RaceScore;
    }
    void SetRaceScore(int score);
    bool IsConnected() {
        return !(GetState() == OPS_LOST_CONNECTION || GetState() == OPS_QUIT || GetState() == OPS_DISCONNECTED || GetState() == OPS_DISCERROR);
    }
    bool IsServer() const {
        return bIsServer;
    }
    bool IsFinishedRacing();
    bool IsSameSideOfRestart();
    float GetBadnessCountdown(int *r_reason);
    float GetEndRaceCountdown();
    FinishedRaceStatsEntry *GetFinishedRaceStats() {
        return &FinishedRaceStats;
    }
    uint16 GetCheatTally(eOnlineCheatReason cheat_reason) {
        return CheatTally[cheat_reason];
    }
    uint8 GetCheatScore();
    bool IsCheating();

    void SetLastSpamRealTime(float time) {
        LastSpamRealTime = time;
    }
    float GetLastSpamRealTime() const {
        return LastSpamRealTime;
    }
    void ChangeState(eOnlineRacerState new_state);
    void SetPersona(const char *persona);
    void UpdateLocal(float t);
    void Finish(int nRank, bool bBlinkBlinkPoof, int raceFinishReason);
    void SignalFinish(SmartBitStream &data);
    void UpdateEndRaceStats();
#ifdef EA_PLATFORM_WIN32
    // PC (speed.exe 0x754C30), sin nombre en ningun binario: el GRacerInfo de este corredor (y su simable).
    struct GRacerInfo *Win32Unknown754C30(ISimable **simable);
#endif
    void DriverDisconnect(eOnlineRacerState new_state, int finish_reason);
    void SetDisconnectTime(float time) {
        DisconnectTime.SetTime(time);
    }
    float GetDisconnectTime() {
        return DisconnectTime.GetSeconds();
    }
    void ClearCheatInfo();
    void DetectedCheat(int reason);
    uint32 GetDataCRC(bool recalc);

  protected:
    eOnlineRacerState State;                  // offset 0x1300
    int8 DriverNumber;                        // offset 0x1304
    int8 PlayerID;                            // offset 0x1305
    uint32 Reputation;                        // offset 0x1308
    int RaceScore;                            // offset 0x130C
    bool bIsServer;                           // offset 0x1310
    bool bShouldRestart;                      // offset 0x1314
    char Persona[16];                         // offset 0x1318
    FinishedRaceStatsEntry FinishedRaceStats; // offset 0x1328
    uint32 PhysicsDataCRC;                    // offset 0x13E8
    uint16 CheatTally[16];                    // offset 0x13EC
    float LastSpamRealTime;                   // offset 0x140C
    uint32 SyncScoreMsgID;                    // offset 0x1410
    Timer GraceCountdown;                     // offset 0x1414
    Timer BadnessCountdown;                   // offset 0x1418
    Timer DisconnectTime;                     // offset 0x141C
    int BadnessReason;                        // offset 0x1420
    float EndRaceCountdown;                   // offset 0x1424
};

// total size: 0x1704
class OnlineManager {
  public:
    friend struct OnlineRacer;
    friend struct Client;
    friend struct Server;
#ifdef EA_BUILD_A124
    // DisplayDebugScreenPrints (Main.cpp) leia mMasterTime/mStartTime en la alpha a traves de los
    // accesores inline de su OnlineManager.hpp (linea 72 en su mapa de lineas); aqui basta el acceso.
    friend void DisplayDebugScreenPrints();
#endif

    OnlineManager();
    ~OnlineManager();
    void Initialize(int argc, char **argv);
    void InitForRace();
    void UninitForRace();
    void Disconnect(bool force);
    void Update(bool receive);
    float SyncWorldTimestep(float timestep);
    void StartSimFrame();
    void EndSimFrame();
    eOnlineState GetState() {
        return State;
    }
    bool IsOnlineRace();
    bool IsOnline();
    bool IsServer();
    OnlineRacer *GetOnlineRacer(int driver_number);
    OnlineRacer *GetOnlineRacer(const char *racerName);
    OnlineRacer *GetLocalRacer() {
        return pLocalRacer;
    }
    bool IsValidRacer(int driver_number) {
        return pRacers[driver_number] != nullptr;
    }
    int GetNumRacers();
    void StartLobby();
    void StartOnlineRace();
    void EndOnlineRace(bool bForced);
    void TrackLoaded();
    void ReadyStartLine();
    int AreAllPlayersFinishedRacing();
    void NotifyDiscEjected();
    bool FinishDriver(int driver_number, int nRank, bool bBlinkBlinkPoof, int raceFinishReason);
    void SendSyncAnimations();
    void InitAntiCheating(int num_players);
    bool HasAnyoneCheated();
    int GetGetAwayLeaderDriverNumber();
    Timer GetGetAwayFinishTime(int driver_num);
    Timer GetGetAwayLeaderTime(int driver_num);
    bool GetGetAwayRaceTimedOut();
    void SetPlayerIDs(const char (*personas)[16]);
    void SetGameSeed(unsigned int seed) {
        GameSeed = seed;
    }
    unsigned int GetGameSeed() const {
        return GameSeed;
    }
    uint32 GetMasterTime();
    void SetServerTime(uint32 time);
    uint32 GetServerTime();
    int GetDrift();
    bool RaceStartTimeSet();
    void SetPostCountdownStartRaceTime(float time) {
        mPostCountdownStartRaceTime = time;
    }
    bool GetRestartingRace();
    void SetRestartingRace(bool r);
    Timer GetRaceRestartTimer();
    void RequestRestart();
    void RestartLastRace();
    void RejectRestartRequest();
    bool IsRestartRequested();
    bool CanRestartRace();
    bool RaceStartAborted();
    void SendStyleMoments();
    bool IsAntiCheatingEnabled();
    eOnlineRaceEndReason GetRaceEndReason();
    void SetRaceEndDisconnect();
    void SetRaceTimeup() {
        RaceTimeup = true;
    }
    bool IsRaceTimeup() {
        return RaceTimeup;
    }
    void UpdateOutgoing();

  private:
    void UpdateIncoming();

  protected:
    void ImportRaceParams(int driver_number, int server_driver_number, SmartBitStream &bitstream_data);
    void ExportRaceParams(int driver_number, SmartBitStream &bitstream_data);
    void ImportDriverInfo(int driver_number, SmartBitStream &bitstream_data);
    void ExportDriverInfo(int driver_number, SmartBitStream &bitstream_data);
#if defined(EA_PLATFORM_WIN32) || defined(EA_PLATFORM_XENON)
    // PC (speed.exe 0x75F4F0, `ret 0xC`): sin timestamp (la A124 tampoco lo usa).
    void ImportPositionData(int driver_number, SmartBitStream &bitstream_data, ePosDataPriorityMask priority_mask);
#else
    void ImportPositionData(int driver_number, SmartBitStream &bitstream_data, float timestamp,
                            ePosDataPriorityMask priority_mask);
#endif
    void ExportPositionData(int driver_number, SmartBitStream &bitstream_data, ePosDataPriorityMask priority_mask);
    void SignalLoad();
    void SetStartRaceTime(uint32 tick, float time, float ping);
    float GetStartRaceTime(uint32 tick);
    void SetPosUpdatePing(float ping);
    void SignalDriverFinish(SmartBitStream &bitstream_data);
    void DriverLeft(int driver_number, bool he_quit);
    void SignalScoreMessage(SmartBitStream &bitstream_data);
    void SignalSyncAnimationMessage(SmartBitStream &bitstream_data);
    void SignalDataCRCMessage(SmartBitStream &bitstream_data);
    void SignalGetAwayLeaderMessage(SmartBitStream &bitstream_data);
    void SignalGetAwayLostMessage(SmartBitStream &bitstream_data);
    void SignalRestartLoad(int driver_num);
    uint32 PackRideInfo(RideInfo &ride, SmartBitStream &data, bool pack_parts);
    uint32 UnpackRideInfo(RideInfo &ride, SmartBitStream &data, int skin_number);
    void SetupRaceParams();
    void SetupLocalDriver(int driver_num);
    void ChangeState(eOnlineState new_state);
    void MapRacers2PlayerIDs();
    void StartRace();
    void CreateOnlineRacer(int driver_num, SmartBitStream *pdata, bool is_server, const char *persona);
    void PurgeDisconnectedRacers();
    int GetNumConnectedRacers();
    OnlineRacer *GetServerRacer();
    bool IsRaceOver() {
        return State == OLS_RACE_END;
    }
    void FinalizeCheatDetection();
    void CalculateFinishOrder();
    void SendEndOfRaceResults();
    void ClearAnimWorldObjects();
    void BuildAnimWorldObjects();
    void SyncRunningAnimations(float delta_t, bool resetfirst);
    void SendLocalPlayerDataCRC();
    void CheckWorldTimerHacking();
    void PrintQuantizersUsageReport();
    void PrintCheatTallies(bool to_screen);
    void CheckGetAwayLeaderChange();
    void SetupStartingPositions();
    void SetupRestartRace();
    void FinishGetAwayRace();
    void SelfDisconnect();
    Timer &GetTimeupStartTime() {
        return TimeupStartTime;
    }
    float GetTimeupLength() {
        return TimeupLength;
    }

  public:
    void InitQuantizers();

  protected:
    eOnlineState State;                   // offset 0x0
    OnlineRacer *pLocalRacer;             // offset 0x4
    OnlineRacer *pRacers[4];              // offset 0x8
    uint8 NumRacers;                      // offset 0x18
    bool bOnlineRace;                     // offset 0x1C
    bool bHasStalled;                     // offset 0x20
    uint32 mLastUpdateTime;               // offset 0x24
    uint32 mMasterTime;                   // offset 0x28
    uint32 mStartTime;                    // offset 0x2C
    uint32 mServerTime;                   // offset 0x30
    bool mStartRaceIsSet;                 // offset 0x34
    uint32 mStartRaceTick;                // offset 0x38
    float mStartRaceTime;                 // offset 0x3C
    float mPostCountdownStartRaceTime;    // offset 0x40
#if defined(EA_PLATFORM_WIN32) || defined(EA_PLATFORM_XENON)
    // PC (speed.exe): una palabra mas aqui (pAnimWorldObjects en +0x48, CountdownSyncAnims en +0x6C, RaceEndReason
    // en +0xC4; el ctor 0x740FF0 pone 0 en +0x3C). Nombre y tipo de Carbon PS3 (DWARF de la Alpha 56), que la tiene
    // en ese mismo sitio, detras de mPostCountdownStartRaceTime.
    float mAnimFinishAndLoadHudTime;
#endif
    float mStartRacePing;                 // offset 0x44
    float mPosUpdatePing;                 // offset 0x48
    WorldObject *pAnimWorldObjects[8];    // offset 0x4C
    uint8 NumWorldObjects;                // offset 0x6C
    Timer CountdownSyncAnims;             // offset 0x70
    Timer CountdownSendDataCRC;           // offset 0x74
    Timer LastSyncedWTTimeStamp;          // offset 0x78
    Timer TimeRaceFinished;               // offset 0x7C
    Timer TimeTrackLoaded;                // offset 0x80
    char PersonaMap[4][16];               // offset 0x84
    unsigned int GameSeed;                // offset 0xC4
    eOnlineRaceEndReason RaceEndReason;   // offset 0xC8
    Timer LastAntiCheatWorldTimer;        // offset 0xCC
    Timer LastAntiCheatRealTimer;         // offset 0xD0
    uint32 SavedLocalPhysicsCRC;          // offset 0xD4
    int8 GetAwayLeaderDriverNumber;       // offset 0xD8
    Timer GetAwayLeaderCheckTimer;        // offset 0xDC
    Timer GetAwayFinishTime[4];           // offset 0xE0
    Timer GetAwayLeaderTime[4];           // offset 0xF0
    Timer LastGetAwayLeaderChangeTime;    // offset 0x100
    Timer RaceRestartTimer;               // offset 0x104
    bool GetAwayRaceTimedOut;             // offset 0x108
    bool FrameRateIsTooLow;               // offset 0x10C
    Timer TimeupStartTime;                // offset 0x110
    float TimeupLength;                   // offset 0x114
    bool RaceTimeup;                      // offset 0x118
    bool RestartingRace;                  // offset 0x11C
    bool RestartRequested;                // offset 0x120
    int32 m_ticker;                       // offset 0x124
#if defined(EA_PLATFORM_WIN32) || defined(EA_PLATFORM_XENON)
    // PC (speed.exe): dos bytes mas antes de los cuantizadores (QuantCarSlotID en +0x11C): el ctor 0x740FF0 y el caso
    // OLS_LOBBY_IN_LOBBY de Update (0x761B30) los ponen a false. Sin nombre en ninguna fuente (Carbon no los tiene).
    bool win32_unknown_0x118;
    bool win32_unknown_0x119;
#endif

  public:
    IntQuantizer QuantCarSlotID;          // offset 0x128
    IntQuantizer QuantCarType;            // offset 0x26C
    IntQuantizer QuantCarPartIndex;       // offset 0x3B0
    IntQuantizer QuantInt2Bit;            // offset 0x4F4
    IntQuantizer QuantInt3Bit;            // offset 0x638
    IntQuantizer QuantInt4Bit;            // offset 0x77C
    FloatQuantizer QuantFloatTime;        // offset 0x8C0
    FloatQuantizer QuantFloatSimTime;     // offset 0xA0C
    FloatQuantizer PositionQuantizerX;    // offset 0xB58
    FloatQuantizer PositionQuantizerY;    // offset 0xCA4
    FloatQuantizer PositionQuantizerZ;    // offset 0xDF0
    FloatQuantizer VelocityQuantizer;     // offset 0xF3C
    FloatQuantizer MatrixQuantizer;       // offset 0x1088
    FloatQuantizer AngleQuantizer;        // offset 0x11D4
    FloatQuantizer AccelerationQuantizer; // offset 0x1320
    FloatQuantizer AVelocityQuantizer;    // offset 0x146C
    FloatQuantizer ControlQuantizer;      // offset 0x15B8
};

#else
class OnlineManager {
  public:
    void InitForRace();

    void UninitForRace();

    void Disconnect(bool force);

    void Update(bool receive);

    float SyncWorldTimestep(float timestep);

    bool IsOnline();

    bool IsServer();

    uint32 GetMasterTime();

    uint32 GetServerTime();

    void DriverLeft(int driver_number, bool he_quit);

    void SetServerTime(uint32 time);

    void SignalRestartLoad(int driver_num);

    void SetStartRaceTime(uint32 tick, float time, float ping);

    float GetStartRaceTime(uint32 tick);

    bool GetRestartingRace();

    void RequestRestart();

    void SignalLoad();

    void PrintQuantizersUsageReport();

    void PrintCheatTallies(bool to_screen);

    void StartLobby();

    void StartOnlineRace();

    void ReadyStartLine();

    int AreAllPlayersFinishedRacing();

    void NotifyDiscEjected();

    void SendSyncAnimations();

    void InitAntiCheating(int num_players);

    bool HasAnyoneCheated();

    int GetGetAwayLeaderDriverNumber();

    Timer GetGetAwayFinishTime(int driver_num);

    Timer GetGetAwayLeaderTime(int driver_num);

    bool GetGetAwayRaceTimedOut();

    void SetPlayerIDs(const char (*personas)[16]);

    int GetDrift();

    bool RaceStartTimeSet();

    void SetPostCountdownStartRaceTime(float time);

    void SetRestartingRace(bool r);

    Timer GetRaceRestartTimer();

    void RestartLastRace();

    void RejectRestartRequest();

    bool IsRestartRequested();

    bool CanRestartRace();

    bool RaceStartAborted();

    bool IsAntiCheatingEnabled();

    void SetRaceEndDisconnect();

    void SetRaceTimeup();

    bool IsRaceTimeup();

    void UpdateOutgoing();

    void SetPosUpdatePing(float ping);

    void SetupRaceParams();

    void SetupLocalDriver(int driver_num);

    void ChangeState(eOnlineState new_state);

    void MapRacers2PlayerIDs();

    void StartRace();

    void PurgeDisconnectedRacers();

    int GetNumConnectedRacers();

    bool IsRaceOver();

    void FinalizeCheatDetection();

    void CalculateFinishOrder();

    void SendEndOfRaceResults();

    void ClearAnimWorldObjects();

    void BuildAnimWorldObjects();

    void SyncRunningAnimations(float delta_t, bool resetfirst);

    void SendLocalPlayerDataCRC();

    void CheckWorldTimerHacking();

    void CheckGetAwayLeaderChange();

    void SetupStartingPositions();

    void SetupRestartRace();

    void FinishGetAwayRace();

    void SelfDisconnect();

    Timer &GetTimeupStartTime();

    float GetTimeupLength();

    void UpdateIncoming();

    int GetNumRacers();

    void StartSimFrame();
    void InitQuantizers();

    OnlineManager() {
        InitQuantizers();
    }

#ifdef EA_PLATFORM_PLAYSTATION2 // SLUS tiene _$_13OnlineManager (0x411400) y lo llama el _GLOBAL_$D$ de zWorld; en GC no existe
    ~OnlineManager();
#endif

    void EndSimFrame() {}

    void Initialize(int argc, char **argv) {}

    bool IsOnlineRace() {
#ifndef ONLINE_ENABLED
        return false;
#else
        // TODO
        return false;
#endif
    }

    void EndOnlineRace(bool bForced) {
        // TODO
    }

    void TrackLoaded() {
        // TODO
    }

    // TODO
    eOnlineState GetState() {
        return OLS_DISCONNECTED;
    }
};
#endif

extern OnlineManager TheOnlineManager;

#if ONLINE_SUPPORT
// Del frontend del prototipo (zFe): avisa de que un corredor se ha desconectado.
void FEOnlineDisconnectMsg(OnlineRacer *racer);
#endif

#endif
