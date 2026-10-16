// La parte de PS2 de RealmcIface::MemcardInterfaceImpl (source/lib/ps2/ps2_memcard_interface_impl.cpp en el .mdebug
// de la A124, realmemcard 3.04.00). La logica es la del gemelo de GameCube (gc/gc_memcard_interface_impl.cpp, 3.04.01);
// cambian la tarjeta (puerto y ranura), los directorios de libmc ("<TitleId>-<entrada>"), los mensajes de las
// directrices de Sony y las funciones que la 3.04.01 ya no tiene (SaveCheckAlternate, SaveAlternate, SetBootFlow).

#include <stdio.h>
#include <string.h>

#include <libmc.h>

#include "../../../include/common/realmemcard/memcard_utilities.h"
#include "ps2_interface.h"

namespace RealmcIface {

const char *ALL_ENTRIES = "*";

// Los segundos transcurridos desde el 1 de enero de 2000 de una fecha de la tarjeta.
static unsigned int _ChangeFileTime(Realmc::DateTime timestamp)
{
    // la hora del dia
    unsigned int elapsedTime = timestamp.second + (timestamp.minute + (timestamp.hour + timestamp.day * 24) * 60) * 60;

    // los dias de los meses ya transcurridos
    unsigned short daysToMonth[12] = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };
    elapsedTime += daysToMonth[timestamp.month - 1] * 86400;

    // los anos completos
    unsigned int nElapsedYears = timestamp.year - 2000;
    elapsedTime += nElapsedYears * 31536000;

    // un dia mas por cada bisiesto; el de este ano solo cuenta pasado febrero
    unsigned int nLeapDays;
    if ((nElapsedYears % 4) == 0 && timestamp.month < 3)
    {
        nLeapDays = nElapsedYears / 4;
    }
    else
    {
        nLeapDays = nElapsedYears / 4 + 1;
    }
    elapsedTime += nLeapDays * 86400;

    return elapsedTime;
}

Realmc::CardID _ChangeToRealmcCardId(CardId cardId)
{
    Realmc::CardID memcard;

    switch (cardId)
    {
    case PORT1_SLOT1:
        memcard.port = 0;
        memcard.slot = 0;
        break;

    case PORT1_SLOT2:
        memcard.port = 0;
        memcard.slot = 1;
        break;

    case PORT1_SLOT3:
        memcard.port = 0;
        memcard.slot = 2;
        break;

    case PORT1_SLOT4:
        memcard.port = 0;
        memcard.slot = 3;
        break;

    case PORT2_SLOT1:
        memcard.port = 1;
        memcard.slot = 0;
        break;

    case PORT2_SLOT2:
        memcard.port = 1;
        memcard.slot = 1;
        break;

    case PORT2_SLOT3:
        memcard.port = 1;
        memcard.slot = 2;
        break;

    case PORT2_SLOT4:
        memcard.port = 1;
        memcard.slot = 3;
        break;

    default:
        break;
    }

    return memcard;
}

CardId _ChangeToRealmcIfaceCardId(Realmc::CardID cardId)
{
    return static_cast<CardId>(1 << (cardId.port * 4) << cardId.slot);
}

void _SplitPath(const char *path, char *dirname, char *filename)
{
    char tempPath[63];
    char *separator;

    {
        unsigned int pos;

        strcpy(tempPath, path);
        separator = nullptr;
        pos = strlen(path);
        if (pos != 0)
        {
            do
            {
                if (tempPath[pos] == '\\' || tempPath[pos] == '/')
                {
                    separator = &tempPath[pos];
                    *separator = 0;
                }
                pos--;
            } while (separator == nullptr && pos != 0);
        }
    }

    strncpy(dirname, tempPath, 0x1f);
    strncpy(filename, separator + 1, 0x1f);
}

MemcardInterfaceImpl::MemcardInterfaceImpl(Realmc::SystemInterface *iSystem, IGameInterface *iGame, GameInfo *gameInfo)
    : mISystem(*iSystem)
    , mIGame(iGame)
    , mGameInfo(*gameInfo)
    , mActiveCard()
    , mAutosaveCard()
    , mAutosaveEnabled(false)
    , mActiveTask(TASK_NONE)
    , mActiveSubtask(SUBTASK_NONE)
    , mUserChoice(CHOICE_NONE)
    , mMessageShowing(false)
    , mHiddenMessages(0)
    , mForceMessages(0)
    , mIsResettable(true)
    , mMonitorState(MONITOR_OFF)
    , mCardInfo()
    , mStartGameInfo()
    , mFileSize(0)
    , mEntryInfo()
    , mBlockSize(0)
    , mFileInfo()
    , mFilehandle(nullptr)
    , mFileHeader()
    , mUserHeader(nullptr)
    , mUserBody(nullptr)
    , mEntryFound(false)
    , mDataFormat(FORMAT_LAYER2)
    , mTaskStatus(TASK_CONTINUE)
    , mBlocksNeeded(0)
    , mEntryList(nullptr)
    , mNumEntries(0)
    , mCurEntry(0)
    , mDirInfo()
    , mEntryTable(nullptr)
    , mNumFiles(0)
    , mFileFound(false)
    , mNameType(NAME_ENTRY)
{
    this->mTaskManager = new TaskManager(this, iGame);

    memset(this->mEntryName, 0, 0x20);
    memset(&this->mFileInfo, 0, 0xc);
    memset(&this->mDirInfo, 0, 0x30);
    memset(&this->mStartGameInfo, 0, 0x20);

    if (this->mGameInfo.mMultitapSupported)
    {
        Realmc::Interface::InitMultitap(true, true);
    }

    this->mIMemcard = Realmc::Interface::CreateInstance(Realmc::SystemInterface(iSystem));

    static_cast<Realmc::PS2Interface *>(this->mIMemcard)->SetGameTitle(this->mGameInfo.mGameTitle);
    this->SetActiveCard(PORT1_DEFAULT);
}

inline void FileHeader::Clear()
{
    this->mFileHeaderVersion = 0;
    this->mUserHeaderSize = 0;
    this->mUserBodySize = 0;
    this->mFileSize = 0;
    memset(&this->mUserHeaderSignature, 0, sizeof(this->mUserHeaderSignature));
    memset(&this->mUserBodySignature, 0, sizeof(this->mUserBodySignature));
    memset(&this->mFileHeaderSignature, 0, sizeof(this->mFileHeaderSignature));
}

void MemcardInterfaceImpl::BootupCheck(const BootupCheckParams *params)
{
    unsigned int nBlocksNeeded = 0;

    {
        unsigned int iSaveType;

        for (iSaveType = 0; iSaveType < params->mNumSaveTypes; iSaveType++)
        {
            SaveInfo *saveInfo = params->mSaveReqs[iSaveType]->mSaveInfo;

            if (saveInfo->mHeaderSize + saveInfo->mBodySize != 0)
            {
                nBlocksNeeded += this->CalcSaveSize(saveInfo, FORMAT_LAYER2) * params->mSaveReqs[iSaveType]->mNumSaves;
            }
        }
    }

    sprintf(this->mEntryName, "%s-%s", this->mGameInfo.mTitleId, params->mEntryNamePattern);

    memset(&this->mDirInfo, 0, 0x30);
    this->mDirInfo.directoryName = this->mEntryName;

    memset(&this->mStartGameInfo, 0, 0x20);
    this->mStartGameInfo.totalBlocksNeeded = nBlocksNeeded;
    this->mStartGameInfo.gameTitle = this->mGameInfo.mGameTitle;
    this->mStartGameInfo.autoSave = false;
    this->mStartGameInfo.checkAllSlots = true;
    this->mStartGameInfo.checkCardID.port = params->mValidCardIds;
    this->mStartGameInfo.fileInfo.directoryInfo = &this->mDirInfo;
    this->mStartGameInfo.fileInfo.fileName = this->mEntryName;

    this->mEntryFound = false;
    this->mActiveTask = TASK_BOOTUPCHECK;
    this->mIMemcard->TrcStartGame(this->mStartGameInfo);
}

