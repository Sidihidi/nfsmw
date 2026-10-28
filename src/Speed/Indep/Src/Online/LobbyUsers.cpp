#include "LobbyUsers.hpp"
#include "Speed/Indep/Src/Frontend/Database/VehicleDB.hpp"
#include "Speed/Indep/Src/Misc/BuildRegion.hpp"
#include "Speed/Indep/Src/Physics/PhysicsInfo.hpp"

void *operator new(size_t size, const char *file, int line, int allocationParams);

extern "C" {
int TagFieldGetStructure(const char *field, void *buffer, int bufferSize, const char *format);
float TagFieldGetFloat(const char *field, float defaultValue);
void TagFieldSetFloat(char *record, int recordLength, const char *name, float value);
#if defined(EA_PLATFORM_WIN32) || defined(EA_PLATFORM_XENON)
int TagFieldSetStructure(char *record, int recordLength, const char *name, const void *data, int size, const char *format);
int TagFieldDelete(char *record, const char *name);
#endif
void *LobbyApiInfoPtr(LobbyApiRefT *lobbyRef, int selector);
void LobbyApiExtractUserRecord(LobbyApiUserT *user, const char *record);
}

LobbyUsers *pLobbyUsersInstance;

int PlayerDataT::ExtractRecord(const char *record) {
    if (!record || !record[0]) {
        return -1;
    }

    Clear();
    TagFieldGetString(TagFieldFind(record, "PERS"), persona, sizeof(persona), "");
    TagFieldGetStructure(TagFieldFind(record, "STAT"), &stats, sizeof(stats), "l*");
    TagFieldGetStructure(TagFieldFind(record, "LWSTAT"), &lastWeekStats, sizeof(lastWeekStats),
                         "l*");
    TagFieldGetStructure(TagFieldFind(record, "LMSTAT"), &lastMonthStats,
                         sizeof(lastMonthStats), "l*");
    lastGameTime = TagFieldGetNumber(TagFieldFind(record, "LGAME"), 0);
    return 0;
}

int PlayerDataT::ExtractRecord(const LobbyApiUserT &user) {
    Clear();
    if (!user.name[0]) {
        return -1;
    }
    bStrCpy(persona, user.name);
    TagFieldGetStructure(user.stat, &stats, sizeof(stats), "l*");
    return 0;
}

uint32 PlayerDataT::GetTotalGamesPlayedAsClient() {
    return GetTotalWinsAsClient() + GetTotalLossesAsClient();
}

uint32 PlayerDataT::GetTotalGamesPlayedAsHost() {
    return GetTotalWinsAsHost() + GetTotalLossesAsHost();
}

uint32 PlayerDataT::GetTotalGamesPlayed(int mode) {
    if (mode == 3) {
        return GetTotalWins(mode) + GetTotalLosses(mode);
    }
    return (stats.raceModeStats[mode].clientLosses + stats.raceModeStats[mode].hostLosses) +
           (stats.raceModeStats[mode].hostWins + stats.raceModeStats[mode].clientWins);
}

uint32 PlayerDataT::GetTotalWinsAsClient() {
    uint32 sum = 0;
    for (int mode = 0; mode < 3; mode++) {
        sum += stats.raceModeStats[mode].clientWins;
    }
    return sum;
}

uint32 PlayerDataT::GetTotalWinsAsHost() {
    uint32 sum = 0;
    for (int mode = 0; mode < 3; mode++) {
        sum += stats.raceModeStats[mode].hostWins;
    }
    return sum;
}

uint32 PlayerDataT::GetTotalWins(int mode) {
    uint32 sum = 0;
    if (mode == 3) {
        for (int i = 0; i < 3; i++) {
            sum += stats.raceModeStats[i].clientWins + stats.raceModeStats[i].hostWins;
        }
        return sum;
    }
    return stats.raceModeStats[mode].clientWins + stats.raceModeStats[mode].hostWins;
}

