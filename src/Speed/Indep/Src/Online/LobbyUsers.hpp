#ifndef ONLINE_LOBBY_USERS_HPP
#define ONLINE_LOBBY_USERS_HPP

#ifdef EA_PRAGMA_ONCE_SUPPORTED
#pragma once
#endif

#include "LobbyCore.hpp"
#include "Speed/Indep/Src/Misc/Timer.hpp"

namespace Physics {
namespace Info {
struct Performance;
}
}

// total size: 0x1C
struct RankingDataT {
    uint32 points;            // offset 0x0, size 0x4
    uint32 wins;              // offset 0x4, size 0x4
    uint32 losses;            // offset 0x8, size 0x4
    uint32 disconnects;       // offset 0xC, size 0x4
    uint32 topspeed;          // offset 0x10, size 0x4
    uint32 fastestFinishTime; // offset 0x14, size 0x4
    uint32 totalNOS;          // offset 0x18, size 0x4

    RankingDataT();
    ~RankingDataT() {}
    void Clear();
};

// Clear() va antes que el constructor: la A124 lo expande dentro de RankingDataT() (siete ceros, del ultimo al primero).
inline void RankingDataT::Clear() {
    points = wins = losses = disconnects = topspeed = fastestFinishTime = totalNOS = 0;
}

inline RankingDataT::RankingDataT() {
    Clear();
}

struct PlayerDataT {
    struct Stats {
        struct OnlineModeStats {
            uint32 points;
            uint32 clientWins;
            uint32 clientLosses;
            uint32 hostWins;
            uint32 hostLosses;
            uint32 topSpeed;
            uint32 jumpTime;
            uint32 avgNOSused;
            uint32 totalNOSused;
            uint32 fastestFinishTime;
            uint32 winPercentage;
#if defined(EA_PLATFORM_WIN32) || defined(EA_PLATFORM_XENON) // el EGM tambien: 0x30 B (X360-A, ctor 0x82573100)
            uint32 win32_unknown_0x2c; // PC: 0x30 B por modo (PlayerDataT::PlayerDataT, 0x51A2F0); nombre neutro: casi seguro las desconexiones del modo (0x53D1D0 lo muestra en 0x134dac12, donde la A124 pone GetTotalDisconnectPercentage; Stats::Clear 0x78E7D0 lo pone a cero; 0x791D50 lo divide por GetTotalGamesPlayed)
#endif

#if defined(EA_BUILD_A124) || defined(EA_PLATFORM_WIN32)
            // La A124 lo define tras Stats() (playerdata.hpp:306): Stats() lo llama fuera de linea y solo pone a
            // cero los cinco primeros campos.
            OnlineModeStats();
#else
            OnlineModeStats() { Clear(); }
#endif
            ~OnlineModeStats() {}

            void Clear() {
#if defined(EA_PLATFORM_WIN32) || defined(EA_PLATFORM_XENON) // EGM: Stats::Clear 0x82589AD0 con paso 0x30
                win32_unknown_0x2c = 0; // Stats::Clear de PC (0x78E7D0) lo pone a cero el primero de cada modo
#endif
                winPercentage = 0;
                fastestFinishTime = 0;
                totalNOSused = 0;
                avgNOSused = 0;
                jumpTime = 0;
                topSpeed = 0;
                hostLosses = 0;
                hostWins = 0;
                clientLosses = 0;
                clientWins = 0;
                points = 0;
            }
        };

        uint32 disqualifications;
        uint32 clientDisconnects;
        uint32 hostDisconnects;
        uint32 topSpeed;
        uint32 longestPowerSlideDistance;
        uint32 longestJumpDuration;
        uint32 fastestDragFinish;
        uint32 totalNOSusage;
        OnlineModeStats raceModeStats[3];

        Stats()
            : disqualifications(0) //
            , clientDisconnects(0) //
            , hostDisconnects(0) //
            , topSpeed(0) //
            , longestPowerSlideDistance(0) //
            , longestJumpDuration(0) //
            , fastestDragFinish(0) //
            , totalNOSusage(0) {}
        ~Stats() {}

        void Clear();
    };

