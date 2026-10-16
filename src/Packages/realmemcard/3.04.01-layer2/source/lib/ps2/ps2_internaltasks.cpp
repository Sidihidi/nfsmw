#include <string.h>

#include <sifdev.h>

#include "ps2_interface.h"

namespace Realmc {

// Las tareas internas de la capa baja de PS2: las que otras tareas encadenan (la comprobacion comun del arranque, el
// espacio libre, grabar un fichero entero, cambiar de directorio y comprobar que un directorio de partida esta sano).

// La comprobacion del arranque: recorre las ranuras validas mirando la tarjeta y el espacio, y se queda con la primera
// tarjeta buena; si no hay, pregunta al usuario (sin tarjeta o sin espacio) y vuelve a empezar si quiere reintentar.
void PS2Interface::UpdateTaskStartGameCommon() {
    if (!this->mMsgTimer.IsExpired()) {
        return;
    }

    switch (this->mTaskStartGameCommon.GetState()) {
    case TS_START:
        this->GetNextCardId(this->mTaskStartGameCommon.mCardID, true);
        this->mTaskStartGameCommon.mFirstCardId = this->mTaskStartGameCommon.mCardID;
        this->mTaskStartGameCommon.mTaskResult = RESULT_UNKNOWN;
        this->mTaskStartGameCommon.mCardStatus = STATUS_UNKNOWN;
        this->mTaskStartGameCommon.SetState(TS_CHECK_CARD, TS_START);

    case TS_CHECK_CARD:
        switch (this->mTaskStartGameCommon.GetSubstate()) {
        case TS_START:
            this->mMsg.TRC_msg(6, 0, this->PortStr(this->mTaskStartGameCommon.mCardID));
            this->mMsgTimer.Set(1);
            this->mTaskStartGameCommon.SetSubstate(TS_CHECK_CARD);
            break;

        case TS_CHECK_CARD:
            this->mTaskStartGameCommon.SetSubstate(TS_CHECK_CARD_RESULT);
            this->mTaskGetCardInfo.Start(this->mTaskStartGameCommon.mCardID, TID_CARD_EXISTS, false);
            break;

        case TS_CHECK_CARD_RESULT:
            this->mTaskStartGameCommon.mCardStatus = this->mTaskGetCardInfo.mCardStatus;
            switch (this->mTaskStartGameCommon.mCardStatus) {
            case STATUS_OK:
                this->mTaskStartGameCommon.SetState(TS_CHECK_SPACE, TS_START);
                break;

            case STATUS_CARD_CHANGED:
                this->mTaskStartGameCommon.SetState(TS_CHECK_SPACE, TS_START);
                break;

            case STATUS_CARD_UNFORMATTED:
                this->mTaskStartGameCommon.SetState(TS_CHECK_RESULT, TS_START);
                break;

            default:
                this->mTaskStartGameCommon.mActualSpaceNeeded = this->mTaskStartGameCommon.mNewFileSpaceNeeded;
                this->mTaskStartGameCommon.mCardStatus = STATUS_NO_CARD;
                this->mTaskStartGameCommon.SetState(TS_CHECK_RESULT, TS_START);
                break;
            }
            break;
        }
        break;

    case TS_CHECK_SPACE:
        switch (this->mTaskStartGameCommon.GetSubstate()) {
        case TS_START:
            this->mTaskStartGameCommon.SetSubstate(TS_CHECK_SPACE);
            this->mTaskCheckSpace.Start(this->mTaskStartGameCommon.mCardID, &this->mTaskTrcStartGame.mInfo,
                                        this->mTaskGetCardInfo.mFreeSpace, true);
            break;

        case TS_CHECK_SPACE:
            this->mTaskStartGameCommon.SetState(TS_CHECK_RESULT, TS_START);
            this->mTaskStartGameCommon.mCardStatus = this->mTaskCheckSpace.mCardStatus;
            switch (this->mTaskStartGameCommon.mCardStatus) {
            case STATUS_OK:
                break;

            case STATUS_NO_CARD:
                this->mTaskStartGameCommon.mActualSpaceNeeded = this->mTaskStartGameCommon.mNewFileSpaceNeeded;
                break;

            case STATUS_INSUFFICIENT_SPACE:
                this->mTaskStartGameCommon.mActualSpaceNeeded = this->mTaskCheckSpace.mSpaceNeeded;
                break;
            }
            break;
        }
        break;

    case TS_CHECK_RESULT:
        this->mTaskStartGameCommon.RecordCardState(this->mTaskStartGameCommon.mCardID,
                                                   this->mTaskStartGameCommon.mCardStatus,
                                                   this->mTaskCheckSpace.mEntryFound);
        this->mTaskStartGameCommon.SetState(TS_CHECK_NEXT_CARD, TS_START);

    case TS_CHECK_NEXT_CARD:
        if (this->mTaskTrcStartGame.mInfo.checkAllSlots &&
            this->GetNextCardId(this->mTaskStartGameCommon.mCardID, false)) {
            this->mTaskStartGameCommon.SetState(TS_CHECK_CARD, TS_START);
        } else {
            this->mTaskStartGameCommon.SetState(TS_DISPLAY_RESULT, TS_START);
        }
        break;

    case TS_DISPLAY_RESULT:
        switch (this->mTaskStartGameCommon.GetSubstate()) {
        case TS_START: {
            // Primero una tarjeta buena (o sin formatear); si no, una a la que le falte espacio.
            bool foundGoodCard;
            bool foundFullCard;

            foundGoodCard = false;
            for (int iPort = 0; iPort < 2 && !foundGoodCard; iPort++) {
                for (int iSlot = 0; iSlot < 4 && !foundGoodCard; iSlot++) {
                    if (this->mTaskStartGameCommon.mSlotStatus[iPort][iSlot] == STATUS_OK ||
                        this->mTaskStartGameCommon.mSlotStatus[iPort][iSlot] == STATUS_CARD_UNFORMATTED) {
                        this->mTaskStartGameCommon.mCardID.port = iPort;
                        this->mTaskStartGameCommon.mCardID.slot = iSlot;
                        this->mTaskStartGameCommon.mCardStatus = STATUS_OK;
                        foundGoodCard = true;
                    }
                }
            }

            foundFullCard = false;
            for (int iPort = 0; iPort < 2 && !foundFullCard && !foundGoodCard; iPort++) {
                for (int iSlot = 0; iSlot < 4 && !foundFullCard; iSlot++) {
                    if (this->mTaskStartGameCommon.mSlotStatus[iPort][iSlot] == STATUS_INSUFFICIENT_SPACE) {
                        this->mTaskStartGameCommon.mCardID.port = iPort;
                        this->mTaskStartGameCommon.mCardID.slot = iSlot;
                        this->mTaskStartGameCommon.mCardStatus = STATUS_INSUFFICIENT_SPACE;
                        foundFullCard = true;
                        break;
                    }
                }
            }

            if (foundGoodCard) {
                this->mTaskStartGameCommon.mTaskResult = RESULT_SUCCESS;
                this->mTaskStartGameCommon.SetState(TS_DONE, TS_START);
                break;
            }

            if (foundFullCard) {
                this->mTaskStartGameCommon.SetSubstate(TS_INSUFFICIENT_SPACE_ASK_USER);
                break;
            }

            this->mTaskStartGameCommon.mCardID = this->mTaskStartGameCommon.mFirstCardId;
            this->mTaskStartGameCommon.mCardStatus = STATUS_NO_CARD;
            this->mTaskStartGameCommon.SetSubstate(TS_NO_CARD_ASK_USER);
            break;
        }

        case TS_INSUFFICIENT_SPACE_ASK_USER:
            this->mMsg.TRC_msg(11, PS2Message::PackMsgOptions(4, 3, 0, 0),
                               this->PortStr(this->mTaskStartGameCommon.mCardID),
                               this->mTaskStartGameCommon.mActualSpaceNeeded);
            this->mUserMsg = UMSG_NONE;
            this->mTaskStartGameCommon.SetSubstate(TS_WAIT_FOR_USER_REPLY);
            break;

        case TS_NO_CARD_ASK_USER:
            this->mMsg.TRC_msg(10, PS2Message::PackMsgOptions(4, 3, 0, 0),
                               this->PortStr(this->mTaskStartGameCommon.mCardID),
                               this->mTaskStartGameCommon.mActualSpaceNeeded);
            this->mUserMsg = UMSG_NONE;
            this->mTaskStartGameCommon.SetSubstate(TS_WAIT_FOR_USER_REPLY);
            break;

        case TS_WAIT_FOR_USER_REPLY:
            switch (this->mUserMsg) {
            case UMSG_OPTION1:
                this->mTaskStartGameCommon.SetState(TS_START, TS_START);
                break;

            case UMSG_OPTION2:
                this->mTaskStartGameCommon.mTaskResult = RESULT_CANCELLED;
                this->mTaskStartGameCommon.SetState(TS_DONE, TS_START);
                break;
            }
            break;
        }
        break;

    case TS_DONE:
        if (this->mTaskStartGameCommon.mCardStatus !=
            this->CheckCard(this->mTaskStartGameCommon.mCardID, nullptr, nullptr, nullptr)) {
            this->mTaskStartGameCommon.SetState(TS_START, TS_START);
        }
        this->mTaskStartGameCommon.End();
        break;
    }
}

// Cuenta los clusteres que ya ocupa en la tarjeta cada directorio que casa con el de la partida (la tabla del
// directorio y sus ficheros) y compara lo que falta con el espacio libre.
void PS2Interface::UpdateTaskCheckSpace() {
    int iFile;

    switch (this->mTaskCheckSpace.GetState()) {
    case TS_START:
        this->mTaskCheckSpace.mTaskResult = RESULT_UNKNOWN;
        this->mTaskCheckSpace.mCardStatus = STATUS_UNKNOWN;
        this->mTaskCheckSpace.SetState(TS_FIND_DIRECTORY, TS_START);
        this->CheckSceResult(sceMcChdir(this->mTaskCheckSpace.mCardID.port, this->mTaskCheckSpace.mCardID.slot,
                                        ROOT_DIRECTORY_NAME, nullptr));
        break;

    case TS_FIND_DIRECTORY:
        switch (this->mTaskCheckSpace.GetSubstate()) {
        case TS_START:
            this->mCardStatus = STATUS_UNKNOWN;
            this->mNumFilesFound = 0;
            this->CheckSceResult(sceMcGetDir(this->mTaskCheckSpace.mCardID.port, this->mTaskCheckSpace.mCardID.slot,
                                             this->mTaskCheckSpace.mFileInfo.directoryInfo->directoryName, 0, -1,
                                             nullptr));
            this->mTaskCheckSpace.SetSubstate(TS_CREATE_DIRECTORY_TABLE);
            break;

        case TS_CREATE_DIRECTORY_TABLE:
            if (this->mNumFilesFound > 0) {
                this->mTaskCheckSpace.mEntryFound = true;
                this->mTaskCheckSpace.mNumEntries = this->mNumFilesFound;
                this->mTaskCheckSpace.mEntries = static_cast<sceMcTblGetDir *>(this->mISystem.mAllocator->Alloc(
                    this->mTaskCheckSpace.mNumEntries * sizeof(sceMcTblGetDir),
                    EA::TagValuePair(EA::Allocator::ATT_NAME, "CheckSpaceTa") +
                        EA::TagValuePair(EA::Allocator::ATT_ALIGNMENT, 64)));
                memset(this->mTaskCheckSpace.mEntries, 0, this->mTaskCheckSpace.mNumEntries * sizeof(sceMcTblGetDir));
                this->CheckSceResult(sceMcGetDir(this->mTaskCheckSpace.mCardID.port,
                                                 this->mTaskCheckSpace.mCardID.slot,
                                                 this->mTaskCheckSpace.mFileInfo.directoryInfo->directoryName, 0,
                                                 this->mTaskCheckSpace.mNumEntries, this->mTaskCheckSpace.mEntries));
                this->mTaskCheckSpace.SetState(TS_FIND_FILE, TS_START);
            } else {
                this->mTaskCheckSpace.SetState(TS_DONE, TS_START);
            }
            break;
        }
        break;

    case TS_FIND_FILE:
        switch (this->mTaskCheckSpace.GetSubstate()) {
        case TS_START:
            this->mNumFilesFound = 0;
            this->mCardStatus = STATUS_UNKNOWN;

        case TS_CHANGE_DIRECTORY:
            this->CheckSceResult(sceMcChdir(this->mTaskCheckSpace.mCardID.port, this->mTaskCheckSpace.mCardID.slot,
                                            reinterpret_cast<char *>(
                                                this->mTaskCheckSpace.mEntries[this->mTaskCheckSpace.mCurEntry].EntryName),
                                            nullptr));
            this->mTaskCheckSpace.SetSubstate(TS_FIND_FILE);
            break;

        case TS_FIND_FILE:
            this->CheckSceResult(sceMcGetDir(this->mTaskCheckSpace.mCardID.port, this->mTaskCheckSpace.mCardID.slot,
                                             FILENAME_ALL_FILES, 0, 7, mPs2FileTableEntry));
            this->mTaskCheckSpace.SetSubstate(TS_FIND_FILE_RESULT);
            break;

        case TS_FIND_FILE_RESULT:
            switch (this->mCardStatus) {
            case STATUS_OK:
            case STATUS_CARD_CHANGED:
                if (this->mNumFilesFound > 0) {
                    // La tabla del directorio ocupa sus clusteres fijos mas uno por cada dos entradas.
                    this->mTaskCheckSpace.mExistingFileBlockSize +=
                        NUM_CLUSTERS_PER_DIRECTORY_TABLE + (this->mNumFilesFound - 1) / 2;

                    for (iFile = 0; iFile < this->mNumFilesFound; iFile++) {
                        if (strcmp(reinterpret_cast<char *>(mPs2FileTableEntry[iFile].EntryName), ".") != 0 &&
                            strcmp(reinterpret_cast<char *>(mPs2FileTableEntry[iFile].EntryName), "..") != 0) {
                            this->mTaskCheckSpace.mExistingFileBlockSize +=
                                mPs2FileTableEntry[iFile].FileSizeByte / PS2_CLUSTER_SIZE + 1;
                        }

                        if (strcmp(reinterpret_cast<char *>(mPs2FileTableEntry[iFile].EntryName),
                                   this->mTaskCheckSpace.mFileInfo.fileName) == 0) {
                            this->mTaskCheckSpace.mFileExists = true;
                        }
                    }
                }
                this->mTaskCheckSpace.SetSubstate(TS_FIND_NEXT);
                break;

            case STATUS_NO_CARD:
            default:
                this->mTaskCheckSpace.mTaskResult = RESULT_FAILED;
                this->mTaskCheckSpace.mCardStatus = this->mCardStatus;
                this->mTaskCheckSpace.SetState(TS_DONE, TS_START);
                break;
            }
            break;

        case TS_FIND_NEXT:
            this->mTaskCheckSpace.mCurEntry++;
            if (this->mTaskCheckSpace.mCurEntry < this->mTaskCheckSpace.mNumEntries) {
                this->mTaskCheckSpace.SetSubstate(TS_START);
                this->CheckSceResult(sceMcChdir(this->mTaskCheckSpace.mCardID.port,
                                                this->mTaskCheckSpace.mCardID.slot, ROOT_DIRECTORY_NAME, nullptr));
            } else {
                this->mTaskCheckSpace.SetState(TS_DONE, TS_START);
            }
            break;
        }
        break;

    case TS_DONE: {
        CardStatus newStatus = this->CheckCard(this->mTaskCheckSpace.mCardID, nullptr, nullptr, nullptr);

        if (newStatus == STATUS_NO_CARD) {
            this->mTaskCheckSpace.mTaskResult = RESULT_FAILED;
            this->mTaskCheckSpace.mCardStatus = newStatus;
        } else {
            this->mTaskCheckSpace.mTaskResult = RESULT_SUCCESS;
            if (this->mTaskCheckSpace.mTotalBlocksRequired > this->mTaskCheckSpace.mExistingFileBlockSize) {
                this->mTaskCheckSpace.mExtraBlocksNeeded =
                    this->mTaskCheckSpace.mTotalBlocksRequired - this->mTaskCheckSpace.mExistingFileBlockSize;
            }

            if (this->mTaskCheckSpace.mExtraBlocksNeeded > this->mTaskCheckSpace.mFreeCardSpace) {
                this->mTaskCheckSpace.mCardStatus = STATUS_INSUFFICIENT_SPACE;
                this->mTaskCheckSpace.mSpaceNeeded = this->mTaskCheckSpace.mTotalBlocksRequired;
            } else {
                this->mTaskCheckSpace.mCardStatus = STATUS_OK;
            }
        }

        if (this->mTaskCheckSpace.mEntries != nullptr) {
            this->mISystem.mAllocator->Free(this->mTaskCheckSpace.mEntries, 0);
            this->mTaskCheckSpace.mEntries = nullptr;
            this->mTaskCheckSpace.mNumEntries = 0;
        }

        this->mTaskCheckSpace.End();
        break;
    }
    }
}

// Graba un fichero entero de una vez: entra en el directorio, lo abre (creandolo si no existe), lo escribe y lo cierra.
void PS2Interface::UpdateTaskSaveFile() {
    switch (this->mCurrentTask->GetState()) {
    case TS_START:
        if (this->mTaskSaveFile.mFileName == nullptr || this->mTaskSaveFile.mData == nullptr ||
            this->mTaskSaveFile.mDataSize == 0) {
            this->mCurrentTask->SetState(TS_DONE, TS_START);
            break;
        }

        this->mTaskSaveFile.mFileDescriptor = nullptr;
        this->mTaskSaveFile.mTaskResult = RESULT_UNKNOWN;
        this->mTaskSaveFile.mCardStatus = STATUS_UNKNOWN;
        this->mCurrentTask->SetState(TS_CHANGE_DIRECTORY, TS_START);

    case TS_CHANGE_DIRECTORY:
        this->mCurrentTask->SetState(TS_CHANGE_DIRECTORY_RESULT, TS_START);
        this->mTaskChangeDirectory.Start(this->mTaskSaveFile.mCardID, this->mTaskSaveFile.mDirectoryName, false);
        break;

    case TS_CHANGE_DIRECTORY_RESULT:
        this->mTaskSaveFile.mCardStatus = this->mTaskChangeDirectory.mCardStatus;
        if (this->mTaskSaveFile.mCardStatus != STATUS_OK) {
            this->mCurrentTask->SetState(TS_DONE, TS_START);
            break;
        }
        this->mCurrentTask->SetState(TS_OPEN_FILE, TS_START);

    case TS_OPEN_FILE:
        this->mCardStatus = STATUS_UNKNOWN;
        this->mTaskSaveFile.mFileDescriptor =
            this->SceOpenFile(this->mTaskSaveFile.mCardID, this->mTaskSaveFile.mFileName, SCE_RDWR | SCE_CREAT);
        this->mCurrentTask->SetState(TS_OPEN_FILE_RESULT, TS_START);
        break;

    case TS_OPEN_FILE_RESULT:
        this->mTaskSaveFile.mCardStatus = this->mCardStatus;
        if (this->mCardStatus != STATUS_OK) {
            this->mCurrentTask->SetState(TS_DONE, TS_START);
            break;
        }

        if (this->mTaskSaveFile.mData == nullptr || this->mTaskSaveFile.mDataSize == 0) {
            this->mCurrentTask->SetState(TS_CLOSE_FILE, TS_START);
            break;
        }
        this->mCurrentTask->SetState(TS_WRITE, TS_START);

    case TS_WRITE:
        this->mCardStatus = STATUS_UNKNOWN;
        this->mCurrentFileDescriptor = this->mTaskSaveFile.mFileDescriptor;
        this->mCurrentFileDescriptor->mBytesWritten = 0;
        this->CheckSceResult(sceMcWrite(this->mCurrentFileDescriptor->mSceFileHandle, this->mTaskSaveFile.mData,
                                        this->mTaskSaveFile.mDataSize));
        this->mCurrentTask->SetState(TS_CLOSE_FILE, TS_START);
        break;

    case TS_CLOSE_FILE:
        this->mTaskSaveFile.mCardStatus = this->mCardStatus;
        this->mTaskSaveFile.mBytesWritten = this->mTaskSaveFile.mFileDescriptor->mBytesWritten;
        this->mCardStatus = STATUS_UNKNOWN;
        this->SceCloseFile(this->mTaskSaveFile.mFileDescriptor);
        this->mCurrentTask->SetState(TS_DONE, TS_START);
        break;

    case TS_DONE:
        this->mTaskSaveFile.mTaskResult =
            this->mTaskSaveFile.mBytesWritten != this->mTaskSaveFile.mDataSize ? RESULT_FAILED : RESULT_SUCCESS;
        if (this->mTaskSaveFile.mNotifyUser) {
            this->mMsg.mTaskResult = this->mTaskSaveFile.mTaskResult;
            this->mMsg.mCardStatus = this->mTaskSaveFile.mCardStatus;
        }
        this->mTaskSaveFile.End();
        break;
    }
}

void PS2Interface::UpdateTaskChangeDirectory() {
    switch (this->mCurrentTask->GetState()) {
    case TS_START:
        this->mTaskChangeDirectory.mTaskResult = RESULT_UNKNOWN;
        this->mTaskChangeDirectory.mCardStatus = STATUS_UNKNOWN;

        // Primero a la raiz; despues, si hace falta, al directorio pedido.
        this->mCardStatus = STATUS_UNKNOWN;
        this->CheckSceResult(sceMcChdir(this->mTaskChangeDirectory.mCardID.port, this->mTaskChangeDirectory.mCardID.slot,
                                        ROOT_DIRECTORY_NAME, nullptr));
        this->mCurrentTask->SetState(TS_CHANGE_DIRECTORY, TS_START);
        break;

    case TS_CHANGE_DIRECTORY:
        this->mCurrentTask->SetState(TS_DONE, TS_START);
        if (strcmp(this->mTaskChangeDirectory.mDirectoryName, ROOT_DIRECTORY_NAME) != 0) {
            if (this->mTaskChangeDirectory.mDirectoryName != nullptr && this->mTaskChangeDirectory.mDirectoryName[0] != '\0') {
                this->mCardStatus = STATUS_UNKNOWN;
                this->CheckSceResult(sceMcChdir(this->mTaskChangeDirectory.mCardID.port,
                                                this->mTaskChangeDirectory.mCardID.slot,
                                                this->mTaskChangeDirectory.mDirectoryName, nullptr));
                break;
            }
        }

    case TS_DONE:
        this->mTaskChangeDirectory.mTaskResult = this->mCardStatus != STATUS_OK ? RESULT_FAILED : RESULT_SUCCESS;
        this->mTaskChangeDirectory.mCardStatus = this->mCardStatus;
        this->mTaskChangeDirectory.End();
        break;
    }
}

// Un directorio de partida vale si se puede entrar en el y tiene su icon.sys y el fichero con su nombre.
void PS2Interface::UpdateTaskCheckDirectory() {
    switch (this->mTaskCheckDirectory.GetState()) {
    case TS_START:
        this->mTaskCheckDirectory.mTaskResult = RESULT_UNKNOWN;
        this->mTaskCheckDirectory.mCardStatus = STATUS_UNKNOWN;

        if (strcmp(this->mTaskCheckDirectory.mDirectoryName, ROOT_DIRECTORY_NAME) == 0) {
            this->mTaskCheckDirectory.mTaskResult = RESULT_SUCCESS;
            this->mTaskCheckDirectory.mCardStatus = STATUS_OK;
            this->mTaskCheckDirectory.SetState(TS_DONE, TS_START);
        } else {
            this->mTaskCheckDirectory.mCardStatus = STATUS_UNKNOWN;
            this->mTaskCheckDirectory.SetState(TS_CHANGE_DIRECTORY_RESULT, TS_START);
            this->mTaskChangeDirectory.Start(this->mTaskCheckDirectory.mCardID, this->mTaskCheckDirectory.mDirectoryName,
                                             false);
        }
        break;

    case TS_CHANGE_DIRECTORY_RESULT:
        this->mTaskCheckDirectory.mCardStatus = STATUS_DIRECTORY_NOT_FOUND;
        this->mTaskCheckDirectory.SetState(TS_DONE, TS_START);

        this->mTaskCheckDirectory.mCardStatus = this->mTaskChangeDirectory.mCardStatus;
        // Los errores que SceResultToStatus da para sceMcChdir, con sus huecos: la A124 despacha con una tabla de 11
        // entradas, y una lista sin huecos (1..10) se agrupa en un rango y sale con comparaciones.
        switch (this->mTaskChangeDirectory.mCardStatus) {
        case STATUS_NO_CARD:
        case STATUS_CARD_CHANGED:
        case STATUS_CARD_UNFORMATTED:
        case STATUS_CARD_FULL:
        case STATUS_ACCESS_DENIED:
        case STATUS_ENTRY_NOT_FOUND:
            break;
        case STATUS_OK:
            this->mTaskCheckDirectory.SetState(TS_SEARCH, TS_START);
            break;
        }
        break;

    case TS_SEARCH:
        switch (this->mTaskCheckDirectory.GetSubstate()) {
        case TS_START:
            this->mCardStatus = STATUS_UNKNOWN;
            this->mNumFilesFound = 0;
            this->CheckSceResult(sceMcGetDir(this->mTaskCheckDirectory.mCardID.port,
                                             this->mTaskCheckDirectory.mCardID.slot, gPs2IconSysFileName, 0, 1,
                                             mPs2FileTableEntry));
            this->mTaskCheckDirectory.SetSubstate(TS_FIND_ICONSYS_RESULT);
            break;

        case TS_FIND_ICONSYS_RESULT:
            if (this->mCardStatus == STATUS_OK || this->mCardStatus == STATUS_CARD_CHANGED) {
                if (this->mNumFilesFound > 0) {
                    this->mCardStatus = STATUS_UNKNOWN;
                    this->mNumFilesFound = 0;
                    this->CheckSceResult(sceMcGetDir(this->mTaskCheckDirectory.mCardID.port,
                                                     this->mTaskCheckDirectory.mCardID.slot,
                                                     this->mTaskCheckDirectory.mDirectoryName, 0, 1,
                                                     mPs2FileTableEntry));
                    this->mTaskCheckDirectory.SetSubstate(TS_FIND_DUMMYFILE_RESULT);
                } else {
                    this->mTaskCheckDirectory.mCardStatus = STATUS_FILE_CORRUPTED;
                    this->mTaskCheckDirectory.SetState(TS_DONE, TS_START);
                }
            } else {
                this->mTaskCheckDirectory.mCardStatus = this->mCardStatus;
                this->mTaskCheckDirectory.SetState(TS_DONE, TS_START);
            }
            break;

        case TS_FIND_DUMMYFILE_RESULT:
            if (this->mCardStatus == STATUS_OK || this->mCardStatus == STATUS_CARD_CHANGED) {
                if (this->mNumFilesFound > 0) {
                    this->mTaskCheckDirectory.mCardStatus = STATUS_OK;
                } else {
                    this->mTaskCheckDirectory.mCardStatus = STATUS_FILE_CORRUPTED;
                }
            } else {
                this->mTaskCheckDirectory.mCardStatus = this->mCardStatus;
            }
            this->mTaskCheckDirectory.SetState(TS_DONE, TS_START);
            break;
        }
        break;

    case TS_DONE:
        this->mTaskCheckDirectory.mTaskResult =
            this->mTaskCheckDirectory.mCardStatus != STATUS_OK ? RESULT_FAILED : RESULT_SUCCESS;
        this->mTaskCheckDirectory.End();
        break;
    }
}

} // namespace Realmc
