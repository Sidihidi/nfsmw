// MemoryCardImp de PC (speed.exe 0x6CBE20-0x6CC008, detras de Platform_PC.cpp en zPlatform). En PC la "tarjeta" es
// la carpeta "Mis documentos\NFS Most Wanted": la partida no lleva icono ni comentarios (el SaveInfo solo tiene el
// tipo, el contenido y los tamanos) y BootupCheckDone, en vez de avisar a la pantalla de la tarjeta como GameCube,
// crea la carpeta, se la pasa a la biblioteca (SetRootPath) y mira si ya hay partidas dentro.
#include "Speed/Indep/Src/Frontend/MemoryCard/MemoryCardImp.hpp"
#include "Speed/Indep/Src/Frontend/MemoryCard/MemoryCard.hpp"
#include "Speed/Indep/bWare/Inc/Strings.hpp"
#include "Speed/Indep/bWare/Inc/bWare.hpp"
#include "Speed/PC/Src/Win32Min.hpp"

// El nombre de la carpeta de las partidas (.data 0x901748 del exe, solo lo lee BootupCheckDone): el mismo nombre y la
// misma cadena que el comentario 1 de la partida de GameCube.
const char *gComment1 = "NFS Most Wanted";

const char *MemoryCardImp::GetPrefix() {
    return "";
}

const char *MemoryCardImp::GetTitleId() {
    return "";
}

RealmcIface::SaveInfo *MemoryCardImp::ConstructSaveInfo(MemoryCard::SaveType type, const char *DisplayName, int aSize) {
    m_SaveReq.mSaveInfo = new ("MC_SaveInfo", 0) RealmcIface::SaveInfo;
    m_SaveReq.mSaveInfo->Clear();
    m_SaveReq.mSaveInfo->mTypeName = reinterpret_cast<const wchar_t *>(gEntryType[type]);
    m_SaveReq.mSaveInfo->mContentName = reinterpret_cast<const wchar_t *>(gContentName);
    m_SaveReq.mSaveInfo->mHeaderSize = 8;
    m_SaveReq.mSaveInfo->mBodySize = aSize;
    return m_SaveReq.mSaveInfo;
}

void MemoryCardImp::DestructSaveInfo() {
    delete m_SaveReq.mSaveInfo;
    m_SaveReq.mSaveInfo = 0;
}

void MemoryCardImp::BootupCheckDone(RealmcIface::CardStatus status, RealmcIface::BootupCheckResults *pParam) {
    char path[256];
    WIN32_FIND_DATAA findData;

    SHGetFolderPathA(0, CSIDL_PERSONAL | CSIDL_FLAG_CREATE, 0, 0, path);
    CreateDirectoryA(path, 0);
    bStrCat(path, path, "\\");
    bStrCat(path, path, gComment1);
    CreateDirectoryA(path, 0);
    MemoryCard::GetInstance()->m_pIMemcard->SetRootPath(path);
    bStrCat(path, path, "\\*");

    HANDLE hFind = FindFirstFileA(path, &findData);
    if (hFind != INVALID_HANDLE_VALUE) {
        unsigned int numFiles = 1;
        while (FindNextFileA(hFind, &findData)) {
            numFiles++;
        }
        // "." y ".." siempre estan.
        if (numFiles > 2) {
            pParam->mEntryFound = true;
        }
        FindClose(hFind);
    } else {
        pParam->mEntryFound = false;
    }
}
