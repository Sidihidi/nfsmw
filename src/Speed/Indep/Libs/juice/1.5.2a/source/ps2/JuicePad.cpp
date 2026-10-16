// Juice 1.5.2a: JuicePad.cpp, el mando virtual de Juice: mascara de
// botones pulsados y sus valores analogicos (el tJuicePad que se cruza con
// el juego va a +0 y GetMasterPad lo devuelve), mascara de deshabilitados
// y el boton de script que se repite cada cierto numero de frames (delta).
// Escrito del asm de la A124 (JuicePad.o: .text 0x4B58A8, 16 etiquetas).
// Unidad solo enlaza en A124. Los botones se numeran desde 1 en las
// mascaras (bit eButton-1) y desde 0 en la tabla de valores.

namespace Juice {

enum eJuiceButtonNames {
};

struct tJuicePad {
    int m_iButtonMask;    // +0x00
    float m_fButtons[32]; // +0x04
};

class JuiceMonkey {
public:
    static JuiceMonkey *Instance();
    void CheckMonkey();
};

class PadConfigManager {
public:
    static PadConfigManager *Instance();
    void SendAutoScriptCommand(int iCommand);
};

class JuicePad {
public:
    JuicePad();
    static JuicePad *Instance();
    void SetButton(eJuiceButtonNames eButton, float fValue);
    void SetScriptButton(eJuiceButtonNames eButton, float fValue);
    void SetScriptButtonDelta(int iDelta);
    void StopScriptButtonDelta();
    void EnableButton(eJuiceButtonNames eButton);
    void DisableButton(eJuiceButtonNames eButton);
    int IsButtonDisabled(eJuiceButtonNames eButton);
    int IsButtonPressed(eJuiceButtonNames eButton);
    int DidInputHappen();
    tJuicePad *GetMasterPad();
    void ResetButtons();
    void SetButtonMask(unsigned int iMask);
    void SetScriptButtonMask(unsigned int iMask);
    void UpdateJuicePad();

private:
    tJuicePad m_tMasterPad;      // +0x00
    int m_iUnused132;            // +0x84
    int m_iDisabledMask;         // +0x88
    int m_iScriptButton;         // +0x8C
    unsigned int m_iScriptMask;  // +0x90
    float m_fScriptValue;        // +0x94
    int m_iScriptDelta;          // +0x98
    int m_iScriptCounter;        // +0x9C
};

JuicePad::JuicePad() {
    m_iScriptButton = -1;
    m_iScriptDelta = 1;
    m_iScriptMask = 0;
    m_fScriptValue = 0.0f;
    m_iScriptCounter = 0;
}

JuicePad *JuicePad::Instance() {
    static JuicePad JP;
    return &JP;
}

void JuicePad::SetButton(eJuiceButtonNames eButton, float fValue) {
    if (!IsButtonDisabled(eButton)) {
        m_tMasterPad.m_iButtonMask |= (1 << ((int)eButton - 1));
        m_tMasterPad.m_fButtons[eButton] = fValue;
    }
}

void JuicePad::SetScriptButton(eJuiceButtonNames eButton, float fValue) {
    if (!IsButtonDisabled(eButton)) {
        m_tMasterPad.m_iButtonMask |= (1 << ((int)eButton - 1));
        m_tMasterPad.m_fButtons[eButton] = fValue;
        m_iScriptButton = (int)eButton;
        m_fScriptValue = fValue;
        m_iScriptMask = 0;
        m_iScriptCounter = 0;
    }
}

void JuicePad::SetScriptButtonDelta(int iDelta) {
    m_iScriptDelta = iDelta;
}

void JuicePad::StopScriptButtonDelta() {
    m_iScriptCounter = m_iScriptDelta;
}

void JuicePad::EnableButton(eJuiceButtonNames eButton) {
    m_iDisabledMask &= ~(1 << ((int)eButton - 1));
}

void JuicePad::DisableButton(eJuiceButtonNames eButton) {
    m_iDisabledMask |= (1 << ((int)eButton - 1));
}

int JuicePad::IsButtonDisabled(eJuiceButtonNames eButton) {
    return (m_iDisabledMask >> ((int)eButton - 1)) & 1;
}

int JuicePad::IsButtonPressed(eJuiceButtonNames eButton) {
    return (m_tMasterPad.m_iButtonMask >> ((int)eButton - 1)) & 1;
}

int JuicePad::DidInputHappen() {
    return m_tMasterPad.m_iButtonMask != 0;
}

tJuicePad *JuicePad::GetMasterPad() {
    return &m_tMasterPad;
}

void JuicePad::ResetButtons() {
    int i;

    m_tMasterPad.m_iButtonMask = 0;
    for (i = 31; i >= 0; i--) {
        m_tMasterPad.m_fButtons[i] = 0.0f;
    }
    if (m_iScriptCounter < m_iScriptDelta) {
        if (m_iScriptMask != 0) {
            SetButtonMask(m_iScriptMask);
        } else if (m_iScriptButton >= 0) {
            SetButton((eJuiceButtonNames)m_iScriptButton, m_fScriptValue);
        }
        m_iScriptCounter++;
    }
}

void JuicePad::SetButtonMask(unsigned int iMask) {
    int i;

    if (iMask != 0) {
        m_tMasterPad.m_iButtonMask = iMask;
        for (i = 0; i < 21; i++) {
            if (IsButtonPressed((eJuiceButtonNames)i) == 1) {
                m_tMasterPad.m_fButtons[i] = 255.0f;
            }
        }
    }
}

void JuicePad::SetScriptButtonMask(unsigned int iMask) {
    int i;

    if (iMask != 0) {
        m_tMasterPad.m_iButtonMask = iMask;
        for (i = 0; i < 21; i++) {
            if (IsButtonPressed((eJuiceButtonNames)i) == 1) {
                m_tMasterPad.m_fButtons[i] = 255.0f;
            }
        }
        m_iScriptMask = iMask;
        m_iScriptButton = -1;
        m_iScriptCounter = 0;
    }
}

void JuicePad::UpdateJuicePad() {
    JuiceMonkey::Instance()->CheckMonkey();
    PadConfigManager::Instance()->SendAutoScriptCommand((int)m_tMasterPad.m_iButtonMask);
}

}
