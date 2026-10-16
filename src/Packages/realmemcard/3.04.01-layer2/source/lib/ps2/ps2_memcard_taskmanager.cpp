#include "../../../include/common/realmemcard/impl/memcard_interface_impl.h"

namespace RealmcIface {

// Los mensajes TRC de la tarjeta de PS2 (Message::DetailInfo::Trc::mMsgId) son numeros sin enum en los stabs de la A124.
unsigned int TaskManager::FilterGuidelinesMessage(const Realmc::Message *message) {
    unsigned int messageMask = 0;

    switch (this->mMainTask) {
    case TASK_BOOTUPCHECK:
        if (this->mTaskList[this->mCurTask].mTask != TASK_BOOTUPCHECK && this->mCheckingMsgShown) {
            messageMask = 1;
        } else if (message->info.trc.mMsgId == 6) {
            this->mCheckingMsgShown = true;
        }
        if (message->mCardStatus == Realmc::STATUS_FILE_NOT_FOUND) {
            messageMask |= 0x8000;
        }
        if (this->mWarningMsgShown) {
            messageMask |= 0x2000;
        } else if (message->info.trc.mMsgId == 0xF) {
            this->mWarningMsgShown = true;
        }
        if (message->info.trc.mMsgId == 8) {
            messageMask |= 4;
        }
        break;
    case TASK_NONE:
    case TASK_SAVECHECK:
    case TASK_SAVE:
    case TASK_LOAD:
    case TASK_DELETE:
    case TASK_FINDENTRIES:
    case TASK_SETAUTOSAVE:
        break;
    case TASK_MONITOR:
        if (message->info.trc.mMsgId == 7 &&
            (this->mLastMessageId == 7 || this->mLastMessageId == 0x27 || this->mLastMessageId == 0x28 ||
             this->mLastMessageId == 0x14)) {
            messageMask = 2;
        } else if (message->info.trc.mMsgId == 8 &&
                   (this->mLastMessageId == 8 || this->mLastMessageId == 0x16 || this->mLastMessageId == 0x1B)) {
            messageMask = 4;
        }
        if (message->info.trc.mMsgId == 6) {
            if (this->mCheckingMsgShown) {
                messageMask |= 1;
                break;
            }
            this->mLastMessageId = message->info.trc.mMsgId;
            this->mCheckingMsgShown = true;
            break;
        }
        this->mLastMessageId = message->info.trc.mMsgId;
        break;
    default:
        break;
    }

    return messageMask;
}

} // namespace RealmcIface