void MemcardInterfaceImpl::SaveCheck(const char *entryName, unsigned int nSaveReqs, SaveReq **saveReqs)
{
    SaveInfo *saveInfo;

    sprintf(this->mEntryName, "%s-%s", this->mGameInfo.mTitleId, entryName);

    this->mBlocksNeeded = 0;
    {
        unsigned int iSaveReq;

        for (iSaveReq = 0; iSaveReq < nSaveReqs; iSaveReq++)
        {
            this->mBlocksNeeded += this->CalcSaveSize(saveReqs[iSaveReq]->mSaveInfo, FORMAT_LAYER2) * saveReqs[iSaveReq]->mNumSaves;
        }
    }

    saveInfo = saveReqs[0]->mSaveInfo;
    memset(&this->mDirInfo, 0, 0x30);
    this->mDirInfo.directoryName = this->mEntryName;
    this->mDirInfo.iconSysData = saveInfo->mPs2Info.mIconSysData;
    this->mDirInfo.iconSysDataSize = saveInfo->mPs2Info.mIconSysDataSize;
    this->mDirInfo.staticIconData = saveInfo->mPs2Info.mStaticIconData;
    this->mDirInfo.staticIconDataSize = saveInfo->mPs2Info.mStaticIconDataSize;
    this->mDirInfo.staticIconFilename = saveInfo->mPs2Info.mStaticIconFilename;
    this->mDirInfo.copyIconData = saveInfo->mPs2Info.mCopyIconData;
    this->mDirInfo.copyIconDataSize = saveInfo->mPs2Info.mCopyIconDataSize;
    this->mDirInfo.copyIconFilename = saveInfo->mPs2Info.mCopyIconFilename;
    this->mDirInfo.deleteIconData = saveInfo->mPs2Info.mDeleteIconData;
    this->mDirInfo.deleteIconDataSize = saveInfo->mPs2Info.mDeleteIconDataSize;
    this->mDirInfo.deleteIconFilename = saveInfo->mPs2Info.mDeleteIconFilename;

    memset(&this->mFileInfo, 0, 0xc);
    this->mFileInfo.directoryInfo = &this->mDirInfo;
    this->mFileInfo.fileName = this->mEntryName;
    this->mFileInfo.fileByteSize = (saveInfo->mHeaderSize + sizeof(FileHeader)) + saveInfo->mBodySize;

    this->mIMemcard->TrcSaveFile(this->mActiveCard, this->mFileInfo, Realmc::SAVETASK_CHECK, this->mBlocksNeeded, 0);
    this->mActiveTask = TASK_SAVECHECK;
}

void MemcardInterfaceImpl::SaveCheckAlternate(const char *entryName, unsigned int nSaveReqs, SaveReq **saveReqs,
                                              const TitleInfo *titleInfo)
{
    SaveInfo *saveInfo;
    int uFileHeaderSize;

    if (titleInfo->mNameType == NAME_FILE)
    {
        sprintf(this->mEntryName, "%s", entryName);
    }
    else
    {
        sprintf(this->mEntryName, "%s-%s", this->mGameInfo.mTitleId, entryName);
    }

    this->mBlocksNeeded = 0;
    {
        unsigned int iSaveReq;

        for (iSaveReq = 0; iSaveReq < nSaveReqs; iSaveReq++)
        {
            this->mBlocksNeeded += this->CalcSaveSize(saveReqs[iSaveReq]->mSaveInfo, titleInfo->mDataFormat) * saveReqs[iSaveReq]->mNumSaves;
        }
    }

    saveInfo = saveReqs[0]->mSaveInfo;
    memset(&this->mDirInfo, 0, 0x30);
    this->mDirInfo.directoryName = this->mEntryName;
    this->mDirInfo.iconSysData = saveInfo->mPs2Info.mIconSysData;
    this->mDirInfo.iconSysDataSize = saveInfo->mPs2Info.mIconSysDataSize;
    this->mDirInfo.staticIconData = saveInfo->mPs2Info.mStaticIconData;
    this->mDirInfo.staticIconDataSize = saveInfo->mPs2Info.mStaticIconDataSize;
    this->mDirInfo.staticIconFilename = saveInfo->mPs2Info.mStaticIconFilename;
    this->mDirInfo.copyIconData = saveInfo->mPs2Info.mCopyIconData;
    this->mDirInfo.copyIconDataSize = saveInfo->mPs2Info.mCopyIconDataSize;
    this->mDirInfo.copyIconFilename = saveInfo->mPs2Info.mCopyIconFilename;
    this->mDirInfo.deleteIconData = saveInfo->mPs2Info.mDeleteIconData;
    this->mDirInfo.deleteIconDataSize = saveInfo->mPs2Info.mDeleteIconDataSize;
    this->mDirInfo.deleteIconFilename = saveInfo->mPs2Info.mDeleteIconFilename;

    memset(&this->mFileInfo, 0, 0xc);
    this->mFileInfo.directoryInfo = &this->mDirInfo;
    this->mFileInfo.fileName = this->mEntryName;

    uFileHeaderSize = 0;
    if (titleInfo->mDataFormat == FORMAT_LAYER2)
    {
        uFileHeaderSize = saveInfo->mHeaderSize + sizeof(FileHeader);
    }
    this->mFileInfo.fileByteSize = uFileHeaderSize + saveInfo->mBodySize;

    this->mIMemcard->TrcSaveFile(this->mActiveCard, this->mFileInfo, Realmc::SAVETASK_CHECK, this->mBlocksNeeded, 0);
    this->mActiveTask = TASK_SAVECHECK;
}

void MemcardInterfaceImpl::Save(const char *entryName, const char *header, const char *body, const SaveInfo *saveInfo)
{
    sprintf(this->mEntryName, "%s-%s", this->mGameInfo.mTitleId, entryName);

    this->mUserHeader = const_cast<char *>(header);
    this->mUserBody = const_cast<char *>(body);

    this->mFileHeader.Init(saveInfo->mHeaderSize, saveInfo->mBodySize,
                           sizeof(FileHeader) + saveInfo->mHeaderSize + saveInfo->mBodySize,
                           this->_CalcSignature(header, saveInfo->mHeaderSize),
                           this->_CalcSignature(body, saveInfo->mBodySize));
    memset(&this->mFileHeader.mFileHeaderSignature, 0, sizeof(this->mFileHeader.mFileHeaderSignature));
    this->mFileHeader.mFileHeaderSignature = this->_CalcSignature(&this->mFileHeader, 0x18);

    memset(&this->mDirInfo, 0, 0x30);
    this->mDirInfo.directoryName = this->mEntryName;
    this->mDirInfo.iconSysData = saveInfo->mPs2Info.mIconSysData;
    this->mDirInfo.iconSysDataSize = saveInfo->mPs2Info.mIconSysDataSize;
    this->mDirInfo.staticIconData = saveInfo->mPs2Info.mStaticIconData;
    this->mDirInfo.staticIconDataSize = saveInfo->mPs2Info.mStaticIconDataSize;
    this->mDirInfo.staticIconFilename = saveInfo->mPs2Info.mStaticIconFilename;
    this->mDirInfo.copyIconData = saveInfo->mPs2Info.mCopyIconData;
    this->mDirInfo.copyIconDataSize = saveInfo->mPs2Info.mCopyIconDataSize;
    this->mDirInfo.copyIconFilename = saveInfo->mPs2Info.mCopyIconFilename;
    this->mDirInfo.deleteIconData = saveInfo->mPs2Info.mDeleteIconData;
    this->mDirInfo.deleteIconDataSize = saveInfo->mPs2Info.mDeleteIconDataSize;
    this->mDirInfo.deleteIconFilename = saveInfo->mPs2Info.mDeleteIconFilename;

    memset(&this->mFileInfo, 0, 0xc);
    this->mFileInfo.directoryInfo = &this->mDirInfo;
    this->mFileInfo.fileName = this->mEntryName;
    this->mFileInfo.fileByteSize = (saveInfo->mHeaderSize + sizeof(FileHeader)) + saveInfo->mBodySize;

    this->mIMemcard->TrcSaveFile(this->mActiveCard, this->mFileInfo, Realmc::SAVETASK_SAVE, 0, 0);
    this->mActiveTask = TASK_SAVE;
}

