// In the PS2 prototype this unit is the online subsystem: the A124 .mdebug lists these files, in this link
// order, for the functions of the zOnline range. GameCube strips it at link time and the retail PS2 build has
// no network objects. The commented files are not separate files here: the branch that brought the code
// merged each one into a neighbour, in the same place of the link order.
// El prefijo comun de las unidades de juego (bWare, UMath y la STL de UStandard): la .rodata de zOnline empieza
// por sus cadenas ("PSX2", la ruta de bware.hpp, "bad_alloc", las de Coord3AsString/Coord4AsString y "STL") tambien
// en la A124 (0x5AF9B0). Sin el subsistema online la unidad se queda solo con eso: su constructor de estaticos pone
// kFloatScaleUp y kFloatScaleDown de UVectorMath.h. Asi en GameCube (el DWARF de zOnline.cpp, .over 0x803A41B8) y
// en SLUS-21351 (el principio de .over, 0x510380).
#include "Speed/Indep/bWare/Inc/bWare.hpp"
#include "Speed/Indep/Libs/Support/Utility/UMath.h"
#include "Speed/Indep/Libs/Support/Utility/UStandard.h"
#include "Speed/Indep/Libs/Support/Utility/UCOM.h"
#include "Speed/Indep/Libs/Support/Utility/UCollections.h"
#ifdef EA_BUILD_A124
// online/intquantizer.cpp: at the start of SmartBitstream.cpp
// online/bitstream.cpp: in SmartBitstream.cpp, after IntQuantizer
#include "Speed/Indep/Src/Online/SmartBitstream.cpp"
// online/floatquantizer.cpp: at the end of SmartBitstream.cpp
#include "Speed/Indep/Src/Online/OnlineCfg.cpp"
// online/OnlineMemory.cpp: at the end of OnlineCfg.cpp (AllocOnlineMemoryPool...)
#include "Speed/PSX2/Src/Online/PS2Isp.cpp"
#include "Speed/Indep/Src/Online/OnlineMutex.cpp"
#include "Speed/PSX2/Src/Online/PlatformNetworkCore.cpp"
#include "Speed/Indep/Src/Online/NetworkCore.cpp"
#include "Speed/Indep/Src/Online/LobbyLogin.cpp"
#include "Speed/Indep/Src/Online/LobbyCore.cpp"
#include "Speed/Indep/Src/Online/LobbyRanks.cpp"
#include "Speed/Indep/Src/Online/LobbyChat.cpp"
#include "Speed/Indep/Src/Online/LobbyGames.cpp"
#include "Speed/Indep/Src/Online/LobbyGameSessions.cpp"
#include "Speed/Indep/Src/Online/ConnectionCore.cpp"
#include "Speed/Indep/Src/Online/VoiceCore.cpp"
#include "Speed/Indep/Src/Online/BuddyCore.cpp"
// online/RichPresence.cpp: at the end of BuddyCore.cpp
// online/WebOffer.cpp: CWebOffer, at the start of WebOfferUG2.cpp
#include "Speed/Indep/Src/Online/WebOfferUG2.cpp"
// online/LobbyRooms.cpp: at the start of LobbyAccount.cpp
#include "Speed/Indep/Src/Online/LobbyAccount.cpp"
#include "Speed/Indep/Src/Online/PlayerData.cpp"
#include "Speed/Indep/Src/Online/LobbyUsers.cpp"
#include "Speed/Indep/Src/Online/ConditionSimulator.cpp"
#include "Speed/Indep/Src/Online/InGame/Client.cpp"
#include "Speed/Indep/Src/Online/InGame/Online.cpp"
#include "Speed/Indep/Src/Online/InGame/OnlineGame.cpp"
// online/ingame/onlineplayer.cpp: its three functions are defined in InGame/OnlinePlayer.hpp
#include "Speed/Indep/Src/Online/InGame/Server.cpp"
// online/ingame/messagetypes.cpp: DescribeMessageType, at the end of Server.cpp
#include "Speed/Indep/Src/Online/InGame/CSCommon.cpp"
#include "Speed/Indep/Src/Online/InGame/OnlinePlayerMgr.cpp"
#include "Speed/Indep/Src/Online/InGame/OnlineTimer.cpp"
#endif
