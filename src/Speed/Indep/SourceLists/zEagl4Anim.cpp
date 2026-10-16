// Prefijo comun de las unidades de juego del original (como zFe.cpp y zSim.cpp): el DWARF de GameCube de zEagl4Anim
// abre con los mismos tipos y en el mismo orden (bHash32, FastMem, UTransform, UTL::COM::Object, _type_UStorage,
// _KeyedNode, bVector2, bChunk, UCrc32). En PC hace falta: sin UCollections.h (UTLVector.h -> UMath.h) antes del
// primer global con constructor, cl 7.1 hace dinamicos UMath::PI, OOPI, TWOPI y OOTWOPI, y speed.exe no los tiene.
#include "Speed/Indep/bWare/Inc/bWare.hpp"
#include "Speed/Indep/Libs/Support/Utility/UStandard.h"
#include "Speed/Indep/Libs/Support/Utility/UCOM.h"
#include "Speed/Indep/Libs/Support/Utility/UCollections.h"

#include "Speed/Indep/Src/EAGL4Anim/eagl4AnimBank.cpp"

#include "Speed/Indep/Src/EAGL4Anim/AnimStat.cpp"

#include "Speed/Indep/Src/EAGL4Anim/AnimUtil.cpp"

#include "Speed/Indep/Src/EAGL4Anim/Attribute.cpp"

#include "Speed/Indep/Src/EAGL4Anim/AttributeDictionary.cpp"

#include "Speed/Indep/Src/EAGL4Anim/BoneMask.cpp"

#include "Speed/Indep/Src/EAGL4Anim/ClipSet.cpp"

#include "Speed/Indep/Src/EAGL4Anim/CompoundChannel.cpp"

#include "Speed/Indep/Src/EAGL4Anim/CsisEventChannel.cpp"

#include "Speed/Indep/Src/EAGL4Anim/DeltaChan.cpp"

#include "Speed/Indep/Src/EAGL4Anim/DeltaCompressedData.cpp"

#include "Speed/Indep/Src/EAGL4Anim/eagl4supportbase.cpp"

#include "Speed/Indep/Src/EAGL4Anim/eagl4supportconspool.cpp"

#include "Speed/Indep/Src/EAGL4Anim/eagl4supportdlopen.cpp"

#include "Speed/Indep/Src/EAGL4Anim/eagl4supportprintmessage.cpp"

#include "Speed/Indep/Src/EAGL4Anim/eagl4supportsympool.cpp"

#include "Speed/Indep/Src/EAGL4Anim/eagl4runtimetransform.cpp"

#include "Speed/Indep/Src/EAGL4Anim/EventTarget.cpp"

#include "Speed/Indep/Src/EAGL4Anim/FnAnim.cpp"

#include "Speed/Indep/Src/EAGL4Anim/FnAnimMemoryMap.cpp"

#include "Speed/Indep/Src/EAGL4Anim/FnDeltaF1.cpp"

#include "Speed/Indep/Src/EAGL4Anim/FnDeltaF3.cpp"

#include "Speed/Indep/Src/EAGL4Anim/FnDeltaQ.cpp"

#include "Speed/Indep/Src/EAGL4Anim/FnDeltaQFast.cpp"

#include "Speed/Indep/Src/EAGL4Anim/FnDeltaSingleQ.cpp"

#include "Speed/Indep/Src/EAGL4Anim/FnEventBlender.cpp"

#include "Speed/Indep/Src/EAGL4Anim/FnPoseAnim.cpp"

#include "Speed/Indep/Src/EAGL4Anim/FnPoseBlender.cpp"

#include "Speed/Indep/Src/EAGL4Anim/FnRawPoseChannel.cpp"

#include "Speed/Indep/Src/EAGL4Anim/FnRunBlender.cpp"

#include "Speed/Indep/Src/EAGL4Anim/FnStatelessF3.cpp"

#include "Speed/Indep/Src/EAGL4Anim/FnStatelessQ.cpp"

#include "Speed/Indep/Src/EAGL4Anim/FnTurnBlender.cpp"

#include "Speed/Indep/Src/EAGL4Anim/IK.cpp"

#include "Speed/Indep/Src/EAGL4Anim/MemoryPoolManager.cpp"

#include "Speed/Indep/Src/EAGL4Anim/PhaseChan.cpp"

#include "Speed/Indep/Src/EAGL4Anim/PoseAnim.cpp"

#include "Speed/Indep/Src/EAGL4Anim/RawEventChannel.cpp"

#include "Speed/Indep/Src/EAGL4Anim/RawPoseChannel.cpp"

#include "Speed/Indep/Src/EAGL4Anim/RawStateChan.cpp"

#include "Speed/Indep/Src/EAGL4Anim/ScratchBuffer.cpp"

#include "Speed/Indep/Src/EAGL4Anim/Skeleton.cpp"

#include "Speed/Indep/Src/EAGL4Anim/StatelessF3.cpp"

#include "Speed/Indep/Src/EAGL4Anim/StatelessQ.cpp"

#include "Speed/Indep/Src/EAGL4Anim/system.cpp"

#include "Speed/Indep/Src/EAGL4Anim/SystemCmn.cpp"
