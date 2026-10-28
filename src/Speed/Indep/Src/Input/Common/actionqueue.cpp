#include "Speed/Indep/Src/Input/ActionQueue.h"
#include "Speed/Indep/Libs/Support/Utility/UListable.h"
#include "Speed/Indep/Libs/Support/Utility/UStandard.h"
#include "Speed/Indep/Src/Generated/AttribSys/Classes/controller.h"
#include "Speed/Indep/Src/Input/Action.h"
#include "Speed/Indep/Src/Input/ActionData.h"
#include "Speed/Indep/Src/Input/ActionRef.h"
#include "Speed/Indep/Src/Input/IOModule.h"
#include "Speed/Indep/Src/Input/InputDefParser.h"
#include "Speed/Indep/Src/Input/InputDevice.h"
#include "Speed/Indep/Src/Main/AttribSupport.h"
#include "Speed/Indep/Src/Misc/Joylog.hpp"
#include "Speed/Indep/Src/Misc/Timer.hpp"
#include "Speed/Indep/Tools/AttribSys/Runtime/AttribSys.h"
#ifdef MILESTONE_BUILD
#include "Speed/Indep/Src/Misc/TestHooks/JuiceHooks/JuiceHooks.h"
#endif

static const int PrintActionQueue = 0;

bool ActionQueue::sInJoylogFrame = false;
Timer ActionQueue::mLastAnyActionTime(0);

IMPLEMENT_LISTABLE(ActionQueue)

ActionQueue::ActionQueue(bool required) {
    this->mRequired = required;
    this->mPort = -1;
    this->mMappings = nullptr;
    this->mConfig = 0;
    this->mActionTime = 0;
    this->mActivationTime = 0;
    this->mState = AQS_ENABLED;
    this->mUniqueID = this->AssignUniqueID();
    this->mQueueName = "Unnamed";
    this->mConnected = false;

    this->fQueue.reset();
}

ActionQueue::ActionQueue(int port, unsigned int config, const char *queue_name, bool required) {
    this->mRequired = required;
    this->mPort = -1;
    this->mMappings = nullptr;
    this->mConfig = 0;
    this->mActionTime = 0;
    this->mActivationTime = 0;
    this->mState = AQS_ENABLED;
    this->mUniqueID = this->AssignUniqueID();
    this->mConnected = false;

    this->fQueue.reset();
    this->mPort = port;

    this->SetConfig(config, queue_name);
}

ActionQueue::~ActionQueue() {
    if (this->mMappings != nullptr) {
        delete this->mMappings;
    }
}

// const ActionRef ActionQueue::operator[](int i) {

// }

bool ActionQueue::IsConnected() const {
    if (this->mPort < 0) {
        return false;
    }
    return this->mConnected;
}

void ActionQueue::IO_SetConnected(bool plugged) {
    if (this->mPort < 0) {
        return;
    }
    if (this->mConnected == plugged) {
        return;
    }
    if (plugged) {
        ActionData a(ACTION_PLUGGED, 0.0f, this->mPort);

        this->ReceiveAction(a);
    } else {
        ActionData a(ACTION_UNPLUGGED, 0.0f, this->mPort);

        this->ReceiveAction(a);
    }
}

void ActionQueue::IO_Flush() {
    if (this->mPort < 0) {
        return;
    }

    ActionData a(ACTION_FLUSH, 0.0f, this->mPort);

    this->ReceiveAction(a);
}

void ActionQueue::Init(int port, unsigned int config) {
    if (port == this->mPort && config == this->mConfig) {
        return;
    }
    this->mPort = port;
    this->mConfig = config;

    if (this->mMappings != nullptr) {
        delete this->mMappings;
        this->mMappings = nullptr;
    }

    if (port >= 0) {
        InputDevice *device = IOModule::GetIOModule().GetDevice(port);
        if (device != nullptr) {
            const Attrib::Collection *spec = Attrib::FindCollection(Attrib::Gen::controller::ClassKey(), this->mConfig);
            if (spec != nullptr) {
                this->mMappings = new InputMapping(device, spec);
            }
        }
    }

    this->OnActivationChange();
}

