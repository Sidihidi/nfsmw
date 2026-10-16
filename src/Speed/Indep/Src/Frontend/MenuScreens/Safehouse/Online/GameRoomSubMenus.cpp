#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/Online/GameRoomSubMenus.hpp"

#include "Speed/Indep/Src/FEng/FEList.h"
#include "Speed/Indep/Src/Frontend/Careers/UnlockSystem.hpp"
#include "Speed/Indep/Src/Frontend/Database/FEDatabase.hpp"
#include "Speed/Indep/Src/Frontend/Database/RaceDB.hpp"
#include "Speed/Indep/Src/Frontend/FECarViewer.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterface.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterfaceFEImages.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterfaceFEObjects.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterfaceFEStrings.hpp"
#include "Speed/Indep/Src/Frontend/Localization/Localize.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/Online/uiOnlineMain.hpp"
#include "Speed/Indep/Src/Generated/AttribSys/Classes/pvehicle.h"
#include "Speed/Indep/Src/Online/BuddyCore.hpp"
#include "Speed/Indep/Src/Online/LobbyCore.hpp"
#include "Speed/Indep/Src/Online/LobbyGameSessions.hpp"
#include "Speed/Indep/Src/Online/LobbyGames.hpp"
#include "Speed/Indep/Src/Online/VoiceCore.hpp"
#include "Speed/Indep/Src/Physics/PhysicsInfo.hpp"
#include "Speed/Indep/Src/World/CarInfo.hpp"
#include "Speed/Indep/bWare/Inc/Strings.hpp"

MenuScreen *CreateUIOLGameRoomHostScreen(ScreenConstructorData *sd) {
    return new ("UIOLGameRoomHost", 0) UIOLGameRoomHost(sd);
}

UIOLGameRoomHost::UIOLGameRoomHost(ScreenConstructorData *sd)
    : MenuScreen(sd) //
    , optionScroller(GetPackageName(), nullptr, nullptr, true, true, false, true) //
    , m_scollbar(GetPackageName(), "ScrollBar", true, false, false) {
    LobbyGameSessions::Instance().StartSessionChanges();
    Setup();
}

UIOLGameRoomHost::~UIOLGameRoomHost() {
    LobbyGameSessions::Instance().FinishSessionChanges();
    cFEng::Get()->QueueGameMessage(0xd9656bec, g_GameRoomName, 0xff);
    optionScroller.DeleteScrollerData();
}

void UIOLGameRoomHost::NotificationMessage(u32 msg, FEObject *pobj, u32 param1, u32 param2) {
    switch (msg) {
        case 0x406415e3:
        case 0x0c407210: {
            OptionScrollerDatum *datum = static_cast<OptionScrollerDatum *>(optionScroller.GetSelectedDatum());
            if (datum) {
                cFEng::Get()->QueueGameMessage(datum->msgHash, g_GameRoomName, 0xff);
            }
            cFEng::Get()->QueuePackagePop(1);
            FEngSetScript(g_GameRoomName, 0x5d974277, 0x1ca7c0, true);
            break;
        }
        case 0x911ab364: {
            cFEng::Get()->QueuePackagePop(1);
            FEngSetScript(g_GameRoomName, 0x5d974277, 0x1ca7c0, true);
            break;
        }
        case 0x72619778:
            optionScroller.ScrollPrev();
            optionScroller.HighlightSelected();
            break;
        case 0x911c0a4b:
            optionScroller.ScrollNext();
            optionScroller.HighlightSelected();
            break;
        case 0x35f8620b:
            break;
    }
}

void UIOLGameRoomHost::Setup() {
    char sztemp[36];
    int option;

    FEngSetLanguageHash(GetPackageName(), 0xb2756356, UIOLGameRoom::mSelectedPlayerDatum->carNameHash);
    FEPrintf(GetPackageName(), 0xeb406fec, UIOLGameRoom::mSelectedPlayerDatum->name);
    if (FEDatabase->GetOnlineSettings()->PerformanceMatching) {
        FEngSetScript(GetPackageName(), 0x3755e286, 0x1ca7c0, true);
    } else {
        FEngSetScript(GetPackageName(), 0x3755e286, 0x16a259, true);
    }

    for (uint8 x = 0; x < 3; x++) {
        ScrollerSlot *pslot = new ("ScrollerSlot", 0) ScrollerSlot;
        FEngSNPrintf(sztemp, 36, "Option_%d", x + 1);
        FEString *str = FEngFindString(GetPackageName(), FEHashUpper(sztemp));
        pslot->AddData(str);
        optionScroller.AddSlot(pslot);
        if (x == 0) {
            optionScroller.SetSelected(pslot);
        }
    }

    option = 1;
    OptionScrollerDatum *datum = new ("OptionScrollerDatum", 0) OptionScrollerDatum;
    datum->AddData(GetLocalizedString(0x69b36b15), 0);
    datum->msgHash = 0xc3dad747;
    optionScroller.AddData(datum);
    optionScroller.Enable(datum);
    option++;

    if (LobbyGames::Instance().GetMyGame()) {
        const char *persona = UIOLGameRoom::mSelectedPlayerDatum->name;
        BuddySettings buddySettings = EA_Messenger_GetBuddySettingsByName(persona);
        if (is_this_guy_voice_chatting(persona, buddySettings)) {
            datum = new ("OptionScrollerDatum", 0) OptionScrollerDatum;
            if (VoiceCore::instance().IsMuted(persona)) {
                datum->AddData(GetLocalizedString(0x97cbc6d8), 0);
            } else {
                datum->AddData(GetLocalizedString(0x90f8ac46), 0);
            }
            datum->msgHash = 0x613697d7;
            optionScroller.AddData(datum);
            optionScroller.Enable(datum);
            option++;
        }
    }

    if (option == 1) {
        FEngSetInvisible(GetPackageName(), 0x06745352);
    }
    while (option < 4u) {
        FEngSNPrintf(sztemp, 36, "Option_%d", option);
        FEPrintf(GetPackageName(), FEHashUpper(sztemp), "%s", " ");
        option++;
    }

    optionScroller.HighlightSelected();
    optionScroller.Update(true);
}