uint32 PlayerDataT::GetTotalLossesAsClient() {
    uint32 sum = 0;
    for (int mode = 0; mode < 3; mode++) {
        sum += stats.raceModeStats[mode].clientLosses;
    }
    return sum;
}

uint32 PlayerDataT::GetTotalLossesAsHost() {
    uint32 sum = 0;
    for (int mode = 0; mode < 3; mode++) {
        sum += stats.raceModeStats[mode].hostLosses;
    }
    return sum;
}

uint32 PlayerDataT::GetTotalLosses(int mode) {
    uint32 sum = 0;
    if (mode == 3) {
        for (int i = 0; i < 3; i++) {
            sum += stats.raceModeStats[i].clientLosses + stats.raceModeStats[i].hostLosses;
        }
        return sum;
    }
    return stats.raceModeStats[mode].clientLosses + stats.raceModeStats[mode].hostLosses;
}

uint32 PlayerDataT::GetMonthlyPointTotals(int mode) {
    // PC (0x78E6A0): test/jl y cmp 3/jge, las dos comparaciones con signo; GCC junta el rango en una sin signo.
    if (mode < 0 || mode >= 3) {
        return 0;
    }
    return stats.raceModeStats[mode].points - lastMonthStats.raceModeStats[mode].points;
}

float PlayerDataT::GetWinPercentage(int mode) {
    uint32 gamesPlayed = GetTotalGamesPlayed(mode);
#ifdef EA_PLATFORM_WIN32
    // PC (0x791B70): el 0.0f sale justo tras el test (jne); con el if de la A124 MSVC lo deja al final
    if (gamesPlayed == 0) {
        return 0.0f;
    }
    return (static_cast<float>(GetTotalWins(mode)) / static_cast<float>(gamesPlayed)) * 100.0f;
#else
    if (gamesPlayed != 0) {
        return (static_cast<float>(GetTotalWins(mode)) / static_cast<float>(gamesPlayed)) *
               100.0f;
    }
    return 0.0f;
#endif
}

float PlayerDataT::GetTotalDisconnectPercentage() {
    // Una sola suma: PC (0x791CC0) suma el cliente antes; la A124 da el mismo objeto con las dos formas.
    uint32 gamesPlayed = GetTotalGamesPlayedAsHost() + GetTotalGamesPlayedAsClient();
#ifdef EA_PLATFORM_WIN32
    // PC: el retorno temprano, como GetWinPercentage
    if (gamesPlayed == 0) {
        return 0.0f;
    }
    float totalDisconnects = static_cast<float>(stats.hostDisconnects + stats.clientDisconnects);
    return (totalDisconnects / static_cast<float>(gamesPlayed)) * 100.0f;
#else
    float totalDisconnects = static_cast<float>(stats.hostDisconnects + stats.clientDisconnects);
    if (gamesPlayed != 0) {
        return (totalDisconnects / static_cast<float>(gamesPlayed)) * 100.0f;
    }
    return 0.0f;
#endif
}

float PlayerDataT::GetTopSpeedInMPS(int mode) {
    return static_cast<float>(stats.raceModeStats[mode].topSpeed) * 0.001f;
}

uint32 PlayerDataT::GetTotalNOS(int mode) { return stats.raceModeStats[mode].totalNOSused; }

float PlayerDataT::GetLongestJumpDuration(int mode) {
    return static_cast<float>(stats.raceModeStats[mode].jumpTime) * 0.001f;
}

void PlayerDataT::Clear() {
    persona[0] = '\0';
    lastGameTime = 0;
    stats.Clear();
    lastMonthStats.Clear();
}

void PlayerDataT::Stats::Clear() {
    topSpeed = 0;
    hostDisconnects = 0;
    clientDisconnects = 0;
    disqualifications = 0;
    totalNOSusage = 0;
    fastestDragFinish = 0;
    longestJumpDuration = 0;
    longestPowerSlideDistance = 0;
    for (int mode = 0; mode < 3; mode++) {
        raceModeStats[mode].Clear();
    }
}

