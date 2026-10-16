// Juice 1.5.2a: JuiceRandom.cpp, generador pseudoaleatorio de estado
// pequeno (seis palabras de 16 bits utiles). Escrito del asm de la A124
// (JuiceRandom.o: .text 0x4B6548, 4 etiquetas). Unidad solo enlaza en A124.
// SetSeed mezcla la semilla con los seis primeros caracteres de
// "ArCaDe-CoinOp"; GetRandom propaga el acarreo de la suma por el estado y
// despues suma uno al contador completo, palabra a palabra desde la ultima.

namespace Juice {

class JuiceRandom {
public:
    JuiceRandom();
    static JuiceRandom *Instance();
    void SetSeed(unsigned int iSeed);
    unsigned int GetRandom();

private:
    unsigned int m_iSeed[6]; // +0x00..+0x14
};

JuiceRandom::JuiceRandom() {
    m_iSeed[0] = 0xAFE5;
    m_iSeed[1] = 0xF527;
    m_iSeed[2] = 0xD2F5;
    m_iSeed[3] = 0x56B7;
    m_iSeed[4] = 0xACD;
    m_iSeed[5] = 0x1638;
}

JuiceRandom *JuiceRandom::Instance() {
    static JuiceRandom JR;
    return &JR;
}

void JuiceRandom::SetSeed(unsigned int iSeed) {
    const char *pStr = "ArCaDe-CoinOp";
    unsigned int *pSeed = m_iSeed;
    int i;

    for (i = 5; i >= 0; i--) {
        *pSeed++ = iSeed + *pStr++;
    }
}

unsigned int JuiceRandom::GetRandom() {
    unsigned int iCarry;
    unsigned int iVal;
    unsigned int *pSeed;
    unsigned int i;

    iVal = m_iSeed[5];
    iVal += m_iSeed[4];
    iCarry = iVal >> 16;
    iVal &= 0xFFFF;
    m_iSeed[4] = iVal;
    pSeed = &m_iSeed[3];
    for (i = 0; i < 4; i++) {
        iVal += *pSeed + iCarry;
        iCarry = iVal >> 16;
        iVal &= 0xFFFF;
        *pSeed = iVal;
        pSeed--;
    }
    m_iSeed[5] = (m_iSeed[5] + 1) & 0xFFFF;
    if (m_iSeed[5] == 0) {
        m_iSeed[4] = (m_iSeed[4] + 1) & 0xFFFF;
        if (m_iSeed[4] == 0) {
            m_iSeed[3] = (m_iSeed[3] + 1) & 0xFFFF;
            if (m_iSeed[3] == 0) {
                m_iSeed[2] = (m_iSeed[2] + 1) & 0xFFFF;
                if (m_iSeed[2] == 0) {
                    m_iSeed[1] = (m_iSeed[1] + 1) & 0xFFFF;
                    if (m_iSeed[1] == 0) {
                        pSeed = m_iSeed;
                        *pSeed = (*pSeed + 1) & 0xFFFF;
                        iVal = (iVal + 1) & 0xFFFF;
                    }
                }
            }
        }
    }
    return iVal & 0x7FFFFFFF;
}

}