MenuScreen *CreateGameRoomCarSelectScreen(ScreenConstructorData *sd) {
    return new ("OL_GameRoomCarSelect", 0) OL_GameRoomCarSelect(sd);
}

OL_GameRoomCarSelect::OL_GameRoomCarSelect(ScreenConstructorData *sd)
    : MenuScreen(sd) //
    , m_optionList(GetPackageName(), nullptr, nullptr, true, true, false, true) {
    LobbyGameSessions::Instance().StartSessionChanges();
    m_stable = FEDatabase->GetPlayerCarStable(0);
    Setup();
}

OL_GameRoomCarSelect::~OL_GameRoomCarSelect() {
    cFEng::Get()->QueueGameMessage(0xd9656bec, g_GameRoomName, 0xff);
    LobbyGameSessions::Instance().FinishSessionChanges();
}

void OL_GameRoomCarSelect::NotificationMessage(u32 msg, FEObject *pobj, u32 param1, u32 param2) {
    switch (msg) {
        case 0x911ab364:
            m_currentCarHandle = FEDatabase->GetQuickRaceSettings(GRace::kRaceType_NumTypes)->GetSelectedCar(0);
            BuildRideAndUpdate();
        case 0x406415e3:
        case 0x0c407210:
            FEDatabase->GetQuickRaceSettings(GRace::kRaceType_NumTypes)->SetSelectedCar(m_currentCarHandle, 0);
            cFEng::Get()->QueueGameMessage(0xd9656bec, g_GameRoomName, 0xff);
            cFEng::Get()->QueuePackagePop(1);
            FEngSetScript(g_GameRoomName, 0x5d974277, 0x1ca7c0, true);
            break;
        case 0x72619778:
            FEngSetScript(GetPackageName(), 0xa808e057, 0x1b20c2, true);
            m_optionList.ScrollPrev();
            m_optionList.HighlightSelected();
            m_optionList.Update(true);
            break;
        case 0x911c0a4b:
            FEngSetScript(GetPackageName(), 0xa808e057, 0x1b20c3, true);
            m_optionList.ScrollNext();
            m_optionList.HighlightSelected();
            m_optionList.Update(true);
            break;
        case 0xb5971bf1:
            EditOption(1);
            break;
        case 0x9120409e:
            EditOption(0);
            break;
        case 0x35f8620b:
            break;
        case 0xc98356ba:
            break;
    }
}

void OL_GameRoomCarSelect::Setup() {
    InitStatsSliders();
    char sztemp[32] = "";

    ScrollerSlot *pslot = new ("ScrollerSlot", 0) ScrollerSlot;
    FEString *str = FEngFindString(GetPackageName(), 0x4b4e3888);
    pslot->AddData(str);
    m_optionList.AddSlot(pslot);

    pslot = new ("ScrollerSlot", 0) ScrollerSlot;
    str = FEngFindString(GetPackageName(), 0x4b4e3889);
    pslot->AddData(str);
    m_optionList.AddSlot(pslot);

    ScrollerDatum *datum = new ("ScrollerDatum", 0) ScrollerDatum;
    datum->AddData("11", 0);
    m_optionList.AddData(datum);
    m_optionList.Enable(datum);

    datum = new ("ScrollerDatum", 0) ScrollerDatum;
    datum->AddData("22", 0);
    m_optionList.AddData(datum);
    m_optionList.Enable(datum);

    m_currentCarHandle = FEDatabase->GetQuickRaceSettings(GRace::kRaceType_NumTypes)->GetSelectedCar(0);
    m_filter = 1;
    BuildCarList();
    while (!LookupCurrentList(m_currentCarHandle)) {
        FilterNext();
        if (m_filter == 1) {
            m_index = 0;
            pCurrentNode = CarsList.GetHead();
            m_currentCarHandle = pCurrentNode->mHandle;
            break;
        }
    }

    FEPrintf(GetPackageName(), 0x6f25a248, "%d", m_index + 1);
    FEPrintf(GetPackageName(), 0xb2037bdc, "%d", m_total);
    BuildRideAndUpdate();
}