#ifdef EA_PLATFORM_WIN32
// Las que la A124 tiene en linea en la clase (LobbyUsers.hpp) y PC emite fuera de linea; formas de speed.exe.
uint32 PlayerDataT::GetWeeklyPointTotals(int mode) {
    if (mode < 0 || mode >= 3) {
        return 0;
    }
    return stats.raceModeStats[mode].points - lastWeekStats.raceModeStats[mode].points;
}

float PlayerDataT::GetClientDisconnectPercentage() {
    uint32 gamesPlayedAsClient = GetTotalGamesPlayedAsClient();
    if (gamesPlayedAsClient == 0) {
        return 0.0f;
    }
    return (static_cast<float>(stats.clientDisconnects) / static_cast<float>(gamesPlayedAsClient)) * 100.0f;
}

float PlayerDataT::GetHostDisconnectPercentage() {
    uint32 gamesPlayedAsHost = GetTotalGamesPlayedAsHost();
    if (gamesPlayedAsHost == 0) {
        return 0.0f;
    }
    return (static_cast<float>(stats.hostDisconnects) / static_cast<float>(gamesPlayedAsHost)) * 100.0f;
}

// 0x791D50: el contador de PC a +0x2C de cada modo partido por las partidas de ese modo (nombre nuestro).
float PlayerDataT::GetWin32Unknown0x2cPercentage(int mode) {
    uint32 gamesPlayed = GetTotalGamesPlayed(mode);
    if (gamesPlayed == 0) {
        return 0.0f;
    }
    return (static_cast<float>(stats.raceModeStats[mode].win32_unknown_0x2c) / static_cast<float>(gamesPlayed)) * 100.0f;
}

float PlayerDataT::GetFastestDragFinish() {
    return static_cast<float>(stats.fastestDragFinish) * 0.001f;
}

// PC: PrintToString(time, 0) (push 0 en 0x791DD8 y 0x791E28).
char *PlayerDataT::GetFastestDragFinishString() {
    static char time[32];
    Timer tmptime(GetFastestDragFinish());
    tmptime.PrintToString(time, 0);
    return time;
}

char *PlayerDataT::GetLongestJumpDurationString() {
    static char time[32];
    Timer tmptime(static_cast<float>(stats.longestJumpDuration) * 0.001f);
    tmptime.PrintToString(time, 0);
    return time;
}

float PlayerDataT::GetLongestPowerSlideInMetres() {
    return static_cast<float>(stats.longestPowerSlideDistance) * 0.001f;
}

// PC (0x78E790): y - epsilon < x y despues y + epsilon > x, sin restar x - y.
bool PlayerDataT::IsFloatEqual(float x, float y, float epsilon) {
    return x > y - epsilon && x < y + epsilon;
}
#endif

LobbyUsers &LobbyUsers::Instance() {
    static LobbyUsers theLobbyUsers;
    pLobbyUsersInstance = &theLobbyUsers;
    return theLobbyUsers;
}

