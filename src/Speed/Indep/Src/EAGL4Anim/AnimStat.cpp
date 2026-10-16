#include "AnimStat.h"
#include "eagl4AnimBank.h"

namespace EAGL4Anim {

// Las funciones de BankStat y AllStat (las de AnimStat.h) estan estripadas en las tres versiones: no hay codigo
// contra el que escribirlas. El puntero si sobrevive en el .data de las dos PS2.
AllStat *AllStat::mpStat = nullptr;

}; // namespace EAGL4Anim