bool OL_GameRoomCarSelect::LookupCurrentList(int m_currentCarHandle) {
    pCurrentNode = CarsList.GetHead();
    m_index = 0;
    do {
        if (pCurrentNode->mHandle == m_currentCarHandle) {
            return true;
        }
        pCurrentNode = CarsList.GetNextCircular(pCurrentNode);
        m_index++;
    } while (pCurrentNode != CarsList.GetHead());
    return false;
}

void OL_GameRoomCarSelect::BuildCarList() {
    ClearCarList();
    m_total = 0;
    m_stable = FEDatabase->GetPlayerCarStable(0);
    for (int i = 0; i < 200; i++) {
        FECarRecord *fe_car = m_stable->GetCarByIndex(i);
        if (!fe_car->IsValid()) {
            continue;
        }
        if (fe_car->MatchesFilter(m_filter | 0xf0000)) {
            bool bCarUnLocked = UnlockSystem::IsCarUnlocked(
                static_cast<eUnlockFilters>(UNLOCK_QUICK_RACE | UNLOCK_CAREER_MODE | UNLOCK_ONLINE), fe_car->Handle, 0);
            if (bCarUnLocked) {
                m_total++;
                CarsList.AddTail(new ("SelectableCar", 0) SelectableCar(fe_car->Handle, !bCarUnLocked));
            }
        }
    }
    if (!CarsList.IsEmpty()) {
        pCurrentNode = CarsList.GetHead();
        m_index = 0;
    }
}

void OL_GameRoomCarSelect::ClearCarList() {
    CarsList.DeleteAllElements();
}

void OL_GameRoomCarSelect::UpdateCarChoiceDisplay() {
    ScrollerSlot *pslot = m_optionList.GetSlot(1);
    ScrollerDatum *datum = m_optionList.FindDatumInSlot(pslot);
    FECarRecord *fe_car = m_stable->GetCarRecordByHandle(m_currentCarHandle);
    if (fe_car->GetNameHash() != 0x9bb9ccc3 && DoesStringExist(fe_car->GetNameHash())) {
        bStrCpy(datum->GetTopDatumModeString(), GetLocalizedString(fe_car->GetNameHash()));
    } else {
        bStrCpy(datum->GetTopDatumModeString(), fe_car->GetDebugName());
    }
}

void OL_GameRoomCarSelect::UpdateCarListName() {
    uint32 texthash;

    m_filter = m_stable->GetCarRecordByHandle(m_currentCarHandle)->FilterBits & 0xf;
    if (m_filter == 0) {
        m_filter = 1;
    }
    switch (m_filter) {
        case 1:
            texthash = 0xd9d6b954;
            break;
        case 2:
            texthash = 0xee053562;
            break;
        case 4:
            texthash = 0x2414de28;
            break;
        case 8:
            texthash = 0xd8a058f7;
            break;
        case 16:
            texthash = 0x0d8500c3;
            break;
        default:
            texthash = 0;
            break;
    }
    ScrollerSlot *pslot = m_optionList.GetSlot(0);
    ScrollerDatum *datum = m_optionList.FindDatumInSlot(pslot);
    bStrCpy(datum->GetTopDatumModeString(), GetLocalizedString(texthash));
    UpdateSliders();
}

void OL_GameRoomCarSelect::FilterNext() {
    switch (m_filter) {
        case 1:
            m_filter = 2;
            break;
        case 2:
            m_filter = 4;
            break;
        case 4:
            m_filter = 1;
            break;
    }
    BuildCarList();
}

void OL_GameRoomCarSelect::FilterPrev() {
    switch (m_filter) {
        case 1:
            m_filter = 4;
            break;
        case 2:
            m_filter = 1;
            break;
        case 4:
            m_filter = 2;
            break;
    }
    BuildCarList();
}

void OL_GameRoomCarSelect::EditOption(int direction) {
    int index = m_optionList.GetSelectedNodeIndex();

    switch (index) {
        case 1: {
            if (direction) {
                FilterNext();
                while (CarsList.IsEmpty()) {
                    FilterNext();
                }
            } else {
                FilterPrev();
                while (CarsList.IsEmpty()) {
                    FilterPrev();
                }
            }
            pCurrentNode = CarsList.GetHead();
            m_currentCarHandle = pCurrentNode->mHandle;
            BuildRideAndUpdate();
            break;
        }
        case 2: {
            if (direction) {
                pCurrentNode = CarsList.GetNextCircular(pCurrentNode);
                m_index--;
                if (m_index < 0) {
                    m_index = m_total - 1;
                }
            } else {
                pCurrentNode = CarsList.GetPrevCircular(pCurrentNode);
                m_index++;
                if (m_index >= m_total) {
                    m_index = 0;
                }
            }
            m_currentCarHandle = pCurrentNode->mHandle;
            BuildRideAndUpdate();
            break;
        }
    }
    FEPrintf(GetPackageName(), 0x6f25a248, "%d", m_index + 1);
    FEPrintf(GetPackageName(), 0xb2037bdc, "%d", m_total);
}

void OL_GameRoomCarSelect::BuildRideAndUpdate() {
    RideInfo ride;

    m_stable->BuildRideForPlayer(m_currentCarHandle, 0, &ride);
    CarViewer::SetRideInfo(&ride, SET_RIDE_INFO_REASON_LOAD_CAR, eCARVIEWER_PLAYER1_CAR);
    UpdateCarChoiceDisplay();
    UpdateCarListName();
    m_optionList.Update(true);
}

