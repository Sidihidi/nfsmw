#ifndef REALMEMCARD_PS2_TASK_H
#define REALMEMCARD_PS2_TASK_H

#ifdef EA_PRAGMA_ONCE_SUPPORTED
#pragma once
#endif

// Las tareas de la capa baja de PS2 (source/lib/ps2/ps2_task.h en el .mdebug de la A124). Ninguna redefine una funcion
// virtual, asi que todas comparten la tabla virtual de Ps2Task (los constructores guardan _vt$Q26Realmc7Ps2Task). Cada
// Start llama a Ps2Task::Init sin pasar por la tabla y termina entregandose al gestor de tareas.

#include <stdio.h>
#include <string.h>

#include <libmc.h>

#include "../../../include/common/realmemcard/driver.h"
#include "../../../include/common/realmemcard/impl/interfaceimp.h"
#include "ps2_blockcalculator.h"

namespace Realmc {

struct Ps2FileDescriptor : public CmnFileDescriptor {
    Ps2FileDescriptor() {
        this->Clear();
    }

    virtual void Clear() {
        this->CmnFileDescriptor::Clear();
        this->mSceFileMode = 0;
        this->mSceFileHandle = 0;
        this->mBytesRead = 0;
        this->mBytesWritten = 0;
        this->mSeekPosition = 0;
    }

    virtual void Init(const CardID &cardID, const FileInfo &fileInfo, FileOpenMode mode) {
        this->Clear();
        this->CmnFileDescriptor::Init(cardID, fileInfo, mode);
    }

    int mSceFileMode;
    int mSceFileHandle;
    int mBytesRead;
    int mBytesWritten;
    unsigned int mSeekPosition;
    char mPath[128];
};

Ps2FileDescriptor *ConvertFileHandleToDescriptor(OpenFileDescriptor *fileHandle);

struct Ps2Task : public Task {
    Ps2Task(TaskID id, TaskManager *parent)
        : Task(id, parent) {
        this->Init(false);
    }

    virtual void Init(bool notifyUser) {
        this->mCardID.port = 0;
        this->mCardID.slot = 0;
        this->mTaskResult = RESULT_UNKNOWN;
        this->mCardStatus = STATUS_UNKNOWN;
        this->mNotifyUser = notifyUser;
        this->mState = TS_START;
    }

    void End() {
        this->mParent->EndTask(this);
    }

    CardID mCardID;
    TaskResult mTaskResult;
    CardStatus mCardStatus;
    bool mNotifyUser;
};

struct TaskTrcStartGame : public Ps2Task {
    TaskTrcStartGame(TaskManager *parent)
        : Ps2Task(TID_TRC_STARTGAME, parent)
        , mInfo() {
        memset(&this->mInfo, 0, sizeof(this->mInfo));
        this->mEnableAutosave = false;
        this->mSpaceNeeded = 0;
    }

    void Start(const StartGameInfo *info) {
        this->Ps2Task::Init(true);
        this->mCardID.port = 0;
        this->mCardID.slot = 0;
        this->mInfo = *info;
        this->mEnableAutosave = info->autoSave;
        this->mSpaceNeeded = info->totalBlocksNeeded;

        this->mParent->StartTask(this);
    }

    StartGameInfo mInfo;
    bool mEnableAutosave;
    int mSpaceNeeded;
};

struct TaskTrcGetCardInfo : public Ps2Task {
    TaskTrcGetCardInfo(TaskManager *parent)
        : Ps2Task(TID_TRC_GETCARDINFO, parent) {}

    void Start(const CardID &cID, TaskID taskID) {
        this->Ps2Task::Init(true);
        this->mCardID = cID;
        this->mTaskID = taskID;

        this->mParent->StartTask(this);
    }

    TaskID mTaskID;
};

struct TaskTrcSaveFile : public Ps2Task {
    TaskTrcSaveFile(TaskManager *parent)
        : Ps2Task(TID_TRC_SAVEFILE, parent)
        , mFileInfo() {
        memset(&this->mFileInfo, 0, sizeof(this->mFileInfo));
        this->mSpaceNeeded = 0;
        this->mFileDescriptor = nullptr;
        this->mConfirmedByUser = false;
        this->mIsSaving = false;
    }