void LobbyUsers::UpdateCarName() {
#if defined(EA_PLATFORM_WIN32) || defined(EA_PLATFORM_XENON) // PC (FEDatabase + 0x19C >= 2) y Xbox 360 (+ 0x1A8): los datos auxiliares solo salen con el lobby en marcha
    if (FEDatabase->GetOnlineSettings()->GetState() < cOnlineSettings::OLS_LOBBY_IDLE) {
        return;
    }
#endif
    lobbyMutex.Lock("LobbyUsers::SetAuxiCarName");
    uint32 selectedCar = FEDatabase->GetQuickRaceSettings(GRace::kRaceType_NumTypes)->SelectedCar[0];
    if (mCar != selectedCar) {
        mCar = selectedCar;
        MaybeCreateAuxiBuffer();
        FEPlayerCarDB *stable = &FEDatabase->CurrentUserProfiles[0]->PlayersCarStable;
        FECarRecord *car = stable->GetCarRecordByHandle(selectedCar);
        TagFieldSetNumber(auxiData, 128, "CN", car->GetNameHash());
        Attrib::Key collectionKey = car->VehicleKey;
        Attrib::Gen::pvehicle pvehicle(collectionKey, 0, nullptr);
        if (car->IsCustomized()) {
            FECustomizationRecord *customization =
                stable->GetCustomizationRecordByHandle(car->Customization);
            customization->WriteRecordIntoPhysics(pvehicle);
        }
        Physics::Info::Performance performance;
#ifdef EA_PLATFORM_XENON // EGM 0x82589CF8: la estimacion, no el calculo
        Physics::Info::EstimatePerformance(pvehicle, performance);
#else
        Physics::Info::ComputePerformance(pvehicle, performance);
#endif
        TagFieldSetFloat(auxiData, 128, "PT", performance.TopSpeed);
        TagFieldSetFloat(auxiData, 128, "PH", performance.Handling);
        TagFieldSetFloat(auxiData, 128, "PA", performance.Acceleration);
        SendAuxiData();
    }
    lobbyMutex.Unlock(nullptr);
}

int32 LobbyUsers::GetUserOnlineRecord(const char *persona, CommandCBFunc func, void *context) {
    lobbyMutex.Lock("LobbyUsers::GetUserOnlineRecord");
    if (bStrCmp(persona, FEDatabase->OnlineSettings.GetLobbyPersona()) == 0) {
        lobbyMutex.Unlock("LobbyUsers::GetUserOnlineRecord");
        return 0;
    }

    OnlineUsersData *oud;
    for (oud = userList.GetHead(); oud != userList.EndOfList(); oud = oud->GetNext()) {
        if (bStrCmp(oud->user.name, persona) == 0) {
            int32 rc = oud->commandID;
            lobbyMutex.Unlock("LobbyUsers::GetUserOnlineRecord");
            return rc;
        }
    }

    if (userList.CountElements() == 32) {
        oud = userList.GetTail();
        userList.Remove(oud);
        userList.AddHead(oud);
        oud->Reset();
        bStrCpy(oud->user.name, persona);
    } else {
#ifdef EA_PLATFORM_XENON // EGM 0x8258A038: bMalloc(0x250, ruta, 145, 8)
        oud = new ("e:\\mw\\speed\\indep\\src\\online/LobbyUsers.cpp", 145, 8) OnlineUsersData;
#else
        oud = new ("d:/p4_apex1666_d1001856/mw/speed/indep/src/online/LobbyUsers.cpp", 0x84,
                   8) OnlineUsersData;
#endif
        if (!oud) {
            lobbyMutex.Unlock("LobbyUsers::GetUserOnlineRecord");
            return -1;
        }
        bStrCpy(oud->user.name, persona);
        userList.AddHead(oud);
    }

    char buf[64] = "";
    TagFieldSetString(buf, sizeof(buf), "PERS", persona);
    int32 rc = oud->commandID =
        LobbyCore::Instance().QueueCommand('onln', buf, OnlnCB, this, func, context, false);
    if (rc == -1) {
        oud->Reset();
        lobbyMutex.Unlock("LobbyUsers::GetUserOnlineRecord");
        return -1;
    }
    lobbyMutex.Unlock("LobbyUsers::GetUserOnlineRecord");
    return rc;
}

void LobbyUsers::ClearUserOnlineRecordCache() {
    OnlineUsersData *oud;
    while ((oud = userList.GetHead()) != userList.EndOfList()) {
        oud = userList.Remove(oud);
        delete oud;
    }
}