    int ExtractRecord(const char *record);
    int ExtractRecord(const LobbyApiUserT &user);
    uint32 GetTotalGamesPlayedAsClient();
    uint32 GetTotalGamesPlayedAsHost();
    uint32 GetTotalGamesPlayed(int mode);
    uint32 GetTotalWinsAsClient();
    uint32 GetTotalWinsAsHost();
    uint32 GetTotalWins(int mode);
    uint32 GetTotalLossesAsClient();
    uint32 GetTotalLossesAsHost();
    uint32 GetTotalLosses(int mode);
#ifdef EA_PLATFORM_WIN32
    // PC (speed.exe): todas fuera de linea en zOnline, aunque nadie llame a la mitad (0x78E670-0x78E7C3 y
    // 0x791B70-0x791E43): MSVC no emite un inline sin llamantes, asi que en PC no lo eran. Sus formas de PC van en
    // LobbyUsers.cpp. GetWin32Unknown0x2cPercentage (0x791D50, nombre nuestro) es el porcentaje por modo de
    // win32_unknown_0x2c que UIOnlinePersonalRankings de PC pone en 0x134dac12 (donde la A124 pone el de desconexiones).
    uint32 GetWeeklyPointTotals(int mode);
    uint32 GetMonthlyPointTotals(int mode);
    float GetWinPercentage(int mode);
    float GetClientDisconnectPercentage();
    float GetHostDisconnectPercentage();
    float GetTotalDisconnectPercentage();
    float GetWin32Unknown0x2cPercentage(int mode); // nombre neutro: casi seguro el porcentaje de desconexiones del modo
    float GetTopSpeedInMPS(int mode);
    float GetFastestDragFinish();
    char *GetFastestDragFinishString();
    uint32 GetTotalNOS(int mode);
    float GetLongestJumpDuration(int mode);
    char *GetLongestJumpDurationString();
    float GetLongestPowerSlideInMetres();
    void Clear();

  private:
    bool IsFloatEqual(float x, float y, float epsilon);
#else
    uint32 GetWeeklyPointTotals(int mode) {
        if (static_cast<uint32>(mode) > 2) {
            return 0;
        }
        return stats.raceModeStats[mode].points - lastWeekStats.raceModeStats[mode].points;
    }
    uint32 GetMonthlyPointTotals(int mode);
    float GetWinPercentage(int mode);
    float GetClientDisconnectPercentage() {
        uint32 gamesPlayedAsClient = GetTotalGamesPlayedAsClient();
        if (gamesPlayedAsClient != 0) {
            return (static_cast<float>(stats.clientDisconnects) /
                    static_cast<float>(gamesPlayedAsClient)) *
                   100.0f;
        }
        return 0.0f;
    }
    float GetHostDisconnectPercentage() {
        uint32 gamesPlayedAsHost = GetTotalGamesPlayedAsHost();
        if (gamesPlayedAsHost != 0) {
            return (static_cast<float>(stats.hostDisconnects) /
                    static_cast<float>(gamesPlayedAsHost)) *
                   100.0f;
        }
        return 0.0f;
    }
    float GetTotalDisconnectPercentage();
    float GetTopSpeedInMPS(int mode);
    float GetFastestDragFinish() {
        return static_cast<float>(stats.fastestDragFinish) * 0.001f;
    }
    char *GetFastestDragFinishString() {
        static char time[32];
        Timer tmptime(GetFastestDragFinish());
        tmptime.PrintToString(time, 4);
        return time;
    }
    uint32 GetTotalNOS(int mode);
    float GetLongestJumpDuration(int mode);
    char *GetLongestJumpDurationString() {
        static char time[32];
        Timer tmptime(static_cast<float>(stats.longestJumpDuration) * 0.001f);
        tmptime.PrintToString(time, 4);
        return time;
    }
    float GetLongestPowerSlideInMetres() {
        return static_cast<float>(stats.longestPowerSlideDistance) * 0.001f;
    }
    void Clear();

  private:
    bool IsFloatEqual(float x, float y, float epsilon) { return bAbs(x - y) < epsilon; }
#endif

  public:
    char persona[16];
    Stats stats;
    Stats lastWeekStats;
    Stats lastMonthStats;
    uint32 lastGameTime;