    void Start(CardID cID, const FileInfo *finfo, SaveTaskType saveTaskType, unsigned int nBlocksNeeded) {
        this->Ps2Task::Init(true);
        this->mCardID = cID;
        this->mFileInfo = *finfo;
        this->mFileDescriptor = nullptr;
        this->mSpaceNeeded = nBlocksNeeded;
        this->mConfirmedByUser = false;
        this->mIsSaving = false;
        if (saveTaskType == SAVETASK_SAVE) {
            this->mID = TID_TRC_SAVEFILE;
        } else {
            this->mID = TID_TRC_SAVECHECK;
        }

        this->mParent->StartTask(this);
    }

    FileInfo mFileInfo;
    int mSpaceNeeded;
    Ps2FileDescriptor *mFileDescriptor;
    bool mConfirmedByUser;
    bool mIsSaving;
};

struct TaskTrcLoadFile : public Ps2Task {
    TaskTrcLoadFile(TaskManager *parent)
        : Ps2Task(TID_TRC_LOADFILE, parent)
        , mFileInfo() {
        memset(&this->mFileInfo, 0, sizeof(this->mFileInfo));
        memset(&this->mIconSys, 0, sizeof(this->mIconSys));
        this->mFileDescriptor = nullptr;
        this->mIsLoading = false;
        this->mIconSysSize = 0;
    }

    void Start(CardID cID, const FileInfo *finfo) {
        this->Ps2Task::Init(true);
        memset(&this->mIconSys, 0, sizeof(this->mIconSys));
        this->mCardID = cID;
        this->mFileInfo = *finfo;
        this->mFileDescriptor = nullptr;
        this->mIsLoading = false;
        this->mIconSysSize = 0;

        this->mParent->StartTask(this);
    }

    FileInfo mFileInfo;
    Ps2FileDescriptor *mFileDescriptor;
    bool mIsLoading;
    sceMcIconSys mIconSys;
    unsigned int mIconSysSize;
};

struct TaskTrcDeleteFile : public Ps2Task {
    TaskTrcDeleteFile(TaskManager *parent)
        : Ps2Task(TID_TRC_DELETEFILE, parent)
        , mFileInfo() {
        memset(&this->mFileInfo, 0, sizeof(this->mFileInfo));
        this->mDeleteDirectory = false;
        this->mCardFoundOnEntry = false;
        this->mIsDeleting = false;
    }

    void Start(CardID cID, const FileInfo *finfo) {
        this->Ps2Task::Init(true);
        this->mCardID = cID;
        this->mFileInfo = *finfo;
        this->mDeleteDirectory = false;
        this->mCardFoundOnEntry = false;
        this->mIsDeleting = false;

        this->mParent->StartTask(this);
    }

    FileInfo mFileInfo;
    bool mDeleteDirectory;
    bool mCardFoundOnEntry;
    bool mIsDeleting;
};

struct TaskTrcList : public Ps2Task {
    TaskTrcList(TaskManager *parent)
        : Ps2Task(TID_TRC_LIST, parent)
        , mFileInfo() {
        memset(&this->mFileInfo, 0, sizeof(this->mFileInfo));
        this->mListType = LISTTASK_ENTRIES;
        this->mFileFound = false;
        this->mCurEntry = 0;
        this->mNumEntries = 0;
        this->mEntries = nullptr;
    }

    void Start(CardID cID, const FileInfo *finfo, ListTaskType listType) {
        this->Ps2Task::Init(true);
        this->mCardID = cID;
        this->mFileInfo = *finfo;
        this->mListType = listType;
        this->mFileFound = false;
        this->mCurEntry = 0;
        this->mNumEntries = 0;
        this->mEntries = nullptr;

        this->mParent->StartTask(this);
    }

    FileInfo mFileInfo;
    ListTaskType mListType;
    bool mFileFound;
    uint32_t mCurEntry;
    uint32_t mNumEntries;
    sceMcTblGetDir *mEntries;
};

struct TaskTrcListEntries : public Ps2Task {
    TaskTrcListEntries(TaskManager *parent)
        : Ps2Task(TID_TRC_LISTENTRIES, parent) {}

