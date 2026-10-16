#include "VoiceCore.hpp"
#include "NetworkMutex.hpp"

#include "Speed/Indep/bWare/Inc/bWare.hpp"
#include "Speed/Indep/Src/Frontend/Database/FEDatabase.hpp"

extern "C" {
VoipRefT *VoipStartup(int maxPeers);
void VoipShutdown(VoipRefT *voip);
void VoipSetLocalUser(VoipRefT *voip, const char *name);
int VoipControl(VoipRefT *voip, int control, int value);
unsigned int VoipRemote(VoipRefT *voip, int channel);
int VoipLocal(VoipRefT *voip);
void VoipMicrophone(VoipRefT *voip, int mask);
void VoipSpeaker(VoipRefT *voip, int mask);
}

VoiceCore *VoiceCore::mInstance;
static NetworkMutex voiceMutex;

VoiceCore::PushToTalk::PushToTalk() { mIsPolling = false; }

void VoiceCore::PushToTalk::StartPolling() {
    if (!mIsPolling) {
        for (int i = 0; i < 2; i++) {
            mIsPressed[i] = false;
            mActionQ[i] = new ActionQueue(i, 0x6689019e, "VOIP", false);
            mActionQ[i]->Enable(true);
        }
        mIsPolling = true;
    }
}

void VoiceCore::PushToTalk::StopPolling() {
    if (mIsPolling) {
        for (int i = 0; i < 2; i++) {
            delete mActionQ[i];
        }
        mIsPolling = false;
    }
}

bool VoiceCore::PushToTalk::IsPressed() {
    bool ispressed = false;
    StartPolling();
    for (int port = 0; port < 2; port++) {
        while (!mActionQ[port]->IsEmpty()) {
            ActionRef aRef = mActionQ[port]->GetAction();
            if (aRef.ID() == 0x34) {
                mIsPressed[port] = aRef.Data() > 0.5f;
            }
            mActionQ[port]->PopAction();
        }
        if (mIsPressed[port]) {
            ispressed = true;
        }
    }
    return ispressed;
}

VoiceCore::VoiceCore()
    : mTransmit(false), //
      VoipRef(nullptr), //
      headset_state_at_last_shutdown(true), //
      headset_ready_remember(false), //
      speaker_state_remember(false) {
    speaker_volume_remember = 0.7f;
    playback_volume_remember = 0.7f;
    all_muted = false;
    startup_timer.UnSet();
}

inline VoiceCore::~VoiceCore() {}

void VoiceCore::Construct() { mInstance = new ("VoiceCore", 0) VoiceCore; }

void VoiceCore::Startup() {
    voiceMutex.Lock();
    VoipRef = VoipStartup(4);
    if (!VoipRef) {
        voiceMutex.Unlock();
        return;
    }
    bMemSet(mute_record, 0, sizeof(mute_record));
    bMemSet(channels, 0, sizeof(channels));
    headset_ready_remember = false;
    all_muted = false;
    startup_timer = RealTimer;
    VoipSetLocalUser(VoipRef, FEDatabase->OnlineSettings.GetLobbyPersona());
    _SetPlaybackVolume(playback_volume_remember);
    SetMicState(false);
    voiceMutex.Unlock();
}

void VoiceCore::Shutdown() {
    voiceMutex.Lock();
    if (!VoipRef) {
        voiceMutex.Unlock();
        return;
    }
    headset_state_at_last_shutdown = _IsHeadsetConnected();
    _RemoveAllPlayers();
    VoipShutdown(VoipRef);
    VoipRef = nullptr;
    mPTT.StopPolling();
    voiceMutex.Unlock();
}

inline void VoiceCore::AddPlayer(int channel, const char *persona) {
    voiceMutex.Lock();
    _AddPlayer(channel, persona);
    voiceMutex.Unlock();
}

int VoiceCore::AddPlayer(const char *persona) {
    voiceMutex.Lock();
    int channel = _AddPlayer(persona);
    voiceMutex.Unlock();
    return channel;
}

inline void VoiceCore::RemovePlayer(int channel) {
    voiceMutex.Lock();
    _RemovePlayer(channel);
    voiceMutex.Unlock();
}

void VoiceCore::RemovePlayer(const char *name) {
    int channel;
    voiceMutex.Lock();
    if (_IsInVOIPChat(name, &channel)) {
        _RemovePlayer(channel);
    }
    voiceMutex.Unlock();
}