void LobbyUsers::ClearUserOnlineRecordCache(const LobbyApiPlayT &game) {
    OnlineUsersData *oud;
    for (oud = userList.GetHead(); oud != userList.EndOfList(); oud = oud->GetNext()) {
        bool deleteUser = true;
        for (int i = 0; i < game.iCount; i++) {
            if (bStrCmp(oud->user.name, game.aOpponents[i].strPers) == 0) {
                deleteUser = false;
                break;
            }
        }

        if (deleteUser == true) {
            oud = oud->GetPrev();
            delete userList.Remove(oud->GetNext());
        }
    }
}

LobbyApiUserT *LobbyUsers::GetUserRecord(const char *persona) {
    LobbyApiUserT *user = nullptr;
    lobbyMutex.Lock("LobbyUsers::GetUserRecord");
    if (bStrCmp(persona, FEDatabase->OnlineSettings.GetLobbyPersona()) == 0) {
        lobbyMutex.Unlock("LobbyUsers::GetUserRecord");
        return GetMyUserRecord();
    }

    for (OnlineUsersData *oud = userList.GetHead(); oud != userList.EndOfList();
         oud = oud->GetNext()) {
        if (bStrCmp(oud->user.name, persona) == 0 && oud->commandID == 0) {
            user = &oud->user;
            break;
        }
    }

    lobbyMutex.Unlock("LobbyUsers::GetUserRecord");
    return user;
}

bool LobbyUsers::GetCarNameFromUserRecord(const char *persona, uint32 &carNameHash,
                                          Physics::Info::Performance &performance) {
    lobbyMutex.Lock("LobbyUsers::GetRideInfoFromUserRecord");
    if (bStrCmp(persona, FEDatabase->OnlineSettings.GetLobbyPersona()) == 0) {
        LobbyApiUserT *me = GetMyUserRecord();
        if (me) {
            if (me->aux[0]) {
                carNameHash = TagFieldGetNumber(TagFieldFind(me->aux, "CN"), 0);
                performance.TopSpeed =
                    TagFieldGetFloat(TagFieldFind(me->aux, "PT"), 0.0f);
                performance.Handling =
                    TagFieldGetFloat(TagFieldFind(me->aux, "PH"), 0.0f);
                performance.Acceleration =
                    TagFieldGetFloat(TagFieldFind(me->aux, "PA"), 0.0f);
                lobbyMutex.Unlock("LobbyUsers::GetCarNameFromUserRecord");
                return true;
            }
        }
    } else {
        for (OnlineUsersData *oud = userList.GetHead(); oud != userList.EndOfList();
             oud = oud->GetNext()) {
            if (bStrCmp(oud->user.name, persona) == 0) {
                if (oud->commandID == 0) {
                    if (oud->user.aux[0]) {
                        carNameHash =
                            TagFieldGetNumber(TagFieldFind(oud->user.aux, "CN"), 0);
                        lobbyMutex.Unlock("LobbyUsers::GetCarNameFromUserRecord");
                        return true;
                    }
                }
            }
        }
    }

    lobbyMutex.Unlock("LobbyUsers::GetCarNameFromUserRecord");
    return false;
}

bool LobbyUsers::GetPointsFromUserRecord(const char *persona, uint32 &points,
                                         OnlineRaceModeE mode) {
    lobbyMutex.Lock("LobbyUsers::GetPointsFromUserRecord");
    if (bStrCmp(persona, FEDatabase->OnlineSettings.GetLobbyPersona()) == 0) {
        if (gotMyStats == true) {
            points = myStats.stats.raceModeStats[mode].points;
            lobbyMutex.Unlock("LobbyUsers::GetPointsFromUserRecord");
            return true;
        }

        LobbyApiUserT *me = GetMyUserRecord();
        if (me) {
            PlayerDataT tmp;
            tmp.ExtractRecord(*me);
            points = tmp.stats.raceModeStats[mode].points;
            lobbyMutex.Unlock("LobbyUsers::GetPointsFromUserRecord");
            return true;
        }
        lobbyMutex.Unlock("LobbyUsers::GetPointsFromUserRecord");
        return false;
    } else {
        for (OnlineUsersData *oud = userList.GetHead(); oud != userList.EndOfList();
             oud = oud->GetNext()) {
            if (bStrCmp(oud->user.name, persona) == 0) {
                if (oud->commandID == 0) {
                    PlayerDataT tmp;
                    tmp.ExtractRecord(oud->user);
                    points = tmp.stats.raceModeStats[mode].points;
                    lobbyMutex.Unlock("LobbyUsers::GetPointsFromUserRecord");
                    return true;
                }
            }
        }
    }

    lobbyMutex.Unlock("LobbyUsers::GetPointsFromUserRecord");
    return false;
}