void ActionQueue::SetPort(int port) {
    this->Init(port, this->mConfig);
}

void ActionQueue::SetConfig(unsigned int config, const char *queue_name) {
    this->mQueueName = queue_name;
    this->Init(this->mPort, config);
}

bool ActionQueue::IsEnabled() const {
    return this->mState > AQS_DISABLED;
}

void ActionQueue::Enable(bool b) {
    if (b) {
        if (this->mState == AQS_DISABLED) {
            this->mState = AQS_ENABLED;
            this->Flush();
        }
    } else {
        if (this->mState != AQS_DISABLED) {
            this->mState = AQS_DISABLED;
            this->Flush();
        }
    }

    this->OnActivationChange();
}

void ActionQueue::FetchCurrentValues(InputDevice *device) {

    UTL::Std::list<InputMapEntry, _type_list> &e = this->mMappings->GetEntries();

    for (UTL::Std::list<InputMapEntry, _type_list>::iterator iter = e.begin(); iter != e.end(); iter++) {

        InputMapEntry &entry = *iter;
        DeviceScalar *button = device->GetDeviceScalar(entry.DeviceScalarIndex);

        if (button == nullptr) {
            entry.PreviousValue = -1.0f;
            entry.CurrentValue = -1.0f;
            continue;
        }

        switch (entry.UpdateType) {

        case kUpdate:
            entry.CurrentValue = UMath::Ramp(button->GetValue(), entry.LowerDZ, 1.0f - entry.UpperDZ);
            break;

        case kPress:
        case kRelease:
            if (button->GetValue() >= 0.5f) {
                entry.CurrentValue = 1.0f;
            } else {
                entry.CurrentValue = 0.0f;
            }
            break;

        case kAnalogPress:
        case kAnalogRelease:
            if (button->GetValue() >= entry.LowerDZ) {
                entry.CurrentValue = 1.0f;
            } else {
                entry.CurrentValue = 0.0f;
            }
            break;
        }
    }
}

ActionQueue *ActionQueue::FindActionQueue(int id) {
    for (ActionQueue *const *iter = ActionQueue::GetList().begin(); iter != ActionQueue::GetList().end(); iter++) {
        ActionQueue *q = *iter;

        if (q->mUniqueID == id) {
            return q;
        }
    }

    return nullptr;
}

// ActionQueue::FindActionQueue(unsigned int config, int port)

int ActionQueue::AssignUniqueID() {
    int id = 0;
    while (ActionQueue::FindActionQueue(id) != nullptr) {
        id += 1;
    }

    return id;
}