    void Start() {
        this->Ps2Task::Init(true);

        this->mParent->StartTask(this);
    }
};

struct TaskTrcListFiles : public Ps2Task {
    TaskTrcListFiles(TaskManager *parent)
        : Ps2Task(TID_TRC_LISTFILES, parent) {
        this->mFindNext = false;
    }

    void Start() {
        this->Ps2Task::Init(true);
        this->mFindNext = false;

        this->mParent->StartTask(this);
    }

    bool mFindNext;
    char mPath[73];
};

struct TaskStartGameCommon : public Ps2Task {
    TaskStartGameCommon(TaskManager *parent)
        : Ps2Task(TID_STARTGAME_COMMON, parent)
        , mFirstCardId() {
        this->mNewFileSpaceNeeded = 0;
        this->mActualSpaceNeeded = 0;
    }

    void Start(CardID cID, int spaceNeeded) {
        int port;
        int slot;

        this->Ps2Task::Init(true);
        this->mCardID = cID;
        this->mFirstCardId.port = 0;
        this->mFirstCardId.slot = 0;
        this->mNewFileSpaceNeeded = spaceNeeded;
        this->mActualSpaceNeeded = spaceNeeded;

        for (port = 0; port < 2; port++) {
            for (slot = 0; slot < 4; slot++) {
                this->mSlotStatus[port][slot] = STATUS_UNKNOWN;
                this->mEntryFound[port][slot] = false;
            }
        }

        this->mParent->StartTask(this);
    }

    void RecordCardState(CardID cID, CardStatus status, bool entryFound) {
        this->mSlotStatus[cID.port][cID.slot] = status;
        this->mEntryFound[cID.port][cID.slot] = entryFound;
    }

    int mNewFileSpaceNeeded;
    int mActualSpaceNeeded;
    CardStatus mSlotStatus[2][4];
    bool mEntryFound[2][4];
    CardID mFirstCardId;
};

struct TaskGetCardInfo : public Ps2Task {
    TaskGetCardInfo(TaskManager *parent)
        : Ps2Task(TID_GET_CARD_INFO, parent) {
        this->mType = 0;
        this->mFreeSpace = 0;
        this->mFormatted = 0;
        this->mRetry = 0;
        this->mCardFound = false;
    }

    void Start(CardID cardID, TaskID proxyTaskID, bool notifyUser) {
        this->Ps2Task::Init(notifyUser);
        this->mCardID = cardID;
        this->mProxyTaskID = proxyTaskID;
        this->mType = 0;
        this->mFreeSpace = 0;
        this->mFormatted = 0;
        this->mRetry = 0;
        this->mCardFound = false;

        this->mParent->StartTask(this);
    }

    TaskID mProxyTaskID;
    int mType;
    int mFreeSpace;
    int mFormatted;
    int mRetry;
    bool mCardFound;
};

struct TaskCreateDirectory : public Ps2Task {
    TaskCreateDirectory(TaskManager *parent)
        : Ps2Task(TID_CREATE_DIRECTORY, parent) {
        memset(this->mDirectoryName, 0, sizeof(this->mDirectoryName));
    }

    void Start(CardID cID, const char *directoryName, bool notifyUser) {
        this->Ps2Task::Init(notifyUser);
        this->mCardID = cID;
        strncpy(this->mDirectoryName, directoryName, sizeof(this->mDirectoryName));

        this->mParent->StartTask(this);
    }

    char mDirectoryName[36];
};

struct TaskOpen : public Ps2Task {
    TaskOpen(TaskManager *parent)
        : Ps2Task(TID_OPEN, parent) {
        memset(this->mDirectoryName, 0, sizeof(this->mDirectoryName));
        memset(this->mFileName, 0, sizeof(this->mFileName));
        this->mMode = FOM_READ;
        this->mAttribute = FA_NONE;
        this->mFileDescriptor = nullptr;
    }

