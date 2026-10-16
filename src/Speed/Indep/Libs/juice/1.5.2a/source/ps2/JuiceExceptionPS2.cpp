// Juice 1.5.2a: JuiceExceptionPS2.cpp, el manejador de excepciones de la
// EE: instala JuiceTrapHandler0 (la parte en asm de JuiceExceptionPS2.s,
// que vuelca los registros en juiceTrapReg) para las excepciones 1-7 y
// 10-12, clasifica la excepcion, reconstruye la pila de llamadas
// escaneando el epilogo en busca del jr ra, y la manda al servidor con el
// comando 703 antes de reiniciar. Escrito del asm de la A124
// (JuiceExceptionPS2.o: .text 0x4B3F98, 6 etiquetas). Unidad solo enlaza
// en A124. Casan al 100 % JuiceTrapHandlerPS2, StoreCallStackPS2 y
// JuiceTrapHandler; JuiceTrapLocateCaller queda al 96,1 % (el bloque
// compartido de "return 0" sale en otro sitio) y GetExceptionType al
// 95,2 % (s0/s1 cruzados para el codigo de excepcion y la referencia).

#include "eekernel.h"

namespace Juice {

enum Command {
};
enum Channels {
};

class cJuiceCom {
public:
    void NewMsg(Command cmd, Channels channel);
    void EndMsg(Channels channel);
    void Send(Channels channel);
    void PutInt(int i, Channels channel);
    void PutString(const char *pString, Channels channel);
};

class GameHook {
public:
    static GameHook *(*Instance)();
    int IsConnected();
    void RebootWhenAutomating();

    int m_bConnected;
    int m_iFrameCount;
    int m_iUnused8;
    int m_iUnused12;
    int m_iUnused16;
    cJuiceCom *m_pJuiceCom;
};

}

extern "C" void JuiceTrapHandler0(unsigned int, unsigned int, unsigned int, unsigned int, unsigned int,
                                  u_long128 *);

char juiceTrapReg_name[] = "ZRATV0V1A0A1A2A3T0T1T2T3T4T5T6T7S0S1S2S3S4S5S6S7T8T9K0K1GPSPFPRAPCSRHILO";
char *juiceTrapExcName[] = {
    "Watchdog timeout",
    "TLB exception (modify)",
    "TLB exception (load)",
    "TLB exception (store)",
    "Address error on load",
    "Address error on store",
    "Bus error on instruction fetch",
    "Bus error on data refere",
    "Syscall exception",
    "Breakpoint exception",
    "Reserved instruction exception",
    "Coprocessor unusable exception",
    "Arithmetic overflow exception",
};
char juiceTrapExcUnknown[] = "Unknown exception";
char juiceTrapExcDivZero[] = "Division by zero";
char juiceTrapExcDivOvfl[] = "Division overflow";
char juiceTrapExcDdAbort[] = "User Abort";
unsigned int juiceTrapReg[36];
void *orig_handler[12];

static unsigned short juiceTrapInstalled;

void JuiceTrapHandlerPS2() {
    if (juiceTrapInstalled != 0) {
        return;
    }
    orig_handler[0] = SetDebugHandler(1, JuiceTrapHandler0);
    orig_handler[1] = SetDebugHandler(2, JuiceTrapHandler0);
    orig_handler[2] = SetDebugHandler(3, JuiceTrapHandler0);
    orig_handler[3] = SetDebugHandler(4, JuiceTrapHandler0);
    orig_handler[4] = SetDebugHandler(5, JuiceTrapHandler0);
    orig_handler[5] = SetDebugHandler(6, JuiceTrapHandler0);
    orig_handler[6] = SetDebugHandler(7, JuiceTrapHandler0);
    orig_handler[8] = SetDebugHandler(10, JuiceTrapHandler0);
    orig_handler[9] = SetDebugHandler(11, JuiceTrapHandler0);
    orig_handler[10] = SetDebugHandler(12, JuiceTrapHandler0);
    juiceTrapInstalled = 0xB40F;
}

static inline int JuiceTrapValidAddress(unsigned int iAddress) {
    if ((iAddress & 3) != 0) {
        return 0;
    }
    if ((iAddress - 0x80000) <= 0x07F7FFFF) {
        return 1;
    }
    if ((iAddress - 0x80000000) <= 0x07FFFFFF) {
        return 1;
    }
    if ((iAddress - 0xA0080000) <= 0x07F7FFFF) {
        return 1;
    }
    if ((iAddress - 0x20080000) <= 0x07F7FFFF) {
        return 1;
    }
    if ((iAddress - 0x70000000) < 0x4000) {
        return 1;
    }
    return 0;
}