void VoiceCore::RemoveAllPlayers() {
    voiceMutex.Lock();
    _RemoveAllPlayers();
    voiceMutex.Unlock();
}

void VoiceCore::SetSpeakerVolume(float v) {
    voiceMutex.Lock();
    _SetSpeakerVolume(v);
    voiceMutex.Unlock();
}

void VoiceCore::SetPlaybackVolume(float v) {
    voiceMutex.Lock();
    _SetPlaybackVolume(v);
    voiceMutex.Unlock();
}

void VoiceCore::SetSpeakerState(bool enable) {}

void VoiceCore::SetHeadsetPort(int port) {
    voiceMutex.Lock();
    if (VoipRef) {
        VoipControl(VoipRef, 'port', port);
    }
    voiceMutex.Unlock();
}

void VoiceCore::SetMicStatus(bool enable) {
    voiceMutex.Lock();
    SetMicState(enable);
    voiceMutex.Unlock();
}

bool VoiceCore::IsMuted(int channel) {
    voiceMutex.Lock();
    bool rc = _IsMuted(channel);
    voiceMutex.Unlock();
    return rc;
}

bool VoiceCore::IsMuted(const char *persona) {
    voiceMutex.Lock();
    bool rc = _IsMuted(persona);
    voiceMutex.Unlock();
    return rc;
}

void VoiceCore::ToggleMuted(const char *persona) {
    voiceMutex.Lock();
    _ToggleMuted(persona);
    voiceMutex.Unlock();
}

bool VoiceCore::IsSpeaking(int channel) {
    voiceMutex.Lock();
    bool rc = _IsSpeaking(channel);
    voiceMutex.Unlock();
    return rc;
}

bool VoiceCore::IsSpeaking(const char *persona) {
    int channel;
    bool rc = false;
    voiceMutex.Lock();
    if (_IsInVOIPChat(persona, &channel)) {
        rc = _IsSpeaking(channel);
    }
    voiceMutex.Unlock();
    return rc;
}

bool VoiceCore::IsConnected(int channel) {
    voiceMutex.Lock();
    bool rc = _IsConnected(channel);
    voiceMutex.Unlock();
    return rc;
}

bool VoiceCore::IsConnected(const char *persona) {
    int channel;
    bool rc = false;
    voiceMutex.Lock();
    if (_IsInVOIPChat(persona, &channel)) {
        rc = _IsConnected(channel);
    }
    voiceMutex.Unlock();
    return rc;
}

bool VoiceCore::IsActive(int channel) {
    bool rc = false;
    voiceMutex.Lock();
    if (VoipRef) {
        if (_IsConnected(channel)) {
            if (VoipRemote(VoipRef, channel) & 8) {
                rc = true;
            }
        }
    }
    voiceMutex.Unlock();
    return rc;
}

bool VoiceCore::IsHeadsetConnected() {
    voiceMutex.Lock();
    bool rc = _IsHeadsetConnected();
    voiceMutex.Unlock();
    return rc;
}

bool VoiceCore::IsHeadsetSending() {
    voiceMutex.Lock();
    bool rc = _IsHeadsetSending();
    voiceMutex.Unlock();
    return rc;
}

void VoiceCore::Update() {
    voiceMutex.Lock();
    if (!VoipRef) {
        mPTT.StopPolling();
    } else {
        if (mPTT.IsPressed()) {
            if (!mTransmit) {
                SetMicState(true);
                mTransmit = true;
            }
        } else if (mTransmit) {
            SetMicState(false);
            mTransmit = false;
        }

        static int last_local_status;
        int local_status = VoipLocal(VoipRef);
        if (local_status != last_local_status) {
            last_local_status = local_status;
        }

        for (int i = 0; i < 4; i++) {
            if (channels[i].assigned) {
                int remote_status = VoipRemote(VoipRef, i);
                if (remote_status != last_remote_status[i]) {
                    last_remote_status[i] = remote_status;
                }
            }
        }
    }
    voiceMutex.Unlock();
}

bool VoiceCore::IsInVOIPChat(const char *name, int *channel) {
    return _IsInVOIPChat(name, channel);
}

char *VoiceCore::GetChannelPersona(int channel) { return channels[channel].persona_name; }

inline bool VoiceCore::IsAssigned(int channel) { return channels[channel].assigned; }

