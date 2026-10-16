#include "Speed/Indep/Src/FEng/FELocalizer.h"
#include "Speed/Indep/Src/FEng/FEngStandard.h"

// Del FELocalizer.cpp del original solo queda IsDigit, y solo en la A124 (la llama uiOnlineLogin.cpp): el enlazador
// estripa FECopyString, AtoI, ItoA y los metodos de FELocalString y FELocalizer en las tres versiones.

























bool IsDigit(char c) {
    return c >= '0' && c <= '9';
}