    void Start(CardID cID, const char *directoryName, const char *fileName, FileOpenMode mode, bool notifyUser) {
        this->Ps2Task::Init(notifyUser);
        this->mCardID = cID;
        strncpy(this->mDirectoryName, directoryName, sizeof(this->mDirectoryName));
        strncpy(this->mFileName, fileName, sizeof(this->mFileName));
        this->mMode = mode;
        this->mAttribute = FA_NONE;
        this->mFileDescriptor = nullptr;

        this->mParent->StartTask(this);
    }

    char mDirectoryName[36];
    char mFileName[36];
    FileOpenMode mMode;
    FileAttribute mAttribute;
    Ps2FileDescriptor *mFileDescriptor;
};

struct TaskClose : public Ps2Task {
    TaskClose(TaskManager *parent)
        : Ps2Task(TID_CLOSE, parent) {
        this->mFileDescriptor = nullptr;
    }

    void Start(OpenFileDescriptor *fileHandle, bool notifyUser) {
        this->Ps2Task::Init(notifyUser);
        this->mFileDescriptor = ConvertFileHandleToDescriptor(fileHandle);

        this->mParent->StartTask(this);
    }

    Ps2FileDescriptor *mFileDescriptor;
};

struct TaskSeek : public Ps2Task {
    TaskSeek(TaskManager *parent)
        : Ps2Task(TID_SEEK, parent) {
        this->mFileDescriptor = nullptr;
        this->mOffset = 0;
        this->mSeekFrom = SF_SET;
    }

    void Start(OpenFileDescriptor *fileHandle, int offset, SeekFrom from, bool notifyUser) {
        this->Ps2Task::Init(notifyUser);
        this->mFileDescriptor = ConvertFileHandleToDescriptor(fileHandle);
        this->mOffset = offset;
        this->mSeekFrom = from;

        this->mParent->StartTask(this);
    }

    Ps2FileDescriptor *mFileDescriptor;
    int mOffset;
    SeekFrom mSeekFrom;
};

struct TaskRead : public Ps2Task {
    TaskRead(TaskManager *parent)
        : Ps2Task(TID_READ, parent) {
        this->mFileDescriptor = nullptr;
        this->mBuffer = nullptr;
        this->mBufferSize = 0;
        this->mBytesRead = 0;
    }

    void Start(OpenFileDescriptor *fileHandle, void *buffer, int bufsize, bool notifyUser) {
        this->Ps2Task::Init(true);
        this->mFileDescriptor = ConvertFileHandleToDescriptor(fileHandle);
        this->mBuffer = buffer;
        this->mBufferSize = bufsize;
        this->mBytesRead = 0;
        this->mNotifyUser = notifyUser;

        this->mParent->StartTask(this);
    }

    Ps2FileDescriptor *mFileDescriptor;
    void *mBuffer;
    int mBufferSize;
    int mBytesRead;
};

struct TaskWrite : public Ps2Task {
    TaskWrite(TaskManager *parent)
        : Ps2Task(TID_WRITE, parent) {
        this->mFileDescriptor = nullptr;
        this->mBuffer = nullptr;
        this->mBufferSize = 0;
        this->mBytesWritten = 0;
    }

    void Start(OpenFileDescriptor *fileHandle, void *buffer, int bufsize) {
        this->Ps2Task::Init(true);
        this->mFileDescriptor = ConvertFileHandleToDescriptor(fileHandle);
        this->mBuffer = buffer;
        this->mBufferSize = bufsize;
        this->mBytesWritten = 0;

        this->mParent->StartTask(this);
    }

    Ps2FileDescriptor *mFileDescriptor;
    void *mBuffer;
    int mBufferSize;
    int mBytesWritten;
};

struct TaskFlush : public Ps2Task {
    TaskFlush(TaskManager *parent)
        : Ps2Task(TID_FLUSH, parent) {
        this->mFileDescriptor = nullptr;
    }

    void Start(OpenFileDescriptor *fileHandle, bool notifyUser) {
        this->Ps2Task::Init(notifyUser);
        this->mFileDescriptor = ConvertFileHandleToDescriptor(fileHandle);

        this->mParent->StartTask(this);
    }

    Ps2FileDescriptor *mFileDescriptor;
};

struct TaskDelete : public Ps2Task {
    TaskDelete(TaskManager *parent)
        : Ps2Task(TID_DELETE, parent) {
        memset(this->mPath, 0, sizeof(this->mPath));
        this->mDeleteDirectory = false;
    }