void OL_GameRoomCarSelect::InitStatsSliders() {
    AccelerationSlider.Init(GetPackageName(), "ACCELERATION", 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 160.0f);
    TopSpeedSlider.Init(GetPackageName(), "TOPSPEED", 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 160.0f);
    HandlingSlider.Init(GetPackageName(), "HANDLING", 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 160.0f);
}

void OL_GameRoomCarSelect::UpdateSliders() {
    Physics::Info::Performance performance;
    Physics::Info::Performance current_performance;

    FECarRecord *pcar = m_stable->GetCarRecordByHandle(m_currentCarHandle);
    Attrib::Gen::pvehicle pvehicle(pcar->VehicleKey, 0, nullptr);
    if (pcar->IsCustomized()) {
        m_stable->GetCustomizationRecordByHandle(pcar->Customization)->WriteRecordIntoPhysics(pvehicle);
    }
    Physics::Info::EstimatePerformance(pvehicle, performance);

    AccelerationSlider.SetValue(performance.Acceleration);
    AccelerationSlider.SetPreviewValue(current_performance.Acceleration);
    AccelerationSlider.Draw();
    TopSpeedSlider.SetValue(performance.TopSpeed);
    TopSpeedSlider.SetPreviewValue(current_performance.TopSpeed);
    TopSpeedSlider.Draw();
    HandlingSlider.SetValue(performance.Handling);
    HandlingSlider.SetPreviewValue(current_performance.Handling);
    HandlingSlider.Draw();
}

MenuScreen *CreateEditGameDetailsScreen(ScreenConstructorData *sd) {
    return new ("EditGameDetails", 0) EditGameDetails(sd);
}

EditGameDetails::EditGameDetails(ScreenConstructorData *sd)
    : MenuScreen(sd) //
    , m_optionList(GetPackageName(), nullptr, nullptr, true, true, false, true) {
    LobbyGameSessions::Instance().StartSessionChanges();
    mTempMinPlayers = FEDatabase->GetOnlineSettings()->MinOnlinePlayers;
    Setup();
}

EditGameDetails::~EditGameDetails() {
    FEDatabase->GetOnlineSettings()->MinOnlinePlayers = mTempMinPlayers;
    LobbyGameSessions::Instance().FinishSessionChanges();
    LobbyGameSessions::Instance().SettingsHaveChanged();
}

void EditGameDetails::NotificationMessage(u32 msg, FEObject *pobj, u32 param1, u32 param2) {
    switch (msg) {
        case 0x911ab364:
        case 0x406415e3:
        case 0x0c407210: {
            cFEng::Get()->QueuePackagePop(1);
            FEngSetScript(g_GameRoomName, 0x5d974277, 0x1ca7c0, true);
            break;
        }
        case 0x72619778:
            m_optionList.ScrollPrev();
            UpdateEditPosition();
            break;
        case 0x911c0a4b:
            m_optionList.ScrollNext();
            UpdateEditPosition();
            break;
        case 0x9120409e:
            ChangeSelectedItem(0);
            break;
        case 0xb5971bf1:
            ChangeSelectedItem(1);
            break;
        case 0x35f8620b:
            m_optionList.HighlightSelected();
            break;
        case 0xc98356ba:
            break;
    }
}