void MemcardInterfaceImpl::SaveAlternate(const char *entryName, const char *header, const char *body,
                                         const SaveInfo *saveInfo, const TitleInfo *titleInfo)
{
    int uFileHeaderSize;

    this->mDataFormat = titleInfo->mDataFormat;

    if (titleInfo->mNameType == NAME_FILE)
    {
        sprintf(this->mEntryName, "%s", entryName);
    }
    else
    {
        sprintf(this->mEntryName, "%s-%s", this->mGameInfo.mTitleId, entryName);
    }

    this->mUserHeader = const_cast<char *>(header);
    this->mUserBody = const_cast<char *>(body);

    this->mFileHeader.Init(saveInfo->mHeaderSize, saveInfo->mBodySize,
                           sizeof(FileHeader) + saveInfo->mHeaderSize + saveInfo->mBodySize,
                           this->_CalcSignature(header, saveInfo->mHeaderSize),
                           this->_CalcSignature(body, saveInfo->mBodySize));
    memset(&this->mFileHeader.mFileHeaderSignature, 0, sizeof(this->mFileHeader.mFileHeaderSignature));
    this->mFileHeader.mFileHeaderSignature = this->_CalcSignature(&this->mFileHeader, 0x18);

    memset(&this->mDirInfo, 0, 0x30);
    this->mDirInfo.directoryName = this->mEntryName;
    this->mDirInfo.iconSysData = saveInfo->mPs2Info.mIconSysData;
    this->mDirInfo.iconSysDataSize = saveInfo->mPs2Info.mIconSysDataSize;
    this->mDirInfo.staticIconData = saveInfo->mPs2Info.mStaticIconData;
    this->mDirInfo.staticIconDataSize = saveInfo->mPs2Info.mStaticIconDataSize;
    this->mDirInfo.staticIconFilename = saveInfo->mPs2Info.mStaticIconFilename;
    this->mDirInfo.copyIconData = saveInfo->mPs2Info.mCopyIconData;
    this->mDirInfo.copyIconDataSize = saveInfo->mPs2Info.mCopyIconDataSize;
    this->mDirInfo.copyIconFilename = saveInfo->mPs2Info.mCopyIconFilename;
    this->mDirInfo.deleteIconData = saveInfo->mPs2Info.mDeleteIconData;
    this->mDirInfo.deleteIconDataSize = saveInfo->mPs2Info.mDeleteIconDataSize;
    this->mDirInfo.deleteIconFilename = saveInfo->mPs2Info.mDeleteIconFilename;

    memset(&this->mFileInfo, 0, 0xc);
    this->mFileInfo.directoryInfo = &this->mDirInfo;
    this->mFileInfo.fileName = this->mEntryName;

    uFileHeaderSize = 0;
    if (titleInfo->mDataFormat == FORMAT_LAYER2)
    {
        uFileHeaderSize = saveInfo->mHeaderSize + sizeof(FileHeader);
    }
    this->mFileInfo.fileByteSize = uFileHeaderSize + saveInfo->mBodySize;

    this->mIMemcard->TrcSaveFile(this->mActiveCard, this->mFileInfo, Realmc::SAVETASK_SAVE, 0, 0);
    this->mActiveTask = TASK_SAVE;
}

void MemcardInterfaceImpl::Load(const char *entryName, char *header, char *body, const wchar_t *contentName)
{
    this->mDataFormat = FORMAT_LAYER2;
    this->mNameType = NAME_ENTRY;

    sprintf(this->mDirName, "%s-%s", this->mGameInfo.mTitleId, entryName);
    sprintf(this->mEntryName, "%s-%s", this->mGameInfo.mTitleId, entryName);

    this->mUserHeader = header;
    this->mUserBody = body;
    this->mFileHeader.Clear();

    memset(&this->mDirInfo, 0, 0x30);
    this->mDirInfo.directoryName = this->mDirName;

    memset(&this->mFileInfo, 0, 0xc);
    this->mFileInfo.directoryInfo = &this->mDirInfo;
    this->mFileInfo.fileName = this->mEntryName;

    this->mIMemcard->FindFile(this->mActiveCard, this->mEntryName, this->mEntryName);
    this->mActiveTask = TASK_LOAD;
}

void MemcardInterfaceImpl::LoadAlternate(const char *entryName, char *header, char *body, const wchar_t *contentName,
                                         const TitleInfo *titleInfo)
{
    this->mDataFormat = titleInfo->mDataFormat;
    this->mNameType = titleInfo->mNameType;

    if (titleInfo->mNameType == NAME_FILE)
    {
        sprintf(this->mDirName, "%s", entryName);
        sprintf(this->mEntryName, "%s", entryName);
    }
    else if (titleInfo->mNameType == NAME_ENTRY)
    {
        sprintf(this->mDirName, "%s-%s", titleInfo->mTitleId, entryName);
        sprintf(this->mEntryName, "%s-%s", titleInfo->mTitleId, entryName);
    }
    else
    {
        memset(this->mDirName, 0, 0x20);
        memset(this->mEntryName, 0, 0x20);
        _SplitPath(entryName, this->mDirName, this->mEntryName);
    }

    this->mUserHeader = header;
    this->mUserBody = body;
    this->mFileHeader.Clear();

    memset(&this->mDirInfo, 0, 0x30);
    this->mDirInfo.directoryName = this->mDirName;

    memset(&this->mFileInfo, 0, 0xc);
    this->mFileInfo.directoryInfo = &this->mDirInfo;
    this->mFileInfo.fileName = this->mEntryName;

    this->mIMemcard->FindFile(this->mActiveCard, this->mDirName, this->mEntryName);
    this->mActiveTask = TASK_LOAD;
}

void MemcardInterfaceImpl::Delete(const char *entryName, const wchar_t *contentName)
{
    sprintf(this->mEntryName, "%s-%s", this->mGameInfo.mTitleId, entryName);

    memset(&this->mDirInfo, 0, 0x30);
    this->mDirInfo.directoryName = this->mEntryName;

    memset(&this->mFileInfo, 0, 0xc);
    this->mFileInfo.directoryInfo = &this->mDirInfo;
    this->mFileInfo.fileName = Realmc::FILENAME_ALL_FILES;

    this->mIMemcard->TrcDeleteFile(this->mActiveCard, this->mFileInfo);
    this->mActiveTask = TASK_DELETE;
    this->mActiveSubtask = SUBTASK_DELETE_SINGLE;
}