bool LobbyUsers::GetDisqualificationsFromUserRecord(const char *persona, uint32 &disqs) {
    lobbyMutex.Lock("LobbyUsers::GetDisqualificationsFromUserRecord");
    if (bStrCmp(persona, FEDatabase->OnlineSettings.GetLobbyPersona()) == 0) {
        if (gotMyStats == true) {
            disqs = myStats.stats.disqualifications;
            lobbyMutex.Unlock("LobbyUsers::GetDisqualificationsFromUserRecord");
            return true;
        }

        LobbyApiUserT *me = GetMyUserRecord();
        if (me) {
            PlayerDataT tmp;
            tmp.ExtractRecord(*me);
            disqs = tmp.stats.disqualifications;
            lobbyMutex.Unlock("LobbyUsers::GetDisqualificationsFromUserRecord");
            return true;
        }
        lobbyMutex.Unlock("LobbyUsers::GetDisqualificationsFromUserRecord");
        return false;
    } else {
        for (OnlineUsersData *oud = userList.GetHead(); oud != userList.EndOfList();
             oud = oud->GetNext()) {
            if (bStrCmp(oud->user.name, persona) == 0) {
                if (oud->commandID == 0) {
                    PlayerDataT tmp;
                    tmp.ExtractRecord(oud->user);
                    disqs = tmp.stats.disqualifications;
                    lobbyMutex.Unlock("LobbyUsers::GetDisqualificationsFromUserRecord");
                    return true;
                }
            }
        }
    }

    lobbyMutex.Unlock("LobbyUsers::GetDisqualificationsFromUserRecord");
    return false;
}

int32 LobbyUsers::GetMyUserStats(PlayerDataT &userStats, CommandCBFunc func, void *context) {
    lobbyMutex.Lock("LobbyUsers::GetMyUserStats");
    if (gotMyStats == true) {
        bMemCpy(&userStats, &myStats, sizeof(PlayerDataT));
        lobbyMutex.Unlock("LobbyUsers::GetMyUserStats");
        return 0;
    }

    char buf[128] = "";
    TagFieldSetString(buf, sizeof(buf), "PERS", FEDatabase->OnlineSettings.GetLobbyPersona());
    int32 rc = LobbyCore::Instance().QueueCommand('user', buf, UserCB, this, func, context,
                                                   false);
    lobbyMutex.Unlock("LobbyUsers::GetMyUserStats");
    return rc;
}

void LobbyUsers::ClearUserStats() {
    lobbyMutex.Lock("LobbyUsers::ClearUserStats");
    myStats.Clear();
    gotMyStats = false;
    lobbyMutex.Unlock("LobbyUsers::ClearUserStats");
}

int32 LobbyUsers::GetOtherUserStats(PlayerDataT &userStats, const char *persona,
                                    CommandCBFunc func, void *context) {
    lobbyMutex.Lock("LobbyUsers::GetOtherUserStats");
    char buf[128] = "";
    TagFieldSetString(buf, sizeof(buf), "PERS", persona);
    int32 rc = LobbyCore::Instance().QueueCommand('user', buf, LobbyCore::DefaultCB, this, func,
                                                   context, false);
    lobbyMutex.Unlock("LobbyUsers::GetOtherUserStats");
    return rc;
}

