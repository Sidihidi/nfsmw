#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/Online/uiOnlineViewTrack.hpp"

#include "Speed/Indep/Src/Frontend/Database/FEDatabase.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterface.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterfaceFEObjects.hpp"
#include "Speed/Indep/Src/Gameplay/GRaceDatabase.h"

MenuScreen *CreateOnlineViewTrack(ScreenConstructorData *sd) {
    return new ("uiOnlineViewTrack", 0) uiOnlineViewTrack(sd);
}

uiOnlineViewTrack::uiOnlineViewTrack(ScreenConstructorData *sd) : UIQRTrackSelect(sd) {
    Setup();
}

void uiOnlineViewTrack::Setup() {
    FEngSetLanguageHash(GetPackageName(), 0x42adb44c, 0x3bc6066d);
    TrackMap = static_cast<FEMultiImage *>(FEngFindObject(GetPackageName(), FEngHashString("TRACK_MAP")));
    SetCurrentTrack();
    RefreshHeader();
}

void uiOnlineViewTrack::SetCurrentTrack() {
    Tracks.DeleteAllElements();
    uint32 EventHash = FEDatabase->GetOnlineSettings()->GetRaceSettings()->EventHash;
    GRaceParameters *parms = GRaceDatabase::Get().GetRaceFromHash(EventHash);
#ifdef EA_BUILD_A124
    Tracks.AddTail(new ("SelectableTrack", 0) SelectableTrack(parms, true));
#else
    Tracks.AddTail(new ("SelectableTrack", 0) SelectableTrack(parms, true, 0)); // EGM: bin 0 (0x8228AA10)
#endif
    pCurrentNode = Tracks.GetHead();
    pCurrentTrack = pCurrentNode->pRaceParams;
    SetSelectedTrack(pCurrentTrack);
}

void uiOnlineViewTrack::NotificationMessage(u32 msg, FEObject *pobj, u32 param1, u32 param2) {
    switch (msg) {
        case 0xe1fde1d1:
            RefreshHeader();
            break;
        case 0xc519bfc3:
        case 0xc519bfc4:
            break;
        case 0x911ab364:
            cFEng::Get()->QueuePackagePop(1);
            cFEng::Get()->QueueGameMessage(0xcac72e3d, g_GameRoomName, 0xff);
            break;
        case 0x406415e3:
        case 0x5073ef13:
            break;
    }
}
