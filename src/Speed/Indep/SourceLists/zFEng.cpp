// Todas las unidades de juego del original empiezan con el mismo prefijo de cabeceras: en la A124 los 402 primeros
// tipos de los stabs son identicos en 33 unidades (bWare, UMath, la STL, UStandard, UCOM, UCrc...), el DWARF de
// GameCube trae la misma cadena en zFEng y la .rodata de las dos empieza por las cadenas de bWare.hpp. Con la STL
// entran en PS2 los operadores globales de stl_relops.h, que el original expande en FEObject::SetPosition,
// SetRotation y SetColor y en FEPackageReader::ReadObjectTags.
#include "Speed/Indep/bWare/Inc/bWare.hpp"
#include "Speed/Indep/Libs/Support/Utility/UStandard.h"
#include "Speed/Indep/Libs/Support/Utility/UCOM.h"
#include "Speed/Indep/Libs/Support/Utility/UCollections.h"

#include "Speed/Indep/Src/FEng/FEButtonMap.cpp"

#include "Speed/Indep/Src/FEng/FECodeListBox.cpp"

#include "Speed/Indep/Src/FEng/FEEvent.cpp"

#include "Speed/Indep/Src/FEng/FEGroup.cpp"

#include "Speed/Indep/Src/FEng/FEJoyPad.cpp"

#include "Speed/Indep/Src/FEng/FEKeyInterp.cpp"

#include "Speed/Indep/Src/FEng/FEKeyInterpLinear.cpp"

#include "Speed/Indep/Src/FEng/FEKeyInterpNone.cpp"

#include "Speed/Indep/Src/FEng/FEKeyTrack.cpp"

#include "Speed/Indep/Src/FEng/FEKeyTypes.cpp"

#include "Speed/Indep/Src/FEng/FEList.cpp"

#include "Speed/Indep/Src/FEng/FEListBox.cpp"

#include "Speed/Indep/Src/FEng/FELocalizer.cpp"

#include "Speed/Indep/Src/FEng/FEMath.cpp"

#include "Speed/Indep/Src/FEng/FEMessageResponse.cpp"

#include "Speed/Indep/Src/FEng/FEMouse.cpp"

#include "Speed/Indep/Src/FEng/FEMsgTargetList.cpp"

#include "Speed/Indep/Src/FEng/FEngine.cpp"

#include "Speed/Indep/Src/FEng/FEngStandard.cpp"

#include "Speed/Indep/Src/FEng/FEObject.cpp"

#include "Speed/Indep/Src/FEng/FEPackage.cpp"

#include "Speed/Indep/Src/FEng/FEPackageList.cpp"

#include "Speed/Indep/Src/FEng/FEPackageReader.cpp"

#include "Speed/Indep/Src/FEng/FERefList.cpp"

#include "Speed/Indep/Src/FEng/FEScript.cpp"

#include "Speed/Indep/Src/FEng/FESlotPool.cpp"

#include "Speed/Indep/Src/FEng/FEString.cpp"

#include "Speed/Indep/Src/FEng/FETypeLib.cpp"

#include "Speed/Indep/Src/FEng/FETypeNode.cpp"

#include "Speed/Indep/Src/FEng/FETypes.cpp"

#include "Speed/Indep/Src/FEng/FEWideString.cpp"

#include "Speed/Indep/Src/FEng/FEMultiImage.cpp"
