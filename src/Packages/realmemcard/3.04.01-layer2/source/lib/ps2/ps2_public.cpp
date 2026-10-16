#include "ps2_interface.h"

namespace Realmc {

// El nombre de la tarjeta que devuelve GetCardName (texto 35 de Locale con el puerto).
static wchar_t gPs2CardName[64];

// La funcion publica de sondeo: avanza la tarea en curso cuando libmc ha terminado la anterior y devuelve el mensaje
// nuevo, si lo hay.
const Message *PS2Interface::GetMessage(int elapsedTime) {
    this->LockInterfaceMutex();
    this->mMsgTimer.AddElapsedTime(elapsedTime);
    this->mMsg.Clear();

    if (!this->CheckSyncResult()) {
        this->UpdateCurrentTask();
    }

    this->UnlockInterfaceMutex();

    if (this->mNewMsg) {
        this->mNewMsg = false;
        return &this->mMsg;
    }
    return nullptr;
}

void PS2Interface::SendMessage(UserMessage msg, int value) {
    this->LockInterfaceMutex();

    this->mUserMsg = msg;
    switch (msg) {
    case UMSG_SHOW_CARD_CHECKING_MSG:
        this->mNoCheckingCardWarning = value == 0;
        break;
    case UMSG_EXPIRE_DELAY:
        this->mMsgTimer.Stop();
        break;
    }

    this->UnlockInterfaceMutex();
}

const unsigned short *PS2Interface::GetCardName(const CardID &id) {
    const wchar_t *p;

    this->LockInterfaceMutex();

    memset(gPs2CardName, 0, sizeof(gPs2CardName));
    p = Locale::GetString(35, "s", this->PortStr(id));
    int len = Locale::GetWstrLength(p);
    memcpy(gPs2CardName, p, len * sizeof(wchar_t));

    this->UnlockInterfaceMutex();

    return reinterpret_cast<const unsigned short *>(gPs2CardName);
}

void PS2Interface::TrcStartGame(const StartGameInfo &info) {
    this->CheckMessageCompatibility();
    this->LockInterfaceMutex();

    this->CheckFileInfo(&info.fileInfo);
    this->mTaskTrcStartGame.Start(&info);

    this->UnlockInterfaceMutex();
}

void PS2Interface::TrcCardExists(const CardID &cardID) {
    this->mMutex->Lock();
    this->mTaskTrcGetCardInfo.Start(cardID, TID_TRC_CARDEXISTS);
    this->mMutex->Unlock();
}

void PS2Interface::TrcGetCardInfo(const CardID &cardID) {
    this->mMutex->Lock();
    this->mTaskTrcGetCardInfo.Start(cardID, TID_TRC_GETCARDINFO);
    this->mMutex->Unlock();
}

void PS2Interface::TrcLoadFile(const CardID &cardID, const FileInfo &fileInfo) {
    this->LockInterfaceMutex();

    this->CheckFileInfo(&fileInfo);
    this->mTaskTrcLoadFile.Start(cardID, &fileInfo);

    this->UnlockInterfaceMutex();
}

void PS2Interface::TrcSaveFile(const CardID &cardID, const FileInfo &fileInfo, SaveTaskType saveTaskType,
                               unsigned int nBlocksNeeded, unsigned int) {
    this->LockInterfaceMutex();

    this->CheckFileInfo(&fileInfo);
    this->CheckName(fileInfo.directoryInfo->directoryName);
    this->CheckName(fileInfo.fileName);
    this->mTaskTrcSaveFile.Start(cardID, &fileInfo, saveTaskType, nBlocksNeeded);

    this->UnlockInterfaceMutex();
}

void PS2Interface::TrcDeleteFile(const CardID &cardID, const FileInfo &fileInfo) {
    this->LockInterfaceMutex();

    this->CheckFileInfo(&fileInfo);
    this->mTaskTrcDeleteFile.Start(cardID, &fileInfo);

    this->UnlockInterfaceMutex();
}

void PS2Interface::TrcListFiles(const CardID &cardID, const FileInfo &fileInfo, ListTaskType listType) {
    this->LockInterfaceMutex();

    this->CheckFileInfo(&fileInfo);
    this->mTaskTrcList.Start(cardID, &fileInfo, listType);

    this->UnlockInterfaceMutex();
}

void PS2Interface::CardExists(const CardID &cardID) {
    this->LockInterfaceMutex();
    this->mTaskGetCardInfo.Start(cardID, TID_CARD_EXISTS, true);
    this->UnlockInterfaceMutex();
}

void PS2Interface::GetCardInfo(const CardID &cardID) {
    this->LockInterfaceMutex();
    this->mTaskGetCardInfo.Start(cardID, TID_GET_CARD_INFO, true);
    this->UnlockInterfaceMutex();
}

void PS2Interface::Read(OpenFileDescriptor *fh, void *buffer, int bufferSize) {
    this->LockInterfaceMutex();
    this->mTaskRead.Start(fh, buffer, bufferSize, true);
    this->UnlockInterfaceMutex();
}

void PS2Interface::Write(OpenFileDescriptor *fileHandle, void *buffer, int bufferSize) {
    this->LockInterfaceMutex();
    this->mTaskWrite.Start(fileHandle, buffer, bufferSize);
    this->UnlockInterfaceMutex();
}

void PS2Interface::Seek(OpenFileDescriptor *fileHandle, int offset, SeekFrom seekFrom) {
    this->LockInterfaceMutex();
    this->mTaskSeek.Start(fileHandle, offset, seekFrom, true);
    this->UnlockInterfaceMutex();
}

void PS2Interface::Flush(OpenFileDescriptor *fileHandle) {
    this->LockInterfaceMutex();
    this->mTaskFlush.Start(fileHandle, true);
    this->UnlockInterfaceMutex();
}

void PS2Interface::CreateDirectory(const CardID &cardID, const char *directoryName) {
    this->LockInterfaceMutex();
    this->mTaskCreateDirectory.Start(cardID, directoryName, true);
    this->UnlockInterfaceMutex();
}

void PS2Interface::DeleteDirectory(const CardID &cardID, const char *directoryName) {
    this->LockInterfaceMutex();
    this->mTaskDelete.Start(cardID, directoryName, nullptr, true);
    this->UnlockInterfaceMutex();
}

void PS2Interface::OpenFile(const CardID &cardID, const FileInfo &fileInfo, FileOpenMode mode) {
    this->LockInterfaceMutex();
    this->mTaskOpen.Start(cardID, fileInfo.directoryInfo->directoryName, fileInfo.fileName, mode, true);
    this->UnlockInterfaceMutex();
}

void PS2Interface::CloseFile(OpenFileDescriptor *fileHandle) {
    this->LockInterfaceMutex();
    this->mTaskClose.Start(fileHandle, true);
    this->UnlockInterfaceMutex();
}

void PS2Interface::DeleteFile(const CardID &cardID, const char *directoryName, const char *fileName) {
    this->LockInterfaceMutex();
    this->mTaskDelete.Start(cardID, directoryName, fileName, true);
    this->UnlockInterfaceMutex();
}

void PS2Interface::SetFileAttribute(const CardID &cardID, const char *directoryName, const char *fileName,
                                    FileAttribute attribute) {
    this->LockInterfaceMutex();
    this->mTaskSetAttribute.Start(cardID, directoryName, fileName, attribute, true);
    this->UnlockInterfaceMutex();
}

void PS2Interface::FindFile(const CardID &cardID, const char *directoryName, const char *fileName) {
    this->LockInterfaceMutex();
    this->mTaskFind.Start(cardID, directoryName, fileName, true);
    this->UnlockInterfaceMutex();
}

void PS2Interface::Format(const CardID &cardID) {
    this->LockInterfaceMutex();
    this->mTaskFormat.Start(cardID, true);
    this->UnlockInterfaceMutex();
}

} // namespace Realmc
