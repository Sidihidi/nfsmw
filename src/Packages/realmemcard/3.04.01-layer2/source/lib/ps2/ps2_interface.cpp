#include <stdio.h>
#include <string.h>

#include <libmc.h>
#include <sifdev.h>

#include "ps2_interface.h"

namespace Realmc {

// El multitap (mtapman) es opcional: el juego pone estas funciones si lo enlaza (Interface::InitMultitap elige puertos).
int (*gMCMTapInit)() = nullptr;
int (*gMCMTapPortOpen)(int) = nullptr;
int (*gMCMTapPortClose)(int) = nullptr;
int (*gMCMTapGetConnection)(int) = nullptr;
bool gMCMTapUsePort[2] = {true, true};

const char *PARENT_DIRECTORY_NAME = "..";
const char *CURRENT_DIRECTORY_NAME = ".";
const char *ROOT_DIRECTORY_NAME = "..";
const char *FILENAME_ALL_FILES = "*";
const int MAX_FILES_PER_DIRECTORY = 18;
const int PS2_CLUSTER_SIZE = 1024;
const int NUM_CLUSTERS_PER_DIRECTORY_TABLE = 2;

static const wchar_t SLOT_PREFIX_NONE[5] = {'N', 'o', 'n', 'e', 0};

Interface *Interface::CreateInstance(const SystemInterface &iSystem) {
    SetMemAllocator(iSystem.mAllocator);

    if (gInterface == nullptr) {
        gInterface = new PS2Interface(iSystem);
    } else {
        gInterface->AddRef();
    }

    return gInterface;
}

PS2Interface::PS2Interface(const SystemInterface &iSystem)
    : InterfaceImp(iSystem)
    , TaskManager()
    , mTaskTrcStartGame(this)
    , mTaskTrcGetCardInfo(this)
    , mTaskTrcSaveFile(this)
    , mTaskTrcLoadFile(this)
    , mTaskTrcDeleteFile(this)
    , mTaskTrcList(this)
    , mTaskTrcListEntries(this)
    , mTaskTrcListFiles(this)
    , mTaskStartGameCommon(this)
    , mTaskGetCardInfo(this)
    , mTaskCreateDirectory(this)
    , mTaskChangeDirectory(this)
    , mTaskOpen(this)
    , mTaskClose(this)
    , mTaskDelete(this)
    , mTaskSeek(this)
    , mTaskRead(this)
    , mTaskWrite(this)
    , mTaskFlush(this)
    , mTaskSetAttribute(this)
    , mTaskFind(this)
    , mTaskFormat(this)
    , mTaskCheckSpace(this)
    , mTaskSaveFile(this)
    , mTaskCheckDirectory(this)
    , mNewMsg(false)
    , mGameTitle(nullptr)
    , mIsAutosaveEnabled(false)
    , mIsBootFlow(false) {
    // Los modulos del IOP que la tarjeta necesita tienen que estar cargados antes.
    this->CheckIopModule("sio2man");
    this->CheckIopModule("padman");
    this->CheckIopModule("mcman");
    this->CheckIopModule("mcserv");

    sceMcInit();

    if (gMCMTapInit != nullptr) {
        this->CheckIopModule("multitap_manager");
        gMCMTapInit();

        if (gMCMTapUsePort[0]) {
            gMCMTapPortOpen(2);
        }

        if (gMCMTapUsePort[1]) {
            gMCMTapPortOpen(3);
        }
    }

    this->mMsg.Init();
    this->mResultMsg.Clear();
    this->mUserMsg = UMSG_NONE;
    this->mMsgTimer.Init(3);

    this->mSceAsyncResult = 0;
    this->mCardStatus = STATUS_OK;
    this->mNumFilesFound = 0;
    this->mCurrentFileDescriptor = nullptr;
    this->mNoCheckingCardWarning = false;
    this->mCurrentTask = nullptr;
    this->mBlockCalculator.SetBlockSize(this->GetBlockSize(CardID(0, 0)));
}

PS2Interface::~PS2Interface() {
    if (gMCMTapPortClose != nullptr) {
        gMCMTapPortClose(2);
        gMCMTapPortClose(3);
    }

    sceMcEnd();
}

// El texto del puerto para los mensajes: el prefijo de Locale ("Multitap" o nada) y el puerto en ASCII, en UCS-2.
char *PS2Interface::PortStr(CardID cardID) {
    static char pstr[16];
    const wchar_t *prefix;
    unsigned int lenPrefix;

    memset(pstr, 0, sizeof(pstr));

    if (this->MultitapConnected(cardID.port)) {
        prefix = Locale::GetString(37, 0);
    } else {
        prefix = Locale::GetString(36, 0);
    }

    lenPrefix = 0;
    if (memcmp(prefix, SLOT_PREFIX_NONE, sizeof(SLOT_PREFIX_NONE)) != 0) {
        lenPrefix = Locale::GetWstrLength(prefix);
        memcpy(pstr, prefix, lenPrefix * sizeof(wchar_t));
    }

    Locale::ConvertAsciiToUnicode(this->PortStrAscii(cardID),
                                  reinterpret_cast<unsigned short *>(&pstr[lenPrefix * sizeof(wchar_t)]));

    return pstr;
}

// "1", "2" o, con multitap, "1-A".."2-D".
char *PS2Interface::PortStrAscii(CardID cardID) {
    static char pstr[4];

    memset(pstr, 0, sizeof(pstr));
    pstr[0] = '1' + cardID.port;

    if (this->MultitapConnected(cardID.port)) {
        pstr[1] = '-';
        pstr[2] = 'A' + cardID.slot;
    }

    return pstr;
}

void PS2Interface::CheckIopModule(char *moduleName) {
    int moduleId = sceSifSearchModuleByName(moduleName);

    if (moduleId < 0) {
        char msg[512];

        sprintf(msg, "Error: IOP module missing: %s. You must load this module before initilizing the memcard library.\n",
                moduleName);
    }
}

void PS2Interface::CheckName(const char *str) {
    int i;

    if (str != nullptr) {
        for (i = 0; str[i] != '\0'; i++) {
        }
    }
}

void PS2Interface::CheckFileInfo(const FileInfo *) {}

bool PS2Interface::CheckSyncResult() {
    bool busy;
    int cmd = 0;
    int state;

    busy = false;
    state = sceMcSync(1, &cmd, &this->mSceAsyncResult);
    if (state != sceMcExecFinish) {
        if (state == sceMcExecRun) {
            busy = true;
            this->mCardStatus = STATUS_UNKNOWN;
        }
        return busy;
    }

    this->mCardStatus = this->SceResultToStatus(cmd, this->mSceAsyncResult);
    return busy;
}

void PS2Interface::CheckSceResult(int) {}

// El resultado de una llamada de libmc (sceMcSync) como estado de la tarjeta; de paso anota en el descriptor en curso
// lo que ha devuelto la operacion (el manejador al abrir, la posicion, los bytes leidos o escritos).
CardStatus PS2Interface::SceResultToStatus(int cmd, int result) {
    CardStatus status = STATUS_OK;

    if (result < 0) {
        switch (result) {
        case -1:
            status = STATUS_CARD_CHANGED;
            break;
        case -2:
            status = STATUS_CARD_UNFORMATTED;
            break;
        case -3:
            status = STATUS_CARD_FULL;
            break;
        case -4:
            status = STATUS_ENTRY_NOT_FOUND;
            break;
        case -5:
            status = STATUS_ACCESS_DENIED;
            break;
        case -6:
            status = STATUS_DIRECTORY_NOT_EMPTY;
            break;
        case -7:
            status = STATUS_TOO_MANY_OPENED_FILES;
            break;
        case -8:
            status = STATUS_FAILED;
            break;
        default:
            if (result < -9) {
                status = STATUS_NO_CARD;
            } else {
                status = STATUS_FAILED;
            }
            break;
        }
    }

    switch (cmd) {
    case SCE_MC_FUNC_GETINFO:
        break;

    case SCE_MC_FUNC_OPEN:
        if (result >= 0) {
            this->mCurrentFileDescriptor->mSceFileHandle = result;
            this->mCurrentFileDescriptor->mOpen = true;
        } else {
            this->mCurrentFileDescriptor->mSceFileHandle = 0;
            this->mCurrentFileDescriptor->mOpen = false;
            switch (result) {
            case -2:
                status = STATUS_CARD_UNFORMATTED;
                break;
            case -3:
                status = STATUS_CARD_FULL;
                break;
            case -4:
                status = STATUS_FILE_NOT_FOUND;
                break;
            case -5:
                if (this->mCurrentFileDescriptor->mSceFileMode & SCE_CREAT) {
                    status = STATUS_ENTRY_ALREADY_EXISTS;
                }
                break;
            case -7:
                status = STATUS_TOO_MANY_OPENED_FILES;
                break;
            default:
                status = STATUS_NO_CARD;
                break;
            }
        }
        break;

    case SCE_MC_FUNC_CLOSE:
        if (result == -4) {
            status = STATUS_FILE_NOT_OPENED;
        }
        break;

    case SCE_MC_FUNC_SEEK:
        if (result >= 0) {
            this->mCurrentFileDescriptor->mSeekPosition = result;
        } else {
            if (result == -4) {
                status = STATUS_FILE_NOT_OPENED;
            }
            this->mCurrentFileDescriptor->mSeekPosition = 0;
        }
        break;

    case SCE_MC_FUNC_READ:
        if (result >= 0) {
            this->mCurrentFileDescriptor->mBytesRead = result;
        } else {
            this->mCurrentFileDescriptor->mBytesRead = 0;
            switch (result) {
            case -3:
                status = STATUS_FILE_CORRUPTED;
                break;
            case -4:
                status = STATUS_FILE_NOT_OPENED;
                break;
            }
        }
        break;

    case SCE_MC_FUNC_WRITE:
        if (result >= 0) {
            this->mCurrentFileDescriptor->mBytesWritten = result;
        } else {
            this->mCurrentFileDescriptor->mBytesWritten = 0;
            switch (result) {
            case -3:
                status = STATUS_INSUFFICIENT_SPACE;
                break;
            case -4:
                status = STATUS_FILE_NOT_OPENED;
                break;
            case -5:
                status = STATUS_NO_CARD;
                break;
            }
        }
        break;

    case SCE_MC_FUNC_FLUSH:
        switch (result) {
        case -4:
            status = STATUS_FILE_NOT_OPENED;
            break;
        case -5:
            status = STATUS_ACCESS_DENIED;
            break;
        }
        break;

    case SCE_MC_FUNC_MKDIR:
        switch (result) {
        case -3:
            status = STATUS_INSUFFICIENT_SPACE;
            break;
        case -4:
            status = STATUS_ENTRY_ALREADY_EXISTS;
            break;
        case -5:
            status = STATUS_ACCESS_DENIED;
            break;
        }
        break;

    case SCE_MC_FUNC_CHDIR:
        break;

    case SCE_MC_FUNC_GETDIR:
        this->mNumFilesFound = result >= 0 ? result : 0;
        break;

    case SCE_MC_FUNC_SETFILEINFO:
    case SCE_MC_FUNC_DELETE:
    case SCE_MC_FUNC_FORMAT:
    case SCE_MC_FUNC_UNFORMAT:
    case SCE_MC_FUNC_GETENTSPACE:
    case SCE_MC_FUNC_RENAME:
    case SCE_MC_FUNC_CHANGETHREADPRIORITY:
        break;
    }

    return status;
}

CardStatus PS2Interface::CheckCard(CardID cid, int *type, int *freeSpace, int *formatted) {
    int cType;

    this->mCardStatus = STATUS_UNKNOWN;
    cType = 0;
    this->CheckSceResult(sceMcGetInfo(cid.port, cid.slot, &cType, freeSpace, formatted));

    while (this->CheckSyncResult() == true) {
        this->mISystem.mThread->Sleep(1);
    }

    if (cType != sceMcTypePS2) {
        this->mCardStatus = STATUS_NO_CARD;
    }

    if (type != nullptr) {
        *type = cType;
    }

    return this->mCardStatus;
}

bool PS2Interface::MultitapConnected(int logicalport) {
    if (gMCMTapGetConnection != nullptr && gMCMTapUsePort[logicalport]) {
        if (gMCMTapGetConnection(logicalport + 2) == 1) {
            return true;
        }
    }

    return false;
}

// La siguiente ranura valida despues de curCardId (o la primera, con firstId): los identificadores son los bits de
// checkCardID.port de la tarea de arranque, cuatro por puerto; las ranuras 2-4 solo cuentan con multitap.
bool PS2Interface::GetNextCardId(CardID &curCardId, bool firstId) {
    CardID nextCardID(0, 0);
    int currentCardId = (1 << (curCardId.port * 4)) << curCardId.slot;
    int validCardIds = this->mTaskTrcStartGame.mInfo.checkCardID.port;
    bool done;
    bool foundNext;

    if (!firstId) {
        nextCardID = curCardId;
        validCardIds &= ~currentCardId;
    }

    foundNext = false;
    do {
        while (currentCardId & ~validCardIds) {
            if (++nextCardID.slot >= 4) {
                nextCardID.port++;
                nextCardID.slot = 0;
            }
            currentCardId <<= 1;
        }

        done = true;
        if (nextCardID.port < 2) {
            foundNext = true;
        }

        if (nextCardID.port < 2 && nextCardID.slot != 0 && !this->MultitapConnected(nextCardID.port)) {
            validCardIds &= ~currentCardId;
            foundNext = false;
            done = false;
        }
    } while (!done);

    if (foundNext) {
        curCardId = nextCardID;
    }

    return foundNext;
}

void PS2Interface::SetFileTime(FileTime *destTime, sceMcTblGetDir *srcTime) {
    destTime->created.year = srcTime->_Create.Year;
    destTime->created.month = srcTime->_Create.Month;
    destTime->created.day = srcTime->_Create.Day;
    destTime->created.hour = srcTime->_Create.Hour;
    destTime->created.minute = srcTime->_Create.Min;
    destTime->created.second = srcTime->_Create.Sec;

    destTime->lastModified.year = srcTime->_Modify.Year;
    destTime->lastModified.month = srcTime->_Modify.Month;
    destTime->lastModified.day = srcTime->_Modify.Day;
    destTime->lastModified.hour = srcTime->_Modify.Hour;
    destTime->lastModified.minute = srcTime->_Modify.Min;
    destTime->lastModified.second = srcTime->_Modify.Sec;
}

Ps2FileDescriptor *PS2Interface::FindFreeFileDescriptor() {
    Ps2FileDescriptor *fd = this->mFileDescriptors;
    int i;

    for (i = 0; i < 3; i++) {
        if (!fd->mOpen) {
            return fd;
        }
        fd++;
    }

    return nullptr;
}

Ps2FileDescriptor *PS2Interface::SceOpenFile(const CardID &cardID, const char *fileName, int sceMode) {
    this->mCurrentFileDescriptor = this->FindFreeFileDescriptor();
    this->mCurrentFileDescriptor->Clear();
    this->mCurrentFileDescriptor->mCardID = cardID;
    this->mCurrentFileDescriptor->mSceFileMode = sceMode;
    strncpy(this->mCurrentFileDescriptor->mPath, fileName, sizeof(this->mCurrentFileDescriptor->mPath));

    this->CheckSceResult(sceMcOpen(cardID.port, cardID.slot, fileName, sceMode));

    return this->mCurrentFileDescriptor;
}

void PS2Interface::SceCloseFile(Ps2FileDescriptor *fd) {
    if (fd != nullptr) {
        fd->mOpen = false;
        this->CheckSceResult(sceMcClose(fd->mSceFileHandle));

        if (this->mCurrentFileDescriptor == fd) {
            this->mCurrentFileDescriptor = nullptr;
        }
    }
}

void PS2Interface::ClearTask() {
    this->mNewMsg = false;
    this->mMsg.Clear();
    this->TaskManager::ClearTask();
}

// Avanza la tarea en curso y deja listo el mensaje que haya producido.
void PS2Interface::UpdateCurrentTask() {
    if (this->mCurrentTask != nullptr) {
        switch (this->mCurrentTask->GetID()) {
        case TID_TRC_STARTGAME:
            this->UpdateTaskTrcStartGame();
            break;

        case TID_STARTGAME_COMMON:
            this->UpdateTaskStartGameCommon();
            break;

        case TID_TRC_CARDEXISTS:
        case TID_TRC_GETCARDINFO:
            this->UpdateTaskTrcGetCardInfo();
            break;

        case TID_TRC_SAVECHECK:
            this->UpdateTaskTrcSaveCheck();
            break;

        case TID_TRC_SAVEFILE:
            this->UpdateTaskTrcSaveFile();
            break;

        case TID_TRC_LOADFILE:
            this->UpdateTaskTrcLoadFile();
            break;

        case TID_TRC_DELETEFILE:
            this->UpdateTaskTrcDeleteFile();
            break;

        case TID_TRC_LIST:
            this->UpdateTaskTrcList();
            break;

        case TID_TRC_LISTENTRIES:
            this->UpdateTaskTrcListEntries();
            break;

        case TID_TRC_LISTFILES:
            this->UpdateTaskTrcListFiles();
            break;

        case TID_CARD_EXISTS:
        case TID_GET_CARD_INFO:
            this->UpdateTaskGetCardInfo();
            break;

        case TID_CREATE_DIRECTORY:
            this->UpdateTaskCreateDirectory();
            break;

        case TID_OPEN:
            this->UpdateTaskOpen();
            break;

        case TID_CLOSE:
            this->UpdateTaskClose();
            break;

        case TID_DELETE_DIRECTORY:
        case TID_DELETE:
            this->UpdateTaskDelete();
            break;

        case TID_SEEK:
            this->UpdateTaskSeek();
            break;

        case TID_READ:
            this->UpdateTaskRead();
            break;

        case TID_WRITE:
            this->UpdateTaskWrite();
            break;

        case TID_FLUSH:
            this->UpdateTaskFlush();
            break;

        case TID_SET_ATTRIBUTES:
            this->UpdateTaskSetAttribute();
            break;

        case TID_FORMAT:
            this->UpdateTaskFormat();
            break;

        case TID_SAVE_FILE:
            this->UpdateTaskSaveFile();
            break;

        case TID_CHECK_DIRECTORY:
            this->UpdateTaskCheckDirectory();
            break;

        case TID_CHANGE_DIRECTORY:
            this->UpdateTaskChangeDirectory();
            break;

        case TID_CHECK_SPACE:
            this->UpdateTaskCheckSpace();
            break;

        case TID_FIND:
            this->UpdateTaskFind();
            break;
        }

        this->mNewMsg = this->mMsg.mMsg != LMSG_NONE;
    }
}

} // namespace Realmc