void EditGameDetails::Setup() {
    char sztemp[32] = "";
    ScrollerDatum *datum;

    for (int i = 0; i < 10; i++) {
        ScrollerSlot *pslot = new ("ScrollerSlot", 0) ScrollerSlot;
        FEngSNPrintf(sztemp, 32, "option_%d", i + 1);
        FEString *str = FEngFindString(GetPackageName(), FEHashUpper(sztemp));
        pslot->AddData(str);
        m_optionList.AddSlot(pslot);
    }

    for (int i = 0; i < 3; i++) {
        SelectableMode *mode = new ("SelectableMode", 0) SelectableMode(static_cast<GRace::Type>(i));
        switch (mode->m_type) {
            case GRace::kRaceType_Circuit:
                mode->m_textHash = 0x034fa2c1;
                mode->m_textureHash = 0xe9638d3e;
                break;
            case GRace::kRaceType_P2P:
                mode->m_textHash = 0xb94fd70e;
                mode->m_textureHash = 0x2521e5eb;
                break;
            case GRace::kRaceType_Drag:
                mode->m_textHash = 0x6f547e4c;
                mode->m_textureHash = 0xaaab31e9;
                break;
        }
        m_modelist.AddTail(mode);
    }

    for (unsigned int i = 0; i < GRaceDatabase::Get().GetRaceCount(); i++) {
        GRaceParameters *parms = GRaceDatabase::Get().GetRaceParameters(i);
        if (!UnlockSystem::IsTrackUnlocked(UNLOCK_ONLINE, parms->GetEventHash(), 0)) {
            continue;
        }
        if (parms->GetRaceType() == GRace::kRaceType_Circuit) {
            m_CircuitTracks.AddTail(new ("SelectableTrack", 0) SelectableTrack(parms, true));
        }
        if (parms->GetRaceType() == GRace::kRaceType_P2P) {
            m_SprintTracks.AddTail(new ("SelectableTrack", 0) SelectableTrack(parms, true));
        }
        if (parms->GetRaceType() == GRace::kRaceType_Drag) {
            m_DragTracks.AddTail(new ("SelectableTrack", 0) SelectableTrack(parms, true));
        }
    }

    GameSession *session = LobbyGameSessions::Instance().GetMySession();
    uint32 EventHash = TagFieldGetNumber(TagFieldFind(session->strParams, "E"), 0);
    uint8 TrackDirection = (session->uCustFlags & 0x1000) != 0;
    uint8 NumLaps = TagFieldGetNumber(TagFieldFind(session->strParams, "L"), 0);
    uint8 MinPlayers = TagFieldGetNumber(TagFieldFind(session->strParams, "M"), 0);
    GRaceParameters *parms = GRaceDatabase::Get().GetRaceFromHash(EventHash);

    m_currentModePtr = m_modelist.GetHead();
    while (m_currentModePtr->m_type != parms->GetRaceType()) {
        m_currentModePtr = m_modelist.GetNextCircular(m_currentModePtr);
    }
    FEngSetTextureHash(GetPackageName(), 0xceb82625, m_currentModePtr->m_textureHash);

    datum = new ("ScrollerDatum", 0) ScrollerDatum;
    datum->AddData(GetLocalizedString(m_currentModePtr->m_textHash), 0);
    m_optionList.AddData(datum);
    m_optionList.Enable(datum);

    m_currentCircuitTrack = m_CircuitTracks.GetHead();
    m_currentSprintTrack = m_SprintTracks.GetHead();
    m_currentDragTrack = m_DragTracks.GetHead();
    m_currentSelectableTrack = nullptr;
    switch (parms->GetRaceType()) {
        case GRace::kRaceType_Circuit:
            m_currentSelectableTrack = m_currentCircuitTrack;
            break;
        case GRace::kRaceType_P2P:
            m_currentSelectableTrack = m_currentSprintTrack;
            break;
        case GRace::kRaceType_Drag:
            m_currentSelectableTrack = m_currentDragTrack;
            break;
    }
    while (m_currentSelectableTrack->pRaceParams->GetEventHash() != EventHash) {
        switch (parms->GetRaceType()) {
            case GRace::kRaceType_Circuit:
                m_currentSelectableTrack = m_CircuitTracks.GetNextCircular(m_currentSelectableTrack);
                break;
            case GRace::kRaceType_P2P:
                m_currentSelectableTrack = m_SprintTracks.GetNextCircular(m_currentSelectableTrack);
                break;
            case GRace::kRaceType_Drag:
                m_currentSelectableTrack = m_DragTracks.GetNextCircular(m_currentSelectableTrack);
                break;
        }
    }

    uint32 hash = CalcLanguageHash("TRACKNAME_", m_currentSelectableTrack->pRaceParams);
    if (DoesStringExist(hash)) {
        FEngSNPrintf(sztemp, 32, GetLocalizedString(hash));
    } else {
        FEngSNPrintf(sztemp, 32, m_currentSelectableTrack->pRaceParams->GetEventID());
    }
    datum = new ("ScrollerDatum", 0) ScrollerDatum;
    datum->AddData(sztemp, 0);
    m_optionList.AddData(datum);
    m_optionList.Enable(datum);

    FEngSNPrintf(sztemp, 32, "%d", MinPlayers);
    datum = new ("ScrollerDatum", 0) ScrollerDatum;
    datum->AddData(sztemp, 0);
    m_optionList.AddData(datum);
    m_optionList.Enable(datum);
    FEngSetScript(GetPackageName(), 0x4709e252, 0x6ebbfb68, true);
    FEngSetScript(GetPackageName(), 0x4b4e388a, 0x6ebbfb68, true);

    datum = new ("ScrollerDatum", 0) ScrollerDatum;
    if (FEDatabase->GetOnlineSettings()->RankedGame) {
        datum->AddData(GetLocalizedString(0x70e01038), 0);
    } else {
        datum->AddData(GetLocalizedString(0x417b25e4), 0);
    }
    m_optionList.AddData(datum);
    m_optionList.Disable(datum);
    FEngSetScript(GetPackageName(), 0x4709e253, 0x163c76, true);
    FEngSetScript(GetPackageName(), 0x4b4e388b, 0x163c76, true);

    const char *distUnits;
    if (FEDatabase->GetGameplaySettings()->SpeedoUnits == 1) {
        distUnits = GetLocalizedString(0x8569a26a);
        FEngSNPrintf(sztemp, 32, "%0.1f %s", m_currentSelectableTrack->pRaceParams->GetRaceLengthMeters() / 1000.0f, distUnits);
    } else {
        distUnits = GetLocalizedString(0x867dcfd9);
        FEngSNPrintf(sztemp, 32, "%0.1f %s", m_currentSelectableTrack->pRaceParams->GetRaceLengthMeters() / 1609.34f, distUnits);
    }
    datum = new ("ScrollerDatum", 0) ScrollerDatum;
    datum->AddData(sztemp, 0);
    m_optionList.AddData(datum);
    m_optionList.Disable(datum);
    FEngSetScript(GetPackageName(), 0x4709e254, 0x163c76, true);
    FEngSetScript(GetPackageName(), 0x4b4e388c, 0x163c76, true);

    FEngSNPrintf(sztemp, 32, "%d", NumLaps);
    datum = new ("ScrollerDatum", 0) ScrollerDatum;
    if (parms->GetRaceType() == GRace::kRaceType_Circuit) {
        datum->AddData(sztemp, 0);
    } else {
        datum->AddData(GetLocalizedString(0x7f2f7ad6), 0);
    }
    m_optionList.AddData(datum);
    if (parms->GetRaceType() == GRace::kRaceType_Circuit && !FEDatabase->GetOnlineSettings()->RankedGame) {
        m_optionList.Enable(datum);
        FEngSetScript(GetPackageName(), 0x4709e255, 0x6ebbfb68, true);
        FEngSetScript(GetPackageName(), 0x4b4e388d, 0x6ebbfb68, true);
    } else {
        m_optionList.Disable(datum);
        FEngSetScript(GetPackageName(), 0x4709e255, 0x163c76, true);
        FEngSetScript(GetPackageName(), 0x4b4e388d, 0x163c76, true);
    }

    datum = new ("ScrollerDatum", 0) ScrollerDatum;
    if (TrackDirection) {
        datum->AddData(GetLocalizedString(0xa1cd823e), 0);
    } else {
        datum->AddData(GetLocalizedString(0xde6eff34), 0);
    }
    m_optionList.AddData(datum);
    if (FEDatabase->GetOnlineSettings()->RankedGame) {
        m_optionList.Disable(datum);
        FEngSetScript(GetPackageName(), 0x4709e256, 0x163c76, true);
        FEngSetScript(GetPackageName(), 0x4b4e388e, 0x163c76, true);
    } else {
        m_optionList.Enable(datum);
        FEngSetScript(GetPackageName(), 0x4709e256, 0x6ebbfb68, true);
        FEngSetScript(GetPackageName(), 0x4b4e388e, 0x6ebbfb68, true);
    }

    datum = new ("ScrollerDatum", 0) ScrollerDatum;
    if (FEDatabase->GetOnlineSettings()->UseNOS) {
        datum->AddData(GetLocalizedString(0x417b2604), 0);
    } else {
        datum->AddData(GetLocalizedString(0x70dfe5c2), 0);
    }
    m_optionList.AddData(datum);
    if (FEDatabase->GetOnlineSettings()->RankedGame) {
        m_optionList.Disable(datum);
        FEngSetScript(GetPackageName(), 0x4709e257, 0x163c76, true);
        FEngSetScript(GetPackageName(), 0x4b4e388f, 0x163c76, true);
    } else {
        m_optionList.Enable(datum);
        FEngSetScript(GetPackageName(), 0x4709e257, 0x6ebbfb68, true);
        FEngSetScript(GetPackageName(), 0x4b4e388f, 0x6ebbfb68, true);
    }

    datum = new ("ScrollerDatum", 0) ScrollerDatum;
    if (FEDatabase->GetOnlineSettings()->PerformanceMatching) {
        datum->AddData(GetLocalizedString(0x417b2604), 0);
    } else {
        datum->AddData(GetLocalizedString(0x70dfe5c2), 0);
    }
    m_optionList.AddData(datum);
    if (FEDatabase->GetOnlineSettings()->RankedGame) {
        m_optionList.Disable(datum);
        FEngSetScript(GetPackageName(), 0x4709e258, 0x163c76, true);
        FEngSetScript(GetPackageName(), 0x4b4e3890, 0x163c76, true);
    } else {
        m_optionList.Enable(datum);
        FEngSetScript(GetPackageName(), 0x4709e258, 0x6ebbfb68, true);
        FEngSetScript(GetPackageName(), 0x4b4e3890, 0x6ebbfb68, true);
    }

    datum = new ("ScrollerDatum", 0) ScrollerDatum;
    if (FEDatabase->GetOnlineSettings()->CollisionDetection) {
        datum->AddData(GetLocalizedString(0x417b2604), 0);
    } else {
        datum->AddData(GetLocalizedString(0x70dfe5c2), 0);
    }
    m_optionList.AddData(datum);
    if (FEDatabase->GetOnlineSettings()->RankedGame) {
        m_optionList.Disable(datum);
        FEngSetScript(GetPackageName(), 0x28462c80, 0x163c76, true);
        FEngSetScript(GetPackageName(), 0xb51549b8, 0x163c76, true);
    } else {
        m_optionList.Enable(datum);
        FEngSetScript(GetPackageName(), 0x28462c80, 0x6ebbfb68, true);
        FEngSetScript(GetPackageName(), 0xb51549b8, 0x6ebbfb68, true);
    }

    m_optionList.HighlightSelected();
    m_optionList.Update(true);
}