int32 LobbyUsers::SubmitFeedback(const char *persona, const char *feedback, CommandCBFunc func,
                                 void *context) {
    lobbyMutex.Lock("LobbyUsers::SubmitFeedback");
    if (!persona || !persona[0]) {
        lobbyMutex.Unlock("LobbyUsers::SubmitFeedback");
        return -1;
    }

    char buf[64] = "";
    TagFieldSetString(buf, sizeof(buf), "PERS", persona);
    TagFieldSetString(buf, sizeof(buf), "REGN", BuildRegion::GetAbbreviation());
    if (feedback && feedback[0]) {
        TagFieldSetString(buf, sizeof(buf), "REPT", feedback);
    }

    int32 rc = LobbyCore::Instance().QueueCommand('rept', buf, LobbyCore::DefaultCB, nullptr,
                                                   func, context, false);
    lobbyMutex.Unlock("LobbyUsers::SubmitFeedback");
    return rc;
}

LobbyApiUserT *LobbyUsers::GetMyUserRecord() const {
    if (!LobbyCore::Instance().pLobbyRef) {
        return nullptr;
    }
    return static_cast<LobbyApiUserT *>(
        LobbyApiInfoPtr(LobbyCore::Instance().pLobbyRef, 'self'));
}

int32 LobbyUsers::Init() { return 0; }

void LobbyUsers::Reset() {
    ClearUserOnlineRecordCache();
    myStats.Clear();
    gotMyStats = false;
    NetFree(auxiData);
    auxiData = nullptr;
}

void LobbyUsers::MaybeCreateAuxiBuffer() {
    if (!auxiData) {
        auxiData = static_cast<char *>(bMalloc(132, "LobbyUsers::auxiData", 0, 8));
        auxiData[0] = '\0';
    }
}

void LobbyUsers::SetSessionChangeFlag(bool setFlag) {
#if defined(EA_PLATFORM_WIN32) || defined(EA_PLATFORM_XENON) // PC (FEDatabase + 0x19C >= 2) y Xbox 360 (+ 0x1A8): los datos auxiliares solo salen con el lobby en marcha
    if (FEDatabase->GetOnlineSettings()->GetState() < cOnlineSettings::OLS_LOBBY_IDLE) {
        return;
    }
#endif
    MaybeCreateAuxiBuffer();
    TagFieldSetNumber(auxiData, 128, "SCF", setFlag ? 1 : 0);
    SendAuxiData();
}

void LobbyUsers::SetSessionLatency(int late) {
#if defined(EA_PLATFORM_WIN32) || defined(EA_PLATFORM_XENON) // PC (FEDatabase + 0x19C >= 2) y Xbox 360 (+ 0x1A8): los datos auxiliares solo salen con el lobby en marcha
    if (FEDatabase->GetOnlineSettings()->GetState() < cOnlineSettings::OLS_LOBBY_IDLE) {
        return;
    }
#endif
#if defined(EA_PLATFORM_WIN32) || defined(EA_PLATFORM_XENON) // Xbox 360 igual (EGM 0x8258B690)
    // PC (speed.exe 0x799D60): la condicion negada y `<= 25` (MSVC distingue `< 26`: cmp 0x1a/jge frente a cmp 0x19/jg).
    if (!((mLate >= 0 && late < 0) || (mLate < 0 && late >= 0))) {
        if (mLate - late <= 25 && late - mLate <= 25) {
            return;
        }
    }
#else
    if ((mLate < 0 && late < 0) || (mLate >= 0 && late >= 0)) {
        if (mLate - late < 26 && late - mLate < 26) {
            return;
        }
    }
#endif

    mLate = late;
    MaybeCreateAuxiBuffer();
    TagFieldSetNumber(auxiData, 128, "LT", late);
    SendAuxiData();
}