    void Start(CardID cID, const char *directoryName, const char *fileName, bool notifyUser) {
        this->Ps2Task::Init(notifyUser);
        this->mCardID = cID;
        sprintf(this->mPath, "/%s", directoryName);
        if (fileName != nullptr && fileName[0] != '\0') {
            sprintf(&this->mPath[strlen(this->mPath)], "/%s", fileName);
            this->mDeleteDirectory = false;
        } else {
            this->mDeleteDirectory = true;
        }

        this->mParent->StartTask(this);
    }

    char mPath[80];
    bool mDeleteDirectory;
};

struct TaskFormat : public Ps2Task {
    TaskFormat(TaskManager *parent)
        : Ps2Task(TID_FORMAT, parent) {}

    void Start(CardID cID, bool notifyUser) {
        this->Ps2Task::Init(notifyUser);
        this->mCardID = cID;

        this->mParent->StartTask(this);
    }
};

struct TaskSetAttribute : public Ps2Task {
    TaskSetAttribute(TaskManager *parent)
        : Ps2Task(TID_SET_ATTRIBUTES, parent) {
        memset(this->mDirectoryName, 0, sizeof(this->mDirectoryName));
        memset(this->mFileName, 0, sizeof(this->mFileName));
        this->mAttribute = FA_NONE;
    }

    void Start(CardID cID, const char *directoryName, const char *fileName, FileAttribute attribute, bool notifyUser) {
        this->Ps2Task::Init(notifyUser);
        this->mCardID = cID;
        strncpy(this->mDirectoryName, directoryName, sizeof(this->mDirectoryName));
        strncpy(this->mFileName, fileName, sizeof(this->mFileName));
        this->mAttribute = attribute;

        this->mParent->StartTask(this);
    }

    char mDirectoryName[36];
    char mFileName[36];
    FileAttribute mAttribute;
};

struct TaskCheckSpace : public Ps2Task {
    TaskCheckSpace(TaskManager *parent)
        : Ps2Task(TID_CHECK_SPACE, parent)
        , mFileInfo() {
        this->Clear();
    }

    void Start(CardID cID, StartGameInfo *info, unsigned int freeCardSpace, bool notifyUser) {
        this->Ps2Task::Init(notifyUser);
        this->Clear();
        this->mCardID = cID;
        this->mFileInfo = info->fileInfo;
        this->mTotalBlocksRequired = info->totalBlocksNeeded;
        this->mFreeCardSpace = freeCardSpace;

        this->mParent->StartTask(this);
    }

    void Start(CardID cID, FileInfo *fileInfo, unsigned int freeCardSpace, bool notifyUser) {
        this->Ps2Task::Init(notifyUser);
        this->Clear();
        this->mCardID = cID;
        this->mFileInfo = *fileInfo;
        this->mFreeCardSpace = freeCardSpace;
        this->mSpaceNeeded = 0;

        BlockCalculatorImp *blockCalculator = (BlockCalculatorImp *)gInterface->GetBlockCalculator();
        this->mTotalBlocksRequired = blockCalculator->CalculateBlockSize(cID, *fileInfo);

        this->mParent->StartTask(this);
    }

    void Start(CardID cID, FileInfo *fileInfo, unsigned int freeCardSpace, unsigned int totalBlocksRequired,
               bool notifyUser) {
        this->Ps2Task::Init(notifyUser);
        this->Clear();
        this->mCardID = cID;
        this->mFileInfo = *fileInfo;
        this->mFreeCardSpace = freeCardSpace;
        this->mSpaceNeeded = 0;
        this->mTotalBlocksRequired = totalBlocksRequired;

        this->mParent->StartTask(this);
    }

    void Clear();

