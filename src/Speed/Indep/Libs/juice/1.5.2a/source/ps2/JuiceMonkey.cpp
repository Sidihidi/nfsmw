// Juice 1.5.2a: JuiceMonkey.cpp, el mono que pulsa botones del mando solo
// (secuencial o al azar, con una frecuencia por boton). Escrito del asm de
// la A124 (JuiceMonkey.o: .text 0x4B50A8, 8 etiquetas). Unidad solo enlaza
// en A124. El constructor siembra JuiceRandom con la hora del sistema y
// deja las 21 frecuencias a 100; en Random, los botones 17/18 y 19/20 son
// los dos ejes de cada stick y se fijan por pares a 0/128/255.

namespace Juice {

enum eMonkeyState {
    MONKEY_RANDOM = 0,
    MONKEY_SEQUENTIAL = 1,
};

enum eJuiceButtonNames {
};

class JuiceRandom {
public:
    static JuiceRandom *Instance();
    void SetSeed(unsigned int iSeed);
    unsigned int GetRandom();
};

class SystemManager {
public:
    static unsigned int GetCurrentSystemTime();
};

class JuicePad {
public:
    static JuicePad *Instance();
    void SetButton(eJuiceButtonNames eButton, float fValue);
};

class JuiceMonkey {
public:
    JuiceMonkey();
    static JuiceMonkey *Instance();
    void SetButtonFrequency(eJuiceButtonNames eButton, unsigned int iFrequency);
    void Sequential();
    void Random();
    void CheckMonkey();
    void ActivatePadMonkey(bool bActivate);
    void SetMonkeyState(eMonkeyState eState);

private:
    eMonkeyState m_eMonkeyState;          // +0x00
    bool m_bMonkeyActive;                  // +0x04
    unsigned int m_iButtonFrequency[21];   // +0x08
};

JuiceMonkey *JuiceMonkey::Instance() {
    static JuiceMonkey JM;
    return &JM;
}

JuiceMonkey::JuiceMonkey() {
    int i;

    m_eMonkeyState = MONKEY_RANDOM;
    m_bMonkeyActive = false;
    JuiceRandom::Instance()->SetSeed(SystemManager::GetCurrentSystemTime());
    for (i = 20; i >= 0; i--) {
        m_iButtonFrequency[i] = 100;
    }
}

void JuiceMonkey::SetButtonFrequency(eJuiceButtonNames eButton, unsigned int iFrequency) {
    m_iButtonFrequency[eButton] = iFrequency;
}

void JuiceMonkey::Sequential() {
    static int iButton = 0;

    JuicePad::Instance()->SetButton((eJuiceButtonNames)iButton++, 255.0f);
    if (iButton >= 21) {
        iButton = 0;
    }
}

void JuiceMonkey::Random() {
    int i;

    for (i = 0; i < 21; i++) {
        if ((JuiceRandom::Instance()->GetRandom() % 1000000) <= (unsigned int)(m_iButtonFrequency[i] * 10) &&
            m_iButtonFrequency[i] != 0) {
            if (i == 17 || i == 18) {
                if ((JuiceRandom::Instance()->GetRandom() % 100) > 50) {
                    if ((JuiceRandom::Instance()->GetRandom() % 100) > 50) {
                        JuicePad::Instance()->SetButton((eJuiceButtonNames)17, 255.0f);
                        JuicePad::Instance()->SetButton((eJuiceButtonNames)18, 128.0f);
                    } else {
                        JuicePad::Instance()->SetButton((eJuiceButtonNames)17, 0.0f);
                        JuicePad::Instance()->SetButton((eJuiceButtonNames)18, 128.0f);
                    }
                } else if ((JuiceRandom::Instance()->GetRandom() % 100) > 50) {
                    JuicePad::Instance()->SetButton((eJuiceButtonNames)17, 128.0f);
                    JuicePad::Instance()->SetButton((eJuiceButtonNames)18, 255.0f);
                } else {
                    JuicePad::Instance()->SetButton((eJuiceButtonNames)17, 128.0f);
                    JuicePad::Instance()->SetButton((eJuiceButtonNames)18, 0.0f);
                }
            } else if (i == 19 || i == 20) {
                if ((JuiceRandom::Instance()->GetRandom() % 100) > 50) {
                    if ((JuiceRandom::Instance()->GetRandom() % 100) > 50) {
                        JuicePad::Instance()->SetButton((eJuiceButtonNames)19, 255.0f);
                        JuicePad::Instance()->SetButton((eJuiceButtonNames)20, 128.0f);
                    } else {
                        JuicePad::Instance()->SetButton((eJuiceButtonNames)19, 0.0f);
                        JuicePad::Instance()->SetButton((eJuiceButtonNames)20, 128.0f);
                    }
                } else if ((JuiceRandom::Instance()->GetRandom() % 100) > 50) {
                    JuicePad::Instance()->SetButton((eJuiceButtonNames)19, 128.0f);
                    JuicePad::Instance()->SetButton((eJuiceButtonNames)20, 255.0f);
                } else {
                    JuicePad::Instance()->SetButton((eJuiceButtonNames)19, 128.0f);
                    JuicePad::Instance()->SetButton((eJuiceButtonNames)20, 0.0f);
                }
            } else {
                JuicePad::Instance()->SetButton((eJuiceButtonNames)i, 255.0f);
            }
        }
    }
}

void JuiceMonkey::CheckMonkey() {
    if (m_bMonkeyActive) {
        if (m_eMonkeyState == MONKEY_RANDOM) {
            Random();
        } else if (m_eMonkeyState == MONKEY_SEQUENTIAL) {
            Sequential();
        }
    }
}

void JuiceMonkey::ActivatePadMonkey(bool bActivate) {
    m_bMonkeyActive = bActivate;
}

void JuiceMonkey::SetMonkeyState(eMonkeyState eState) {
    m_eMonkeyState = eState;
}

}