    PlayerDataT()
        : lastGameTime(0) {
        persona[0] = '\0';
    }
    ~PlayerDataT() {}
};

#if defined(EA_BUILD_A124) || defined(EA_PLATFORM_WIN32)
inline PlayerDataT::Stats::OnlineModeStats::OnlineModeStats()
    : points(0) //
    , clientWins(0) //
    , clientLosses(0) //
    , hostWins(0) //
    , hostLosses(0) {}
#endif

struct LobbyUsers {
    struct OnlineUsersData : bTNode<OnlineUsersData> {
        OnlineUsersData()
            : commandID(0) {
            bMemSet(&user, 0, sizeof(user));
        }
        ~OnlineUsersData() {}

        void Reset() {
            bMemSet(&user, 0, sizeof(user));
            commandID = 0;
        }

        LobbyApiUserT user;
        int32 commandID;
    };

    static LobbyUsers &Instance();
    void UpdateCarName();
    int32 GetUserOnlineRecord(const char *persona, CommandCBFunc func, void *context);
    LobbyApiUserT *GetUserRecord(const char *persona);
    bool GetCarNameFromUserRecord(const char *persona, uint32 &carNameHash,
                                  Physics::Info::Performance &performance);
    bool GetPointsFromUserRecord(const char *persona, uint32 &points, OnlineRaceModeE mode);
    bool GetDisqualificationsFromUserRecord(const char *persona, uint32 &disqs);
    int32 GetMyUserStats(PlayerDataT &userStats, CommandCBFunc func, void *context);
    void ClearUserStats();
    int32 GetOtherUserStats(PlayerDataT &userStats, const char *persona, CommandCBFunc func,
                            void *context);
    void AbortCommand(int32 commandID) { LobbyCore::Instance().AbortCommand(commandID); }
    int32 SubmitFeedback(const char *persona, const char *feedback, CommandCBFunc func,
                         void *context);
    void ClearUserOnlineRecordCache();
    void ClearUserOnlineRecordCache(const LobbyApiPlayT &game);
    void SetSessionChangeFlag(bool changing);
    void SetSessionLatency(int latency);
#if defined(EA_PLATFORM_WIN32) || defined(EA_PLATFORM_XENON)
    // PC and Xbox 360 (EGM 0x8258B740 / 0x8258B3A8): no SetSessionRaceStatusInfo ("LP"); in its place (0x799DF0, right after SetSessionLatency as in the A124) a
    // setter of one int with the tag "CD", and 0x79D3F0 packs the connection state of the players of my game ("CE").
    // No source names them: neutral names.
    void SetWin32Unknown0x23c(int value);
    void SetWin32UnknownCE();
#else
    void SetSessionRaceStatusInfo(int lap, int mapx, int mapy);
#endif
    LobbyApiUserT *GetMyUserRecord() const;

  private:
    LobbyUsers()
        : gotMyStats(false) //
        , auxiData(nullptr) //
        , mLate(-1) //
        , mCar(-1) //
#if defined(EA_PLATFORM_WIN32) || defined(EA_PLATFORM_XENON)
        , win32_unknown_0x23c(-1) {}
#else
        , mLap(0) //
        , mMapX(0) //
        , mMapY(0) {}
#endif
    ~LobbyUsers() {}
    int32 Init();
    void Reset();
    void MaybeCreateAuxiBuffer();
    int32 SendAuxiData();
    static void OnlnCB(LobbyApiRefT *pRef, LobbyApiMsgT *pMsg, void *pData);
    static void UserCB(LobbyApiRefT *pRef, LobbyApiMsgT *pMsg, void *pData);
    friend int32 LobbyInit();
    friend void LobbyDisconnect();

    bTList<OnlineUsersData> userList;
    PlayerDataT myStats;
    bool gotMyStats;
    char *auxiData;
    int mLate;
    uint32 mCar;
#if defined(EA_PLATFORM_WIN32) || defined(EA_PLATFORM_XENON)
    int win32_unknown_0x23c; // PC (ctor 0x792920: -1; Xbox 360 0x82573B38): the value of the tag "CD" (SetWin32Unknown0x23c)
#else
    int mLap;
    int mMapX;
    int mMapY;
#endif
};

extern LobbyUsers *pLobbyUsersInstance;

#endif
