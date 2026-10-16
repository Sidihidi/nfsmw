#include "Speed/Indep/Src/Misc/ISOCodes.hpp"
#include "Speed/Indep/Src/Frontend/Database/FEDatabase.hpp"
#include "Speed/Indep/Src/Misc/BuildRegion.hpp"

#ifdef EA_BUILD_A124
char *ISOCodes::GetLanguageISOCode() {
    return (char *)BuildRegion::GetLanguageISOCode();
}

char *ISOCodes::GetCountryISOCode() {
    if (FEDatabase->GetOnlineSettings()->CountryCode[0] != '\0') {
        return FEDatabase->GetOnlineSettings()->CountryCode;
    }
    return (char *)BuildRegion::GetCountryISOCode();
}
#endif