void ActionQueue::IO_UpdateFromDevice() {

    if (this->mMappings == nullptr) {
        return;
    }

    if (this->mState == AQS_DISABLED) {
        return;
    }

    if (this->mPort < 0) {
        return;
    }

    InputDevice *device = IOModule::GetIOModule().GetDevice(this->mPort);

    if (device == nullptr) {
        return;
    }

    ActionData ad(0, 0.0f, 0);

    this->FetchCurrentValues(device);

    UTL::Std::list<InputMapEntry, _type_list> &e = this->mMappings->GetEntries();
    UTL::Std::list<InputMapEntry, _type_list>::iterator iaction = e.begin();

    for (UTL::Std::list<InputMapEntry, _type_list>::iterator iter = e.begin(); iter != e.end(); iter++) {

        if ((*iter).Action != (*iaction).Action) {
            iaction = iter;
        }

        InputMapEntry &entry = *iter;

        if (entry.CurrentValue < 0.0f || !entry.HasChanged()) {
            continue;
        }

        bool ignore_device = false;

        if (entry.UpdateType == kUpdate) {

            for (UTL::Std::list<InputMapEntry, _type_list>::iterator dupe = iaction; dupe != e.end(); dupe++) {

                if (dupe == iter) {
                    continue;
                }

                const InputMapEntry &entry2 = *dupe;

                if (entry2.Action != entry.Action) {
                    break;
                }

                if (entry2.UpdateType == kUpdate && entry2.CurrentValue > entry.CurrentValue) {
                    ignore_device = true;
                    break;
                }
            }
        }

        if (!ignore_device) {

            ad.SetID(0);

            switch (entry.UpdateType) {

            case kPress:
            case kAnalogPress:
                if (entry.CurrentValue >= 0.5f && entry.PreviousValue < 0.5f && entry.PreviousValue >= 0.0f) {
                    ad.SetID(entry.Action);
                    ad.SetData(entry.CurrentValue);
                    ad.SetSlot(device->GetDeviceIndex());
                }
                break;

            case kRelease:
            case kAnalogRelease:
                if (entry.CurrentValue < 0.5f && entry.PreviousValue >= 0.5f) {
                    ad.SetID(entry.Action);
                    ad.SetData(entry.CurrentValue);
                    ad.SetSlot(device->GetDeviceIndex());
                }
                break;

            case kUpdate:
                ad.SetID(entry.Action);
                ad.SetData(entry.CurrentValue);
                ad.SetSlot(device->GetDeviceIndex());
                break;
            }

            if (ad.ID() != 0) {
                this->ReceiveAction(ad);
            }
        }

        entry.PreviousValue = entry.CurrentValue;
    }
}

void ActionQueue::OnActivationChange() {
    bool is_active;
    bool was_active = false;

    if (this->IsEnabled() && this->IsConnected()) {
        is_active = this->IsActive();
        if (is_active) {
            was_active = true;
        }
    }
    if (was_active != (this->mActivationTime.IsSet() != 0)) {
        if (was_active) {
            this->mActivationTime = RealTimer;
        } else {
            this->mActivationTime.ResetLow();
        }
    }
}

bool ActionQueue::IsEmpty() {
    return this->fQueue.size() == 0;
}

// unfinished
bool ActionQueue::ReceiveAction(ActionData &action) {
    if (action.Slot() != this->mPort) {
        return false;
    }
    if (action.ID() != ACTION_UNPLUGGED) {
        this->mLastAnyActionTime = RealTimer;
    }
#ifdef MILESTONE_BUILD
    // La A124 graba la accion en la repeticion de Juice cuando Juice esta activo (locales de sus stabs).
    if (gJuiceEnabled) {
        if (Juice::GameHook::Instance()->Replay_IsCapturing() && !Juice::GameHook::Instance()->Replay_IsReplaying()) {
            int queue_id = this->mUniqueID;
            int id = action.ID();
            float data = action.Data();
            Juice::GameHook::Instance()->ReplayData(queue_id, JOYLOG_CHANNEL_JOYEVENTS, "", 0);
            Juice::GameHook::Instance()->ReplayData(id, JOYLOG_CHANNEL_JOYEVENTS, "", 0);
            Juice::GameHook::Instance()->ReplayData(data, JOYLOG_CHANNEL_JOYEVENTS, "", 0);
        }
    } else
#endif
    {
        Joylog::AddData(this->mUniqueID, 8, JOYLOG_CHANNEL_JOYEVENTS);
        Joylog::AddData(action.ID(), 8, JOYLOG_CHANNEL_JOYEVENTS);
        Joylog::AddData(action.Data(), JOYLOG_CHANNEL_JOYEVENTS);
    }

    if (action.ID() == ACTION_UNPLUGGED) {
        this->Flush();
        this->mConnected = false;
        this->OnActivationChange();
    } else if (action.ID() == ACTION_PLUGGED) {
        this->Flush();
        this->mConnected = true;
        this->OnActivationChange();
    } else if (action.ID() == ACTION_FLUSH) {
        this->Flush();
    }
    this->mActionTime = RealTimer;

    this->fQueue.size(); // dwarf nonsense
    this->fQueue.enqueue(action);

    return true;
}

