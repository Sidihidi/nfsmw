#include "Speed/Indep/Src/Misc/ISOCodes.hpp"
#include "Speed/Indep/Src/Frontend/Database/FEDatabase.hpp"
#include "Speed/Indep/Src/Misc/BuildRegion.hpp"

#ifdef EA_BUILD_A124
char *ISOCodes::GetLanguageISOCode() {
    return (char *)BuildRegion::GetLanguageISOCode();
}

char *ISOCodes::GetCountryISOCode() {
#if ONLINE_SUPPORT // el pais de la cuenta online (FEDatabase.hpp solo declara GetOnlineSettings con online)
    if (FEDatabase->GetOnlineSettings()->CountryCode[0] != '\0') {
        return FEDatabase->GetOnlineSettings()->CountryCode;
    }
#endif
    return (char *)BuildRegion::GetCountryISOCode();
}
#elif defined(EA_PLATFORM_WIN32) || defined(EA_PLATFORM_XENON) // Xbox 360 igual: LobbyCore::Init (0x82577B04) la expande
// PC (speed.exe): GetLanguageISOCode es un salto directo a BuildRegion::GetLanguageISOCode
// (0x64BAA0 -> 0x64A1E0) y GetCountryISOCode (0x64BAB0, 26 B) devuelve CountryCode (+0x16F
// desde FEDatabase) si no esta vacio y si no salta a BuildRegion::GetCountryISOCode (0x64A2C0).
const char *ISOCodes::GetLanguageISOCode() {
    return BuildRegion::GetLanguageISOCode();
}

const char *ISOCodes::GetCountryISOCode() {
#if ONLINE_SUPPORT // el pais de la cuenta online (FEDatabase.hpp solo declara GetOnlineSettings con online)
    if (FEDatabase->GetOnlineSettings()->CountryCode[0] != '\0') {
        return FEDatabase->GetOnlineSettings()->CountryCode;
    }
#endif
    return BuildRegion::GetCountryISOCode();
}
#endif