void MemcardInterfaceImpl::DeleteMultiple(unsigned int nEntryNames, const char **entryNames, const wchar_t *contentName)
{
    this->mNumEntries = nEntryNames;
    this->mEntryList = entryNames;
    this->mCurEntry = 0;
    this->mEntryFound = false;

    sprintf(this->mEntryName, "%s-%s", this->mGameInfo.mTitleId, this->mEntryList[this->mCurEntry++]);

    memset(&this->mDirInfo, 0, 0x30);
    this->mDirInfo.directoryName = this->mEntryName;

    memset(&this->mFileInfo, 0, 0xc);
    this->mFileInfo.directoryInfo = &this->mDirInfo;
    this->mFileInfo.fileName = Realmc::FILENAME_ALL_FILES;

    this->SetMessage(MESSAGE_HIDE, 0x40000);

    this->mIMemcard->TrcDeleteFile(this->mActiveCard, this->mFileInfo);
    this->mActiveTask = TASK_DELETE;
    this->mActiveSubtask = SUBTASK_DELETE_MULTIPLE;
}

void MemcardInterfaceImpl::FindEntries(const char *entryNamePattern)
{
    this->mNameType = NAME_ENTRY;

    sprintf(this->mDirName, "%s-%s", this->mGameInfo.mTitleId, entryNamePattern);
    sprintf(this->mEntryName, "%s-%s", this->mGameInfo.mTitleId, entryNamePattern);

    this->mTaskManager->ClearEntries();

    memset(&this->mDirInfo, 0, 0x30);
    this->mDirInfo.directoryName = this->mDirName;

    memset(&this->mFileInfo, 0, 0xc);
    this->mFileInfo.directoryInfo = &this->mDirInfo;
    this->mFileInfo.fileName = this->mEntryName;

    this->mIMemcard->TrcListFiles(this->mActiveCard, this->mFileInfo, Realmc::LISTTASK_ENTRIES);
    this->mActiveTask = TASK_FINDENTRIES;
}

void MemcardInterfaceImpl::FindEntriesAlternate(const char *entryNamePattern, const TitleInfo *titleInfo)
{
    Realmc::ListTaskType listType;

    this->mNameType = titleInfo->mNameType;

    if (titleInfo->mNameType == NAME_FILE)
    {
        listType = Realmc::LISTTASK_FILES;
        sprintf(this->mDirName, "%s%s", titleInfo->mTitleId, entryNamePattern);
        sprintf(this->mEntryName, "%s%s", titleInfo->mTitleId, entryNamePattern);
    }
    else if (titleInfo->mNameType == NAME_ENTRY)
    {
        listType = Realmc::LISTTASK_ENTRIES;
        sprintf(this->mDirName, "%s-%s", titleInfo->mTitleId, entryNamePattern);
        sprintf(this->mEntryName, "%s-%s", titleInfo->mTitleId, entryNamePattern);
    }
    else
    {
        listType = Realmc::LISTTASK_FILES;
        memset(this->mDirName, 0, 0x20);
        memset(this->mEntryName, 0, 0x20);
        _SplitPath(entryNamePattern, this->mDirName, this->mEntryName);
    }

    this->mTaskManager->ClearEntries();

    memset(&this->mDirInfo, 0, 0x30);
    this->mDirInfo.directoryName = this->mDirName;

    memset(&this->mFileInfo, 0, 0xc);
    this->mFileInfo.directoryInfo = &this->mDirInfo;
    this->mFileInfo.fileName = this->mEntryName;

    this->mIMemcard->TrcListFiles(this->mActiveCard, this->mFileInfo, listType);
    this->mActiveTask = TASK_FINDENTRIES;
}

void MemcardInterfaceImpl::CheckCard(CardId cardId)
{
    this->mCardInfo.Clear();

    if (cardId == CARD_UNKNOWN)
    {
        this->mIMemcard->TrcGetCardInfo(this->mActiveCard);
    }
    else
    {
        this->mIMemcard->TrcGetCardInfo(_ChangeToRealmcCardId(cardId));
    }

    this->mActiveTask = TASK_CHECKCARD;
}

void MemcardInterfaceImpl::SetAutosave(AutosaveState state, unsigned int nSaveReqs, SaveReq **saveReqs, const char *entryName,
                                       CardId cardId)
{
    if (cardId == CARD_UNKNOWN)
    {
        this->mAutosaveCard = this->mActiveCard;
    }
    else
    {
        this->mAutosaveCard = _ChangeToRealmcCardId(cardId);
    }

    if (state == AUTOSAVE_ENABLE)
    {
        this->mAutosaveEnabled = state;
        this->mIMemcard->TrcCardExists(this->mAutosaveCard);
        this->mActiveTask = TASK_SETAUTOSAVE;
    }
    else
    {
        this->mAutosaveEnabled = false;
        static_cast<Realmc::PS2Interface *>(this->mIMemcard)->SetAutosaveEnabled(false);
        this->mTaskManager->CompleteTask(RESULT_SUCCESS, STATUS_OK, &state);
    }
}

void MemcardInterfaceImpl::SetActiveCard(CardId cardId)
{
    this->mActiveCard = _ChangeToRealmcCardId(cardId);

    this->mBlockSize = this->mIMemcard->GetBlockSize(this->mActiveCard);
}

void MemcardInterfaceImpl::_ProcessBootupCheck(const Realmc::Message *message)
{
    switch (message->mMsg)
    {
    case Realmc::LMSG_TRC:
        this->_ProcessGuidelinesMessage(message);
        break;

    case Realmc::LMSG_TRC_STARTGAME_DONE:
        {
            BootupCheckResults results;

            this->mActiveTask = TASK_NONE;
            results.mFirstGoodCard = _ChangeToRealmcIfaceCardId(message->info.cardInfo.cardID);
            results.mEntryFound = this->mEntryFound;
            results.mNumBlocksNeeded =
                message->mCardStatus == Realmc::STATUS_EXIT_TO_CARD_MANAGER ? this->mStartGameInfo.totalBlocksNeeded : 0;
            this->_ClearMessage();
            this->mTaskManager->CompleteTask(RESULT_SUCCESS, this->_TranslateCardStatus(message->mCardStatus), &results);

            if (this->mAutosaveEnabled &&
                (message->mCardStatus == Realmc::STATUS_NO_CARD || message->mCardStatus == Realmc::STATUS_CARD_CHANGED ||
                 message->mCardStatus == Realmc::STATUS_CARD_REMOVED))
            {
                this->_DisableAutosave();
            }
        }
        break;

    default:
        break;
    }
}

void MemcardInterfaceImpl::_ProcessCheckCard(const Realmc::Message *message)
{
    switch (message->mMsg)
    {
    case Realmc::LMSG_TRC:
        this->_ProcessGuidelinesMessage(message);
        break;

    case Realmc::LMSG_TRC_GETCARDINFO_DONE:
        this->mCardInfo.Clear();
        this->mCardInfo.mStatus = this->_TranslateCardStatus(message->mCardStatus);
        this->mCardInfo.mCardId = _ChangeToRealmcIfaceCardId(message->info.cardInfo.cardID);
        this->mCardInfo.mFreeSpace = message->info.cardInfo.freeSpace;
        this->mActiveTask = TASK_NONE;
        this->_ClearMessage();
        this->mTaskManager->CompleteTask(RESULT_SUCCESS, this->mCardInfo.mStatus, &this->mCardInfo);

        if (this->mAutosaveEnabled &&
            (message->mCardStatus == Realmc::STATUS_NO_CARD || message->mCardStatus == Realmc::STATUS_CARD_CHANGED ||
             message->mCardStatus == Realmc::STATUS_CARD_REMOVED))
        {
            this->_DisableAutosave();
        }
        break;

    default:
        break;
    }
}

