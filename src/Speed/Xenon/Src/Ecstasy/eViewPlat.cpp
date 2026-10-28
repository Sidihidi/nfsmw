// Xbox 360: el modo de vista de Ecstasy (la parte de plataforma que en GameCube esta en EcstasyE.cpp y en PC en
// eViewPlat.cpp). NFS.exe lee el modo directamente del global (0x82A81E94) donde el juego llama a eGetCurrentViewMode
// (CarRenderConn::OnRender, CarRenderInfo::RenderFlaresOnCar), y eUpdateViewMode es 0x82474F90.
#include "Speed/Indep/Src/Ecstasy/Ecstasy.hpp"
#include "Speed/Indep/Src/Ecstasy/EcstasyE.hpp"
#include "Speed/Indep/Src/Camera/CameraMover.hpp"
#include "Speed/Indep/Src/World/Scenery.hpp"
#include "Speed/Indep/Src/Generated/Hash.hpp"

EVIEWMODE CurrentViewMode = EVIEWMODE_NONE;
EVIEWMODE RenderingViewMode = EVIEWMODE_NONE;
EVIEWMODE TweakerViewMode = EVIEWMODE_NONE;

ePoly::ePoly() {
    Vertices[0].x = 0.0f;
    Vertices[0].y = 0.0f;
    Vertices[0].z = 0.0f;
    Vertices[1].x = 64.0f;
    Vertices[1].y = 0.0f;
    Vertices[1].z = 0.0f;
    Vertices[2].x = 64.0f;
    Vertices[2].y = 64.0f;
    Vertices[2].z = 0.0f;
    Vertices[3].x = 0.0f;
    Vertices[3].y = 64.0f;
    Vertices[3].z = 0.0f;

    *reinterpret_cast<unsigned int *>(Colours[0]) = 0x80808080;
    *reinterpret_cast<unsigned int *>(Colours[1]) = 0x80808080;
    *reinterpret_cast<unsigned int *>(Colours[2]) = 0x80808080;
    *reinterpret_cast<unsigned int *>(Colours[3]) = 0x80808080;

    UVs[0][0] = 0.0f;
    UVs[0][1] = 0.0f;
    UVs[1][0] = 1.0f;
    UVs[1][1] = 0.0f;
    UVs[2][0] = 1.0f;
    UVs[2][1] = 1.0f;
    UVs[3][0] = 0.0f;
    UVs[3][1] = 1.0f;
    UVsMask[0][0] = 1.0f; // el EGM escribe 1.0f en 0x60

    SetFlags(0);
}

EVIEWMODE eGetCurrentViewMode() {
    return CurrentViewMode;
}

void eUpdateViewMode(void) {
    eView *eview_player1 = eGetView(EVIEW_PLAYER1, false);
    eView *eview_player2 = eGetView(EVIEW_PLAYER2, false);
    eView *eview_player1_rvm = eGetView(EVIEW_PLAYER1_RVM, false);

    if (eview_player1->GetCameraMover() != nullptr && eview_player2->GetCameraMover() != nullptr) {
        CurrentViewMode = EVIEWMODE_TWOH;
    } else if (eview_player1->GetCameraMover() != nullptr && eview_player1_rvm->GetCameraMover() != nullptr &&
               !eview_player1->GetCameraMover()->OutsidePOV()) {
        CurrentViewMode = EVIEWMODE_ONE_RVM;
    } else if (eview_player1->GetCameraMover() != nullptr) {
        CurrentViewMode = EVIEWMODE_ONE;
    } else {
        CurrentViewMode = EVIEWMODE_NONE;
    }
}

// Xbox 360: la version de plataforma de NotifySkyLoader (NFS.exe 0x8247B868, en la capa de render de Xenon
// 0x824799D8-0x82490860 que va como segundo .text de zEcstasy, como zPlatform en PC). La llama
// TrackStreamer::FinishedLoading, igual que la de SkyRender.cpp en GC/PS2/PC; el cielo de 360 es la instancia de
// escenario SKYDOME_XENON y aqui solo se sueltan sus modelos.
void NotifySkyLoader() {
    if (FindSceneryInstance(STRINGHASH_SKYDOME_XENON)) {
        SceneryInfo *info = FindSceneryInfo(STRINGHASH_SKYDOME_XENON);
        for (int i = 0; i < 4; i++) {
            if (info->pModel[i]) {
                info->pModel[i]->UnInit();
            }
        }
    }
}