#if defined(EA_PLATFORM_WIN32) || defined(EA_PLATFORM_XENON)
// PC (0x799DF0; Xbox 360 0x8258B740): neutral name (see LobbyUsers.hpp).
void LobbyUsers::SetWin32Unknown0x23c(int value) {
    if (FEDatabase->GetOnlineSettings()->GetState() < cOnlineSettings::OLS_LOBBY_IDLE) {
        return;
    }
    if (win32_unknown_0x23c != value) {
        win32_unknown_0x23c = value;
        MaybeCreateAuxiBuffer();
        TagFieldSetNumber(auxiData, 128, "CD", win32_unknown_0x23c);
        SendAuxiData();
    }
}

// PC (0x79D3F0): neutral name. When I host the game, "CE" carries 4 ints: 3 for me and the connection state of each
// other player (0 if connapi does not know him); otherwise the tag is deleted.
void LobbyUsers::SetWin32UnknownCE() {
    if (FEDatabase->GetOnlineSettings()->GetState() < cOnlineSettings::OLS_LOBBY_IDLE) {
        return;
    }
    lobbyMutex.Lock(nullptr);
    MaybeCreateAuxiBuffer();
    LobbyApiPlayT *game = LobbyGames::Instance().GetMyGame();
    if (game && bStrCmp(FEDatabase->GetOnlineSettings()->GetLobbyPersona(), game->strHost) == 0) {
        ConnectionCore &connection = ConnectionCore::Instance();
        int status[4];
        bMemSet(status, 0, sizeof(status));
        status[0] = 3;
        for (int i = 1; i < game->iMaxsize; i++) {
            ConnApiClientT *player = connection.GetPlayer(game->aOpponents[i].strPers);
            status[i] = player ? player->GameInfo.eStatus : 0;
        }
        TagFieldSetStructure(auxiData, 128, "CE", status, sizeof(status), "l*");
    } else {
        TagFieldDelete(auxiData, "CE");
    }
    SendAuxiData();
    lobbyMutex.Unlock(nullptr);
}
#else
void LobbyUsers::SetSessionRaceStatusInfo(int lap, int mapx, int mapy) {
    if (mLap != lap) {
        mLap = lap;
        mMapX = mapx;
        mMapY = mapy;
        MaybeCreateAuxiBuffer();
        TagFieldSetNumber(auxiData, 128, "LP", mLap);
        SendAuxiData();
    }
}
#endif

int32 LobbyUsers::SendAuxiData() {
    if (!auxiData) {
        return 0;
    }

    TagFieldSetNumber(auxiData, 128, "V", NetworkCore::Instance().GetNetworkVersionNumber());
    char buf[132] = "";
    TagFieldSetString(buf, 128, "TEXT", auxiData);
    return LobbyCore::Instance().QueueCommand('auxi', buf, LobbyCore::DefaultCB, this, nullptr,
                                               nullptr, false);
}

void LobbyUsers::OnlnCB(LobbyApiRefT *pRef, LobbyApiMsgT *pMsg, void *pData) {
    LobbyUsers *lobbyUsers = static_cast<LobbyUsers *>(pData);

    if (pMsg->code == 0) {
        char newName[16] = "";
        TagFieldGetString(TagFieldFind(pMsg->pData, "N"), newName, sizeof(newName), "");

        for (OnlineUsersData *oud = lobbyUsers->userList.GetHead();
             oud != lobbyUsers->userList.EndOfList(); oud = oud->GetNext()) {
            if (bStrCmp(oud->user.name, newName) == 0) {
                LobbyApiExtractUserRecord(&oud->user, pMsg->pData);
                oud->commandID = 0;
                break;
            }
        }
    }

    LobbyCore::Instance().FinishCommand(pMsg, true);
}

void LobbyUsers::UserCB(LobbyApiRefT *pRef, LobbyApiMsgT *pMsg, void *pData) {
    if (pMsg->code == 0) {
        static_cast<LobbyUsers *>(pData)->myStats.ExtractRecord(pMsg->pData);
        static_cast<LobbyUsers *>(pData)->gotMyStats = true;
    }

    LobbyCore::Instance().FinishCommand(pMsg, true);
}