void MemcardInterfaceImpl::_ProcessSave(const Realmc::Message *message)
{
    switch (message->mMsg)
    {
    case Realmc::LMSG_TRC:
        if (message->info.trc.mMsgId == 0x18 || message->info.trc.mMsgId == 0x1c)
        {
            this->mIsResettable = false;
        }
        else
        {
            this->mIsResettable = true;
            this->_ClearMessage();
        }
        this->_ProcessGuidelinesMessage(message);
        break;

    case Realmc::LMSG_OPEN_FILE_DONE:
        this->mFilehandle = message->info.openResult.fileHandle;
        break;

    case Realmc::LMSG_WRITE_READY:
        if (this->mDataFormat == FORMAT_LAYER2)
        {
            this->mActiveSubtask = SUBTASK_WRITE_FILE_HEADER;
            this->mIMemcard->Write(this->mFilehandle, &this->mFileHeader, sizeof(this->mFileHeader));
        }
        else
        {
            this->mActiveSubtask = SUBTASK_WRITE_USER_BODY;
            this->mIMemcard->Write(this->mFilehandle, this->mUserBody, this->mFileHeader.mUserBodySize);
        }
        break;

    case Realmc::LMSG_WRITE_DONE:
        switch (this->mActiveSubtask)
        {
        case SUBTASK_WRITE_FILE_HEADER:
            if (this->mFileHeader.mUserHeaderSize != 0 && this->mUserHeader != nullptr)
            {
                this->mActiveSubtask = SUBTASK_WRITE_USER_HEADER;
                this->mIMemcard->Write(this->mFilehandle, this->mUserHeader, this->mFileHeader.mUserHeaderSize);
            }
            else if (this->mFileHeader.mUserBodySize != 0 && this->mUserBody != nullptr)
            {
                this->mActiveSubtask = SUBTASK_WRITE_USER_BODY;
                this->mIMemcard->Write(this->mFilehandle, this->mUserBody, this->mFileHeader.mUserBodySize);
            }
            else
            {
                this->mActiveSubtask = SUBTASK_NONE;
                this->mIMemcard->SendMessage(Realmc::UMSG_WRITE_COMPLETE, 0);
                this->mFilehandle = nullptr;
            }
            break;

        case SUBTASK_WRITE_USER_HEADER:
            if (this->mFileHeader.mUserBodySize != 0 && this->mUserBody != nullptr)
            {
                this->mActiveSubtask = SUBTASK_WRITE_USER_BODY;
                this->mIMemcard->Write(this->mFilehandle, this->mUserBody, this->mFileHeader.mUserBodySize);
            }
            else
            {
                this->mActiveSubtask = SUBTASK_NONE;
                this->mIMemcard->SendMessage(Realmc::UMSG_WRITE_COMPLETE, 0);
                this->mFilehandle = nullptr;
            }
            break;

        case SUBTASK_WRITE_USER_BODY:
            this->mActiveSubtask = SUBTASK_NONE;
            this->mIMemcard->SendMessage(Realmc::UMSG_WRITE_COMPLETE, 0);
            this->mFilehandle = nullptr;
            break;

        default:
            break;
        }
        break;

    case Realmc::LMSG_TRC_SAVECHECK_DONE:
    case Realmc::LMSG_TRC_SAVEFILE_DONE:
        this->mIsResettable = true;
        this->mActiveTask = TASK_NONE;
        this->_ClearMessage();
        this->mTaskManager->CompleteTask(this->_TranslateTaskResult(message->mTaskResult),
                                         this->_TranslateCardStatus(message->mCardStatus),
                                         &this->mEntryName[sizeof(TitleId)]);

        if (this->mAutosaveEnabled &&
            (message->mCardStatus == Realmc::STATUS_NO_CARD || message->mCardStatus == Realmc::STATUS_CARD_CHANGED ||
             message->mCardStatus == Realmc::STATUS_CARD_REMOVED))
        {
            this->_DisableAutosave();
        }
        break;

    default:
        break;
    }
}

void MemcardInterfaceImpl::_ProcessLoad(const Realmc::Message *message)
{
    switch (message->mMsg)
    {
    case Realmc::LMSG_TRC:
        this->_ProcessGuidelinesMessage(message);
        break;

    case Realmc::LMSG_FILE_INFO:
        this->mFileSize = message->info.fileInfo.fileSize;
        break;

    case Realmc::LMSG_FIND_FILE_DONE:
        this->mIMemcard->TrcLoadFile(this->mActiveCard, this->mFileInfo);
        break;

    case Realmc::LMSG_OPEN_FILE_DONE:
        this->mFilehandle = message->info.openResult.fileHandle;
        break;

    case Realmc::LMSG_SEEK_DONE:
        this->mIMemcard->Read(this->mFilehandle, this->mUserBody, this->mFileHeader.mUserBodySize);
        break;

    case Realmc::LMSG_READ_READY:
        if (this->mDataFormat == FORMAT_LAYER2)
        {
            this->mActiveSubtask = SUBTASK_READ_FILE_HEADER;
            this->mIMemcard->Read(this->mFilehandle, &this->mFileHeader, sizeof(this->mFileHeader));
        }
        else
        {
            if (this->mUserBody == nullptr)
            {
                this->mTaskStatus = this->mIGame->LoadReady(this->mEntryName, 0, this->mFileSize, this->mUserHeader, this->mUserBody);

                if (this->mTaskStatus == TASK_CONTINUE)
                {
                    this->mActiveSubtask = SUBTASK_READ_USER_BODY;
                    this->mIMemcard->Read(this->mFilehandle, this->mUserBody, this->mFileSize);
                }
                else
                {
                    this->mActiveSubtask = SUBTASK_NONE;
                    this->mIMemcard->SendMessage(Realmc::UMSG_READ_COMPLETE, 0);
                    this->mFilehandle = nullptr;
                }
            }
            else
            {
                this->mActiveSubtask = SUBTASK_READ_USER_BODY;
                this->mIMemcard->Read(this->mFilehandle, this->mUserBody, this->mFileSize);
            }
        }
        break;

    case Realmc::LMSG_READ_DONE:
        switch (this->mActiveSubtask)
        {
        case SUBTASK_READ_FILE_HEADER:
            if (this->mFileHeader.mFileSize != this->mFileSize ||
                this->mFileHeader.mFileHeaderVersion != 0x4d433032 ||
                this->mFileHeader.mFileHeaderSignature != this->_CalcSignature(&this->mFileHeader, 0x18))
            {
                this->mActiveSubtask = SUBTASK_NONE;
                this->mIMemcard->SendMessage(Realmc::UMSG_READ_COMPLETE, 0);
                this->mFilehandle = nullptr;
            }
            else
            {
                if (this->mUserHeader == nullptr && this->mUserBody == nullptr)
                {
                    this->mTaskStatus = this->mIGame->LoadReady(&this->mEntryName[sizeof(TitleId)],
                                                                this->mFileHeader.mUserHeaderSize,
                                                                this->mFileHeader.mUserBodySize, this->mUserHeader,
                                                                this->mUserBody);
                }

                if (this->mTaskStatus == TASK_CANCEL)
                {
                    this->mActiveSubtask = SUBTASK_NONE;
                    this->mIMemcard->SendMessage(Realmc::UMSG_READ_COMPLETE, 0);
                    this->mFilehandle = nullptr;
                }
                else if (this->mFileHeader.mUserHeaderSize != 0 && this->mUserHeader != nullptr)
                {
                    this->mActiveSubtask = SUBTASK_READ_USER_HEADER;
                    this->mIMemcard->Read(this->mFilehandle, this->mUserHeader, this->mFileHeader.mUserHeaderSize);
                }
                else if (this->mFileHeader.mUserBodySize != 0 && this->mUserBody != nullptr)
                {
                    this->mActiveSubtask = SUBTASK_READ_USER_BODY;
                    this->mIMemcard->Seek(this->mFilehandle, this->mFileHeader.mUserHeaderSize, Realmc::SF_CUR);
                }
                else
                {
                    this->mActiveSubtask = SUBTASK_NONE;
                    this->mIMemcard->SendMessage(Realmc::UMSG_READ_COMPLETE, 0);
                    this->mFilehandle = nullptr;
                }
            }
            break;

        case SUBTASK_READ_USER_HEADER:
            if (this->mFileHeader.mUserBodySize != 0 && this->mUserBody != nullptr)
            {
                this->mActiveSubtask = SUBTASK_READ_USER_BODY;
                this->mIMemcard->Read(this->mFilehandle, this->mUserBody, this->mFileHeader.mUserBodySize);
            }
            else
            {
                this->mActiveSubtask = SUBTASK_NONE;
                this->mIMemcard->SendMessage(Realmc::UMSG_READ_COMPLETE, 0);
                this->mFilehandle = nullptr;
            }
            break;

        case SUBTASK_READ_USER_BODY:
            this->mActiveSubtask = SUBTASK_NONE;
            this->mIMemcard->SendMessage(Realmc::UMSG_READ_COMPLETE, 0);
            this->mFilehandle = nullptr;
            break;

        default:
            break;
        }
        break;

    case Realmc::LMSG_CHECK_DATA_INTEGRITY:
        if (this->mTaskStatus != TASK_CONTINUE)
        {
            this->mIMemcard->SendMessage(Realmc::UMSG_DATA_CORRUPT, 0);
        }
        else if (this->mDataFormat == FORMAT_LAYER2)
        {
            if (this->mFileHeader.mFileSize != this->mFileSize ||
                this->mFileHeader.mFileHeaderVersion != 0x4d433032 ||
                this->mFileHeader.mFileHeaderSignature != this->_CalcSignature(&this->mFileHeader, 0x18))
            {
                this->mIMemcard->SendMessage(Realmc::UMSG_DATA_CORRUPT, 0);
            }
            else
            {
                bool isUserDataOk = true;

                if (this->mUserHeader != nullptr)
                {
                    if (this->mFileHeader.mUserHeaderSignature !=
                        this->_CalcSignature(this->mUserHeader, this->mFileHeader.mUserHeaderSize))
                    {
                        isUserDataOk = false;
                    }
                }

                if (this->mUserBody != nullptr)
                {
                    if (this->mFileHeader.mUserBodySignature !=
                        this->_CalcSignature(this->mUserBody, this->mFileHeader.mUserBodySize))
                    {
                        isUserDataOk = false;
                    }
                }

                if (isUserDataOk)
                {
                    this->mIMemcard->SendMessage(Realmc::UMSG_DATA_OK, 0);
                }
                else
                {
                    this->mIMemcard->SendMessage(Realmc::UMSG_DATA_CORRUPT, 0);
                }
            }
        }
        else
        {
            if (this->mIGame->CheckLoadedData(this->mUserBody) == DATA_OK)
            {
                this->mIMemcard->SendMessage(Realmc::UMSG_DATA_OK, 0);
            }
            else
            {
                this->mIMemcard->SendMessage(Realmc::UMSG_DATA_CORRUPT, 0);
            }
        }
        break;

    case Realmc::LMSG_TRC_LOADFILE_DONE:
        this->_ClearMessage();
        this->mActiveTask = TASK_NONE;
        this->mTaskManager->CompleteTask(this->_TranslateTaskResult(message->mTaskResult),
                                         this->_TranslateCardStatus(message->mCardStatus),
                                         &this->mEntryName[sizeof(TitleId)]);

        if (this->mAutosaveEnabled &&
            (message->mCardStatus == Realmc::STATUS_NO_CARD || message->mCardStatus == Realmc::STATUS_CARD_CHANGED ||
             message->mCardStatus == Realmc::STATUS_CARD_REMOVED))
        {
            this->_DisableAutosave();
        }
        break;

    default:
        break;
    }
}

