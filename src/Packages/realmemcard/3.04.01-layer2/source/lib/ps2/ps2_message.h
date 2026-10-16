#ifndef REALMEMCARD_PS2_MESSAGE_H
#define REALMEMCARD_PS2_MESSAGE_H

#ifdef EA_PRAGMA_ONCE_SUPPORTED
#pragma once
#endif

// El mensaje de la capa baja de PS2 (source/lib/ps2/ps2_message.h en el .mdebug de la A124). Los TRC_msg rellenan el
// mensaje de las directrices de Sony: el identificador del texto, el texto de Locale y las opciones empaquetadas de a
// ocho bits (PackMsgOptions). Las cadenas de formato de GetString salen en la .rodata de cada unidad que lo incluye.

#include <string.h>

#include "../../../include/common/realmemcard/impl/memcard_interface_impl.h"

namespace Realmc {

struct PS2Message : public Message {
    PS2Message() {
        this->Init();
    }

    virtual ~PS2Message() {}

    void Init() {
        this->Clear();
    }

    void Clear() {
        memset(this, 0, sizeof(*this));
    }

    void Set(LibMessage msg) {
        this->Clear();
        this->mMsg = msg;
    }

    void Set(LibMessage msg, TaskResult result, CardStatus status) {
        this->Clear();
        this->mMsg = msg;
        this->mTaskResult = result;
        this->mCardStatus = status;
    }

    void TRC_msg(int msgId, int options) {
        this->Set(LMSG_TRC);
        this->info.trc.mMsgId = msgId;
        this->info.trc.mMsg = Locale::GetString(msgId, 0);
        this->_SetMsgOptions(options);
    }

    void TRC_msg(int msgId, int options, char *str) {
        this->Set(LMSG_TRC);
        this->info.trc.mMsgId = msgId;
        this->info.trc.mMsg = Locale::GetString(msgId, "s", str);
        this->_SetMsgOptions(options);
    }

    void TRC_msg(int msgId, int options, char *str, int num) {
        this->Set(LMSG_TRC);
        this->info.trc.mMsgId = msgId;
        this->info.trc.mMsg = Locale::GetString(msgId, "sd", str, num);
        this->_SetMsgOptions(options);
    }

    void TRC_msg(int msgId, int options, char *str1, char *str2) {
        this->Set(LMSG_TRC);
        this->info.trc.mMsgId = msgId;
        this->info.trc.mMsg = Locale::GetString(msgId, "ss", str1, str2);
        this->_SetMsgOptions(options);
    }

    static int PackMsgOptions(int option1, int option2, int option3, int option4) {
        return option1 | (option2 << 8) | (option3 << 16) | (option4 << 24);
    }

  private:
    void _SetMsgOptions(int options) {
        int curOption;
        int iOption;

        this->info.trc.mNumOptions = 0;
        while (options != 0) {
            curOption = options & 0xFF;
            iOption = this->info.trc.mNumOptions++;
            this->info.trc.mOptions[iOption].mMsgId = curOption;
            this->info.trc.mOptions[iOption].mMsg = Locale::GetString(curOption, 0);
            options >>= 8;
        }
    }
};

} // namespace Realmc

#endif
