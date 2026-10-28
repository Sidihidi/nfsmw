#include <string.h>

#include "../../../include/common/realmemcard/memcard_interface.h"
#include "../../../include/common/realmemcard/impl/memcard_interface_impl.h"

namespace RealmcIface {

static MemcardInterface *gInstance = nullptr;

MemcardInterface *MemcardInterface::CreateInstance(Realmc::SystemInterface *iSystem, IGameInterface *iGame, GameInfo *gameInfo) {
    Realmc::SetMemAllocator(iSystem->mAllocator);
    if (gInstance == nullptr) {
        gInstance = new MemcardInterface(iSystem, iGame, gameInfo);
    }
    return gInstance;
}

#ifdef EA_PLATFORM_XENON
// Xbox 360 (r63): emitida tras CreateInstance (0x825E8A68, 12 B: devuelve la direccion del
// literal "*" de 0x82062FF8 directamente, sin cargar el puntero ALL_ENTRIES).
const char *MemcardInterface::GetFilterForAllEntries() {
    return "*";
}
#endif

MemcardInterface::MemcardInterface(Realmc::SystemInterface *iSystem, IGameInterface *iGame, GameInfo *gameInfo) {
    this->mImpl = new MemcardInterfaceImpl(iSystem, iGame, gameInfo);
}

#if defined(EA_PLATFORM_WIN32) || defined(EA_PLATFORM_XENON)
MemcardInterface::~MemcardInterface() {
    delete this->mImpl;
}
#endif

void MemcardInterface::BootupCheck(const BootupCheckParams *params, unsigned int nEntries, const char **entryNames, wchar_t *content) {
    this->mImpl->TaskManagerBootupCheck(params, nEntries, entryNames, content);
}

void MemcardInterface::Save(const char *entryName, const char *header, const char *body, const SaveInfo *saveInfo, const TitleInfo *titleInfo) {
    if (titleInfo != nullptr) {
        this->mImpl->TaskManagerSave(entryName, header, body, saveInfo, titleInfo);
    } else {
        this->mImpl->TaskManagerSave(entryName, header, body, saveInfo, nullptr);
    }
}

#if REALMEMCARD_VERSION < 30401
// La 3.04.00 no pasa el nombre del tipo, y una entrada con nombre de fichero (NAME_FILE) tambien va por la carga alternativa.
void MemcardInterface::Load(const char *entryName, char *header, char *body, const wchar_t *contentName, const TitleInfo *titleInfo) {
    if (titleInfo != nullptr && (titleInfo->mNameType == NAME_FILE || titleInfo->mDataFormat == FORMAT_RAW || titleInfo->mTitleType == TITLE_ALTERNATE)) {
        this->mImpl->TaskManagerLoad(entryName, header, body, contentName, titleInfo);
    } else {
        this->mImpl->TaskManagerLoad(entryName, header, body, contentName, nullptr);
    }
}
#elif !REALMEMCARD_HAS_TYPENAME
void MemcardInterface::Load(const char *entryName, char *header, char *body, const wchar_t *contentName, const TitleInfo *titleInfo) {
    if (titleInfo != nullptr && (titleInfo->mDataFormat == FORMAT_RAW || titleInfo->mTitleType == TITLE_ALTERNATE)) {
        this->mImpl->TaskManagerLoad(entryName, header, body, contentName, titleInfo);
    } else {
        this->mImpl->TaskManagerLoad(entryName, header, body, contentName, nullptr);
    }
}
#else
void MemcardInterface::Load(const char *entryName, char *header, char *body, const wchar_t *contentName, const TitleInfo *titleInfo, const wchar_t *typeName) {
    if (titleInfo != nullptr && (titleInfo->mTitleType == TITLE_ALTERNATE || titleInfo->mDataFormat == FORMAT_RAW)) {
        this->mImpl->TaskManagerLoad(entryName, header, body, contentName, typeName, titleInfo);
    } else {
        this->mImpl->TaskManagerLoad(entryName, header, body, contentName, typeName, nullptr);
    }
}
#endif

void MemcardInterface::Delete(const char *entryName, const wchar_t *contentName) {
    this->mImpl->TaskManagerDelete(entryName, contentName);
}

#if defined(EA_PLATFORM_WIN32) || defined(EA_PLATFORM_XENON)
void MemcardInterface::DeleteMultiple(unsigned int nEntryNames, const char **entryNames, const wchar_t *contentName) {
    this->mImpl->TaskManagerDeleteMultiple(nEntryNames, entryNames, contentName);
}
#endif

void MemcardInterface::FindEntries(const char *entryNamePattern, const TitleInfo *titleInfo) {
#if REALMEMCARD_VERSION < 30401
    if (titleInfo != nullptr && (titleInfo->mNameType == NAME_FILE || titleInfo->mDataFormat == FORMAT_RAW || titleInfo->mTitleType == TITLE_ALTERNATE)) {
#else
    if (titleInfo != nullptr && (titleInfo->mDataFormat == FORMAT_RAW || titleInfo->mTitleType == TITLE_ALTERNATE)) {
#endif
        this->mImpl->TaskManagerFindEntries(entryNamePattern, titleInfo);
    } else {
        this->mImpl->TaskManagerFindEntries(entryNamePattern, nullptr);
    }
}

void MemcardInterface::MessageDone(MessageChoices choice) {
    this->mImpl->MessageDone(choice);
}

void MemcardInterface::CheckCard(CardId cardId) {
    this->mImpl->TaskManagerCheckCard(cardId);
}

#if defined(EA_PLATFORM_WIN32) || defined(EA_PLATFORM_XENON)
void MemcardInterface::SetActiveCard(CardId cardId) {
    this->mImpl->SetActiveCard(cardId);
}
#endif

void MemcardInterface::SetAutosave(AutosaveState state, unsigned int nSaveReqs, SaveReq **saveReqs, const char *entryName, CardId cardId) {
    this->mImpl->TaskManagerSetAutosave(state, nSaveReqs, saveReqs, entryName, cardId);
}

void MemcardInterface::SetMonitor(MonitorState state) {
    this->mImpl->TaskManagerSetMonitor(state);
}

void MemcardInterface::SetMessage(MessageState state, unsigned int message) {
    this->mImpl->SetMessage(state, message);
}

#ifdef EA_PLATFORM_XENON
// Xbox 360 (r63): emitidas en esta TU (0x825E8D88, 28 B, y 0x825E8DA8, 72 B, entre SetMessage y
// Update, como en el orden de declaracion de la cabecera). En GC la TU original no las emite y en
// PC la TU cerro 40/40 sin ellas: solo la revision de Xenon las define aqui.
const wchar_t *MemcardInterface::GetCardName() {
    return this->mImpl->GetCardName();
}

const wchar_t *MemcardInterface::GetCardName(CardId cardId) {
    return this->mImpl->GetCardName(cardId);
}
#endif

MemcardTask MemcardInterface::Update(unsigned int elapsedTime) {
    return this->mImpl->Update(elapsedTime);
}

#ifdef EA_PLATFORM_PLAYSTATION2
void MemcardInterface::ConvertAsciiToSjisString(const char *asciiStr, wchar_t *sjisStr) {
    this->mImpl->ConvertAsciiToSjisString(asciiStr, sjisStr);
}
#endif

bool MemcardInterface::IsResettable() {
    return this->mImpl->IsResettable();
}

#if defined(EA_PLATFORM_WIN32) || defined(EA_PLATFORM_XENON)
unsigned int MemcardInterface::CalcSaveSize(const SaveInfo *saveInfo, DataFormat dataFormat) {
    return this->mImpl->CalcSaveSize(saveInfo, dataFormat);
}

#ifdef EA_PLATFORM_XENON
// Xbox 360 (r63): emitida en esta TU (0x825E8E28, 12 B).
Realmc::Interface *MemcardInterface::GetLowlevelInterface() {
    return this->mImpl->GetLowlevelInterface();
}
#endif

void MemcardInterface::SetRootPath(const char *path) {
    this->mImpl->SetRootPath(path);
}
#endif

void BootupCheckParams::Clear() {
    this->mEntryNamePattern = 0;
    this->mNumSaveTypes = 0;
    this->mSaveReqs = 0;
    this->mValidCardIds = -1;
}

void BootupCheckResults::Clear() {
    this->mFirstGoodCard = CARD_UNKNOWN;
    this->mEntryFound = false;
    this->mNumBlocksNeeded = 0;
}

CardInfo::CardInfo() {
    this->Clear();
}

void CardInfo::Clear() {
    this->mCardId = CARD_UNKNOWN;
    this->mStatus = STATUS_UNKNOWN;
    this->mFreeSpace = 0;
    this->mFreeFiles = 0;
    this->mTotalSpace = 0;
    this->mFreeSpaceOverLimit = false;
    this->mTotalSpaceOverLimit = false;
}

EntryInfo::EntryInfo() {
    this->Clear();
}

void EntryInfo::Clear() {
    this->mTime.Clear();
    this->mName = 0;
    this->mStatus = STATUS_ENTRY_CORRUPTED;
    this->mEntryBlocks = 0;
    this->mUserDataSize = 0;
    this->mCompanyCode[1] = 0;
    this->mCompanyCode[0] = 0;
    this->mGameCode[3] = 0;
    this->mGameCode[2] = 0;
    this->mGameCode[1] = 0;
    this->mGameCode[0] = 0;
#ifdef EA_PLATFORM_XENON
    this->x360_unknown_0x24 = -1;
    this->x360_unknown_0x28 = false;
#endif
}

GameInfo::GameInfo(const wchar_t *gameTitle, const TitleId titleId, bool multipleSaveTypesUsed, bool multitapSupported) {
    memset(this->mGameTitle, 0, 0x42);
    memcpy(this->mGameTitle, gameTitle, Realmc::Locale::GetWstrLength(gameTitle) * 2);
#ifdef EA_PLATFORM_PLAYSTATION2
    memset(this->mTitleId, 0, sizeof(this->mTitleId));
    strncpy(this->mTitleId, titleId, sizeof(this->mTitleId) - 1);
#else
    this->mTitleId = titleId;
#endif
    this->mMultipleSaveTypesUsed = multipleSaveTypesUsed;
    this->mMultitapSupported = multitapSupported;
}

#ifdef EA_PLATFORM_XENON
// En el XEX de Xbox 360 queda emitida (0x825E9020); en las demas el enlazador la descarta sin llamantes.
void GameInfo::Clear() {
    memset(this->mGameTitle, 0, 0x42);
    this->mTitleId = 0;
    this->mMultipleSaveTypesUsed = false;
    this->mMultitapSupported = false;
}
#endif

GcSaveInfo::GcSaveInfo() {
    this->Clear();
}

void GcSaveInfo::Clear() {
    this->mComment1 = 0;
    this->mComment1Size = 0;
    this->mComment2 = 0;
    this->mComment2Size = 0;
    this->mIconDataInfo = 0;
    this->mBannerDataInfo = 0;
}

Ps2SaveInfo::Ps2SaveInfo() {
    this->Clear();
}

void Ps2SaveInfo::Clear() {
    this->mIconSysData = 0;
    this->mIconSysDataSize = 0;
    this->mStaticIconData = 0;
    this->mStaticIconDataSize = 0;
    this->mStaticIconFilename = 0;
    this->mCopyIconData = 0;
    this->mCopyIconDataSize = 0;
    this->mCopyIconFilename = 0;
    this->mDeleteIconData = 0;
    this->mDeleteIconDataSize = 0;
    this->mDeleteIconFilename = 0;
}

SaveInfo::SaveInfo() {
    this->Clear();
}

void SaveInfo::Clear() {
    this->mPs2Info.Clear();
    this->mXboxInfo.Clear();
    this->mGcInfo.Clear();
    this->mHeaderSize = 0;
    this->mBodySize = 0;
    this->mTypeName = 0;
    this->mContentName = 0;
#ifdef EA_PLATFORM_XENON
    this->x360_unknown_0x68 = 0;
#endif
}

SaveReq::SaveReq() {
    this->Clear();
}

void SaveReq::Clear() {
    this->mNumSaves = 0;
    this->mSaveInfo = 0;
#ifdef EA_PLATFORM_XENON
    this->mAlwaysCheckAtBoot = false;
#endif
}

TimeInfo::TimeInfo() {
    this->Clear();
}

void TimeInfo::Clear() {
    this->mCreated = 0;
    this->mLastModified = 0;
    this->mLastAccessed = 0;
}

void TitleInfo::Clear() {
    this->mTitleType = static_cast<TitleType>(0);
#ifdef EA_PLATFORM_PLAYSTATION2
    memset(this->mTitleId, 0, sizeof(this->mTitleId));
#else
    this->mTitleId = 0;
#endif
    this->mNameType = static_cast<NameType>(0);
    this->mDataFormat = static_cast<DataFormat>(0);
#ifdef EA_PLATFORM_XENON
    this->x360_unknown_0x10 = false;
#endif
}

void TitleInfo::Init(TitleType titleType, const TitleId titleId, NameType nameType, DataFormat dataFormat) {
    this->mTitleType = titleType;
#ifdef EA_PLATFORM_PLAYSTATION2
    strncpy(this->mTitleId, titleId, sizeof(this->mTitleId) - 1);
#else
    this->mTitleId = titleId;
#endif
    this->mNameType = nameType;
    this->mDataFormat = dataFormat;
}

XboxSaveInfo::XboxSaveInfo() {
    this->Clear();
}

void XboxSaveInfo::Clear() {
    this->mImageData = 0;
    this->mImageDataSize = 0;
}

} // namespace RealmcIface