void MemcardInterfaceImpl::_ProcessDelete(const Realmc::Message *message)
{
    static bool sDeleteStarted = false;
    static bool sShowFailedMsg = false;

    switch (message->mMsg)
    {
    case Realmc::LMSG_TRC:
        if (message->info.trc.mMsgId == 0x20)
        {
            this->mIsResettable = false;
            sDeleteStarted = true;
        }
        else
        {
            this->mIsResettable = true;
        }

        if (message->info.trc.mMsgId == 0x21 && this->mActiveSubtask == SUBTASK_DELETE_MULTIPLE &&
            message->mCardStatus == Realmc::STATUS_FILE_NOT_FOUND)
        {
            this->mIMemcard->SendMessage(Realmc::UMSG_OPTION1, 0);
        }
        else if (sDeleteStarted && this->mActiveSubtask == SUBTASK_DELETE_MULTIPLE &&
                 (message->info.trc.mMsgId == 7 || message->info.trc.mMsgId == 0x27 || message->info.trc.mMsgId == 8))
        {
            sShowFailedMsg = true;
            this->mIMemcard->SendMessage(Realmc::UMSG_OPTION1, 0);
        }
        else
        {
            this->_ProcessGuidelinesMessage(message);
        }
        break;

    case Realmc::LMSG_TRC_DELETEFILE_DONE:
        this->mIsResettable = true;

        if (this->mActiveSubtask == SUBTASK_DELETE_SINGLE)
        {
            sDeleteStarted = false;
            this->mActiveTask = TASK_NONE;
            this->_ClearMessage();
            this->mTaskManager->CompleteTask(this->_TranslateTaskResult(message->mTaskResult),
                                             this->_TranslateCardStatus(message->mCardStatus),
                                             &this->mEntryName[sizeof(TitleId)]);
        }
        else if (message->mTaskResult == Realmc::RESULT_SUCCESS || message->mCardStatus == Realmc::STATUS_FILE_NOT_FOUND)
        {
            if (message->mTaskResult == Realmc::RESULT_SUCCESS)
            {
                this->mEntryFound = true;
            }

            if (this->mCurEntry < this->mNumEntries)
            {
                this->SetMessage(MESSAGE_HIDE, 0x70001);
                sprintf(this->mEntryName, "%s-%s", this->mGameInfo.mTitleId, this->mEntryList[this->mCurEntry++]);
                this->mDirInfo.directoryName = this->mEntryName;
                this->mIMemcard->TrcDeleteFile(this->mActiveCard, this->mFileInfo);
            }
            else
            {
                this->SetMessage(MESSAGE_SHOW, 0x70001);

                if (this->mEntryFound)
                {
                    Realmc::PS2Message message;

                    message.TRC_msg(0x22, Realmc::PS2Message::PackMsgOptions(Realmc::UMSG_OK, 0, 0, 0));
                    this->mUserChoice = CHOICE_NONE;
                    this->_ProcessGuidelinesMessage(&message);
                }
                else
                {
                    Realmc::PS2Message message;

                    message.TRC_msg(0x21, Realmc::PS2Message::PackMsgOptions(Realmc::UMSG_OK, 0, 0, 0));
                    this->mUserChoice = CHOICE_NONE;
                    this->_ProcessGuidelinesMessage(&message);
                }

                static_cast<Realmc::PS2Interface *>(this->mIMemcard)->SetDummyMessage();
            }
        }
        else if (message->mTaskResult == Realmc::RESULT_RETRY)
        {
            sDeleteStarted = false;
            this->mActiveTask = TASK_NONE;
            this->mActiveSubtask = SUBTASK_NONE;
            this->SetMessage(MESSAGE_SHOW, 0x30001);
            this->_ClearMessage();
            this->mTaskManager->CompleteTask(this->_TranslateTaskResult(message->mTaskResult),
                                             this->_TranslateCardStatus(message->mCardStatus),
                                             &this->mEntryName[sizeof(TitleId)]);
        }
        else
        {
            sDeleteStarted = false;
            this->SetMessage(MESSAGE_SHOW, 0x70001);

            if (sShowFailedMsg)
            {
                Realmc::PS2Message message;

                message.TRC_msg(0x21, Realmc::PS2Message::PackMsgOptions(Realmc::UMSG_OK, 0, 0, 0));
                this->mUserChoice = CHOICE_NONE;
                this->_ProcessGuidelinesMessage(&message);
                static_cast<Realmc::PS2Interface *>(this->mIMemcard)->SetDummyMessage();
            }
            else
            {
                this->mActiveTask = TASK_NONE;
                this->mActiveSubtask = SUBTASK_NONE;
                this->_ClearMessage();
                this->mTaskManager->CompleteTask(this->_TranslateTaskResult(message->mTaskResult),
                                                 this->_TranslateCardStatus(message->mCardStatus),
                                                 &this->mEntryName[sizeof(TitleId)]);
            }
        }

        if (this->mAutosaveEnabled &&
            (message->mCardStatus == Realmc::STATUS_NO_CARD || message->mCardStatus == Realmc::STATUS_CARD_CHANGED ||
             message->mCardStatus == Realmc::STATUS_CARD_REMOVED || sShowFailedMsg))
        {
            this->_DisableAutosave();
        }
        break;

    case Realmc::LMSG_NONE:
        if (this->mUserChoice == CHOICE_NONE)
        {
            static_cast<Realmc::PS2Interface *>(this->mIMemcard)->SetDummyMessage();
        }
        else
        {
            sDeleteStarted = false;
            this->mActiveTask = TASK_NONE;
            this->mActiveSubtask = SUBTASK_NONE;
            this->_ClearMessage();

            if (sShowFailedMsg)
            {
                sShowFailedMsg = false;
                this->mTaskManager->CompleteTask(RESULT_FAILED, STATUS_CARD_REMOVED, &this->mEntryName[sizeof(TitleId)]);
            }
            else if (this->mEntryFound)
            {
                this->mTaskManager->CompleteTask(RESULT_SUCCESS, STATUS_OK, &this->mEntryName[sizeof(TitleId)]);
            }
            else
            {
                this->mTaskManager->CompleteTask(RESULT_FAILED, STATUS_ENTRY_NOT_FOUND, &this->mEntryName[sizeof(TitleId)]);
            }
        }
        break;

    default:
        break;
    }
}