unsigned int JuiceTrapLocateCaller(unsigned int iPC, unsigned int iRA, unsigned int *pSP) {
    unsigned int iSP = *pSP;
    unsigned int iTarget = 0;
    unsigned int iInstr;

    if (iPC == 0) {
        *pSP = iSP;
        return iPC;
    }
    for (;;) {
        if (!JuiceTrapValidAddress(iPC)) {
            return 0;
        }
        if (!JuiceTrapValidAddress(iRA)) {
            return 0;
        }
        if (!JuiceTrapValidAddress(iSP)) {
            return 0;
        }
        iInstr = *(unsigned int *)iPC;
        iPC += 4;
        if (iTarget != 0) {
            iPC = iTarget;
        }
        if (iInstr == 0x03E00008) {
            iTarget = iRA;
            if (!JuiceTrapValidAddress(iRA - 8)) {
                return 0;
            }
            iInstr = *(unsigned int *)(iRA - 8);
            if ((iInstr & 0xFC000000) != 0x0C000000 && (iInstr & 0xFC1D07FF) != 9) {
                return 0;
            }
        } else if ((iInstr >> 16) == 0x8FBF || (iInstr >> 16) == 0xDFBF || (iInstr >> 16) == 0x7BBF) {
            iRA = iSP + (int)(short)iInstr;
            if (!JuiceTrapValidAddress(iRA)) {
                return 0;
            }
            iRA = *(unsigned int *)iRA;
        } else if ((iInstr >> 16) == 0x27BD) {
            iSP += (int)(short)iInstr;
        } else if (iInstr == 0x4300DEAD) {
            return 0;
        }
        if (iPC == iTarget) {
            break;
        }
    }
    *pSP = iSP;
    return iPC;
}

unsigned int StoreCallStackPS2(unsigned int iPC, unsigned int iRA, unsigned int iSP, unsigned int *pBuffer,
                               unsigned int iMax) {
    unsigned int i = 1;

    unsigned int *pOut = pBuffer + 1;

    pBuffer[0] = iPC;
    for (i = 1; i < iMax; i++, pOut++) {
        iRA = JuiceTrapLocateCaller(iPC, iRA, &iSP);
        if (iRA != 0) {
            iPC = iRA;
            *pOut = iRA;
        } else {
            break;
        }
    }
    return i;
}

char *GetExceptionType(unsigned int iCause, unsigned int &iPC) {
    unsigned int iInstr = 0;
    unsigned int iBreakCode = 0;
    unsigned int iExcCode = (iCause >> 2) & 0x1F;

    if (iCause & 0x80000000) {
        iPC += 4;
    }
    if (iExcCode == 9) {
        SetDebugHandler(9, 0);
        if (JuiceTrapValidAddress(iPC)) {
            iBreakCode = *(unsigned int *)iPC >> 6;
            if (iBreakCode < 1280) {
                return "Invalid PC";
            }
        }
    }
    if (iExcCode == 10) {
        if (JuiceTrapValidAddress(iPC)) {
            iInstr = *(unsigned int *)iPC;
        }
    }
    if (iExcCode == 10 && iInstr == 0x4E000666) {
        return juiceTrapExcDdAbort;
    }
    if (iExcCode == 9) {
        switch (iBreakCode) {
        case 6144:
            return juiceTrapExcDivOvfl;
        case 1638:
            return juiceTrapExcDdAbort;
        case 7168:
            return juiceTrapExcDivZero;
        }
    }
    if (iExcCode < 13) {
        return juiceTrapExcName[iExcCode];
    }
    return juiceTrapExcUnknown;
}

extern "C" void JuiceTrapHandler(unsigned int iCause) {
    unsigned int iCallStack[32];
    unsigned int iPC;
    unsigned int iCount;
    char *pType;
    Juice::cJuiceCom *pCom;
    unsigned int i;

    if (Juice::GameHook::Instance()->IsConnected()) {
        iPC = juiceTrapReg[32];
        pType = GetExceptionType(iCause, iPC);
        iCount = StoreCallStackPS2(iPC, juiceTrapReg[31], juiceTrapReg[29], iCallStack, 32);
        pCom = Juice::GameHook::Instance()->m_pJuiceCom;
        pCom->NewMsg((Juice::Command)703, (Juice::Channels)0);
        pCom->PutInt(Juice::GameHook::Instance()->m_iFrameCount, (Juice::Channels)0);
        pCom->PutInt((int)iCount, (Juice::Channels)0);
        if ((int)iCount > 0) {
            unsigned int *pStack = iCallStack;

            i = iCount;
            do {
                pCom->PutInt((int)*pStack, (Juice::Channels)0);
                pStack++;
                i--;
            } while (i != 0);
        }
        pCom->PutString(pType, (Juice::Channels)0);
        pCom->EndMsg((Juice::Channels)0);
        pCom->Send((Juice::Channels)0);
    }
    Juice::GameHook::Instance()->RebootWhenAutomating();
    for (;;) {
    }
}