void VoiceCore::SetMuted(int channel, bool mute) {
    if (!channels[channel].assigned) {
        return;
    }
    channels[channel].muted = mute;
    int mask = 0;
    if (!all_muted) {
        for (int i = 0; i < 4; i++) {
            if (!channels[i].muted) {
                mask |= 1 << i;
            }
        }
    }
    VoipMicrophone(VoipRef, mTransmit ? mask : 0);
    VoipSpeaker(VoipRef, mask);
}

inline bool VoiceCore::IsMicOn() {
    return VoipRef && (VoipLocal(VoipRef) & 1) != 0;
}

void VoiceCore::SetMicState(bool enable) {
    if (VoipRef) {
        VoipControl(VoipRef, 'micr', enable);
    }
}

void VoiceCore::_AddPlayer(int channel, const char *persona) {
    if (!channels[channel].assigned || bStrCmp(persona, channels[channel].persona_name) != 0) {
        _RemovePlayer(channel);
        channels[channel].assigned = true;
        channels[channel].muted = false;
        bStrCpy(channels[channel].persona_name, persona);
        last_remote_status[channel] = -1;
        for (int i = 0; i < 4; i++) {
            if (bStrCmp(channels[channel].persona_name, mute_record[i].persona_name) == 0) {
                channels[channel].muted = mute_record[i].muted;
            }
        }
    }
}

int VoiceCore::_AddPlayer(const char *persona) {
    for (int i = 0; i < 4; i++) {
        if (!channels[i].assigned) {
            _AddPlayer(i, persona);
            return i;
        }
    }
    return -1;
}

void VoiceCore::_RemovePlayer(int channel) {
    if (channels[channel].assigned) {
        bMemCpy(&mute_record[channel], &channels[channel], sizeof(channel_type));
        bMemSet(&channels[channel], 0, sizeof(channel_type));
    }
}

void VoiceCore::_RemoveAllPlayers() {
    if (VoipRef) {
        for (int i = 0; i < 4; i++) {
            _RemovePlayer(i);
        }
    }
}

bool VoiceCore::_IsInVOIPChat(const char *name, int *channel) {
    for (int i = 0; i < 4; i++) {
        if (channels[i].assigned && bStrCmp(name, channels[i].persona_name) == 0) {
            if (channel) {
                *channel = i;
            }
            return true;
        }
    }
    if (channel) {
        *channel = -1;
    }
    return false;
}

bool VoiceCore::_IsMuted(int channel) {
    bool rc = false;
    if (channels[channel].assigned) {
        rc = channels[channel].muted != 0;
    }
    return rc;
}

bool VoiceCore::_IsMuted(const char *persona) {
    int channel;
    bool rc;
    if (_IsInVOIPChat(persona, &channel)) {
        rc = _IsMuted(channel);
    } else {
        rc = false;
    }
    return rc;
}

void VoiceCore::_ToggleMuted(const char *persona) {
    int channel;
    if (_IsInVOIPChat(persona, &channel)) {
        SetMuted(channel, !IsMuted(channel));
    }
}

bool VoiceCore::_IsSpeaking(int c) {
    if (VoipRef) {
        return channels[c].assigned && (VoipRemote(VoipRef, c) & 2);
    }
    return false;
}

bool VoiceCore::_IsConnected(int c) {
    if (VoipRef) {
        return channels[c].assigned && (VoipRemote(VoipRef, c) & 1);
    }
    return false;
}

bool VoiceCore::_IsHeadsetConnected() {
    if (VoipRef) {
        return (VoipLocal(VoipRef) & 0x40) != 0;
    }
    return false;
}

bool VoiceCore::_IsHeadsetSending() {
    if (VoipRef) {
        return (VoipLocal(VoipRef) & 0x100) != 0;
    }
    return false;
}

inline void VoiceCore::_SetSpeakerState(bool enable) {}

void VoiceCore::_SetSpeakerVolume(float v) {
    speaker_volume_remember = VU0_floatmin(1.0f, VU0_floatmax(0.0f, v));
}

void VoiceCore::_SetPlaybackVolume(float v) {
    v = VU0_floatmin(1.0f, VU0_floatmax(0.0f, v));
    playback_volume_remember = v;
    if (VoipRef) {
        VoipControl(VoipRef, 'play', static_cast<int>(v * 99.0f));
    }
}