void MemcardInterfaceImpl::_ProcessFindEntries(const Realmc::Message *message)
{
    switch (message->mMsg)
    {
    case Realmc::LMSG_TRC:
        this->_ProcessGuidelinesMessage(message);
        break;

    case Realmc::LMSG_FILE_INFO:
        if (this->mNameType == NAME_FILE)
        {
            char tempPath[63];
            char *separator;

            this->mEntryInfo.Clear();
            {
                unsigned int pos;

                strcpy(tempPath, message->info.fileInfo.fileName);
                separator = nullptr;
                pos = strlen(message->info.fileInfo.fileName);
                if (pos != 0)
                {
                    do
                    {
                        if (tempPath[pos] == '\\' || tempPath[pos] == '/')
                        {
                            separator = &tempPath[pos];
                            *separator = 0;
                        }
                        pos--;
                    } while (separator == nullptr && pos != 0);
                }
            }

            this->mEntryInfo.mName = separator + 1;
            this->mEntryInfo.mEntryBlocks = message->info.fileInfo.fileSize / this->mBlockSize + 1;
            this->mEntryInfo.mUserDataSize = message->info.fileInfo.fileSize;
            this->mEntryInfo.mStatus = STATUS_OK;
            this->mEntryInfo.mTime.mCreated = _ChangeFileTime(message->info.fileInfo.fileTime.created);
            this->mEntryInfo.mTime.mLastModified = _ChangeFileTime(message->info.fileInfo.fileTime.lastModified);
            this->mEntryInfo.mTime.mLastAccessed = 0;
            this->mTaskManager->FoundEntry(&this->mEntryInfo);
        }
        else if (this->mNameType == NAME_ENTRY)
        {
            int userDataSize;

            userDataSize = message->mTaskResult - sizeof(FileHeader);

            this->mEntryInfo.Clear();
            this->mEntryInfo.mName = message->info.fileInfo.fileName + sizeof(TitleId);
            this->mEntryInfo.mEntryBlocks = message->info.fileInfo.fileSize;
            this->mEntryInfo.mUserDataSize = userDataSize < 0 ? 0 : userDataSize;
            this->mEntryInfo.mStatus = userDataSize < 0 ? STATUS_ENTRY_CORRUPTED : this->_TranslateCardStatus(message->mCardStatus);
            this->mEntryInfo.mTime.mCreated = _ChangeFileTime(message->info.fileInfo.fileTime.created);
            this->mEntryInfo.mTime.mLastModified = _ChangeFileTime(message->info.fileInfo.fileTime.lastModified);
            this->mEntryInfo.mTime.mLastAccessed = 0;
            this->mTaskManager->FoundEntry(&this->mEntryInfo);
        }
        else
        {
            this->mEntryInfo.Clear();
            this->mEntryInfo.mName = message->info.fileInfo.fileName;
            this->mEntryInfo.mEntryBlocks = message->info.fileInfo.fileSize / this->mBlockSize + 1;
            this->mEntryInfo.mUserDataSize = message->info.fileInfo.fileSize;
            this->mEntryInfo.mStatus = STATUS_OK;
            this->mEntryInfo.mTime.mCreated = _ChangeFileTime(message->info.fileInfo.fileTime.created);
            this->mEntryInfo.mTime.mLastModified = _ChangeFileTime(message->info.fileInfo.fileTime.lastModified);
            this->mEntryInfo.mTime.mLastAccessed = 0;
            this->mTaskManager->FoundEntry(&this->mEntryInfo);
        }
        break;

    case Realmc::LMSG_TRC_LIST_DONE:
        this->_ClearMessage();
        this->mActiveTask = TASK_NONE;
        this->mTaskManager->CompleteTask(this->_TranslateTaskResult(message->mTaskResult),
                                         this->_TranslateCardStatus(message->mCardStatus), nullptr);

        if (this->mAutosaveEnabled &&
            (message->mCardStatus == Realmc::STATUS_NO_CARD || message->mCardStatus == Realmc::STATUS_CARD_CHANGED ||
             message->mCardStatus == Realmc::STATUS_CARD_REMOVED))
        {
            this->_DisableAutosave();
        }
        break;

    default:
        break;
    }
}

void MemcardInterfaceImpl::_ProcessSetAutosave(const Realmc::Message *message)
{
    switch (message->mMsg)
    {
    case Realmc::LMSG_TRC:
        this->_ProcessGuidelinesMessage(message);
        break;

    case Realmc::LMSG_TRC_CARDEXISTS_DONE:
        this->mActiveTask = TASK_NONE;
        this->_ClearMessage();

        if (message->mCardStatus == Realmc::STATUS_OK || message->mCardStatus == Realmc::STATUS_CARD_CHANGED ||
            message->mCardStatus == Realmc::STATUS_CARD_UNFORMATTED)
        {
            AutosaveState state;

            static_cast<Realmc::PS2Interface *>(this->mIMemcard)->SetAutosaveEnabled(true);
            state = AUTOSAVE_ENABLE;
            this->mTaskManager->CompleteTask(RESULT_SUCCESS, STATUS_OK, &state);
        }
        else
        {
            AutosaveState state;

            this->mAutosaveEnabled = false;
            static_cast<Realmc::PS2Interface *>(this->mIMemcard)->SetAutosaveEnabled(false);
            state = AUTOSAVE_DISABLE;
            this->mTaskManager->CompleteTask(RESULT_FAILED, this->_TranslateCardStatus(message->mCardStatus), &state);
        }
        break;

    case Realmc::LMSG_TRC_SAVECHECK_DONE:
        this->mActiveTask = TASK_NONE;
        this->_ClearMessage();

        if (message->mCardStatus == Realmc::STATUS_OK)
        {
            AutosaveState state;

            state = AUTOSAVE_ENABLE;
            this->mTaskManager->CompleteTask(RESULT_SUCCESS, STATUS_OK, &state);
        }
        else
        {
            AutosaveState state;

            this->mAutosaveEnabled = false;
            state = AUTOSAVE_DISABLE;
            this->mTaskManager->CompleteTask(this->_TranslateTaskResult(message->mTaskResult),
                                             this->_TranslateCardStatus(message->mCardStatus), &state);
        }
        break;

    default:
        break;
    }
}

