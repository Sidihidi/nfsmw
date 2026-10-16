#include "Speed/Indep/Src/Frontend/MemoryCard/MemoryCardImp.hpp"
#include "Speed/Indep/Src/Frontend/Database/FEDatabase.hpp"
#include "Speed/Indep/Src/Misc/BuildRegion.hpp"
#include "Speed/Indep/Src/Misc/bFile.hpp"
#include "Speed/Indep/bWare/Inc/Strings.hpp"
#include "Speed/Indep/bWare/Inc/bPrintf.hpp"
#include "Speed/Indep/bWare/Inc/bWare.hpp"

#include <libmc.h>

char *sFullName = "memcard/slime1.ico";
char *sIconName = "slime1.ico";

// El identificador de las partidas de Need for Speed Underground 2, para encontrar las viejas.
char *GetOldPrefix() {
    if (BuildRegion::IsEurope()) {
        return "BESLES-52725";
    } else if (BuildRegion::IsJapan()) {
        return "BISLPM-65766";
    } else if (BuildRegion::IsKorea()) {
        return "BKSLKA-25241";
    } else if (BuildRegion::IsAsia()) {
        return "BISLAJ-25054";
    }

    return "BASLUS-21065";
}

const char *MemoryCardImp::GetPrefix() {
    return "";
}

#ifdef EA_BUILD_A124
// La del prototipo: el identificador de cada region, escrito entero.
const char *MemoryCardImp::GetTitleId() {
    if (BuildRegion::IsEurope()) {
        return "BESLES-53557";
    } else if (BuildRegion::IsJapan()) {
        return "BISLPM-90000";
    } else if (BuildRegion::IsKorea()) {
        return "BKSLKA-90000";
    } else if (BuildRegion::IsAsia()) {
        return "BISLAJ-90000";
    }

    return "BASLUS-21267";
}
#else
// La de la venta: el prefijo de la region delante del codigo del disco ("BASLUS_21351"), con un guion en lugar del
// '_'. Devuelve la cadena local, como el original; MemoryCard::Init la copia en seguida.
const char *MemoryCardImp::GetTitleId() {
    char titleId[16];
    char *slusCode = BuildRegion::GetSlusCode();

    if (BuildRegion::IsEurope()) {
        bStrCat(titleId, "BE", slusCode);
    } else if (BuildRegion::IsKorea()) {
        bStrCat(titleId, "BK", slusCode);
    } else if (BuildRegion::IsAsia()) {
        bStrCat(titleId, "BI", slusCode);
    } else {
        bStrCat(titleId, "BA", slusCode);
    }
    titleId[6] = '-';
    titleId[12] = '\0';
    return titleId;
}
#endif