void EditGameDetails::UpdateEditPosition() {
    char buf[32] = "";

    m_optionList.HighlightSelected();
    unsigned int index = m_optionList.GetSelectedNodeIndex();
    FEngSNPrintf(buf, 32, "POS%d", index);
    FEngSetScript(GetPackageName(), 0xa808e057, FEHashUpper(buf), true);
    m_optionList.Update(true);
}

void EditGameDetails::ChangeSelectedItem(int direction) {
    char sztemp[32] = "";
    int index = m_optionList.GetSelectedNodeIndex();

    switch (index - 1) {
        case eGameOption_MODE: {
            if (direction) {
                m_currentModePtr = m_modelist.GetNextCircular(m_currentModePtr);
            } else {
                m_currentModePtr = m_modelist.GetPrevCircular(m_currentModePtr);
            }
            FixUpOptionsForModeSwitch();
            break;
        }
        case eGameOption_TRACK: {
            if (direction) {
                switch (m_currentModePtr->m_type) {
                    case GRace::kRaceType_Circuit:
                        m_currentSelectableTrack = m_CircuitTracks.GetNextCircular(m_currentSelectableTrack);
                        break;
                    case GRace::kRaceType_P2P:
                        m_currentSelectableTrack = m_SprintTracks.GetNextCircular(m_currentSelectableTrack);
                        break;
                    case GRace::kRaceType_Drag:
                        m_currentSelectableTrack = m_DragTracks.GetNextCircular(m_currentSelectableTrack);
                        break;
                }
            } else {
                switch (m_currentModePtr->m_type) {
                    case GRace::kRaceType_Circuit:
                        m_currentSelectableTrack = m_CircuitTracks.GetPrevCircular(m_currentSelectableTrack);
                        break;
                    case GRace::kRaceType_P2P:
                        m_currentSelectableTrack = m_SprintTracks.GetPrevCircular(m_currentSelectableTrack);
                        break;
                    case GRace::kRaceType_Drag:
                        m_currentSelectableTrack = m_DragTracks.GetPrevCircular(m_currentSelectableTrack);
                        break;
                }
            }
            UpdateTrackDisplay();
            break;
        }
        case eGameOption_MINPLAYERS: {
            if (direction) {
                mTempMinPlayers++;
                if (mTempMinPlayers > cOnlineSettings::MaxOnlinePlayers) {
                    mTempMinPlayers = 2;
                }
            } else {
                mTempMinPlayers--;
                if (mTempMinPlayers < 2) {
                    mTempMinPlayers = cOnlineSettings::MaxOnlinePlayers;
                }
            }
            ScrollerSlot *pslot = m_optionList.GetSlot(eGameOption_MINPLAYERS);
            ScrollerDatum *datum = m_optionList.FindDatumInSlot(pslot);
            FEngSNPrintf(sztemp, 32, "%d", mTempMinPlayers);
            bStrCpy(datum->GetTopDatumModeString(), sztemp);
            break;
        }
        case eGameOption_LAPS: {
            if (direction) {
                FEDatabase->GetOnlineSettings()->GetRaceSettings()->NumLaps++;
                if (FEDatabase->GetOnlineSettings()->GetRaceSettings()->NumLaps > 8) {
                    FEDatabase->GetOnlineSettings()->GetRaceSettings()->NumLaps = 1;
                }
            } else {
                FEDatabase->GetOnlineSettings()->GetRaceSettings()->NumLaps--;
                if (FEDatabase->GetOnlineSettings()->GetRaceSettings()->NumLaps == 0) {
                    FEDatabase->GetOnlineSettings()->GetRaceSettings()->NumLaps = 8;
                }
            }
            ScrollerSlot *pslot = m_optionList.GetSlot(eGameOption_LAPS);
            ScrollerDatum *datum = m_optionList.FindDatumInSlot(pslot);
            FEngSNPrintf(sztemp, 32, "%d", FEDatabase->GetOnlineSettings()->GetRaceSettings()->NumLaps);
            bStrCpy(datum->GetTopDatumModeString(), sztemp);
            break;
        }
        case eGameOption_DIR: {
            direction = FEDatabase->GetOnlineSettings()->GetRaceSettings()->TrackDirection ? 0 : 1;
            FEDatabase->GetOnlineSettings()->GetRaceSettings()->TrackDirection = direction;
            ScrollerSlot *pslot = m_optionList.GetSlot(eGameOption_DIR);
            ScrollerDatum *datum = m_optionList.FindDatumInSlot(pslot);
            if (direction) {
                bStrCpy(datum->GetTopDatumModeString(), GetLocalizedString(0xa1cd823e));
            } else {
                bStrCpy(datum->GetTopDatumModeString(), GetLocalizedString(0xde6eff34));
            }
            break;
        }
        case eGameOption_NOS: {
            direction = FEDatabase->GetOnlineSettings()->UseNOS ? 0 : 1;
            FEDatabase->GetOnlineSettings()->UseNOS = direction;
            ScrollerSlot *pslot = m_optionList.GetSlot(eGameOption_NOS);
            ScrollerDatum *datum = m_optionList.FindDatumInSlot(pslot);
            if (direction) {
                bStrCpy(datum->GetTopDatumModeString(), GetLocalizedString(0x417b2604));
            } else {
                bStrCpy(datum->GetTopDatumModeString(), GetLocalizedString(0x70dfe5c2));
            }
            break;
        }
        case eGameOption_RANKED: {
            direction = FEDatabase->GetOnlineSettings()->RankedGame ? 0 : 1;
            FEDatabase->GetOnlineSettings()->RankedGame = direction;
            ScrollerSlot *pslot = m_optionList.GetSlot(eGameOption_RANKED);
            ScrollerDatum *datum = m_optionList.FindDatumInSlot(pslot);
            if (direction) {
                bStrCpy(datum->GetTopDatumModeString(), GetLocalizedString(0x70e01038));
            } else {
                bStrCpy(datum->GetTopDatumModeString(), GetLocalizedString(0x417b25e4));
            }
            break;
        }
        case eGameOption_PERF: {
            direction = FEDatabase->GetOnlineSettings()->PerformanceMatching ? 0 : 1;
            FEDatabase->GetOnlineSettings()->PerformanceMatching = direction;
            ScrollerSlot *pslot = m_optionList.GetSlot(eGameOption_PERF);
            ScrollerDatum *datum = m_optionList.FindDatumInSlot(pslot);
            if (direction) {
                bStrCpy(datum->GetTopDatumModeString(), GetLocalizedString(0x417b2604));
            } else {
                bStrCpy(datum->GetTopDatumModeString(), GetLocalizedString(0x70dfe5c2));
            }
            break;
        }
        case eGameOption_COLL: {
            direction = FEDatabase->GetOnlineSettings()->CollisionDetection ? 0 : 1;
            FEDatabase->GetOnlineSettings()->CollisionDetection = direction;
            ScrollerSlot *pslot = m_optionList.GetSlot(eGameOption_COLL);
            ScrollerDatum *datum = m_optionList.FindDatumInSlot(pslot);
            if (direction) {
                bStrCpy(datum->GetTopDatumModeString(), GetLocalizedString(0x417b2604));
            } else {
                bStrCpy(datum->GetTopDatumModeString(), GetLocalizedString(0x70dfe5c2));
            }
            break;
        }
    }
    m_optionList.Update(true);
}