void MemcardInterfaceImpl::_ProcessGuidelinesMessage(const Realmc::Message *message)
{
    bool showMessage;
    Realmc::UserMessage defaultResponse;
    unsigned int hiddenMessages;

    defaultResponse = Realmc::UMSG_NONE;
    hiddenMessages = this->mHiddenMessages;
    if (this->mAutosaveEnabled && this->mActiveCard.port == this->mAutosaveCard.port &&
        this->mActiveCard.slot == this->mAutosaveCard.slot)
    {
        hiddenMessages |= 0x100;
    }

    hiddenMessages |= this->mTaskManager->FilterGuidelinesMessage(message);

    if (this->mForceMessages != 0)
    {
        hiddenMessages &= ~this->mForceMessages;
    }

    const Realmc::Message::DetailInfo::Trc &trcMessage = message->info.trc;

    switch (trcMessage.mMsgId)
    {
    case 6:
        showMessage = (hiddenMessages & 0x1) == 0;
        break;

    case 7:
        showMessage = (hiddenMessages & 0x2) == 0;
        defaultResponse = Realmc::UMSG_OPTION1;
        break;

    case 9:
    case 14:
        showMessage = (hiddenMessages & 0x8) == 0;
        defaultResponse = Realmc::UMSG_OPTION1;
        break;

    case 8:
        showMessage = (hiddenMessages & 0x4) == 0;
        defaultResponse = Realmc::UMSG_OPTION1;
        break;

    case 19:
    case 23:
        showMessage = (hiddenMessages & 0x100) == 0;
        defaultResponse = Realmc::UMSG_OPTION2;
        break;

    case 24:
        showMessage = (hiddenMessages & 0x200) == 0;
        break;

    case 26:
        showMessage = (hiddenMessages & 0x400) == 0;
        defaultResponse = Realmc::UMSG_OPTION1;
        break;

    case 25:
        showMessage = (hiddenMessages & 0x800) == 0;
        defaultResponse = Realmc::UMSG_OPTION2;
        break;

    case 15:
        showMessage = (hiddenMessages & 0x2000) == 0;
        break;

    case 18:
        showMessage = (hiddenMessages & 0x4000) == 0;
        defaultResponse = Realmc::UMSG_OPTION1;
        break;

    case 16:
    case 17:
        showMessage = (hiddenMessages & 0x8000) == 0;
        defaultResponse = Realmc::UMSG_OPTION1;
        break;

    case 31:
        showMessage = (hiddenMessages & 0x10000) == 0;
        defaultResponse = Realmc::UMSG_OPTION2;
        break;

    case 32:
        showMessage = (hiddenMessages & 0x20000) == 0;
        break;

    case 34:
        showMessage = (hiddenMessages & 0x40000) == 0;
        defaultResponse = Realmc::UMSG_OPTION1;
        break;

    case 33:
        showMessage = (hiddenMessages & 0x80000) == 0;
        defaultResponse = Realmc::UMSG_OPTION1;
        break;

    default:
        showMessage = true;
        break;
    }

    if (showMessage)
    {
        this->_ShowGuidelinesMessage(&trcMessage);
    }
    else if (defaultResponse != Realmc::UMSG_NONE)
    {
        if (trcMessage.mNumOptions == 0)
        {
            this->mIMemcard->SendMessage(Realmc::UMSG_EXPIRE_DELAY, 0);
        }
        else
        {
            this->mIMemcard->SendMessage(defaultResponse, 0);
        }
    }
    else
    {
        this->mIMemcard->SendMessage(Realmc::UMSG_EXPIRE_DELAY, 0);
    }
}

void MemcardInterfaceImpl::_CheckForCardRemoval()
{
    int result;

    switch (sceMcSync(1, nullptr, &result))
    {
    case sceMcExecFinish:
        if (result != 0)
        {
            this->_DisableAutosave();
        }
        break;

    case sceMcExecRun:
        break;

    case sceMcExecIdle:
        sceMcGetInfo(this->mAutosaveCard.port, this->mAutosaveCard.slot, nullptr, nullptr, nullptr);
        break;
    }
}

void MemcardInterfaceImpl::_DisableAutosave()
{
    this->mAutosaveEnabled = false;
    static_cast<Realmc::PS2Interface *>(this->mIMemcard)->SetAutosaveEnabled(false);
    this->SetMessage(MESSAGE_SHOW, 0x100);
    this->mIGame->CardRemoved();
}

unsigned int MemcardInterfaceImpl::_CalcSignature(const void *data, unsigned int size)
{
    return RealmcUtils::Crc32(data, size);
}

unsigned int MemcardInterfaceImpl::CalcSaveSize(const SaveInfo *saveInfo, const DataFormat dataFormat)
{
    char dummy = '!';
    Realmc::DirectoryInfo dirInfo;
    Realmc::BlockCalculator *calculator;

    memset(&dirInfo, 0, 0x30);
    dirInfo.directoryName = &dummy;
    dirInfo.iconSysData = saveInfo->mPs2Info.mIconSysData;
    dirInfo.iconSysDataSize = saveInfo->mPs2Info.mIconSysDataSize;
    dirInfo.staticIconData = saveInfo->mPs2Info.mStaticIconData;
    dirInfo.staticIconDataSize = saveInfo->mPs2Info.mStaticIconDataSize;
    dirInfo.staticIconFilename = saveInfo->mPs2Info.mStaticIconFilename;
    dirInfo.copyIconData = saveInfo->mPs2Info.mCopyIconData;
    dirInfo.copyIconDataSize = saveInfo->mPs2Info.mCopyIconDataSize;
    dirInfo.copyIconFilename = saveInfo->mPs2Info.mCopyIconFilename;
    dirInfo.deleteIconData = saveInfo->mPs2Info.mDeleteIconData;
    dirInfo.deleteIconDataSize = saveInfo->mPs2Info.mDeleteIconDataSize;
    dirInfo.deleteIconFilename = saveInfo->mPs2Info.mDeleteIconFilename;

    calculator = static_cast<Realmc::BlockCalculator *>(this->mIMemcard->GetBlockCalculator());
    calculator->Clear();
    calculator->SetDirectoryInfo(this->mActiveCard, dirInfo);

    if (dataFormat == FORMAT_LAYER2)
    {
        calculator->AddDataFile(&dummy, (saveInfo->mHeaderSize + sizeof(FileHeader)) + saveInfo->mBodySize);
    }
    else
    {
        calculator->AddDataFile(&dummy, saveInfo->mBodySize);
    }

    return calculator->GetResult();
}

void MemcardInterfaceImpl::ConvertAsciiToSjisString(const char *asciiStr, wchar_t *sjisStr)
{
    this->mIMemcard->GetSjisInterface()->Ascii2SjisString((const unsigned char *)asciiStr, (unsigned short *)sjisStr);
}

void MemcardInterfaceImpl::SetBootFlow(bool isBootFlow)
{
    static_cast<Realmc::PS2Interface *>(this->mIMemcard)->SetBootFlow(isBootFlow);
}

} // namespace RealmcIface
