#include <sifdev.h>

#include "ps2_interface.h"

namespace Realmc {

const char *gPs2IconSysFileName = "icon.sys";
static char gPs2DummyFileData[8] = "dummy ";
int gPs2DummyFileSize = sizeof(gPs2DummyFileData);

void PS2Interface::UpdateTaskTrcStartGame()
{
    switch (this->mTaskTrcStartGame.GetState())
    {
    case TS_START:
        this->mTaskStartGameCommon.Start(this->mTaskTrcStartGame.mCardID, this->mTaskTrcStartGame.mSpaceNeeded);
        this->mTaskTrcStartGame.SetState(TS_DONE, TS_START);
        break;

    case TS_DONE:
        this->mTaskTrcStartGame.mCardID = this->mTaskStartGameCommon.mCardID;
        this->mTaskTrcStartGame.mTaskResult = this->mTaskStartGameCommon.mTaskResult;
        this->mTaskTrcStartGame.mCardStatus = this->mTaskStartGameCommon.mCardStatus;
        this->mMsg.Set(LMSG_TRC_STARTGAME_DONE, this->mTaskTrcStartGame.mTaskResult,
                       this->mTaskTrcStartGame.mCardStatus);
        this->mMsg.info.cardInfo.cardID = this->mTaskTrcStartGame.mCardID;
        this->mTaskTrcStartGame.End();
        break;
    }
}

void PS2Interface::UpdateTaskTrcGetCardInfo()
{
    if (!this->mMsgTimer.IsExpired())
    {
        return;
    }

    switch (this->mTaskTrcGetCardInfo.GetState())
    {
    case TS_START:
        this->mTaskTrcGetCardInfo.SetState(TS_CHECK_CARD, TS_START);
        if (!this->mNoCheckingCardWarning)
        {
            this->mMsg.TRC_msg(6, 0, this->PortStr(this->mTaskTrcGetCardInfo.mCardID));
            this->mMsgTimer.Set(1);
            break;
        }

    case TS_CHECK_CARD:
        this->mTaskTrcGetCardInfo.SetState(TS_CHECK_CARD_RESULT, TS_START);
        this->mTaskGetCardInfo.Start(this->mTaskTrcGetCardInfo.mCardID, TID_TRC_GETCARDINFO, false);
        break;

    case TS_CHECK_CARD_RESULT:
        switch (this->mTaskTrcGetCardInfo.GetSubstate())
        {
        case TS_START:
            this->mTaskTrcGetCardInfo.mTaskResult = this->mTaskGetCardInfo.mTaskResult;
            this->mTaskTrcGetCardInfo.mCardStatus = this->mTaskGetCardInfo.mCardStatus;

            switch (this->mTaskTrcGetCardInfo.mCardStatus)
            {
            case STATUS_NO_CARD:
            case STATUS_ACCESS_DENIED:
                this->mMsg.TRC_msg(7, PS2Message::PackMsgOptions(5, 0, 0, 0),
                                   this->PortStr(this->mTaskTrcGetCardInfo.mCardID));
                this->mUserMsg = UMSG_NONE;
                this->mTaskTrcGetCardInfo.SetSubstate(TS_WAIT_FOR_USER_REPLY);
                break;

            case STATUS_CARD_UNFORMATTED:
                this->mMsg.TRC_msg(8, PS2Message::PackMsgOptions(5, 0, 0, 0),
                                   this->PortStr(this->mTaskTrcGetCardInfo.mCardID));
                this->mUserMsg = UMSG_NONE;
                this->mTaskTrcGetCardInfo.SetSubstate(TS_WAIT_FOR_USER_REPLY);
                break;

            default:
                this->mTaskTrcGetCardInfo.SetState(TS_END, TS_START);
                break;
            }
            break;
        }

    case TS_WAIT_FOR_USER_REPLY:
        if (this->mUserMsg == UMSG_OPTION1)
        {
            this->mTaskTrcGetCardInfo.SetState(TS_END, TS_START);
        }
        break;

    case TS_END:
        if (this->mTaskTrcGetCardInfo.mTaskID == TID_TRC_CARDEXISTS)
        {
            this->mMsg.Set(LMSG_TRC_CARDEXISTS_DONE);
        }
        else
        {
            this->mMsg.Set(LMSG_TRC_GETCARDINFO_DONE);
        }

        this->mMsg.mTaskResult = this->mTaskGetCardInfo.mTaskResult;
        this->mMsg.mCardStatus = this->mTaskGetCardInfo.mCardStatus;
        this->mMsg.info.cardInfo.cardID = this->mTaskGetCardInfo.mCardID;
        this->mMsg.info.cardInfo.freeSpace = this->mTaskGetCardInfo.mFreeSpace;
        this->mTaskTrcGetCardInfo.End();
        break;
    }
}

void PS2Interface::UpdateTaskTrcSaveCheck()
{
    if (!this->mMsgTimer.IsExpired())
    {
        return;
    }

    switch (this->mTaskTrcSaveFile.GetState())
    {
    case TS_START:
        this->mTaskTrcSaveFile.SetState(TS_CHECK_CARD, TS_START);

    case TS_CHECK_CARD:
        switch (this->mTaskTrcSaveFile.GetSubstate())
        {
        case TS_START:
            this->mTaskTrcSaveFile.mTaskResult = RESULT_UNKNOWN;
            this->mTaskTrcSaveFile.mCardStatus = STATUS_UNKNOWN;
            this->mTaskTrcSaveFile.mIsSaving = false;
            this->mTaskTrcSaveFile.mFileDescriptor = nullptr;
            this->mTaskTrcSaveFile.SetSubstate(TS_CHECK_CARD);
            if (!this->mNoCheckingCardWarning)
            {
                this->mMsg.TRC_msg(6, 0, this->PortStr(this->mTaskTrcSaveFile.mCardID));
                this->mMsgTimer.Set(1);
                break;
            }

        case TS_CHECK_CARD:
            this->mTaskTrcSaveFile.SetSubstate(TS_CHECK_CARD_RESULT);
            this->mTaskGetCardInfo.Start(this->mTaskTrcSaveFile.mCardID, TID_CARD_EXISTS, false);
            break;

        case TS_CHECK_CARD_RESULT:
            this->mTaskTrcSaveFile.mCardStatus = this->mTaskGetCardInfo.mCardStatus;
            switch (this->mTaskTrcSaveFile.mCardStatus)
            {
            case STATUS_OK:
            case STATUS_CARD_CHANGED:
                this->mTaskTrcSaveFile.SetState(TS_CHECK_SPACE, TS_START);
                break;

            case STATUS_CARD_UNFORMATTED:
                this->mTaskTrcSaveFile.mCardStatus = STATUS_CARD_UNFORMATTED;
                this->mTaskTrcSaveFile.mTaskResult = RESULT_SUCCESS;
                this->mTaskTrcSaveFile.SetState(TS_DONE, TS_START);
                break;

            default:
                this->mTaskTrcSaveFile.mTaskResult = RESULT_FAILED;
                this->mTaskTrcSaveFile.mCardStatus = STATUS_NO_CARD;
                this->mTaskTrcSaveFile.SetState(TS_DONE, TS_START);
                break;
            }
            break;
        }
        break;

    case TS_CHECK_SPACE:
        switch (this->mTaskTrcSaveFile.GetSubstate())
        {
        case TS_START:
            this->mTaskTrcSaveFile.SetSubstate(TS_CHECK_SPACE_RESULT);
            this->mTaskCheckSpace.Start(this->mTaskTrcSaveFile.mCardID, &this->mTaskTrcSaveFile.mFileInfo,
                                        this->mTaskGetCardInfo.mFreeSpace, this->mTaskTrcSaveFile.mSpaceNeeded,
                                        false);
            break;

        case TS_CHECK_SPACE_RESULT:
            this->mTaskTrcSaveFile.mTaskResult = this->mTaskCheckSpace.mTaskResult;
            this->mTaskTrcSaveFile.mCardStatus = this->mTaskCheckSpace.mCardStatus;
            switch (this->mTaskTrcSaveFile.mCardStatus)
            {
            case STATUS_OK:
                if (this->mTaskCheckSpace.mEntryFound)
                {
                    this->mTaskTrcSaveFile.mCardStatus = STATUS_ENTRY_ALREADY_EXISTS;
                    this->mTaskTrcSaveFile.SetState(TS_OVERWRITE_ASK_USER, TS_START);
                }
                else
                {
                    this->mTaskTrcSaveFile.SetState(TS_CHECK_DIRECTORY, TS_START);
                }
                break;

            case STATUS_INSUFFICIENT_SPACE:
                this->mTaskTrcSaveFile.mTaskResult = RESULT_FAILED;
                this->mTaskTrcSaveFile.mCardStatus = STATUS_INSUFFICIENT_SPACE;
                this->mTaskTrcSaveFile.mSpaceNeeded = this->mTaskCheckSpace.mSpaceNeeded;
                this->mTaskTrcSaveFile.SetState(TS_DONE, TS_START);
                break;

            case STATUS_NO_CARD:
                this->mTaskTrcSaveFile.mTaskResult = RESULT_FAILED;
                this->mTaskTrcSaveFile.mCardStatus = STATUS_CARD_REMOVED;
                this->mTaskTrcSaveFile.SetState(TS_DONE, TS_START);
                break;
            }
            break;
        }
        break;

    case TS_CHECK_DIRECTORY:
        switch (this->mTaskTrcSaveFile.GetSubstate())
        {
        case TS_START:
            this->mTaskTrcSaveFile.SetSubstate(TS_CHECK_DIRECTORY_RESULT);
            this->mTaskCheckDirectory.Start(this->mTaskTrcSaveFile.mCardID,
                                            this->mTaskTrcSaveFile.mFileInfo.directoryInfo->directoryName);
            break;

        case TS_CHECK_DIRECTORY_RESULT:
            this->mTaskTrcSaveFile.mCardStatus = this->mTaskCheckDirectory.mCardStatus;
            switch (this->mTaskTrcSaveFile.mCardStatus)
            {
            case STATUS_OK:
            case STATUS_FILE_CORRUPTED:
                this->mTaskTrcSaveFile.SetState(TS_OVERWRITE_ASK_USER, TS_START);
                break;

            case STATUS_ENTRY_NOT_FOUND:
            default:
                this->mTaskTrcSaveFile.mTaskResult = RESULT_SUCCESS;
                this->mTaskTrcSaveFile.mCardStatus = STATUS_OK;
                this->mTaskTrcSaveFile.SetState(TS_DONE, TS_START);
                break;
            }
            break;
        }
        break;

    case TS_OVERWRITE_ASK_USER:
        switch (this->mTaskTrcSaveFile.GetSubstate())
        {
        case TS_START:
            this->mMsg.TRC_msg(19, PS2Message::PackMsgOptions(4, 3, 0, 0));
            this->mUserMsg = UMSG_NONE;
            this->mTaskTrcSaveFile.SetSubstate(TS_WAIT_FOR_USER_REPLY);
            break;

        case TS_WAIT_FOR_USER_REPLY:
            this->mTaskTrcSaveFile.mCardStatus =
                this->CheckCard(this->mTaskTrcSaveFile.mCardID, nullptr, nullptr, nullptr);
            if (this->mTaskTrcSaveFile.mCardStatus == STATUS_OK)
            {
                switch (this->mUserMsg)
                {
                case UMSG_OPTION1:
                    this->mTaskTrcSaveFile.mTaskResult = RESULT_CANCELLED;
                    this->mTaskTrcSaveFile.SetState(TS_DONE, TS_START);
                    break;

                case UMSG_OPTION2:
                    this->mTaskTrcSaveFile.mTaskResult = RESULT_SUCCESS;
                    this->mTaskTrcSaveFile.SetState(TS_DONE, TS_START);
                    break;
                }
            }
            else
            {
                this->mTaskTrcSaveFile.mTaskResult = RESULT_FAILED;
                this->mTaskTrcSaveFile.mCardStatus = STATUS_NO_CARD;
                this->mTaskTrcSaveFile.SetState(TS_DONE, TS_START);
            }
            break;
        }
        break;

    case TS_DONE:
        switch (this->mTaskTrcSaveFile.GetSubstate())
        {
        case TS_START:
            this->mTaskTrcSaveFile.SetSubstate(TS_END);
            if (this->mTaskTrcSaveFile.mTaskResult == RESULT_FAILED)
            {
                switch (this->mTaskTrcSaveFile.mCardStatus)
                {
                case STATUS_OK:
                    break;

                case STATUS_NO_CARD:
                    this->mMsg.TRC_msg(10, PS2Message::PackMsgOptions(4, 3, 0, 0),
                                       this->PortStr(this->mTaskTrcSaveFile.mCardID),
                                       this->mTaskTrcSaveFile.mSpaceNeeded);
                    this->mUserMsg = UMSG_NONE;
                    this->mTaskTrcSaveFile.SetSubstate(TS_WAIT_FOR_USER_REPLY);
                    break;

                case STATUS_INSUFFICIENT_SPACE:
                    this->mMsg.TRC_msg(11, PS2Message::PackMsgOptions(4, 3, 0, 0),
                                       this->PortStr(this->mTaskTrcSaveFile.mCardID),
                                       this->mTaskTrcSaveFile.mSpaceNeeded);
                    this->mUserMsg = UMSG_NONE;
                    this->mTaskTrcSaveFile.SetSubstate(TS_WAIT_FOR_USER_REPLY);
                    break;

                case STATUS_CARD_UNFORMATTED:
                    break;

                case STATUS_FILE_NOT_FOUND:
                    break;
                }
            }
            break;

        case TS_WAIT_FOR_USER_REPLY:
            switch (this->mUserMsg)
            {
            case UMSG_OPTION1:
                this->mTaskTrcSaveFile.mTaskResult = RESULT_CANCELLED;
                this->mTaskTrcSaveFile.SetSubstate(TS_END);
                break;

            case UMSG_OPTION2:
                this->mTaskTrcSaveFile.mTaskResult = RESULT_SUCCESS;
                this->mTaskTrcSaveFile.SetSubstate(TS_END);
                break;
            }
            break;

        case TS_END:
        {
            CardStatus status = this->CheckCard(this->mTaskTrcSaveFile.mCardID, nullptr, nullptr, nullptr);

            if ((status == STATUS_NO_CARD || status == STATUS_CARD_UNFORMATTED) &&
                this->mTaskTrcSaveFile.mCardStatus != STATUS_NO_CARD)
            {
                this->mTaskTrcSaveFile.mCardStatus = STATUS_CARD_REMOVED;
            }
            else if (status == STATUS_OK && this->mTaskTrcSaveFile.mCardStatus == STATUS_CARD_REMOVED)
            {
                this->mTaskTrcSaveFile.mCardStatus = STATUS_CARD_CHANGED;
            }
            else if (status == STATUS_OK && this->mTaskTrcSaveFile.mCardStatus == STATUS_INSUFFICIENT_SPACE)
            {
                this->mTaskTrcSaveFile.mCardStatus = STATUS_INSUFFICIENT_SPACE;
            }
            else if (status == STATUS_OK && this->mTaskTrcSaveFile.mCardStatus == STATUS_CARD_UNFORMATTED)
            {
                this->mTaskTrcSaveFile.mCardStatus = STATUS_CARD_UNFORMATTED;
            }
            else
            {
                this->mTaskTrcSaveFile.mCardStatus = status;
            }

            this->mMsg.Set(LMSG_TRC_SAVECHECK_DONE, this->mTaskTrcSaveFile.mTaskResult,
                           this->mTaskTrcSaveFile.mCardStatus);
            this->mTaskTrcSaveFile.End();
            break;
        }
        }
        break;
    }
}

void PS2Interface::UpdateTaskTrcSaveFile()
{
    if (!this->mMsgTimer.IsExpired())
    {
        return;
    }

    switch (this->mTaskTrcSaveFile.GetState())
    {
    case TS_START:
        this->mTaskTrcSaveFile.mSpaceNeeded =
            this->mBlockCalculator.CalculateBlockSize(this->mTaskTrcSaveFile.mCardID, this->mTaskTrcSaveFile.mFileInfo);
        this->mTaskTrcSaveFile.SetState(TS_CHECK_CARD, TS_START);

    case TS_CHECK_CARD:
        switch (this->mTaskTrcSaveFile.GetSubstate())
        {
        case TS_START:
            this->mTaskTrcSaveFile.mTaskResult = RESULT_UNKNOWN;
            this->mTaskTrcSaveFile.mCardStatus = STATUS_UNKNOWN;
            this->mTaskTrcSaveFile.mIsSaving = false;
            this->mTaskTrcSaveFile.mFileDescriptor = nullptr;
            this->mTaskTrcSaveFile.SetSubstate(TS_CHECK_CARD);
            if (!this->mNoCheckingCardWarning)
            {
                this->mMsg.TRC_msg(6, 0, this->PortStr(this->mTaskTrcSaveFile.mCardID));
                this->mMsgTimer.Set(1);
                break;
            }

        case TS_CHECK_CARD:
            this->mTaskTrcSaveFile.SetSubstate(TS_CHECK_CARD_RESULT);
            this->mTaskGetCardInfo.Start(this->mTaskTrcSaveFile.mCardID, TID_CARD_EXISTS, false);
            break;

        case TS_CHECK_CARD_RESULT:
            this->mTaskTrcSaveFile.mCardStatus = this->mTaskGetCardInfo.mCardStatus;
            switch (this->mTaskTrcSaveFile.mCardStatus)
            {
            case STATUS_OK:
            case STATUS_CARD_CHANGED:
                this->mTaskTrcSaveFile.SetState(TS_CHECK_SPACE, TS_START);
                break;

            case STATUS_CARD_UNFORMATTED:
                this->mTaskTrcSaveFile.SetState(TS_FORMAT, TS_START);
                break;

            default:
                this->mTaskTrcSaveFile.mTaskResult = RESULT_FAILED;
                this->mTaskTrcSaveFile.mCardStatus = STATUS_NO_CARD;
                this->mTaskTrcSaveFile.SetState(TS_DONE, TS_START);
                break;
            }
            break;
        }
        break;

    case TS_CHECK_SPACE:
        switch (this->mTaskTrcSaveFile.GetSubstate())
        {
        case TS_START:
            this->mTaskTrcSaveFile.SetSubstate(TS_CHECK_SPACE_RESULT);
            this->mTaskCheckSpace.Start(this->mTaskTrcSaveFile.mCardID, &this->mTaskTrcSaveFile.mFileInfo,
                                        this->mTaskGetCardInfo.mFreeSpace, false);
            break;

        case TS_CHECK_SPACE_RESULT:
            this->mTaskTrcSaveFile.mTaskResult = this->mTaskCheckSpace.mTaskResult;
            this->mTaskTrcSaveFile.mCardStatus = this->mTaskCheckSpace.mCardStatus;
            switch (this->mTaskTrcSaveFile.mCardStatus)
            {
            case STATUS_OK:
                if (this->mTaskCheckSpace.mEntryFound)
                {
                    this->mTaskTrcSaveFile.mCardStatus = STATUS_ENTRY_ALREADY_EXISTS;
                    this->mTaskTrcSaveFile.SetState(TS_OVERWRITE_ASK_USER, TS_START);
                }
                else
                {
                    this->mTaskTrcSaveFile.SetState(TS_CHECK_MAX_FILES, TS_START);
                }
                break;

            case STATUS_INSUFFICIENT_SPACE:
                this->mTaskTrcSaveFile.mTaskResult = RESULT_FAILED;
                this->mTaskTrcSaveFile.mSpaceNeeded = this->mTaskCheckSpace.mSpaceNeeded;
                this->mTaskTrcSaveFile.SetState(TS_DONE, TS_START);
                break;

            case STATUS_NO_CARD:
                this->mTaskTrcSaveFile.mTaskResult = RESULT_FAILED;
                this->mTaskTrcSaveFile.mCardStatus = STATUS_CARD_REMOVED;
                this->mTaskTrcSaveFile.SetState(TS_DONE, TS_START);
                break;
            }
            break;
        }
        break;

    case TS_FORMAT:
        switch (this->mTaskTrcSaveFile.GetSubstate())
        {
        case TS_START:
            this->mMsg.TRC_msg(22, PS2Message::PackMsgOptions(4, 3, 0, 0),
                               this->PortStr(this->mTaskTrcSaveFile.mCardID));
            this->mUserMsg = UMSG_NONE;
            this->mTaskTrcSaveFile.mConfirmedByUser = false;
            this->mTaskTrcSaveFile.SetSubstate(TS_FORMAT_WAIT_FOR_USER_CONFIRMATION);
            break;

        case TS_FORMAT_WAIT_FOR_USER_CONFIRMATION:
            switch (this->CheckCard(this->mTaskTrcSaveFile.mCardID, nullptr, nullptr, nullptr))
            {
            case STATUS_NO_CARD:
            case STATUS_CARD_CHANGED:
                this->mTaskTrcSaveFile.mTaskResult = RESULT_FAILED;
                this->mTaskTrcSaveFile.mCardStatus = STATUS_CARD_REMOVED;
                break;
            }

            switch (this->mUserMsg)
            {
            case UMSG_OPTION1:
                this->mTaskTrcSaveFile.mTaskResult = RESULT_CANCELLED;
                this->mTaskTrcSaveFile.SetState(TS_DONE, TS_START);
                break;

            case UMSG_OPTION2:
                if (this->mTaskTrcSaveFile.mCardStatus == STATUS_CARD_REMOVED)
                {
                    this->mTaskTrcSaveFile.SetState(TS_DONE, TS_START);
                }
                else if (!this->mTaskTrcSaveFile.mConfirmedByUser)
                {
                    this->mMsg.TRC_msg(27, PS2Message::PackMsgOptions(4, 3, 0, 0),
                                       this->PortStr(this->mTaskTrcSaveFile.mCardID));
                    this->mUserMsg = UMSG_NONE;
                    this->mTaskTrcSaveFile.mConfirmedByUser = true;
                }
                else
                {
                    this->mMsg.TRC_msg(28, 0, this->PortStr(this->mTaskTrcSaveFile.mCardID));
                    this->mMsgTimer.Set(0);
                    this->mTaskTrcSaveFile.SetSubstate(TS_FORMAT);
                }
                break;
            }
            break;

        case TS_FORMAT:
            this->mTaskTrcSaveFile.SetSubstate(TS_CHECK_FORMAT_RESULT);
            this->mTaskFormat.Start(this->mTaskTrcSaveFile.mCardID, false);
            break;

        case TS_CHECK_FORMAT_RESULT:
            if (this->mTaskFormat.mCardStatus == STATUS_OK)
            {
                this->mMsg.TRC_msg(30, 0);
                this->mMsgTimer.Set(0);
                this->mTaskTrcSaveFile.SetSubstate(TS_CHECK_CARD);
            }
            else
            {
                this->mUserMsg = UMSG_NONE;
                this->mMsg.TRC_msg(29, PS2Message::PackMsgOptions(4, 3, 0, 0),
                                   this->PortStr(this->mTaskFormat.mCardID));
                this->mTaskTrcSaveFile.SetSubstate(TS_WAIT_FOR_USER_REPLY);
            }
            break;

        case TS_WAIT_FOR_USER_REPLY:
            if (this->mUserMsg != UMSG_NONE)
            {
                if (this->mTaskFormat.mCardStatus == STATUS_OK)
                {
                    this->mTaskTrcSaveFile.SetState(TS_CHECK_CARD, TS_START);
                }
                else
                {
                    this->mTaskTrcSaveFile.SetState(TS_DONE, TS_START);
                    if (this->mUserMsg == UMSG_OPTION1)
                    {
                        this->mTaskTrcSaveFile.mTaskResult = RESULT_RETRY;
                    }
                    else
                    {
                        this->mTaskTrcSaveFile.mTaskResult = RESULT_CANCELLED;
                    }
                }
            }
            break;

        case TS_CHECK_CARD:
            if (this->CheckCard(this->mTaskTrcSaveFile.mCardID, nullptr, nullptr, nullptr) == STATUS_OK)
            {
                this->mTaskTrcSaveFile.SetState(TS_CHECK_CARD, TS_START);
            }
            else
            {
                this->mTaskTrcSaveFile.mTaskResult = RESULT_FAILED;
                this->mTaskTrcSaveFile.mCardStatus = STATUS_CARD_REMOVED;
                this->mTaskTrcSaveFile.SetState(TS_DONE, TS_START);
            }
            break;
        }
        break;

    case TS_CHECK_MAX_FILES:
        switch (this->mTaskTrcSaveFile.GetSubstate())
        {
        case TS_START:
        {
            char path[64];

            sprintf(path, "/%s/*", this->mTaskTrcSaveFile.mFileInfo.directoryInfo->directoryName);
            this->mCardStatus = STATUS_UNKNOWN;
            this->mNumFilesFound = 0;
            this->CheckSceResult(sceMcGetDir(this->mTaskTrcSaveFile.mCardID.port, this->mTaskTrcSaveFile.mCardID.slot,
                                             path, 0, -1, nullptr));
            this->mTaskTrcSaveFile.SetSubstate(TS_CHECK_MAX_FILES_RESULT);
            break;
        }

        case TS_CHECK_MAX_FILES_RESULT:
            this->mNumFilesFound -= 2;
            if (this->mNumFilesFound < MAX_FILES_PER_DIRECTORY)
            {
                this->mTaskTrcSaveFile.SetState(TS_CHECK_DIRECTORY, TS_START);
            }
            else
            {
                this->mMsg.TRC_msg(38, PS2Message::PackMsgOptions(5, 0, 0, 0), (char *)this->mGameTitle);
                this->mTaskTrcSaveFile.SetSubstate(TS_FILE_LIMIT_WAIT_FOR_REPLY);
                this->mUserMsg = UMSG_NONE;
            }
            break;

        case TS_FILE_LIMIT_WAIT_FOR_REPLY:
            if (this->mUserMsg == UMSG_OPTION1)
            {
                if (this->CheckCard(this->mTaskTrcSaveFile.mCardID, nullptr, nullptr, nullptr) == STATUS_OK)
                {
                    this->mTaskTrcSaveFile.mTaskResult = RESULT_CANCELLED;
                    this->mTaskTrcSaveFile.mCardStatus = STATUS_OK;
                }
                else
                {
                    this->mTaskTrcSaveFile.mTaskResult = RESULT_FAILED;
                    this->mTaskTrcSaveFile.mCardStatus = STATUS_CARD_REMOVED;
                }

                this->mTaskTrcSaveFile.SetState(TS_DONE, TS_START);
            }
            break;
        }
        break;

    case TS_CHECK_DIRECTORY:
        switch (this->mTaskTrcSaveFile.GetSubstate())
        {
        case TS_START:
            this->mTaskTrcSaveFile.SetSubstate(TS_CHECK_DIRECTORY_RESULT);
            this->mTaskCheckDirectory.Start(this->mTaskTrcSaveFile.mCardID,
                                            this->mTaskTrcSaveFile.mFileInfo.directoryInfo->directoryName);
            break;

        case TS_CHECK_DIRECTORY_RESULT:
            this->mTaskTrcSaveFile.mCardStatus = this->mTaskCheckDirectory.mCardStatus;
            switch (this->mTaskTrcSaveFile.mCardStatus)
            {
            case STATUS_OK:
            case STATUS_FILE_CORRUPTED:
                this->mTaskTrcSaveFile.SetState(TS_OVERWRITE_ASK_USER, TS_START);
                break;

            case STATUS_ENTRY_NOT_FOUND:
            default:
                this->mTaskTrcSaveFile.SetState(TS_SHOW_SAVE_WARNING, TS_START);
                break;
            }
            break;
        }
        break;

    case TS_OVERWRITE_ASK_USER:
        switch (this->mTaskTrcSaveFile.GetSubstate())
        {
        case TS_START:
            this->mMsg.TRC_msg(23, PS2Message::PackMsgOptions(4, 3, 0, 0));
            this->mUserMsg = UMSG_NONE;
            this->mTaskTrcSaveFile.SetSubstate(TS_WAIT_FOR_USER_REPLY);
            break;

        case TS_WAIT_FOR_USER_REPLY:
            this->mTaskTrcSaveFile.mCardStatus =
                this->CheckCard(this->mTaskTrcSaveFile.mCardID, nullptr, nullptr, nullptr);
            if (this->mTaskTrcSaveFile.mCardStatus == STATUS_OK)
            {
                switch (this->mUserMsg)
                {
                case UMSG_OPTION1:
                    this->mTaskTrcSaveFile.mTaskResult = RESULT_CANCELLED;
                    this->mTaskTrcSaveFile.SetState(TS_DONE, TS_START);
                    break;

                case UMSG_OPTION2:
                    this->mTaskTrcSaveFile.SetState(TS_SHOW_SAVE_WARNING, TS_START);
                    break;
                }
            }
            else
            {
                this->mTaskTrcSaveFile.mTaskResult = RESULT_FAILED;
                this->mTaskTrcSaveFile.mCardStatus = STATUS_CARD_REMOVED;
                this->mTaskTrcSaveFile.SetState(TS_DONE, TS_START);
            }
            break;
        }
        break;

    case TS_SHOW_SAVE_WARNING:
        this->mTaskTrcSaveFile.mIsSaving = true;
        this->mMsg.TRC_msg(24, 0, this->PortStr(this->mTaskTrcSaveFile.mCardID));
        this->mMsgTimer.Set(0);
        if (this->mTaskCheckSpace.mEntryFound)
        {
            this->mTaskTrcSaveFile.SetState(TS_SAVE_DIRECTORY_FILES, TS_START);
        }
        else
        {
            this->mTaskTrcSaveFile.SetState(TS_CREATE_DIRECTORY, TS_START);
        }
        break;

    case TS_CREATE_DIRECTORY:
        switch (this->mTaskTrcSaveFile.GetSubstate())
        {
        case TS_START:
            this->mCardStatus = STATUS_UNKNOWN;
            this->CheckSceResult(sceMcMkdir(this->mTaskTrcSaveFile.mCardID.port, this->mTaskTrcSaveFile.mCardID.slot,
                                            this->mTaskTrcSaveFile.mFileInfo.directoryInfo->directoryName));
            this->mTaskTrcSaveFile.SetSubstate(TS_CREATE_DIRECTORY_RESULT);
            break;

        case TS_CREATE_DIRECTORY_RESULT:
            this->mTaskTrcSaveFile.mCardStatus = this->mCardStatus;
            if (this->mCardStatus != STATUS_OK)
            {
                this->mTaskTrcSaveFile.mTaskResult = RESULT_FAILED;
                this->mTaskTrcSaveFile.SetState(TS_DONE, TS_START);
            }
            else
            {
                this->mTaskTrcSaveFile.SetState(TS_SAVE_DIRECTORY_FILES, TS_START);
            }
            break;
        }
        break;

    case TS_SAVE_DIRECTORY_FILES:
        switch (this->mTaskTrcSaveFile.GetSubstate())
        {
        case TS_START:
            this->mTaskTrcSaveFile.SetSubstate(TS_SAVE_STATIC_ICON);

        case TS_SAVE_STATIC_ICON:
            this->mTaskTrcSaveFile.SetSubstate(TS_SAVE_STATIC_ICON_RESULT);
            this->mTaskSaveFile.Start(this->mTaskTrcSaveFile.mCardID,
                                      this->mTaskTrcSaveFile.mFileInfo.directoryInfo->directoryName,
                                      this->mTaskTrcSaveFile.mFileInfo.directoryInfo->staticIconFilename,
                                      this->mTaskTrcSaveFile.mFileInfo.directoryInfo->staticIconData,
                                      this->mTaskTrcSaveFile.mFileInfo.directoryInfo->staticIconDataSize);
            break;

        case TS_SAVE_STATIC_ICON_RESULT:
            if (this->mTaskSaveFile.mTaskResult != RESULT_SUCCESS)
            {
                this->mTaskTrcSaveFile.mTaskResult = RESULT_FAILED;
                this->mTaskTrcSaveFile.mCardStatus = this->mTaskSaveFile.mCardStatus;
                this->mTaskTrcSaveFile.SetState(TS_DONE, TS_START);
                break;
            }
            this->mTaskTrcSaveFile.SetSubstate(TS_SAVE_COPY_ICON);

        case TS_SAVE_COPY_ICON:
            if (this->mTaskTrcSaveFile.mFileInfo.directoryInfo->copyIconData == nullptr)
            {
                this->mTaskTrcSaveFile.SetSubstate(TS_SAVE_DELETE_ICON);
                break;
            }
            this->mTaskTrcSaveFile.SetSubstate(TS_SAVE_COPY_ICON_RESULT);
            this->mTaskSaveFile.Start(this->mTaskTrcSaveFile.mCardID,
                                      this->mTaskTrcSaveFile.mFileInfo.directoryInfo->directoryName,
                                      this->mTaskTrcSaveFile.mFileInfo.directoryInfo->copyIconFilename,
                                      this->mTaskTrcSaveFile.mFileInfo.directoryInfo->copyIconData,
                                      this->mTaskTrcSaveFile.mFileInfo.directoryInfo->copyIconDataSize);
            break;

        case TS_SAVE_COPY_ICON_RESULT:
            if (this->mTaskSaveFile.mTaskResult != RESULT_SUCCESS)
            {
                this->mTaskTrcSaveFile.mTaskResult = RESULT_FAILED;
                this->mTaskTrcSaveFile.mCardStatus = this->mTaskSaveFile.mCardStatus;
                this->mTaskTrcSaveFile.SetState(TS_DONE, TS_START);
                break;
            }
            this->mTaskTrcSaveFile.SetSubstate(TS_SAVE_DELETE_ICON);

        case TS_SAVE_DELETE_ICON:
            if (this->mTaskTrcSaveFile.mFileInfo.directoryInfo->deleteIconData == nullptr)
            {
                this->mTaskTrcSaveFile.SetSubstate(TS_CHECK_DUMMY_FILE);
                break;
            }
            this->mTaskTrcSaveFile.SetSubstate(TS_SAVE_DELETE_ICON_RESULT);
            this->mTaskSaveFile.Start(this->mTaskTrcSaveFile.mCardID,
                                      this->mTaskTrcSaveFile.mFileInfo.directoryInfo->directoryName,
                                      this->mTaskTrcSaveFile.mFileInfo.directoryInfo->deleteIconFilename,
                                      this->mTaskTrcSaveFile.mFileInfo.directoryInfo->deleteIconData,
                                      this->mTaskTrcSaveFile.mFileInfo.directoryInfo->deleteIconDataSize);
            break;

        case TS_SAVE_DELETE_ICON_RESULT:
            if (this->mTaskSaveFile.mTaskResult != RESULT_SUCCESS)
            {
                this->mTaskTrcSaveFile.mTaskResult = RESULT_FAILED;
                this->mTaskTrcSaveFile.mCardStatus = this->mTaskSaveFile.mCardStatus;
                this->mTaskTrcSaveFile.SetState(TS_DONE, TS_START);
                break;
            }
            this->mTaskTrcSaveFile.SetSubstate(TS_CHECK_DUMMY_FILE);

        case TS_CHECK_DUMMY_FILE:
            if (strcmp(this->mTaskTrcSaveFile.mFileInfo.fileName,
                       this->mTaskTrcSaveFile.mFileInfo.directoryInfo->directoryName) != 0)
            {
                this->mTaskTrcSaveFile.SetSubstate(TS_CHECK_DUMMY_FILE_RESULT);
                this->mTaskSaveFile.Start(this->mTaskTrcSaveFile.mCardID,
                                          this->mTaskTrcSaveFile.mFileInfo.directoryInfo->directoryName,
                                          this->mTaskTrcSaveFile.mFileInfo.directoryInfo->directoryName,
                                          gPs2DummyFileData, sizeof(gPs2DummyFileData));
            }
            else
            {
                this->mTaskTrcSaveFile.SetState(TS_OPEN_FILE, TS_START);
            }
            break;

        case TS_CHECK_DUMMY_FILE_RESULT:
            if (this->mTaskSaveFile.mTaskResult != RESULT_SUCCESS)
            {
                this->mTaskTrcSaveFile.mTaskResult = RESULT_FAILED;
                this->mTaskTrcSaveFile.mCardStatus = this->mTaskSaveFile.mCardStatus;
                this->mTaskTrcSaveFile.SetState(TS_DONE, TS_START);
                break;
            }
            this->mTaskTrcSaveFile.SetState(TS_OPEN_FILE, TS_START);
            break;
        }
        break;

    case TS_OPEN_FILE:
        switch (this->mTaskTrcSaveFile.GetSubstate())
        {
        case TS_START:
            if (this->mTaskCheckSpace.mFileExists)
            {
                char path[128];

                sprintf(path, "/%s/%s", this->mTaskTrcSaveFile.mFileInfo.directoryInfo->directoryName,
                        this->mTaskTrcSaveFile.mFileInfo.fileName);
                this->CheckSceResult(
                    sceMcDelete(this->mTaskTrcSaveFile.mCardID.port, this->mTaskTrcSaveFile.mCardID.slot, path));
            }
            this->mTaskTrcSaveFile.SetSubstate(TS_OPEN_FILE);
            break;

        case TS_OPEN_FILE:
            this->mCardStatus = STATUS_UNKNOWN;
            this->mTaskTrcSaveFile.mFileDescriptor = this->SceOpenFile(
                this->mTaskTrcSaveFile.mCardID, this->mTaskTrcSaveFile.mFileInfo.fileName, SCE_RDWR | SCE_CREAT);
            this->mTaskTrcSaveFile.SetSubstate(TS_OPEN_FILE_RESULT);
            break;

        case TS_OPEN_FILE_RESULT:
            if (this->mCardStatus == STATUS_OK)
            {
                this->mMsg.Set(LMSG_OPEN_FILE_DONE, RESULT_SUCCESS, STATUS_OK);
                this->mMsg.info.openResult.fileHandle = (OpenFileDescriptor *)this->mTaskTrcSaveFile.mFileDescriptor;
                this->mMsg.info.openResult.fileName = this->mTaskTrcSaveFile.mFileInfo.fileName;
                this->mUserMsg = UMSG_NONE;
                this->mTaskTrcSaveFile.SetState(TS_WRITE_WAIT, TS_START);
            }
            else
            {
                this->mTaskTrcSaveFile.mCardStatus = this->mCardStatus;
                this->mTaskTrcSaveFile.mTaskResult = RESULT_FAILED;
                this->mTaskTrcSaveFile.SetState(TS_DONE, TS_START);
            }
            break;
        }
        break;

    case TS_WRITE_WAIT:
        switch (this->mTaskTrcSaveFile.GetSubstate())
        {
        case TS_START:
            this->mMsg.Set(LMSG_WRITE_READY);
            this->mTaskTrcSaveFile.SetSubstate(TS_WRITE_WAIT);
            break;

        case TS_WRITE_WAIT:
            if (this->mTaskWrite.mTaskResult != RESULT_SUCCESS)
            {
                this->mTaskTrcSaveFile.mTaskResult = RESULT_FAILED;
                this->mTaskTrcSaveFile.mCardStatus = this->mTaskWrite.mCardStatus;
                this->mTaskTrcSaveFile.SetState(TS_DONE, TS_START);
            }
            else if (this->mUserMsg == UMSG_WRITE_COMPLETE)
            {
                this->mTaskTrcSaveFile.SetState(TS_SAVE_ICON_SYS, TS_START);
            }
            else
            {
                break;
            }

            this->SceCloseFile(this->mTaskTrcSaveFile.mFileDescriptor);
            break;
        }
        break;

    case TS_SAVE_ICON_SYS:
        this->mTaskTrcSaveFile.SetState(TS_SAVE_ICON_SYS_RESULT, TS_START);
        this->mTaskSaveFile.Start(this->mTaskTrcSaveFile.mCardID,
                                  this->mTaskTrcSaveFile.mFileInfo.directoryInfo->directoryName, gPs2IconSysFileName,
                                  this->mTaskTrcSaveFile.mFileInfo.directoryInfo->iconSysData,
                                  this->mTaskTrcSaveFile.mFileInfo.directoryInfo->iconSysDataSize);
        break;

    case TS_SAVE_ICON_SYS_RESULT:
        if (this->mTaskSaveFile.mTaskResult == RESULT_SUCCESS)
        {
            this->mTaskTrcSaveFile.mTaskResult = RESULT_SUCCESS;
            this->mTaskTrcSaveFile.mCardStatus = STATUS_OK;
        }
        else
        {
            this->mTaskTrcSaveFile.mTaskResult = RESULT_FAILED;
            this->mTaskTrcSaveFile.mCardStatus = this->mTaskSaveFile.mCardStatus;
        }
        this->mTaskTrcSaveFile.SetState(TS_DONE, TS_START);
        break;

    case TS_DONE:
        switch (this->mTaskTrcSaveFile.GetSubstate())
        {
        case TS_START:
            this->mTaskTrcSaveFile.SetSubstate(TS_END);
            if (this->mTaskTrcSaveFile.mTaskResult == RESULT_SUCCESS)
            {
                if (this->mIsAutosaveEnabled)
                {
                    this->mMsg.TRC_msg(26, 0);
                    this->mMsgTimer.Set(0);
                    this->mTaskTrcSaveFile.SetSubstate(TS_END);
                }
                else
                {
                    this->mMsg.TRC_msg(26, PS2Message::PackMsgOptions(5, 0, 0, 0));
                    this->mUserMsg = UMSG_NONE;
                    this->mTaskTrcSaveFile.SetSubstate(TS_WAIT_FOR_USER_REPLY);
                }
            }
            else if (this->mTaskTrcSaveFile.mTaskResult != RESULT_CANCELLED)
            {
                if (this->mTaskTrcSaveFile.mIsSaving)
                {
                    this->mMsg.TRC_msg(25, PS2Message::PackMsgOptions(4, 3, 0, 0),
                                       this->PortStr(this->mTaskTrcSaveFile.mCardID));
                    this->mUserMsg = UMSG_NONE;
                    this->mTaskTrcSaveFile.SetSubstate(TS_WAIT_FOR_USER_REPLY);
                }
                else
                {
                    switch (this->mTaskTrcSaveFile.mCardStatus)
                    {
                    case STATUS_OK:
                        break;

                    case STATUS_CARD_UNFORMATTED:
                        break;

                    case STATUS_NO_CARD:
                        this->mMsg.TRC_msg(20, PS2Message::PackMsgOptions(4, 3, 0, 0),
                                           this->PortStr(this->mTaskTrcSaveFile.mCardID),
                                           this->mTaskTrcSaveFile.mSpaceNeeded);
                        this->mUserMsg = UMSG_NONE;
                        this->mTaskTrcSaveFile.SetSubstate(TS_WAIT_FOR_USER_REPLY);
                        break;

                    case STATUS_CARD_REMOVED:
                        this->mMsg.TRC_msg(40, PS2Message::PackMsgOptions(3, 4, 0, 0),
                                           this->PortStr(this->mTaskTrcSaveFile.mCardID));
                        this->mUserMsg = UMSG_NONE;
                        this->mTaskTrcSaveFile.SetSubstate(TS_WAIT_FOR_USER_REPLY);
                        break;

                    case STATUS_INSUFFICIENT_SPACE:
                        this->mMsg.TRC_msg(21, PS2Message::PackMsgOptions(4, 3, 0, 0),
                                           this->PortStr(this->mTaskTrcSaveFile.mCardID),
                                           this->mTaskTrcSaveFile.mSpaceNeeded);
                        this->mUserMsg = UMSG_NONE;
                        this->mTaskTrcSaveFile.SetSubstate(TS_WAIT_FOR_USER_REPLY);
                        break;
                    }
                }
            }
            break;

        case TS_WAIT_FOR_USER_REPLY:
            if (this->mUserMsg != UMSG_NONE)
            {
                if (this->mTaskTrcSaveFile.mTaskResult != RESULT_SUCCESS)
                {
                    if (this->mUserMsg == UMSG_OPTION1)
                    {
                        this->mTaskTrcSaveFile.mTaskResult = RESULT_RETRY;
                    }
                    else
                    {
                        this->mTaskTrcSaveFile.mTaskResult = RESULT_CANCELLED;
                    }
                }
                this->mTaskTrcSaveFile.SetSubstate(TS_END);
            }
            break;

        case TS_END:
        {
            CardStatus status = this->CheckCard(this->mTaskTrcSaveFile.mCardID, nullptr, nullptr, nullptr);

            if ((status == STATUS_NO_CARD || status == STATUS_CARD_UNFORMATTED) &&
                this->mTaskTrcSaveFile.mCardStatus != STATUS_NO_CARD)
            {
                this->mTaskTrcSaveFile.mCardStatus = STATUS_CARD_REMOVED;
            }
            else if (status == STATUS_OK && this->mTaskTrcSaveFile.mCardStatus == STATUS_CARD_REMOVED)
            {
                this->mTaskTrcSaveFile.mCardStatus = STATUS_CARD_CHANGED;
            }
            else if (status == STATUS_OK && this->mTaskTrcSaveFile.mCardStatus == STATUS_INSUFFICIENT_SPACE)
            {
                this->mTaskTrcSaveFile.mCardStatus = STATUS_INSUFFICIENT_SPACE;
            }
            else if (status == STATUS_OK && this->mTaskTrcSaveFile.mCardStatus == STATUS_CARD_UNFORMATTED)
            {
                this->mTaskTrcSaveFile.mCardStatus = STATUS_CARD_UNFORMATTED;
            }
            else
            {
                this->mTaskTrcSaveFile.mCardStatus = status;
            }

            this->mMsg.Set(LMSG_TRC_SAVEFILE_DONE, this->mTaskTrcSaveFile.mTaskResult,
                           this->mTaskTrcSaveFile.mCardStatus);
            this->mTaskTrcSaveFile.End();
            break;
        }
        }
        break;
    }
}

void PS2Interface::UpdateTaskTrcLoadFile()
{
    switch (this->mTaskTrcLoadFile.GetState())
    {
    case TS_START:
        this->mTaskTrcLoadFile.mTaskResult = RESULT_UNKNOWN;
        this->mTaskTrcLoadFile.mCardStatus = STATUS_UNKNOWN;
        this->mTaskTrcLoadFile.mIsLoading = false;
        this->mTaskTrcLoadFile.SetState(TS_CHECK_CARD, TS_START);

    case TS_CHECK_CARD:
        switch (this->mTaskTrcLoadFile.GetSubstate())
        {
        case TS_START:
            this->mTaskTrcLoadFile.SetSubstate(TS_CHECK_CARD);
            if (!this->mNoCheckingCardWarning)
            {
                this->mMsg.TRC_msg(6, 0, this->PortStr(this->mTaskTrcLoadFile.mCardID));
                this->mMsgTimer.Set(1);
                break;
            }

        case TS_CHECK_CARD:
            this->mTaskTrcLoadFile.SetSubstate(TS_CHECK_CARD_RESULT);
            this->mTaskGetCardInfo.Start(this->mTaskTrcLoadFile.mCardID, TID_CARD_EXISTS, false);
            break;

        case TS_CHECK_CARD_RESULT:
            if (this->mMsgTimer.IsExpired())
            {
                this->mTaskTrcLoadFile.mCardStatus = this->mTaskGetCardInfo.mCardStatus;
                switch (this->mTaskTrcLoadFile.mCardStatus)
                {
                case STATUS_OK:
                case STATUS_CARD_CHANGED:
                    this->mTaskTrcLoadFile.SetState(TS_FIND_FILE, TS_START);
                    break;

                case STATUS_CARD_UNFORMATTED:
                    this->mTaskTrcLoadFile.mTaskResult = RESULT_FAILED;
                    this->mTaskTrcLoadFile.SetState(TS_DONE, TS_START);
                    break;

                default:
                    this->mTaskTrcLoadFile.mTaskResult = RESULT_FAILED;
                    this->mTaskTrcLoadFile.mCardStatus = STATUS_NO_CARD;
                    this->mTaskTrcLoadFile.SetState(TS_DONE, TS_START);
                    break;
                }
            }
            break;
        }
        break;

    case TS_FIND_FILE:
        switch (this->mTaskTrcLoadFile.GetSubstate())
        {
        case TS_START:
            this->mTaskTrcLoadFile.SetSubstate(TS_CHECK_DIRECTORY_RESULT);
            this->mTaskCheckDirectory.Start(this->mTaskTrcLoadFile.mCardID,
                                            this->mTaskTrcLoadFile.mFileInfo.directoryInfo->directoryName);
            break;

        case TS_CHECK_DIRECTORY_RESULT:
            this->mTaskTrcLoadFile.mCardStatus = this->mTaskCheckDirectory.mCardStatus;
            switch (this->mTaskTrcLoadFile.mCardStatus)
            {
            case STATUS_OK:
                this->mTaskTrcLoadFile.SetSubstate(TS_CHANGE_DIRECTORY);
                break;

            case STATUS_ENTRY_NOT_FOUND:
            case STATUS_DIRECTORY_NOT_FOUND:
                this->mTaskTrcLoadFile.mTaskResult = RESULT_FAILED;
                this->mTaskTrcLoadFile.mCardStatus = STATUS_FILE_NOT_FOUND;
                this->mTaskTrcLoadFile.SetState(TS_DONE, TS_START);
                break;

            case STATUS_FILE_CORRUPTED:
                this->mTaskTrcLoadFile.mTaskResult = RESULT_FAILED;
                this->mTaskTrcLoadFile.SetState(TS_DONE, TS_START);
                break;

            default:
                this->mTaskTrcLoadFile.mTaskResult = RESULT_FAILED;
                this->mTaskTrcLoadFile.SetState(TS_DONE, TS_START);
                break;
            }
            break;

        case TS_CHANGE_DIRECTORY:
            this->mTaskTrcLoadFile.mIsLoading = true;
            this->mMsg.TRC_msg(15, 0, this->PortStr(this->mTaskTrcLoadFile.mCardID));
            this->mMsgTimer.Set(0);
            this->mTaskTrcLoadFile.SetSubstate(TS_CHANGE_DIRECTORY_RESULT);
            this->mTaskChangeDirectory.Start(this->mTaskTrcLoadFile.mCardID,
                                             this->mTaskTrcLoadFile.mFileInfo.directoryInfo->directoryName, false);
            break;

        case TS_CHANGE_DIRECTORY_RESULT:
            this->mTaskTrcLoadFile.mCardStatus = this->mTaskChangeDirectory.mCardStatus;
            switch (this->mTaskTrcLoadFile.mCardStatus)
            {
            case STATUS_OK:
            case STATUS_CARD_CHANGED:
                this->mTaskTrcLoadFile.SetSubstate(TS_CHECK_ICON_SYS_RESULT);
                this->mTaskFind.Start(this->mTaskTrcLoadFile.mCardID,
                                      this->mTaskTrcLoadFile.mFileInfo.directoryInfo->directoryName,
                                      gPs2IconSysFileName, false);
                break;

            default:
                this->mTaskTrcLoadFile.mTaskResult = RESULT_FAILED;
                this->mTaskTrcLoadFile.SetState(TS_DONE, TS_START);
                break;
            }
            break;

        case TS_CHECK_ICON_SYS_RESULT:
            if (this->mTaskFind.mNumFilesFound > 0)
            {
                this->mTaskTrcLoadFile.mIconSysSize = this->mTaskFind.mFileSize;
                this->mTaskTrcLoadFile.mFileDescriptor =
                    this->SceOpenFile(this->mTaskTrcLoadFile.mCardID, gPs2IconSysFileName, SCE_RDONLY);
                this->mTaskTrcLoadFile.SetSubstate(TS_OPEN_FILE_RESULT);
            }
            else
            {
                this->mTaskTrcLoadFile.mTaskResult = RESULT_FAILED;
                this->mTaskTrcLoadFile.mCardStatus = STATUS_FILE_CORRUPTED;
                this->mTaskTrcLoadFile.SetState(TS_DONE, TS_START);
            }
            break;

        case TS_OPEN_FILE_RESULT:
            this->mTaskTrcLoadFile.mCardStatus = this->mCardStatus;
            if (this->mCardStatus == STATUS_OK)
            {
                this->mTaskRead.Start((OpenFileDescriptor *)this->mTaskTrcLoadFile.mFileDescriptor,
                                      &this->mTaskTrcLoadFile.mIconSys, sizeof(this->mTaskTrcLoadFile.mIconSys),
                                      false);
                this->mTaskTrcLoadFile.SetSubstate(TS_CLOSE_FILE);
            }
            else
            {
                this->mTaskTrcLoadFile.mTaskResult = RESULT_FAILED;
                this->mTaskTrcLoadFile.mCardStatus = STATUS_FILE_CORRUPTED;
                this->mTaskTrcLoadFile.SetState(TS_DONE, TS_START);
            }
            break;

        case TS_CLOSE_FILE:
            this->SceCloseFile(this->mTaskTrcLoadFile.mFileDescriptor);
            this->mTaskTrcLoadFile.SetSubstate(TS_CHECK_STATIC_ICON);
            break;

        case TS_CHECK_STATIC_ICON:
            this->mTaskTrcLoadFile.SetSubstate(TS_CHECK_STATIC_ICON_RESULT);
            this->mTaskFind.Start(this->mTaskTrcLoadFile.mCardID,
                                  this->mTaskTrcLoadFile.mFileInfo.directoryInfo->directoryName,
                                  (char *)this->mTaskTrcLoadFile.mIconSys.FnameView, false);
            break;

        case TS_CHECK_STATIC_ICON_RESULT:
            if (this->mTaskFind.mNumFilesFound > 0)
            {
                this->mTaskTrcLoadFile.SetSubstate(TS_CHECK_COPY_ICON);
            }
            else
            {
                this->mTaskTrcLoadFile.mTaskResult = RESULT_FAILED;
                this->mTaskTrcLoadFile.mCardStatus = STATUS_FILE_CORRUPTED;
                this->mTaskTrcLoadFile.SetState(TS_DONE, TS_START);
            }
            break;

        case TS_CHECK_COPY_ICON:
            if (this->mTaskTrcLoadFile.mIconSys.FnameCopy[0] != 0)
            {
                this->mTaskTrcLoadFile.SetSubstate(TS_CHECK_COPY_ICON_RESULT);
                this->mTaskFind.Start(this->mTaskTrcLoadFile.mCardID,
                                      this->mTaskTrcLoadFile.mFileInfo.directoryInfo->directoryName,
                                      (char *)this->mTaskTrcLoadFile.mIconSys.FnameCopy, false);
            }
            else
            {
                this->mTaskTrcLoadFile.SetSubstate(TS_CHECK_DELETE_ICON);
            }
            break;

        case TS_CHECK_COPY_ICON_RESULT:
            if (this->mTaskFind.mNumFilesFound > 0)
            {
                this->mTaskTrcLoadFile.SetSubstate(TS_CHECK_DELETE_ICON);
            }
            else
            {
                this->mTaskTrcLoadFile.mTaskResult = RESULT_FAILED;
                this->mTaskTrcLoadFile.mCardStatus = STATUS_FILE_CORRUPTED;
                this->mTaskTrcLoadFile.SetState(TS_DONE, TS_START);
            }
            break;

        case TS_CHECK_DELETE_ICON:
            if (this->mTaskTrcLoadFile.mIconSys.FnameDel[0] != 0)
            {
                this->mTaskTrcLoadFile.SetSubstate(TS_CHECK_DELETE_ICON_RESULT);
                this->mTaskFind.Start(this->mTaskTrcLoadFile.mCardID,
                                      this->mTaskTrcLoadFile.mFileInfo.directoryInfo->directoryName,
                                      (char *)this->mTaskTrcLoadFile.mIconSys.FnameDel, false);
            }
            else
            {
                this->mTaskTrcLoadFile.SetSubstate(TS_FIND_FILE);
            }
            break;

        case TS_CHECK_DELETE_ICON_RESULT:
            if (this->mTaskFind.mNumFilesFound > 0)
            {
                this->mTaskTrcLoadFile.SetSubstate(TS_FIND_FILE);
            }
            else
            {
                this->mTaskTrcLoadFile.mTaskResult = RESULT_FAILED;
                this->mTaskTrcLoadFile.mCardStatus = STATUS_FILE_CORRUPTED;
                this->mTaskTrcLoadFile.SetState(TS_DONE, TS_START);
            }
            break;

        case TS_FIND_FILE:
            this->mTaskTrcLoadFile.SetSubstate(TS_FIND_FILE_RESULT);
            this->mTaskFind.Start(this->mTaskTrcLoadFile.mCardID,
                                  this->mTaskTrcLoadFile.mFileInfo.directoryInfo->directoryName,
                                  this->mTaskTrcLoadFile.mFileInfo.fileName, false);
            break;

        case TS_FIND_FILE_RESULT:
            if (this->mTaskFind.mNumFilesFound > 0)
            {
                this->mTaskTrcLoadFile.mFileInfo.fileByteSize = this->mTaskFind.mFileSize;
                this->mTaskTrcLoadFile.SetState(TS_OPEN_FILE, TS_START);
            }
            else
            {
                this->mTaskTrcLoadFile.mTaskResult = RESULT_FAILED;
                this->mTaskTrcLoadFile.mCardStatus = STATUS_FILE_CORRUPTED;
                this->mTaskTrcLoadFile.SetState(TS_DONE, TS_START);
            }
            break;
        }
        break;

    case TS_OPEN_FILE:
        switch (this->mTaskTrcLoadFile.GetSubstate())
        {
        case TS_START:
            this->mCardStatus = STATUS_UNKNOWN;
            this->mTaskTrcLoadFile.mFileDescriptor =
                this->SceOpenFile(this->mTaskTrcLoadFile.mCardID, this->mTaskTrcLoadFile.mFileInfo.fileName, SCE_RDONLY);
            this->mUserMsg = UMSG_NONE;
            this->mTaskTrcLoadFile.SetSubstate(TS_OPEN_FILE_RESULT);
            break;

        case TS_OPEN_FILE_RESULT:
            this->mTaskTrcLoadFile.mCardStatus = this->mCardStatus;
            if (this->mCardStatus == STATUS_OK)
            {
                this->mMsg.Set(LMSG_OPEN_FILE_DONE, RESULT_SUCCESS, STATUS_OK);
                this->mMsg.info.openResult.fileHandle = (OpenFileDescriptor *)this->mTaskTrcLoadFile.mFileDescriptor;
                this->mMsg.info.openResult.fileName = this->mTaskTrcLoadFile.mFileInfo.fileName;
                this->mTaskTrcLoadFile.SetState(TS_READ_WAIT, TS_START);
            }
            else
            {
                this->mTaskTrcLoadFile.mTaskResult = RESULT_FAILED;
                this->mTaskTrcLoadFile.SetState(TS_DONE, TS_START);
            }
            break;
        }
        break;

    case TS_READ_WAIT:
        switch (this->mTaskTrcLoadFile.GetSubstate())
        {
        case TS_START:
            this->mMsg.Set(LMSG_READ_READY);
            this->mTaskTrcLoadFile.SetSubstate(TS_READ_WAIT);
            break;

        case TS_READ_WAIT:
            if (this->mTaskRead.mTaskResult != RESULT_SUCCESS)
            {
                this->mTaskTrcLoadFile.mTaskResult = RESULT_FAILED;
                if (this->mTaskRead.mCardStatus == STATUS_ACCESS_DENIED)
                {
                    this->mTaskTrcLoadFile.mCardStatus = STATUS_NO_CARD;
                }
                else
                {
                    this->mTaskTrcLoadFile.mCardStatus = this->mTaskRead.mCardStatus;
                }
            }
            else if (this->mUserMsg != UMSG_READ_COMPLETE)
            {
                break;
            }

            this->mTaskTrcLoadFile.SetState(TS_CLOSE_FILE, TS_START);
            break;
        }
        break;

    case TS_CLOSE_FILE:
        switch (this->mTaskTrcLoadFile.GetSubstate())
        {
        case TS_START:
            this->mCardStatus = STATUS_UNKNOWN;
            this->SceCloseFile(this->mTaskTrcLoadFile.mFileDescriptor);
            this->mTaskTrcLoadFile.SetSubstate(TS_WAIT_FOR_RESULT);
            break;

        case TS_WAIT_FOR_RESULT:
            if (this->mCardStatus == STATUS_OK)
            {
                this->mTaskTrcLoadFile.SetState(TS_USER_CHECK_DATA, TS_START);
            }
            else
            {
                if (this->mTaskTrcLoadFile.mCardStatus == STATUS_OK)
                {
                    this->mTaskTrcLoadFile.mCardStatus = this->mCardStatus;
                }
                this->mTaskTrcLoadFile.mTaskResult = RESULT_FAILED;
                this->mTaskTrcLoadFile.SetState(TS_DONE, TS_START);
            }
            break;
        }
        break;

    case TS_USER_CHECK_DATA:
        this->mTaskTrcLoadFile.mIsLoading = false;
        switch (this->mTaskTrcLoadFile.GetSubstate())
        {
        case TS_START:
            this->mMsg.Set(LMSG_CHECK_DATA_INTEGRITY);
            this->mUserMsg = UMSG_NONE;
            this->mTaskTrcLoadFile.SetSubstate(TS_USER_CHECK_DATA_RESULT);
            break;

        case TS_USER_CHECK_DATA_RESULT:
            switch (this->mUserMsg)
            {
            case UMSG_DATA_OK:
                this->mTaskTrcLoadFile.mTaskResult = RESULT_SUCCESS;
                this->mTaskTrcLoadFile.mCardStatus = STATUS_OK;
                this->mTaskTrcLoadFile.SetState(TS_DONE, TS_START);
                break;

            case UMSG_DATA_CORRUPT:
                this->mTaskTrcLoadFile.mTaskResult = RESULT_FAILED;
                this->mTaskTrcLoadFile.mCardStatus = STATUS_FILE_CORRUPTED;
                this->mTaskTrcLoadFile.SetState(TS_DONE, TS_START);
                break;
            }
            break;
        }
        break;

    case TS_DONE:
        if (!this->mMsgTimer.IsExpired())
        {
            break;
        }

        switch (this->mTaskTrcLoadFile.GetSubstate())
        {
        case TS_START:
            this->mTaskTrcLoadFile.SetSubstate(TS_END);
            if (this->mTaskTrcLoadFile.mTaskResult == RESULT_SUCCESS)
            {
                if (this->mIsBootFlow)
                {
                    this->mMsg.TRC_msg(18, 0);
                    this->mMsgTimer.Set(0);
                    this->mTaskTrcLoadFile.SetSubstate(TS_END);
                }
                else
                {
                    this->mMsg.TRC_msg(18, PS2Message::PackMsgOptions(5, 0, 0, 0));
                    this->mUserMsg = UMSG_NONE;
                    this->mTaskTrcLoadFile.SetSubstate(TS_WAIT_FOR_USER_REPLY);
                }
            }
            else if (this->mTaskTrcLoadFile.mTaskResult != RESULT_CANCELLED)
            {
                if (this->mTaskTrcLoadFile.mIsLoading &&
                    this->mTaskTrcLoadFile.mCardStatus == STATUS_FILE_CORRUPTED)
                {
                    CardStatus status = this->CheckCard(this->mTaskTrcLoadFile.mCardID, nullptr, nullptr, nullptr);

                    if (status == STATUS_NO_CARD || status == STATUS_CARD_CHANGED)
                    {
                        this->mTaskTrcLoadFile.mCardStatus = STATUS_CARD_REMOVED;
                    }
                }

                if (this->mTaskTrcLoadFile.mCardStatus == STATUS_FILE_CORRUPTED)
                {
                    this->mMsg.TRC_msg(16, PS2Message::PackMsgOptions(5, 0, 0, 0),
                                       this->PortStr(this->mTaskTrcLoadFile.mCardID));
                    this->mUserMsg = UMSG_NONE;
                    this->mTaskTrcLoadFile.SetSubstate(TS_WAIT_FOR_USER_REPLY);
                }
                else if (this->mTaskTrcLoadFile.mIsLoading)
                {
                    this->mMsg.TRC_msg(17, PS2Message::PackMsgOptions(5, 0, 0, 0),
                                       this->PortStr(this->mTaskTrcLoadFile.mCardID));
                    this->mUserMsg = UMSG_NONE;
                    this->mTaskTrcLoadFile.SetSubstate(TS_WAIT_FOR_USER_REPLY);
                }
                else
                {
                    switch (this->mTaskTrcLoadFile.mCardStatus)
                    {
                    case STATUS_OK:
                        break;

                    case STATUS_NO_CARD:
                        this->mMsg.TRC_msg(7, PS2Message::PackMsgOptions(5, 0, 0, 0),
                                           this->PortStr(this->mTaskTrcLoadFile.mCardID));
                        this->mUserMsg = UMSG_NONE;
                        this->mTaskTrcLoadFile.SetSubstate(TS_WAIT_FOR_USER_REPLY);
                        break;

                    case STATUS_CARD_UNFORMATTED:
                        this->mMsg.TRC_msg(8, PS2Message::PackMsgOptions(5, 0, 0, 0),
                                           this->PortStr(this->mTaskTrcLoadFile.mCardID));
                        this->mUserMsg = UMSG_NONE;
                        this->mTaskTrcLoadFile.SetSubstate(TS_WAIT_FOR_USER_REPLY);
                        break;

                    case STATUS_FILE_NOT_FOUND:
                        this->mMsg.TRC_msg(14, PS2Message::PackMsgOptions(5, 0, 0, 0), (char *)this->mGameTitle,
                                           this->PortStr(this->mTaskTrcLoadFile.mCardID));
                        this->mUserMsg = UMSG_NONE;
                        this->mMsg.mTaskResult = this->mTaskTrcLoadFile.mTaskResult;
                        this->mMsg.mCardStatus = this->mTaskTrcLoadFile.mCardStatus;
                        this->mTaskTrcLoadFile.SetSubstate(TS_WAIT_FOR_USER_REPLY);
                        break;

                    case STATUS_FAILED:
                    default:
                        this->mMsg.TRC_msg(17, PS2Message::PackMsgOptions(5, 0, 0, 0),
                                           this->PortStr(this->mTaskTrcLoadFile.mCardID));
                        this->mUserMsg = UMSG_NONE;
                        this->mTaskTrcLoadFile.SetSubstate(TS_WAIT_FOR_USER_REPLY);
                        break;
                    }
                }
            }
            break;

        case TS_WAIT_FOR_USER_REPLY:
            if (this->mUserMsg == UMSG_OPTION1)
            {
                this->mTaskTrcLoadFile.SetSubstate(TS_END);
            }
            break;

        case TS_END:
        {
            if (!this->mMsgTimer.IsExpired())
            {
                break;
            }

            CardStatus status = this->CheckCard(this->mTaskTrcLoadFile.mCardID, nullptr, nullptr, nullptr);

            if ((status == STATUS_NO_CARD || status == STATUS_CARD_UNFORMATTED) &&
                this->mTaskTrcLoadFile.mCardStatus != STATUS_NO_CARD)
            {
                this->mTaskTrcLoadFile.mCardStatus = STATUS_CARD_REMOVED;
            }
            else if (status == STATUS_OK && this->mTaskTrcLoadFile.mCardStatus == STATUS_CARD_REMOVED)
            {
                this->mTaskTrcLoadFile.mCardStatus = STATUS_CARD_CHANGED;
            }
            else if (status != STATUS_OK)
            {
                this->mTaskTrcLoadFile.mCardStatus = status;
            }

            this->mMsg.Set(LMSG_TRC_LOADFILE_DONE, this->mTaskTrcLoadFile.mTaskResult,
                           this->mTaskTrcLoadFile.mCardStatus);
            this->mTaskTrcLoadFile.End();
            break;
        }
        }
        break;
    }
}

void PS2Interface::UpdateTaskTrcDeleteFile()
{
    if (!this->mMsgTimer.IsExpired())
    {
        return;
    }

    switch (this->mTaskTrcDeleteFile.GetState())
    {
    case TS_START:
        this->mTaskTrcDeleteFile.mTaskResult = RESULT_UNKNOWN;
        this->mTaskTrcDeleteFile.mCardStatus = STATUS_UNKNOWN;
        this->mTaskTrcDeleteFile.mDeleteDirectory = false;

        switch (this->CheckCard(this->mTaskTrcDeleteFile.mCardID, nullptr, nullptr, nullptr))
        {
        case STATUS_OK:
        case STATUS_CARD_CHANGED:
        case STATUS_CARD_UNFORMATTED:
        case STATUS_CARD_FULL:
        case STATUS_ACCESS_DENIED:
            this->mTaskTrcDeleteFile.mCardFoundOnEntry = true;
            break;

        default:
            this->mTaskTrcDeleteFile.mCardFoundOnEntry = false;
            break;
        }

        this->mTaskTrcDeleteFile.SetState(TS_DELETE_FILE_ASK_USER, TS_START);

    case TS_DELETE_FILE_ASK_USER:
        switch (this->mTaskTrcDeleteFile.GetSubstate())
        {
        case TS_START:
            this->mMsg.TRC_msg(31, PS2Message::PackMsgOptions(4, 3, 0, 0));
            this->mUserMsg = UMSG_NONE;
            this->mTaskTrcDeleteFile.SetSubstate(TS_DELETE_WAIT_FOR_USER_REPLY);
            break;

        case TS_DELETE_WAIT_FOR_USER_REPLY:
            switch (this->CheckCard(this->mTaskTrcDeleteFile.mCardID, nullptr, nullptr, nullptr))
            {
            case STATUS_NO_CARD:
                if (this->mTaskTrcDeleteFile.mCardFoundOnEntry)
                {
                    this->mTaskTrcDeleteFile.mCardStatus = STATUS_CARD_REMOVED;
                }
                else
                {
                    this->mTaskTrcDeleteFile.mCardStatus = STATUS_NO_CARD;
                }
                break;

            case STATUS_CARD_CHANGED:
                this->mTaskTrcDeleteFile.mCardStatus = STATUS_CARD_REMOVED;
                break;
            }

            switch (this->mUserMsg)
            {
            case UMSG_OPTION1:
                this->mTaskTrcDeleteFile.mTaskResult = RESULT_CANCELLED;
                if (this->mTaskTrcDeleteFile.mCardStatus == STATUS_UNKNOWN)
                {
                    this->mTaskTrcDeleteFile.mCardStatus = STATUS_OK;
                }
                this->mTaskTrcDeleteFile.SetState(TS_DONE, TS_START);
                break;

            case UMSG_OPTION2:
                this->mTaskTrcDeleteFile.SetState(TS_CHECK_CARD, TS_START);
                break;
            }
            break;
        }
        break;

    case TS_CHECK_CARD:
        switch (this->mTaskTrcDeleteFile.GetSubstate())
        {
        case TS_START:
            this->mTaskTrcDeleteFile.mIsDeleting = false;
            this->mTaskTrcDeleteFile.SetSubstate(TS_CHECK_CARD);
            if (!this->mNoCheckingCardWarning)
            {
                this->mMsg.TRC_msg(6, 0, this->PortStr(this->mTaskTrcDeleteFile.mCardID));
                this->mMsgTimer.Set(1);
                break;
            }

        case TS_CHECK_CARD:
            if (this->mTaskTrcDeleteFile.mCardStatus == STATUS_NO_CARD ||
                this->mTaskTrcDeleteFile.mCardStatus == STATUS_CARD_REMOVED ||
                this->mTaskTrcDeleteFile.mCardStatus == STATUS_CARD_CHANGED)
            {
                this->mTaskTrcDeleteFile.mTaskResult = RESULT_FAILED;
                this->mTaskTrcDeleteFile.SetState(TS_DONE, TS_START);
                break;
            }

            this->mTaskTrcDeleteFile.SetSubstate(TS_CHECK_CARD_RESULT);
            this->mTaskGetCardInfo.Start(this->mTaskTrcDeleteFile.mCardID, TID_CARD_EXISTS, false);
            break;

        case TS_CHECK_CARD_RESULT:
            this->mTaskTrcDeleteFile.mCardStatus = this->mTaskGetCardInfo.mCardStatus;
            switch (this->mTaskTrcDeleteFile.mCardStatus)
            {
            case STATUS_OK:
                this->mTaskTrcDeleteFile.SetState(TS_CHANGE_DIRECTORY, TS_START);
                break;

            case STATUS_NO_CARD:
            case STATUS_CARD_CHANGED:
                if (this->mTaskTrcDeleteFile.mCardFoundOnEntry)
                {
                    this->mTaskTrcDeleteFile.mCardStatus = STATUS_CARD_REMOVED;
                }
                this->mTaskTrcDeleteFile.mTaskResult = RESULT_FAILED;
                this->mTaskTrcDeleteFile.SetState(TS_DONE, TS_START);
                break;

            case STATUS_CARD_UNFORMATTED:
                this->mTaskTrcDeleteFile.mTaskResult = RESULT_FAILED;
                this->mTaskTrcDeleteFile.SetState(TS_DONE, TS_START);
                break;

            default:
                this->mTaskTrcDeleteFile.mTaskResult = RESULT_FAILED;
                this->mTaskTrcDeleteFile.mCardStatus = STATUS_NO_CARD;
                this->mTaskTrcDeleteFile.SetState(TS_DONE, TS_START);
                break;
            }
            break;
        }
        break;

    case TS_CHANGE_DIRECTORY:
        switch (this->mTaskTrcDeleteFile.GetSubstate())
        {
        case TS_START:
            this->mTaskTrcDeleteFile.mIsDeleting = true;
            this->mMsg.TRC_msg(32, 0, this->PortStr(this->mTaskTrcDeleteFile.mCardID));
            this->mMsgTimer.Set(0);
            this->mTaskTrcDeleteFile.SetSubstate(TS_CHANGE_DIRECTORY_RESULT);
            this->mTaskChangeDirectory.Start(this->mTaskTrcDeleteFile.mCardID,
                                             this->mTaskTrcDeleteFile.mFileInfo.directoryInfo->directoryName, false);
            break;

        case TS_CHANGE_DIRECTORY_RESULT:
            if (this->mTaskChangeDirectory.mCardStatus == STATUS_OK)
            {
                this->mTaskTrcDeleteFile.SetState(TS_DELETE_FILE, TS_START);
            }
            else
            {
                this->mTaskTrcDeleteFile.mTaskResult = RESULT_FAILED;
                this->mTaskTrcDeleteFile.mCardStatus = STATUS_FILE_NOT_FOUND;
                this->mTaskTrcDeleteFile.SetState(TS_DONE, TS_START);
            }
            break;
        }
        break;

    case TS_DELETE_FILE:
        switch (this->mTaskTrcDeleteFile.GetSubstate())
        {
        case TS_START:
            this->mTaskTrcDeleteFile.mCardStatus = STATUS_FILE_NOT_FOUND;
            if (strcmp(this->mTaskTrcDeleteFile.mFileInfo.fileName, FILENAME_ALL_FILES) == 0)
            {
                this->mTaskTrcDeleteFile.mDeleteDirectory = true;
            }

            this->mCardStatus = STATUS_UNKNOWN;
            this->mNumFilesFound = 0;
            this->CheckSceResult(sceMcGetDir(this->mTaskTrcDeleteFile.mCardID.port,
                                             this->mTaskTrcDeleteFile.mCardID.slot,
                                             this->mTaskTrcDeleteFile.mFileInfo.fileName, 0, 1, mPs2FileTableEntry));
            this->mTaskTrcDeleteFile.SetSubstate(TS_FIND_FILE_RESULT);
            break;

        case TS_FIND_NEXT:
            this->mCardStatus = STATUS_UNKNOWN;
            this->mNumFilesFound = 0;
            this->CheckSceResult(sceMcGetDir(this->mTaskTrcDeleteFile.mCardID.port,
                                             this->mTaskTrcDeleteFile.mCardID.slot,
                                             this->mTaskTrcDeleteFile.mFileInfo.fileName, 1, 1, mPs2FileTableEntry));
            this->mTaskTrcDeleteFile.SetSubstate(TS_FIND_FILE_RESULT);
            break;

        case TS_FIND_FILE_RESULT:
            if (this->mNumFilesFound == 0)
            {
                if (this->mTaskTrcDeleteFile.mDeleteDirectory)
                {
                    char rootpath[64];

                    sprintf(rootpath, "/%s", this->mTaskTrcDeleteFile.mFileInfo.directoryInfo->directoryName);
                    this->mCardStatus = STATUS_UNKNOWN;
                    this->CheckSceResult(sceMcDelete(this->mTaskTrcDeleteFile.mCardID.port,
                                                     this->mTaskTrcDeleteFile.mCardID.slot, rootpath));
                    this->mTaskTrcDeleteFile.SetSubstate(TS_DELETE_DIRECTORY_RESULT);
                }
                else
                {
                    this->mTaskTrcDeleteFile.mTaskResult = RESULT_FAILED;
                    this->mTaskTrcDeleteFile.SetState(TS_DONE, TS_START);
                }
            }
            else
            {
                this->mTaskTrcDeleteFile.mCardStatus = STATUS_OK;
                if (mPs2FileTableEntry[0].AttrFile & sceMcFileAttrSubdir)
                {
                    this->mTaskTrcDeleteFile.SetSubstate(TS_FIND_NEXT);
                }
                else
                {
                    this->mCardStatus = STATUS_UNKNOWN;
                    this->CheckSceResult(sceMcDelete(this->mTaskTrcDeleteFile.mCardID.port,
                                                     this->mTaskTrcDeleteFile.mCardID.slot,
                                                     (char *)mPs2FileTableEntry[0].EntryName));
                    this->mTaskTrcDeleteFile.SetSubstate(TS_DELETE_FILE_RESULT);
                }
            }
            break;

        case TS_DELETE_FILE_RESULT:
            if (this->mCardStatus == STATUS_OK)
            {
                this->mTaskTrcDeleteFile.mTaskResult = RESULT_SUCCESS;
                this->mTaskTrcDeleteFile.mCardStatus = STATUS_OK;
            }
            else
            {
                this->mTaskTrcDeleteFile.mTaskResult = RESULT_FAILED;
                this->mTaskTrcDeleteFile.mCardStatus = this->mCardStatus;
            }
            this->mTaskTrcDeleteFile.SetSubstate(TS_FIND_NEXT);
            break;

        case TS_DELETE_DIRECTORY_RESULT:
            if (this->mCardStatus == STATUS_OK)
            {
                this->mTaskTrcDeleteFile.mTaskResult = RESULT_SUCCESS;
                this->mTaskTrcDeleteFile.mCardStatus = STATUS_OK;
            }
            else
            {
                this->mTaskTrcDeleteFile.mTaskResult = RESULT_FAILED;
                this->mTaskTrcDeleteFile.mCardStatus = this->mCardStatus;
            }
            this->mTaskTrcDeleteFile.SetState(TS_DONE, TS_START);
            break;
        }
        break;

    case TS_DONE:
        switch (this->mTaskTrcDeleteFile.GetSubstate())
        {
        case TS_START:
            this->mTaskTrcDeleteFile.SetSubstate(TS_END);
            if (this->mTaskTrcDeleteFile.mTaskResult == RESULT_SUCCESS)
            {
                this->mMsg.TRC_msg(34, PS2Message::PackMsgOptions(5, 0, 0, 0));
                this->mUserMsg = UMSG_NONE;
                this->mTaskTrcDeleteFile.SetSubstate(TS_WAIT_FOR_USER_REPLY);
            }
            else if (this->mTaskTrcDeleteFile.mTaskResult != RESULT_CANCELLED)
            {
                if (this->mTaskTrcDeleteFile.mIsDeleting)
                {
                    this->mMsg.TRC_msg(33, PS2Message::PackMsgOptions(5, 0, 0, 0),
                                       this->PortStr(this->mTaskTrcDeleteFile.mCardID));
                    this->mMsg.mTaskResult = this->mTaskTrcDeleteFile.mTaskResult;
                    this->mMsg.mCardStatus = this->mTaskTrcDeleteFile.mCardStatus;
                    this->mUserMsg = UMSG_NONE;
                    this->mTaskTrcDeleteFile.SetSubstate(TS_WAIT_FOR_USER_REPLY);
                }
                else
                {
                    switch (this->mTaskTrcDeleteFile.mCardStatus)
                    {
                    case STATUS_OK:
                        break;

                    case STATUS_NO_CARD:
                        this->mMsg.TRC_msg(7, PS2Message::PackMsgOptions(5, 0, 0, 0),
                                           this->PortStr(this->mTaskTrcDeleteFile.mCardID));
                        this->mUserMsg = UMSG_NONE;
                        this->mTaskTrcDeleteFile.SetSubstate(TS_WAIT_FOR_USER_REPLY);
                        break;

                    case STATUS_CARD_CHANGED:
                    case STATUS_CARD_REMOVED:
                        this->mMsg.TRC_msg(39, PS2Message::PackMsgOptions(5, 0, 0, 0),
                                           this->PortStr(this->mTaskTrcDeleteFile.mCardID));
                        this->mUserMsg = UMSG_NONE;
                        this->mTaskTrcDeleteFile.SetSubstate(TS_WAIT_FOR_USER_REPLY);
                        break;

                    case STATUS_CARD_UNFORMATTED:
                        this->mMsg.TRC_msg(8, PS2Message::PackMsgOptions(5, 0, 0, 0),
                                           this->PortStr(this->mTaskTrcDeleteFile.mCardID));
                        this->mUserMsg = UMSG_NONE;
                        this->mTaskTrcDeleteFile.SetSubstate(TS_WAIT_FOR_USER_REPLY);
                        break;

                    default:
                        this->mMsg.TRC_msg(33, PS2Message::PackMsgOptions(5, 0, 0, 0),
                                           this->PortStr(this->mTaskTrcDeleteFile.mCardID));
                        this->mMsg.mTaskResult = this->mTaskTrcDeleteFile.mTaskResult;
                        this->mMsg.mCardStatus = this->mTaskTrcDeleteFile.mCardStatus;
                        this->mUserMsg = UMSG_NONE;
                        this->mTaskTrcDeleteFile.SetSubstate(TS_WAIT_FOR_USER_REPLY);
                        break;
                    }
                }
            }
            break;

        case TS_WAIT_FOR_USER_REPLY:
            if (this->mUserMsg == UMSG_OPTION1)
            {
                this->mTaskTrcDeleteFile.SetSubstate(TS_END);
            }
            break;

        case TS_END:
        {
            CardStatus status = this->CheckCard(this->mTaskTrcDeleteFile.mCardID, nullptr, nullptr, nullptr);

            if ((status == STATUS_NO_CARD || status == STATUS_CARD_UNFORMATTED) &&
                this->mTaskTrcDeleteFile.mCardStatus != STATUS_NO_CARD)
            {
                this->mTaskTrcDeleteFile.mCardStatus = STATUS_CARD_REMOVED;
            }
            else if (status == STATUS_OK && this->mTaskTrcDeleteFile.mCardStatus == STATUS_CARD_REMOVED)
            {
                this->mTaskTrcDeleteFile.mCardStatus = STATUS_CARD_CHANGED;
            }
            else if (status == STATUS_OK && this->mTaskTrcSaveFile.mCardStatus == STATUS_CARD_UNFORMATTED)
            {
                this->mTaskTrcSaveFile.mCardStatus = STATUS_CARD_UNFORMATTED;
            }
            else if (status != STATUS_OK)
            {
                this->mTaskTrcDeleteFile.mCardStatus = status;
            }

            this->mMsg.Set(LMSG_TRC_DELETEFILE_DONE, this->mTaskTrcDeleteFile.mTaskResult,
                           this->mTaskTrcDeleteFile.mCardStatus);
            this->mTaskTrcDeleteFile.End();
            break;
        }
        }
        break;
    }
}

void PS2Interface::UpdateTaskTrcList()
{
    if (!this->mMsgTimer.IsExpired())
    {
        return;
    }

    switch (this->mTaskTrcList.GetState())
    {
    case TS_START:
        this->mTaskTrcList.mTaskResult = RESULT_UNKNOWN;
        this->mTaskTrcList.mCardStatus = STATUS_UNKNOWN;
        this->mTaskTrcList.SetState(TS_CHECK_CARD, TS_START);

    case TS_CHECK_CARD:
        switch (this->mTaskTrcList.GetSubstate())
        {
        case TS_START:
            if (!this->mNoCheckingCardWarning)
            {
                this->mMsg.TRC_msg(6, 0, this->PortStr(this->mTaskTrcList.mCardID));
                this->mMsgTimer.Set(1);
            }
            this->mTaskTrcList.SetSubstate(TS_CHECK_CARD_RESULT);
            this->mTaskGetCardInfo.Start(this->mTaskTrcList.mCardID, TID_CARD_EXISTS, false);
            break;

        case TS_CHECK_CARD_RESULT:
            this->mTaskTrcList.mCardStatus = this->mTaskGetCardInfo.mCardStatus;
            switch (this->mTaskTrcList.mCardStatus)
            {
            case STATUS_OK:
            case STATUS_CARD_CHANGED:
                this->mTaskTrcList.SetState(TS_LIST, TS_START);
                break;

            default:
                this->mTaskTrcList.mTaskResult = RESULT_FAILED;
                this->mTaskTrcList.SetState(TS_DONE, TS_START);
                break;
            }
            break;
        }
        break;

    case TS_LIST:
        if (this->mTaskTrcList.mListType == LISTTASK_ENTRIES)
        {
            this->mTaskTrcListEntries.Start();
        }
        else
        {
            this->mTaskTrcListFiles.Start();
        }
        this->mTaskTrcList.SetState(TS_DONE, TS_START);
        break;

    case TS_DONE:
        switch (this->mTaskTrcList.GetSubstate())
        {
        case TS_START:
            switch (this->mTaskTrcList.mCardStatus)
            {
            case STATUS_CARD_UNFORMATTED:
                this->mMsg.TRC_msg(8, PS2Message::PackMsgOptions(5, 0, 0, 0),
                                   this->PortStr(this->mTaskTrcList.mCardID));
                this->mUserMsg = UMSG_NONE;
                this->mTaskTrcList.SetSubstate(TS_WAIT_FOR_USER_REPLY);
                break;

            case STATUS_NO_CARD:
            case STATUS_ACCESS_DENIED:
                this->mMsg.TRC_msg(7, PS2Message::PackMsgOptions(5, 0, 0, 0),
                                   this->PortStr(this->mTaskTrcList.mCardID));
                this->mUserMsg = UMSG_NONE;
                this->mTaskTrcList.SetSubstate(TS_WAIT_FOR_USER_REPLY);
                break;

            default:
                this->mTaskTrcList.SetState(TS_END, TS_START);
                break;
            }
            break;

        case TS_WAIT_FOR_USER_REPLY:
            if (this->mUserMsg == UMSG_OPTION1)
            {
                this->mTaskTrcList.SetState(TS_END, TS_START);
            }
            break;
        }
        break;

    case TS_END:
    {
        CardStatus status = this->CheckCard(this->mTaskTrcList.mCardID, nullptr, nullptr, nullptr);

        if ((status == STATUS_NO_CARD || status == STATUS_CARD_UNFORMATTED) &&
            this->mTaskTrcList.mCardStatus != STATUS_NO_CARD)
        {
            this->mTaskTrcList.mCardStatus = STATUS_CARD_REMOVED;
        }
        else if (status == STATUS_OK && this->mTaskTrcList.mCardStatus == STATUS_CARD_REMOVED)
        {
            this->mTaskTrcList.mCardStatus = STATUS_CARD_CHANGED;
        }
        else if (status == STATUS_OK && this->mTaskTrcList.mCardStatus == STATUS_ENTRY_NOT_FOUND)
        {
            this->mTaskTrcList.mCardStatus = STATUS_ENTRY_NOT_FOUND;
        }
        else if (status == STATUS_OK && this->mTaskTrcList.mCardStatus == STATUS_CARD_UNFORMATTED)
        {
            this->mTaskTrcList.mCardStatus = STATUS_CARD_UNFORMATTED;
        }
        else
        {
            this->mTaskTrcList.mCardStatus = status;
        }

        this->mMsg.Set(LMSG_TRC_LIST_DONE, this->mTaskTrcList.mTaskResult, this->mTaskTrcList.mCardStatus);
        this->mTaskTrcList.End();
        break;
    }
    }
}

void PS2Interface::UpdateTaskTrcListEntries()
{
    switch (this->mTaskTrcListEntries.GetState())
    {
    case TS_START:
        this->mTaskTrcListEntries.SetState(TS_FIND_DIRECTORY, TS_START);
        this->CheckSceResult(sceMcChdir(this->mTaskTrcList.mCardID.port, this->mTaskTrcList.mCardID.slot,
                                        ROOT_DIRECTORY_NAME, nullptr));
        break;

    case TS_FIND_DIRECTORY:
        switch (this->mTaskTrcListEntries.GetSubstate())
        {
        case TS_START:
            this->CheckSceResult(sceMcGetDir(this->mTaskTrcList.mCardID.port, this->mTaskTrcList.mCardID.slot,
                                             this->mTaskTrcList.mFileInfo.directoryInfo->directoryName, 0, -1,
                                             nullptr));
            this->mTaskTrcListEntries.SetSubstate(TS_CREATE_DIRECTORY_TABLE);
            break;

        case TS_CREATE_DIRECTORY_TABLE:
            switch (this->mCardStatus)
            {
            case STATUS_OK:
            case STATUS_CARD_CHANGED:
                this->mTaskTrcList.mTaskResult = RESULT_SUCCESS;
                if (this->mNumFilesFound > 0)
                {
                    this->mTaskTrcList.mFileFound = true;
                    this->mTaskTrcList.mCardStatus = STATUS_OK;
                    this->mTaskTrcList.mCurEntry = 0;
                    this->mTaskTrcList.mNumEntries = this->mNumFilesFound;
                    this->mTaskTrcList.mEntries = static_cast<sceMcTblGetDir *>(this->mISystem.mAllocator->Alloc(
                        this->mTaskTrcList.mNumEntries * sizeof(sceMcTblGetDir),
                        EA::TagValuePair(EA::Allocator::ATT_NAME, "ListEntriesTable") +
                            EA::TagValuePair(EA::Allocator::ATT_ALIGNMENT, 64)));
                    memset(this->mTaskTrcList.mEntries, 0, this->mTaskTrcList.mNumEntries * sizeof(sceMcTblGetDir));
                    this->CheckSceResult(sceMcGetDir(this->mTaskTrcList.mCardID.port, this->mTaskTrcList.mCardID.slot,
                                                     this->mTaskTrcList.mFileInfo.directoryInfo->directoryName, 0,
                                                     this->mTaskTrcList.mNumEntries, this->mTaskTrcList.mEntries));
                    this->mTaskTrcListEntries.SetState(TS_FIND_FILE, TS_START);
                }
                else
                {
                    this->mTaskTrcList.mCardStatus = STATUS_ENTRY_NOT_FOUND;
                    this->mTaskTrcListEntries.SetState(TS_END, TS_START);
                }
                break;

            case STATUS_NO_CARD:
            default:
                this->mTaskTrcList.mTaskResult = RESULT_FAILED;
                this->mTaskTrcList.mCardStatus = this->mCardStatus;
                this->mTaskTrcListEntries.SetState(TS_END, TS_START);
                break;
            }
            break;
        }
        break;

    case TS_FIND_FILE:
        switch (this->mTaskTrcListEntries.GetSubstate())
        {
        case TS_START:
            this->mNumFilesFound = 0;
            this->mCardStatus = STATUS_UNKNOWN;

        case TS_CHANGE_DIRECTORY:
            this->CheckSceResult(sceMcChdir(this->mTaskTrcList.mCardID.port, this->mTaskTrcList.mCardID.slot,
                                            (char *)this->mTaskTrcList.mEntries[this->mTaskTrcList.mCurEntry].EntryName,
                                            nullptr));
            this->mTaskTrcListEntries.SetSubstate(TS_FIND_FILE);
            break;

        case TS_FIND_FILE:
            this->CheckSceResult(sceMcGetDir(this->mTaskTrcList.mCardID.port, this->mTaskTrcList.mCardID.slot,
                                             FILENAME_ALL_FILES, 0, 7, mPs2FileTableEntry));
            this->mTaskTrcListEntries.SetSubstate(TS_FIND_FILE_RESULT);
            break;

        case TS_FIND_FILE_RESULT:
            switch (this->mCardStatus)
            {
            case STATUS_OK:
            case STATUS_CARD_CHANGED:
            {
                int blockSize = 0;
                int dataSize = 0;
                FileTime fileTime;
                CardStatus status;

                if (this->mNumFilesFound > 0)
                {
                    blockSize = NUM_CLUSTERS_PER_DIRECTORY_TABLE + (this->mNumFilesFound - 1) / 2;

                    bool foundSysFile = false;
                    bool foundDataFile = false;
                    bool dataNonzeroLength = false;
                    for (int iFile = 0; iFile < this->mNumFilesFound; iFile++)
                    {
                        if (strcmp((char *)(mPs2FileTableEntry + iFile)->EntryName, ".") != 0 &&
                            strcmp((char *)(mPs2FileTableEntry + iFile)->EntryName, "..") != 0)
                        {
                            blockSize += 1 + (mPs2FileTableEntry + iFile)->FileSizeByte / PS2_CLUSTER_SIZE;
                        }

                        if (strcasecmp((char *)(mPs2FileTableEntry + iFile)->EntryName, "icon.sys") == 0)
                        {
                            foundSysFile = true;
                        }
                        else if (strcasecmp((char *)(mPs2FileTableEntry + iFile)->EntryName,
                                            (char *)this->mTaskTrcList.mEntries[this->mTaskTrcList.mCurEntry].EntryName) == 0)
                        {
                            foundDataFile = true;
                            dataSize = (mPs2FileTableEntry + iFile)->FileSizeByte;
                            this->SetFileTime(&fileTime, mPs2FileTableEntry + iFile);
                            if ((mPs2FileTableEntry + iFile)->FileSizeByte != 0)
                            {
                                dataNonzeroLength = true;
                            }
                        }
                    }

                    if (foundSysFile && foundDataFile && dataNonzeroLength)
                    {
                        status = STATUS_OK;
                    }
                    else
                    {
                        status = STATUS_FILE_CORRUPTED;
                    }
                }
                else
                {
                    status = STATUS_FILE_CORRUPTED;
                }

                this->mMsg.Set(LMSG_FILE_INFO);
                this->mMsg.info.fileInfo.fileName =
                    (char *)this->mTaskTrcList.mEntries[this->mTaskTrcList.mCurEntry].EntryName;
                this->mMsg.info.fileInfo.fileSize = blockSize;
                this->mMsg.info.fileInfo.fileTime = fileTime;
                this->mMsg.mCardStatus = status;
                this->mMsg.mTaskResult = (TaskResult)dataSize;
                this->mTaskTrcListEntries.SetSubstate(TS_FIND_NEXT);
                break;
            }

            case STATUS_NO_CARD:
            default:
                this->mTaskTrcList.mTaskResult = RESULT_FAILED;
                this->mTaskTrcList.mCardStatus = this->mCardStatus;
                this->mTaskTrcListEntries.SetState(TS_END, TS_START);
                break;
            }
            break;

        case TS_FIND_NEXT:
            this->mTaskTrcList.mCurEntry++;
            if (this->mTaskTrcList.mCurEntry < this->mTaskTrcList.mNumEntries)
            {
                this->mTaskTrcListEntries.SetSubstate(TS_START);
                this->CheckSceResult(sceMcChdir(this->mTaskTrcList.mCardID.port, this->mTaskTrcList.mCardID.slot,
                                                ROOT_DIRECTORY_NAME, nullptr));
            }
            else
            {
                this->mTaskTrcListEntries.SetState(TS_END, TS_START);
            }
            break;
        }
        break;

    case TS_END:
        if (this->mTaskTrcList.mEntries != nullptr)
        {
            this->mISystem.mAllocator->Free(this->mTaskTrcList.mEntries, 0);
            this->mTaskTrcList.mEntries = nullptr;
            this->mTaskTrcList.mNumEntries = 0;
        }

        this->mTaskTrcListEntries.End();
        break;
    }
}

void PS2Interface::UpdateTaskTrcListFiles()
{
    switch (this->mTaskTrcListFiles.GetState())
    {
    case TS_START:
        this->mTaskTrcListFiles.SetState(TS_FIND_DIRECTORY, TS_START);
        this->CheckSceResult(sceMcChdir(this->mTaskTrcList.mCardID.port, this->mTaskTrcList.mCardID.slot,
                                        ROOT_DIRECTORY_NAME, nullptr));
        break;

    case TS_FIND_DIRECTORY:
        switch (this->mTaskTrcListFiles.GetSubstate())
        {
        case TS_START:
            this->CheckSceResult(sceMcGetDir(this->mTaskTrcList.mCardID.port, this->mTaskTrcList.mCardID.slot,
                                             this->mTaskTrcList.mFileInfo.directoryInfo->directoryName, 0, -1,
                                             nullptr));
            this->mTaskTrcListFiles.SetSubstate(TS_CREATE_DIRECTORY_TABLE);
            break;

        case TS_CREATE_DIRECTORY_TABLE:
            switch (this->mCardStatus)
            {
            case STATUS_OK:
            case STATUS_CARD_CHANGED:
                this->mTaskTrcList.mTaskResult = RESULT_SUCCESS;
                if (this->mNumFilesFound > 0)
                {
                    this->mTaskTrcList.mFileFound = true;
                    this->mTaskTrcList.mCardStatus = STATUS_OK;
                    this->mTaskTrcList.mCurEntry = 0;
                    this->mTaskTrcList.mNumEntries = this->mNumFilesFound;
                    this->mTaskTrcList.mEntries = static_cast<sceMcTblGetDir *>(this->mISystem.mAllocator->Alloc(
                        this->mTaskTrcList.mNumEntries * sizeof(sceMcTblGetDir),
                        EA::TagValuePair(EA::Allocator::ATT_NAME, "ListFilesTable") +
                            EA::TagValuePair(EA::Allocator::ATT_ALIGNMENT, 64)));
                    memset(this->mTaskTrcList.mEntries, 0, this->mTaskTrcList.mNumEntries * sizeof(sceMcTblGetDir));
                    this->CheckSceResult(sceMcGetDir(this->mTaskTrcList.mCardID.port, this->mTaskTrcList.mCardID.slot,
                                                     this->mTaskTrcList.mFileInfo.directoryInfo->directoryName, 0,
                                                     this->mTaskTrcList.mNumEntries, this->mTaskTrcList.mEntries));
                    this->mTaskTrcListFiles.SetState(TS_FIND_FILE, TS_START);
                }
                else
                {
                    this->mTaskTrcList.mCardStatus = STATUS_ENTRY_NOT_FOUND;
                    this->mTaskTrcListFiles.SetState(TS_END, TS_START);
                }
                break;

            case STATUS_NO_CARD:
            default:
                this->mTaskTrcList.mTaskResult = RESULT_FAILED;
                this->mTaskTrcList.mCardStatus = this->mCardStatus;
                this->mTaskTrcListFiles.SetState(TS_END, TS_START);
                break;
            }
            break;
        }
        break;

    case TS_FIND_FILE:
        switch (this->mTaskTrcListFiles.GetSubstate())
        {
        case TS_START:
            this->mNumFilesFound = 0;
            this->mCardStatus = STATUS_UNKNOWN;

        case TS_CHANGE_DIRECTORY:
            this->CheckSceResult(sceMcChdir(this->mTaskTrcList.mCardID.port, this->mTaskTrcList.mCardID.slot,
                                            (char *)this->mTaskTrcList.mEntries[this->mTaskTrcList.mCurEntry].EntryName,
                                            nullptr));
            this->mTaskTrcListFiles.SetSubstate(TS_FIND_FILE);
            break;

        case TS_FIND_FILE:
            this->CheckSceResult(sceMcGetDir(this->mTaskTrcList.mCardID.port, this->mTaskTrcList.mCardID.slot,
                                             this->mTaskTrcList.mFileInfo.fileName, this->mTaskTrcListFiles.mFindNext,
                                             1, mPs2FileTableEntry));
            this->mTaskTrcListFiles.SetSubstate(TS_FIND_FILE_RESULT);
            break;

        case TS_FIND_FILE_RESULT:
            switch (this->mCardStatus)
            {
            case STATUS_OK:
            case STATUS_CARD_CHANGED:
                if (this->mNumFilesFound > 0)
                {
                    if (strcmp((char *)mPs2FileTableEntry[0].EntryName, ".") != 0 &&
                        strcmp((char *)mPs2FileTableEntry[0].EntryName, "..") != 0)
                    {
                        sprintf(this->mTaskTrcListFiles.mPath, "%s/%s",
                                this->mTaskTrcList.mEntries[this->mTaskTrcList.mCurEntry].EntryName,
                                mPs2FileTableEntry[0].EntryName);
                        this->mMsg.Set(LMSG_FILE_INFO);
                        this->mMsg.info.fileInfo.fileName = this->mTaskTrcListFiles.mPath;
                        this->mMsg.info.fileInfo.fileSize = mPs2FileTableEntry[0].FileSizeByte;
                        this->SetFileTime(&this->mMsg.info.fileInfo.fileTime, &mPs2FileTableEntry[0]);
                    }

                    this->mTaskTrcListFiles.mFindNext = true;
                    this->mTaskTrcListFiles.SetSubstate(TS_FIND_FILE);
                }
                else
                {
                    this->mTaskTrcListFiles.SetSubstate(TS_FIND_NEXT);
                }
                break;

            case STATUS_NO_CARD:
            default:
                this->mTaskTrcList.mTaskResult = RESULT_FAILED;
                this->mTaskTrcList.mCardStatus = this->mCardStatus;
                this->mTaskTrcListFiles.SetState(TS_END, TS_START);
                break;
            }
            break;

        case TS_FIND_NEXT:
            this->mTaskTrcList.mCurEntry++;
            if (this->mTaskTrcList.mCurEntry < this->mTaskTrcList.mNumEntries)
            {
                this->mTaskTrcListFiles.mFindNext = false;
                this->mTaskTrcListFiles.SetSubstate(TS_START);
                this->CheckSceResult(sceMcChdir(this->mTaskTrcList.mCardID.port, this->mTaskTrcList.mCardID.slot,
                                                ROOT_DIRECTORY_NAME, nullptr));
            }
            else
            {
                this->mTaskTrcListFiles.SetState(TS_END, TS_START);
            }
            break;
        }
        break;

    case TS_END:
        if (this->mTaskTrcList.mEntries != nullptr)
        {
            this->mISystem.mAllocator->Free(this->mTaskTrcList.mEntries, 0);
            this->mTaskTrcList.mEntries = nullptr;
            this->mTaskTrcList.mNumEntries = 0;
        }

        this->mTaskTrcListFiles.End();
        break;
    }
}

} // namespace Realmc