void EditGameDetails::FixUpOptionsForModeSwitch() {
    switch (m_currentModePtr->m_type) {
        case GRace::kRaceType_Circuit:
            m_currentSelectableTrack = m_currentCircuitTrack;
            if (FEDatabase->GetOnlineSettings()->RankedGame) {
                FEDatabase->GetOnlineSettings()->GetRaceSettings()->NumLaps = 3;
            }
            break;
        case GRace::kRaceType_P2P:
            m_currentSelectableTrack = m_currentSprintTrack;
            break;
        case GRace::kRaceType_Drag:
            m_currentSelectableTrack = m_currentDragTrack;
            break;
    }
    FEDatabase->GetOnlineSettings()->RaceMode = m_currentModePtr->m_type;
    FEngSetTextureHash(GetPackageName(), 0xceb82625, m_currentModePtr->m_textureHash);
    bStrCpy(m_optionList.GetSelectedDatum()->GetTopDatumModeString(), GetLocalizedString(m_currentModePtr->m_textHash));
    UpdateTrackDisplay();
}

void EditGameDetails::UpdateTrackDisplay() {
    char sztemp[64] = "";
    uint32 hash;
    ScrollerDatum *datum;
    const char *distUnits;

    hash = CalcLanguageHash("TRACKNAME_", m_currentSelectableTrack->pRaceParams);
    if (DoesStringExist(hash)) {
        FEngSNPrintf(sztemp, 64, GetLocalizedString(hash));
    } else {
        FEngSNPrintf(sztemp, 64, m_currentSelectableTrack->pRaceParams->GetEventID());
    }
    datum = m_optionList.FindDatumInSlot(m_optionList.GetSlot(eGameOption_TRACK));
    bStrCpy(datum->GetTopDatumModeString(), sztemp);

    if (FEDatabase->GetGameplaySettings()->SpeedoUnits == 1) {
        distUnits = GetLocalizedString(0x8569a26a);
    } else {
        distUnits = GetLocalizedString(0x867dcfd9);
    }
    FEngSNPrintf(sztemp, 64, "%0.1f %s", m_currentSelectableTrack->pRaceParams->GetRaceLengthMeters() / 1000.0f, distUnits);
    datum = m_optionList.FindDatumInSlot(m_optionList.GetSlot(eGameOption_LENGTH));
    bStrCpy(datum->GetTopDatumModeString(), sztemp);

    FEDatabase->GetOnlineSettings()->GetRaceSettings()->EventHash = m_currentSelectableTrack->pRaceParams->GetEventHash();

    datum = m_optionList.FindDatumInSlot(m_optionList.GetSlot(eGameOption_LAPS));
    if (m_currentModePtr->m_type == GRace::kRaceType_Circuit && !FEDatabase->GetOnlineSettings()->RankedGame) {
        FEngSNPrintf(sztemp, 64, "%d", FEDatabase->GetOnlineSettings()->GetRaceSettings()->NumLaps);
        bStrCpy(datum->GetTopDatumModeString(), sztemp);
        m_optionList.Enable(datum);
    } else {
        bStrCpy(datum->GetTopDatumModeString(), GetLocalizedString(0x7f2f7ad6));
        m_optionList.Disable(datum);
    }
}
