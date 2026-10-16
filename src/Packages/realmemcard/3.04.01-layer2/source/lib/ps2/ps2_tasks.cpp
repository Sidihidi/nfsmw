#include <sifdev.h>

#include "ps2_interface.h"

namespace Realmc {

// Las tareas de fichero de la capa baja de PS2: cada Update avanza la tarea en curso un paso. En TS_START lanzan la
// llamada asincrona de libmc; GetMessage no vuelve a llamarlas hasta que sceMcSync la da por terminada
// (CheckSyncResult deja el resultado en mCardStatus) y en TS_DONE lo copian a la tarea y avisan al usuario.

Ps2FileDescriptor *ConvertFileHandleToDescriptor(OpenFileDescriptor *h) {
    return static_cast<Ps2FileDescriptor *>(h);
}

void PS2Interface::UpdateTaskGetCardInfo() {
    switch (this->mCurrentTask->GetState()) {
    case TS_START:
        this->mTaskGetCardInfo.mRetry = 0;
        this->mCurrentTask->SetState(TS_CHECK_CARD, TS_START);

    case TS_CHECK_CARD:
        this->mTaskGetCardInfo.mTaskResult = RESULT_SUCCESS;
        this->mTaskGetCardInfo.mCardStatus = STATUS_UNKNOWN;
        this->mTaskGetCardInfo.mCardFound = false;
        this->mTaskGetCardInfo.mType = 0;
        this->mTaskGetCardInfo.mFreeSpace = 0;
        this->mTaskGetCardInfo.mFormatted = 0;

        // Las ranuras 2-4 de un puerto solo existen con el multitap conectado.
        if (this->mTaskGetCardInfo.mCardID.slot != 0) {
            if (!this->MultitapConnected(this->mTaskGetCardInfo.mCardID.port)) {
                this->mCardStatus = STATUS_ACCESS_DENIED;
                this->mCurrentTask->SetState(TS_CHECK_CARD_RESULT, TS_START);
                break;
            }
        }

        this->mCardStatus = STATUS_UNKNOWN;
        this->CheckSceResult(sceMcGetInfo(this->mTaskGetCardInfo.mCardID.port, this->mTaskGetCardInfo.mCardID.slot,
                                          &this->mTaskGetCardInfo.mType, &this->mTaskGetCardInfo.mFreeSpace,
                                          &this->mTaskGetCardInfo.mFormatted));
        this->mCurrentTask->SetState(TS_CHECK_CARD_RESULT, TS_START);
        break;

    case TS_CHECK_CARD_RESULT:
        this->mTaskGetCardInfo.mCardStatus = this->mCardStatus;
        switch (this->mCardStatus) {
        case STATUS_OK:
            this->mTaskGetCardInfo.mCardFound = true;
            if (this->mTaskGetCardInfo.mFormatted == 0) {
                this->mTaskGetCardInfo.mCardStatus = STATUS_CARD_UNFORMATTED;
            }
            break;

        case STATUS_CARD_CHANGED:
        case STATUS_CARD_UNFORMATTED:
        case STATUS_ACCESS_DENIED:
            this->mTaskGetCardInfo.mCardFound = true;
            break;

        default:
            this->mTaskGetCardInfo.mCardFound = false;
            break;
        }

        if (this->mTaskGetCardInfo.mCardFound) {
            // Solo vale una tarjeta de PS2.
            if (this->mTaskGetCardInfo.mType != sceMcTypePS2) {
                this->mTaskGetCardInfo.mCardFound = false;
                this->mTaskGetCardInfo.mCardStatus = STATUS_NO_CARD;
            }
            this->mCurrentTask->SetState(TS_DONE, TS_START);
        } else {
            // Sin tarjeta se reintenta una vez antes de darla por ausente.
            if (++this->mTaskGetCardInfo.mRetry < 2) {
                this->mCurrentTask->SetState(TS_CHECK_CARD, TS_START);
            } else {
                this->mCurrentTask->SetState(TS_DONE, TS_START);
            }
        }
        break;

    case TS_DONE:
        if (this->mTaskGetCardInfo.mNotifyUser) {
            this->mMsg.Set(this->mTaskGetCardInfo.mProxyTaskID == TID_CARD_EXISTS ? LMSG_CARD_EXISTS_DONE
                                                                                   : LMSG_CARD_INFO_DONE,
                           this->mTaskGetCardInfo.mTaskResult, this->mTaskGetCardInfo.mCardStatus);
            this->mMsg.info.cardInfo.cardID = this->mTaskGetCardInfo.mCardID;
            this->mMsg.info.cardInfo.freeSpace = this->mTaskGetCardInfo.mFreeSpace;
        }
        this->mTaskGetCardInfo.End();
        break;
    }
}

void PS2Interface::UpdateTaskWrite() {
    switch (this->mCurrentTask->GetState()) {
    case TS_START:
        this->mCardStatus = STATUS_UNKNOWN;
        this->mTaskWrite.mTaskResult = RESULT_SUCCESS;
        this->mTaskWrite.mCardStatus = STATUS_UNKNOWN;
        this->mCurrentFileDescriptor = this->mTaskWrite.mFileDescriptor;
        this->mCurrentFileDescriptor->mBytesWritten = 0;
        this->CheckSceResult(sceMcWrite(this->mCurrentFileDescriptor->mSceFileHandle, this->mTaskWrite.mBuffer,
                                        this->mTaskWrite.mBufferSize));
        this->mCurrentTask->SetState(TS_DONE, TS_START);
        break;

    case TS_DONE:
        this->mTaskWrite.mCardStatus = this->mCardStatus;
        if (this->mCardStatus != STATUS_OK) {
            this->mTaskWrite.mTaskResult = RESULT_FAILED;
        } else if (this->mTaskWrite.mFileDescriptor->mBytesWritten <= 0) {
            this->mTaskWrite.mTaskResult = RESULT_FAILED;
            this->mTaskWrite.mCardStatus = STATUS_RANGE_ERROR;
        }

        if (this->mTaskWrite.mNotifyUser) {
            this->mMsg.Set(LMSG_WRITE_DONE, this->mTaskWrite.mTaskResult, this->mTaskWrite.mCardStatus);
            this->mMsg.info.writeResult.bytesWritten = this->mTaskWrite.mFileDescriptor->mBytesWritten;
        }
        this->mTaskWrite.End();
        break;
    }
}

void PS2Interface::UpdateTaskRead() {
    switch (this->mCurrentTask->GetState()) {
    case TS_START:
        this->mCardStatus = STATUS_UNKNOWN;
        this->mTaskRead.mTaskResult = RESULT_SUCCESS;
        this->mTaskRead.mCardStatus = STATUS_UNKNOWN;
        this->mCurrentFileDescriptor = this->mTaskRead.mFileDescriptor;
        this->mCurrentFileDescriptor->mBytesRead = 0;
        this->CheckSceResult(sceMcRead(this->mCurrentFileDescriptor->mSceFileHandle, this->mTaskRead.mBuffer,
                                       this->mTaskRead.mBufferSize));
        this->mCurrentTask->SetState(TS_DONE, TS_START);
        break;

    case TS_DONE:
        this->mTaskRead.mCardStatus = this->mCardStatus;
        if (this->mCardStatus != STATUS_OK) {
            this->mTaskRead.mTaskResult = RESULT_FAILED;
        } else if (this->mTaskRead.mFileDescriptor->mBytesRead <= 0) {
            this->mTaskRead.mTaskResult = RESULT_FAILED;
            this->mTaskRead.mCardStatus = STATUS_RANGE_ERROR;
        }

        if (this->mTaskRead.mNotifyUser) {
            this->mMsg.Set(LMSG_READ_DONE, this->mTaskRead.mTaskResult, this->mTaskRead.mCardStatus);
            this->mMsg.info.readResult.bytesRead = this->mTaskRead.mFileDescriptor->mBytesRead;
        }
        this->mTaskRead.End();
        break;
    }
}

void PS2Interface::UpdateTaskCreateDirectory() {
    switch (this->mCurrentTask->GetState()) {
    case TS_START:
        this->CheckName(this->mTaskCreateDirectory.mDirectoryName);

        this->mCardStatus = STATUS_UNKNOWN;
        this->mTaskCreateDirectory.mTaskResult = RESULT_SUCCESS;
        this->mTaskCreateDirectory.mCardStatus = STATUS_UNKNOWN;
        this->mCurrentTask->SetState(TS_CREATE_DIRECTORY, TS_START);

        // El directorio se crea desde la raiz.
        this->mTaskChangeDirectory.Start(this->mTaskCreateDirectory.mCardID, ROOT_DIRECTORY_NAME, false);
        break;

    case TS_CREATE_DIRECTORY:
        this->CheckSceResult(sceMcMkdir(this->mTaskCreateDirectory.mCardID.port,
                                        this->mTaskCreateDirectory.mCardID.slot,
                                        this->mTaskCreateDirectory.mDirectoryName));
        this->mCurrentTask->SetState(TS_DONE, TS_START);
        break;

    case TS_DONE:
        this->mTaskCheckDirectory.mTaskResult = this->mCardStatus != STATUS_OK ? RESULT_FAILED : RESULT_SUCCESS;
        this->mTaskCheckDirectory.mCardStatus = this->mCardStatus;
        this->mTaskCreateDirectory.End();

        if (this->mTaskCreateDirectory.mNotifyUser) {
            this->mMsg.Set(LMSG_CREATE_DIRECTORY_DONE, this->mTaskCheckDirectory.mTaskResult,
                           this->mTaskCheckDirectory.mCardStatus);
        }
        break;
    }
}

void PS2Interface::UpdateTaskOpen() {
    char path[128];
    int sceMode;

    switch (this->mCurrentTask->GetState()) {
    case TS_START:
        this->CheckName(this->mTaskOpen.mDirectoryName);
        this->CheckName(this->mTaskOpen.mFileName);

        sprintf(path, "/%s/%s", this->mTaskOpen.mDirectoryName, this->mTaskOpen.mFileName);

        sceMode = 0;
        if (this->mTaskOpen.mMode & FOM_CREATE) {
            sceMode |= SCE_CREAT;
        }
        if (this->mTaskOpen.mMode & FOM_READ) {
            sceMode |= SCE_RDONLY;
        }
        if (this->mTaskOpen.mMode & FOM_WRITE) {
            sceMode |= SCE_WRONLY;
        }

        this->mCardStatus = STATUS_UNKNOWN;
        this->mTaskOpen.mTaskResult = RESULT_SUCCESS;
        this->mTaskOpen.mCardStatus = STATUS_UNKNOWN;
        this->mTaskOpen.mFileDescriptor = this->SceOpenFile(this->mTaskOpen.mCardID, path, sceMode);
        this->mCurrentTask->SetState(TS_DONE, TS_START);
        break;

    case TS_DONE:
        this->mTaskOpen.mTaskResult = this->mCardStatus != STATUS_OK ? RESULT_FAILED : RESULT_SUCCESS;
        this->mTaskOpen.mCardStatus = this->mCardStatus;

        if (this->mTaskOpen.mNotifyUser) {
            this->mMsg.Set(LMSG_OPEN_FILE_DONE, this->mTaskOpen.mTaskResult, this->mTaskOpen.mCardStatus);
            this->mMsg.info.openResult.fileHandle = this->mTaskOpen.mFileDescriptor;
            this->mMsg.info.openResult.fileName = this->mTaskOpen.mFileName;
        }
        this->mTaskOpen.End();
        break;
    }
}

void PS2Interface::UpdateTaskClose() {
    switch (this->mTaskClose.GetState()) {
    case TS_START:
        this->mCardStatus = STATUS_UNKNOWN;
        this->mTaskClose.mTaskResult = RESULT_SUCCESS;
        this->mTaskClose.mCardStatus = STATUS_UNKNOWN;
        this->SceCloseFile(this->mTaskClose.mFileDescriptor);
        this->mTaskClose.SetState(TS_DONE, TS_START);
        break;

    case TS_DONE:
        this->mTaskClose.mTaskResult = this->mCardStatus != STATUS_OK ? RESULT_FAILED : RESULT_SUCCESS;
        this->mTaskClose.mCardStatus = this->mCardStatus;

        if (this->mTaskClose.mNotifyUser) {
            this->mMsg.Set(LMSG_CLOSE_FILE_DONE, this->mTaskClose.mTaskResult, this->mTaskClose.mCardStatus);
        }
        this->mTaskClose.End();
        break;
    }
}

void PS2Interface::UpdateTaskFlush() {
    switch (this->mCurrentTask->GetState()) {
    case TS_START:
        this->mCardStatus = STATUS_UNKNOWN;
        this->mTaskFlush.mTaskResult = RESULT_SUCCESS;
        this->mTaskFlush.mCardStatus = STATUS_UNKNOWN;
        this->CheckSceResult(sceMcFlush(this->mTaskFlush.mFileDescriptor->mSceFileHandle));
        this->mCurrentTask->SetState(TS_DONE, TS_START);
        break;

    case TS_DONE:
        this->mTaskFlush.mTaskResult = this->mCardStatus != STATUS_OK ? RESULT_FAILED : RESULT_SUCCESS;
        this->mTaskFlush.mCardStatus = this->mCardStatus;
        this->mTaskFlush.End();

        if (this->mTaskFlush.mNotifyUser) {
            this->mMsg.Set(LMSG_FLUSH_DONE, this->mTaskFlush.mTaskResult, this->mTaskFlush.mCardStatus);
        }
        break;
    }
}

void PS2Interface::UpdateTaskSeek() {
    switch (this->mCurrentTask->GetState()) {
    case TS_START:
        this->mCardStatus = STATUS_UNKNOWN;
        this->mTaskSeek.mTaskResult = RESULT_SUCCESS;
        this->mTaskSeek.mCardStatus = STATUS_UNKNOWN;
        this->mCurrentFileDescriptor = this->mTaskSeek.mFileDescriptor;
        this->mCurrentFileDescriptor->mSeekPosition = 0;
        this->CheckSceResult(sceMcSeek(this->mCurrentFileDescriptor->mSceFileHandle, this->mTaskSeek.mOffset,
                                       this->mTaskSeek.mSeekFrom));
        this->mCurrentTask->SetState(TS_DONE, TS_START);
        break;

    case TS_DONE:
        this->mTaskSeek.mTaskResult = this->mCardStatus != STATUS_OK ? RESULT_FAILED : RESULT_SUCCESS;
        this->mTaskSeek.mCardStatus = this->mCardStatus;
        this->mTaskSeek.End();

        if (this->mTaskSeek.mNotifyUser) {
            this->mMsg.Set(LMSG_SEEK_DONE, this->mTaskSeek.mTaskResult, this->mTaskSeek.mCardStatus);
            this->mMsg.info.seekResult.filePosition = this->mTaskSeek.mFileDescriptor->mSeekPosition;
        }
        break;
    }
}

void PS2Interface::UpdateTaskSetAttribute() {
    sceMcTblGetDir info;
    char path[64];

    switch (this->mCurrentTask->GetState()) {
    case TS_START:
        memset(&info, 0, sizeof(info));

        if (this->mTaskSetAttribute.mAttribute & FA_READABLE) info.AttrFile |= sceMcFileAttrReadable;
        if (this->mTaskSetAttribute.mAttribute & FA_WRITABLE) info.AttrFile |= sceMcFileAttrWriteable;
        if (this->mTaskSetAttribute.mAttribute & FA_EXECUTABLE) info.AttrFile |= sceMcFileAttrExecutable;
        if (this->mTaskSetAttribute.mAttribute & FA_COPY_PROTECTED) info.AttrFile |= sceMcFileAttrDupProhibit;

        sprintf(path, "/%s/%s", this->mTaskSetAttribute.mDirectoryName, this->mTaskSetAttribute.mFileName);

        this->CheckSceResult(sceMcSetFileInfo(this->mTaskSetAttribute.mCardID.port,
                                              this->mTaskSetAttribute.mCardID.slot, path,
                                              reinterpret_cast<char *>(&info), sceMcFileInfoAttr));

        this->mCardStatus = STATUS_UNKNOWN;
        this->mTaskSetAttribute.mTaskResult = RESULT_SUCCESS;
        this->mTaskSetAttribute.mCardStatus = STATUS_UNKNOWN;
        this->mCurrentTask->SetState(TS_DONE, TS_START);
        break;

    case TS_DONE:
        this->mTaskSetAttribute.mTaskResult = this->mCardStatus != STATUS_OK ? RESULT_FAILED : RESULT_SUCCESS;
        this->mTaskSetAttribute.mCardStatus = this->mCardStatus;
        this->mTaskSetAttribute.End();

        if (this->mTaskSetAttribute.mNotifyUser) {
            this->mMsg.Set(LMSG_SET_ATTRIBUTE_DONE, this->mTaskSetAttribute.mTaskResult,
                           this->mTaskSetAttribute.mCardStatus);
        }
        break;
    }
}

void PS2Interface::UpdateTaskDelete() {
    switch (this->mCurrentTask->GetState()) {
    case TS_START:
        this->mCardStatus = STATUS_UNKNOWN;
        this->mTaskDelete.mTaskResult = RESULT_SUCCESS;
        this->mTaskDelete.mCardStatus = STATUS_UNKNOWN;
        this->CheckSceResult(sceMcDelete(this->mTaskDelete.mCardID.port, this->mTaskDelete.mCardID.slot,
                                         this->mTaskDelete.mPath));
        this->mCurrentTask->SetState(TS_DONE, TS_START);
        break;

    case TS_DONE:
        this->mTaskDelete.mTaskResult = this->mCardStatus != STATUS_OK ? RESULT_FAILED : RESULT_SUCCESS;
        this->mTaskDelete.mCardStatus = this->mCardStatus;
        this->mTaskDelete.End();

        if (this->mTaskDelete.mNotifyUser) {
            this->mMsg.Set(this->mTaskDelete.mDeleteDirectory ? LMSG_DELETE_DIRECTORY_DONE : LMSG_DELETE_FILE_DONE,
                           this->mTaskDelete.mTaskResult, this->mTaskDelete.mCardStatus);
        }
        break;
    }
}

void PS2Interface::UpdateTaskFind() {
    switch (this->mCurrentTask->GetState()) {
    case TS_START:
        this->mTaskFind.mTaskResult = RESULT_SUCCESS;
        this->mTaskFind.mCardStatus = STATUS_UNKNOWN;
        this->mTaskFind.mNumFilesFound = 0;
        this->mTaskFind.mFileSize = 0;

        this->CheckCard(this->mTaskFind.mCardID, nullptr, nullptr, nullptr);

        sprintf(this->mTaskFind.mPath, "/%s/%s", this->mTaskFind.mDirectoryName, this->mTaskFind.mFileName);
        this->mCurrentTask->SetState(TS_FIND_FIRST, TS_START);

    case TS_FIND_FIRST:
        this->mCardStatus = STATUS_UNKNOWN;
        this->mNumFilesFound = 0;
        this->CheckSceResult(sceMcGetDir(this->mTaskFind.mCardID.port, this->mTaskFind.mCardID.slot,
                                         this->mTaskFind.mPath, 0, 1, mPs2FileTableEntry));
        this->mCurrentTask->SetState(TS_FIND_FILE_RESULT, TS_START);
        break;

    case TS_FIND_FILE_RESULT:
        this->mTaskFind.mCardStatus = this->mCardStatus;

        if (this->mNumFilesFound > 0) {
            if (strcmp(reinterpret_cast<char *>(mPs2FileTableEntry[0].EntryName), ".") != 0 &&
                strcmp(reinterpret_cast<char *>(mPs2FileTableEntry[0].EntryName), "..") != 0) {
                this->mTaskFind.mFileSize = mPs2FileTableEntry[0].FileSizeByte;
                this->mTaskFind.mNumFilesFound += this->mNumFilesFound;

                if (this->mTaskFind.mNotifyUser) {
                    this->mMsg.Set(LMSG_FILE_INFO);
                    this->mMsg.info.fileInfo.fileName = reinterpret_cast<char *>(mPs2FileTableEntry[0].EntryName);
                    this->mMsg.info.fileInfo.fileSize = this->mTaskFind.mFileSize;
                    this->SetFileTime(&this->mMsg.info.fileInfo.fileTime, &mPs2FileTableEntry[0]);
                }
            }

            this->mCurrentTask->SetState(TS_FIND_NEXT, TS_START);
            break;
        }

        this->mCurrentTask->SetState(TS_DONE, TS_START);
        break;

    case TS_FIND_NEXT:
        this->mCardStatus = STATUS_UNKNOWN;
        this->mNumFilesFound = 0;
        this->mCurrentTask->SetState(TS_FIND_FILE_RESULT, TS_START);
        this->CheckSceResult(sceMcGetDir(this->mTaskFind.mCardID.port, this->mTaskFind.mCardID.slot,
                                         this->mTaskFind.mPath, 1, 1, mPs2FileTableEntry));
        break;

    case TS_DONE:
        this->mTaskFind.mTaskResult = RESULT_SUCCESS;
        if (this->mTaskFind.mNumFilesFound > 0) {
            this->mCardStatus = STATUS_OK;
        } else {
            this->mTaskFind.mCardStatus = STATUS_ENTRY_NOT_FOUND;
        }
        this->mTaskFind.mCardStatus = this->mCardStatus;

        if (this->mTaskFind.mNotifyUser) {
            this->mMsg.Set(LMSG_FIND_FILE_DONE, this->mTaskFind.mTaskResult, this->mTaskFind.mCardStatus);
        }
        this->mTaskFind.End();
        break;
    }
}

void PS2Interface::UpdateTaskFormat() {
    switch (this->mTaskFormat.GetState()) {
    case TS_START:
        this->mCardStatus = STATUS_UNKNOWN;
        this->mTaskFormat.mTaskResult = RESULT_SUCCESS;
        this->mTaskFormat.mCardStatus = STATUS_UNKNOWN;
        this->CheckSceResult(sceMcFormat(this->mTaskFormat.mCardID.port, this->mTaskFormat.mCardID.slot));
        this->mTaskFormat.SetState(TS_DONE, TS_START);
        break;

    case TS_DONE:
        this->mTaskFormat.mTaskResult = this->mCardStatus != STATUS_OK ? RESULT_FAILED : RESULT_SUCCESS;
        this->mTaskFormat.mCardStatus = this->mCardStatus;

        if (this->mTaskFormat.mNotifyUser) {
            this->mMsg.Set(LMSG_FORMAT_DONE, this->mTaskFormat.mTaskResult, this->mTaskFormat.mCardStatus);
        }
        this->mTaskFormat.End();
        break;
    }
}

// La tabla de directorio que rellena sceMcGetDir (Find la usa entrada a entrada).
sceMcTblGetDir PS2Interface::mPs2FileTableEntry[7];

} // namespace Realmc