// En PS2 la partida lleva un icon.sys: la cabecera "PS2D", los colores de fondo, tres luces y el titulo, que va en
// Shift-JIS con el salto de linea en OffsLF. El icono es el mismo fichero para ver, copiar y borrar.
RealmcIface::SaveInfo *MemoryCardImp::ConstructSaveInfo(MemoryCard::SaveType type, const char *entryName, int aSize) {
    RealmcIface::SaveInfo *pInfo;
    sceMcIconSys *pIcon;
    char szTempName[64];
    int firstline;
    int iIconSize;

    pInfo = new ("MC_SaveInfo", 0) RealmcIface::SaveInfo;
    pIcon = new ("sceMCIcon", 0) sceMcIconSys;

    bMemSet(pIcon, 0, sizeof(sceMcIconSys));
    bStrCpy(reinterpret_cast<char *>(pIcon->Head), "PS2D");

    pInfo->mPs2Info.mIconSysData = reinterpret_cast<const char *>(pIcon);
    pInfo->mPs2Info.mIconSysDataSize = sizeof(sceMcIconSys);

    pIcon->BgColor[0][0] = 0x80; pIcon->BgColor[0][1] = 0; pIcon->BgColor[0][2] = 0; pIcon->BgColor[0][3] = 0;
    pIcon->BgColor[1][0] = 0; pIcon->BgColor[1][1] = 0x80; pIcon->BgColor[1][2] = 0; pIcon->BgColor[1][3] = 0;
    pIcon->BgColor[2][0] = 0; pIcon->BgColor[2][1] = 0; pIcon->BgColor[2][2] = 0x80; pIcon->BgColor[2][3] = 0;
    pIcon->BgColor[3][0] = 0x80; pIcon->BgColor[3][1] = 0x80; pIcon->BgColor[3][2] = 0x80; pIcon->BgColor[3][3] = 0;
    pIcon->LightDir[0][0] = 0.5f; pIcon->LightDir[0][1] = 0.5f; pIcon->LightDir[0][2] = 0.5f; pIcon->LightDir[0][3] = 0.0f;
    pIcon->LightDir[1][0] = 0.0f; pIcon->LightDir[1][1] = -0.4f; pIcon->LightDir[1][2] = -0.1f; pIcon->LightDir[1][3] = 0.0f;
    pIcon->LightDir[2][0] = -0.5f; pIcon->LightDir[2][1] = -0.5f; pIcon->LightDir[2][2] = 0.5f; pIcon->LightDir[2][3] = 0.0f;
    pIcon->LightColor[0][0] = 0.48f; pIcon->LightColor[0][1] = 0.48f; pIcon->LightColor[0][2] = 0.03f; pIcon->LightColor[0][3] = 0.0f;
    pIcon->LightColor[1][0] = 0.5f; pIcon->LightColor[1][1] = 0.33f; pIcon->LightColor[1][2] = 0.2f; pIcon->LightColor[1][3] = 0.0f;
    pIcon->LightColor[2][0] = 0.14f; pIcon->LightColor[2][1] = 0.14f; pIcon->LightColor[2][2] = 0.38f; pIcon->LightColor[2][3] = 0.0f;
    pIcon->Ambient[0] = 0.5f; pIcon->Ambient[1] = 0.5f; pIcon->Ambient[2] = 0.5f; pIcon->Ambient[3] = 0.0f;
    pIcon->Head[0] = 'P'; pIcon->Head[1] = 'S'; pIcon->Head[2] = '2'; pIcon->Head[3] = 'D';

    pIcon->TransRate = 0x60;

    if (type == MemoryCard::ST_PROFILE) {
        bSPrintf(szTempName, "NFS MW%s", entryName);
        firstline = 6;
    } else {
        bSPrintf(szTempName, "NFSMW: %s", FEDatabase->GetUserProfile(0)->GetProfileName());
        firstline = bStrLen(szTempName);
    }

    MemoryCard::GetInstance()->m_pIMemcard->ConvertAsciiToSjisString(szTempName, reinterpret_cast<wchar_t *>(pIcon->TitleName));
    pIcon->OffsLF = firstline * 2;

    pInfo->mPs2Info.mStaticIconData = (const char *)bGetFile(sFullName, &iIconSize, 0);
    pInfo->mPs2Info.mStaticIconFilename = sIconName;
    pInfo->mPs2Info.mStaticIconDataSize = iIconSize;
    bStrCpy(reinterpret_cast<char *>(pIcon->FnameView), sIconName);

    pInfo->mPs2Info.mCopyIconFilename = 0;
    pInfo->mPs2Info.mCopyIconData = 0;
    pInfo->mPs2Info.mCopyIconDataSize = 0;
    bStrCpy(reinterpret_cast<char *>(pIcon->FnameCopy), sIconName);

    pInfo->mPs2Info.mDeleteIconFilename = 0;
    pInfo->mPs2Info.mDeleteIconData = 0;
    pInfo->mPs2Info.mDeleteIconDataSize = 0;
    bStrCpy(reinterpret_cast<char *>(pIcon->FnameDel), sIconName);

    m_SaveReq.mSaveInfo = pInfo;

    pInfo->mTypeName = reinterpret_cast<const wchar_t *>(gEntryType[type]);
    pInfo->mContentName = reinterpret_cast<const wchar_t *>(gContentName);
    pInfo->mHeaderSize = 8;
    pInfo->mBodySize = aSize;

    return pInfo;
}

// La venta comprueba los punteros antes de liberarlos; el prototipo no.
void MemoryCardImp::DestructSaveInfo() {
    RealmcIface::SaveInfo *pInfo = m_SaveReq.mSaveInfo;

#ifdef EA_BUILD_A124
    bFree(const_cast<char *>(pInfo->mPs2Info.mStaticIconData)); pInfo->mPs2Info.mStaticIconData = 0;
    pInfo->mPs2Info.mCopyIconData = 0;
    pInfo->mPs2Info.mDeleteIconData = 0;
    delete pInfo->mPs2Info.mIconSysData; pInfo->mPs2Info.mIconSysData = 0;
    delete pInfo;
    m_SaveReq.mSaveInfo = 0;
#else
    if (pInfo != 0) {
        if (pInfo->mPs2Info.mStaticIconData != 0) {
            bFree(const_cast<char *>(pInfo->mPs2Info.mStaticIconData));
        }
        pInfo->mPs2Info.mStaticIconData = 0;
        pInfo->mPs2Info.mCopyIconData = 0;
        pInfo->mPs2Info.mDeleteIconData = 0;
        if (pInfo->mPs2Info.mIconSysData != 0) {
            delete pInfo->mPs2Info.mIconSysData;
        }
        pInfo->mPs2Info.mIconSysData = 0;
        delete pInfo;
        m_SaveReq.mSaveInfo = 0;
    }
#endif
}

void MemoryCardImp::BootupCheckDone(RealmcIface::CardStatus status, RealmcIface::BootupCheckResults *pParam) {}