    FileInfo mFileInfo;
    unsigned int mTotalBlocksRequired;
    unsigned int mFreeCardSpace;
    unsigned int mFileBlockSizeFound;
    unsigned int mExistingFileBlockSize;
    unsigned int mExtraBlocksNeeded;
    unsigned int mSpaceNeeded;
    int mNumEntries;
    int mCurEntry;
    sceMcTblGetDir *mEntries;
    bool mEntryFound;
    bool mFileExists;
};

// Definida detras de la clase: el constructor y los Start la llaman sin expandirla (jal en el original).
inline void TaskCheckSpace::Clear() {
    memset(&this->mFileInfo, 0, sizeof(this->mFileInfo));
    this->mTotalBlocksRequired = 0;
    this->mFileBlockSizeFound = 0;
    this->mExistingFileBlockSize = 0;
    this->mEntryFound = false;
    this->mFileExists = false;
    this->mExtraBlocksNeeded = 0;
    this->mSpaceNeeded = 0;
    this->mNumEntries = 0;
    this->mCurEntry = 0;
    this->mEntries = nullptr;
}

struct TaskSaveFile : public Ps2Task {
    TaskSaveFile(TaskManager *parent)
        : Ps2Task(TID_SAVE_FILE, parent) {
        memset(this->mDirectoryName, 0, sizeof(this->mDirectoryName));
        memset(this->mFileName, 0, sizeof(this->mFileName));
        this->mData = nullptr;
        this->mDataSize = 0;
        this->mFileDescriptor = nullptr;
        this->mBytesWritten = 0;
    }

    void Start(CardID cID, const char *directoryName, const char *fileName, const void *data, int dataSize) {
        this->Ps2Task::Init(false);
        this->mCardID = cID;
        strncpy(this->mDirectoryName, directoryName, sizeof(this->mDirectoryName));
        strncpy(this->mFileName, fileName, sizeof(this->mFileName));
        this->mData = const_cast<void *>(data);
        this->mDataSize = dataSize;
        this->mFileDescriptor = nullptr;
        this->mBytesWritten = 0;

        this->mParent->StartTask(this);
    }

    char mDirectoryName[36];
    char mFileName[36];
    void *mData;
    int mDataSize;
    Ps2FileDescriptor *mFileDescriptor;
    int mBytesWritten;
};

struct TaskFind : public Ps2Task {
    TaskFind(TaskManager *parent)
        : Ps2Task(TID_FIND, parent) {
        memset(this->mDirectoryName, 0, sizeof(this->mDirectoryName));
        memset(this->mFileName, 0, sizeof(this->mFileName));
        this->mFileSize = 0;
        this->mNumFilesFound = 0;
    }

    void Start(CardID cID, const char *directoryName, const char *fileName, bool notifyUser) {
        this->Ps2Task::Init(notifyUser);
        this->mCardID = cID;
        strncpy(this->mDirectoryName, directoryName, sizeof(this->mDirectoryName));
        strncpy(this->mFileName, fileName, sizeof(this->mFileName));
        this->mFileSize = 0;
        this->mNumFilesFound = 0;

        this->mParent->StartTask(this);
    }

    char mDirectoryName[36];
    char mFileName[36];
    char mPath[76];
    int mFileSize;
    int mNumFilesFound;
};

struct TaskCheckDirectory : public Ps2Task {
    TaskCheckDirectory(TaskManager *parent)
        : Ps2Task(TID_CHECK_DIRECTORY, parent) {
        memset(this->mDirectoryName, 0, sizeof(this->mDirectoryName));
    }

    void Start(CardID cID, const char *directoryName) {
        this->Ps2Task::Init(false);
        this->mCardID = cID;
        strncpy(this->mDirectoryName, directoryName, sizeof(this->mDirectoryName));

        this->mParent->StartTask(this);
    }

    char mDirectoryName[36];
};

struct TaskChangeDirectory : public Ps2Task {
    TaskChangeDirectory(TaskManager *parent)
        : Ps2Task(TID_CHANGE_DIRECTORY, parent) {
        memset(this->mDirectoryName, 0, sizeof(this->mDirectoryName));
    }

    void Start(CardID cID, const char *directoryName, bool notifyUser) {
        this->Ps2Task::Init(notifyUser);
        this->mCardID = cID;
        strncpy(this->mDirectoryName, directoryName, sizeof(this->mDirectoryName));

        this->mParent->StartTask(this);
    }

    char mDirectoryName[36];
};

} // namespace Realmc

#endif