void ActionQueue::PopAction() {
    if (this->fQueue.size() > 0) {
        this->fQueue.dequeue();
    }
}

void ActionQueue::Flush() {
    this->fQueue.reset();

    if (this->mMappings != nullptr) {
        UTL::Std::list<InputMapEntry, _type_list> &e = this->mMappings->GetEntries();

        for (UTL::Std::list<InputMapEntry, _type_list>::iterator iter = e.begin(); iter != e.end(); iter++) {
            InputMapEntry &entry = *iter;
            entry.PreviousValue = -1.0;
            entry.CurrentValue = -1.0;
        }
    }
}

const ActionRef ActionQueue::GetAction() {
    if (this->fQueue.size() > 0) {
        ActionData *a = &this->fQueue.tail();
        return ActionRef(a);
    } else {
        return ActionRef(nullptr);
    }
}

void ActionQueue::BeginJoylogFrame() {
    sInJoylogFrame = true;
#ifdef MILESTONE_BUILD
    // La A124 lee las acciones de la repeticion de Juice cuando Juice esta activo (locales de sus stabs).
    if (gJuiceEnabled) {
        if (Juice::GameHook::Instance()->Replay_IsReplaying()) {
            while (true) {
                int queue_id;
                Juice::GameHook::Instance()->ReplayData(queue_id, JOYLOG_CHANNEL_JOYEVENTS, "", 0);
                if (queue_id == -1) {
                    if (!Juice::GameHook::Instance()->Replay_IsReplaying()) {
                        MWJuicePad::Instance()->SetIsLastFrame(true);
                    }
                    break;
                }
                ActionQueue *q = ActionQueue::FindActionQueue(queue_id);
                int action_id;
                float data;
                Juice::GameHook::Instance()->ReplayData(action_id, JOYLOG_CHANNEL_JOYEVENTS, "", 0);
                Juice::GameHook::Instance()->ReplayData(data, JOYLOG_CHANNEL_JOYEVENTS, "", 0);
                if (q != nullptr) {
                    ActionData a = ActionData(action_id, data, q->mPort);
                    q->ReceiveAction(a);
                    MWJuicePad::Instance()->TrackButtonPress(action_id, data, q->mPort);
                }
            }
        }
    } else
#endif
    if (Joylog::IsReplaying()) {
        while (true) {
            int queue_id = Joylog::GetSignedData(0x8, JOYLOG_CHANNEL_JOYEVENTS);
            if (queue_id == -1) break;

            ActionQueue *q = ActionQueue::FindActionQueue(queue_id);
            int action_id = Joylog::GetData(0x8, JOYLOG_CHANNEL_JOYEVENTS);
            float data = Joylog::GetData(JOYLOG_CHANNEL_JOYEVENTS);
            ActionData a = ActionData(action_id, data, q->mPort);
            q->ReceiveAction(a);
        }
    }
}

void ActionQueue::EndJoylogFrame() {
#ifdef MILESTONE_BUILD
    // La A124 cierra el cuadro de la repeticion de Juice con un -1 (local `i` de sus stabs).
    if (gJuiceEnabled) {
        if (Juice::GameHook::Instance()->Replay_IsCapturing() && !Juice::GameHook::Instance()->Replay_IsReplaying() &&
            !MWJuicePad::Instance()->IsLastJoyFrame()) {
            int i = -1;
            Juice::GameHook::Instance()->ReplayData(i, JOYLOG_CHANNEL_JOYEVENTS, "", 0);
        } else if (MWJuicePad::Instance()->IsLastJoyFrame()) {
            MWJuicePad::Instance()->SetIsLastFrame(false);
        }
    } else
#endif
    if (!Joylog::IsReplaying()) {
        Joylog::AddData(-1, 0x8, JOYLOG_CHANNEL_JOYEVENTS);
    }
    sInJoylogFrame = false;
}
