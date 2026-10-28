#include "BuildRegion.hpp"
#include "Speed/Indep/Src/Frontend/Database/FEDatabase.hpp"
#include "Speed/Indep/Src/Frontend/Localization/Localize.hpp"

namespace BuildRegion {

#if defined(EA_REGION_FRANCE) || defined(EA_REGION_GERMANY)
#define EA_REGION_EUROPE
#endif

// STRIPPED en GameCube. En PC la region se decide al arrancar: GetBuildRegion esta en Platform_PC.cpp (speed.exe
// 0x6CBB40) y las funciones de abajo la consultan (las ramas EA_PLATFORM_WIN32, 0x649F10-0x64A3E3).
eBuildRegion GetBuildRegion();

// STRIPPED
const char *GetName() {
#ifdef EA_PLATFORM_WIN32
    eBuildRegion region = GetBuildRegion();
    if (region == BUILD_REGION_AMERICA) {
        return "America";
    }
    if (region == BUILD_REGION_EU) {
        return "Europe";
    }
    if (region == BUILD_REGION_UK) {
        return "EuropeEng";
    }
    if (region == BUILD_REGION_FRANCE) {
        return "EuropeFr";
    }
    if (region == BUILD_REGION_GERMANY) {
        return "EuropeGer";
    }
    if (region == BUILD_REGION_ITALY) {
        return "EuropeItal";
    }
    if (region == BUILD_REGION_SPAIN) {
        return "EuropeSpan";
    }
    if (region == BUILD_REGION_KOREA) {
        return "Korea";
    }
    if (region == BUILD_REGION_JAPAN) {
        return "Japan";
    }
    if (region == BUILD_REGION_THAILAND) {
        return "Thailand";
    }
    if (region == BUILD_REGION_CHINA) {
        return "China";
    }
    if (region == BUILD_REGION_TAIWAN) {
        return "ChinaTraditional";
    }
    return "Unknown BUILD_REGION_xxxx";
#endif
}

#ifdef EA_PLATFORM_WIN32
// Las seis regiones europeas (PC: IsEurope, IsPal y las tres ISO las comparan en este orden).
#define BUILD_REGION_IS_EUROPE(region)                                                                                 \
    ((region) == BUILD_REGION_EU || (region) == BUILD_REGION_UK || (region) == BUILD_REGION_FRANCE ||                  \
     (region) == BUILD_REGION_GERMANY || (region) == BUILD_REGION_ITALY || (region) == BUILD_REGION_SPAIN)
#endif

const char *GetAbbreviation() {
#ifdef EA_PLATFORM_WIN32
    // Cadena ternaria: es la forma que deja el bloque "EU" tras el primer cmp en speed.exe
    // (0x649FA0), con el resto de regiones europeas saltando atras a el.
    eBuildRegion region = GetBuildRegion();
    return region == BUILD_REGION_AMERICA ? "NA"
         : region == BUILD_REGION_EU ? "EU"
         : region == BUILD_REGION_UK ? "EU"
         : region == BUILD_REGION_FRANCE ? "EU"
         : region == BUILD_REGION_GERMANY ? "EU"
         : region == BUILD_REGION_ITALY ? "EU"
         : region == BUILD_REGION_SPAIN ? "EU"
         : region == BUILD_REGION_KOREA ? "KO"
         : region == BUILD_REGION_JAPAN ? "JP"
         : region == BUILD_REGION_THAILAND ? "TH"
         : region == BUILD_REGION_CHINA ? "CH"
         : region == BUILD_REGION_TAIWAN ? "CH"
         : "Unknown BUILD_REGION_xxxx";
#elif defined(EA_REGION_EUROPE)
    return "EU";
#else
    return "NA";
#endif
}

bool IsAmerica() {
#ifdef EA_PLATFORM_WIN32
    if (GetBuildRegion() == BUILD_REGION_AMERICA) {
        return true;
    }
    return false;
#elif defined(EA_REGION_AMERICA)
    return true;
#else
    return false;
#endif
}

bool IsEurope() {
#ifdef EA_PLATFORM_WIN32
    eBuildRegion region = GetBuildRegion();
    if (BUILD_REGION_IS_EUROPE(region)) {
        return true;
    }
    return false;
#elif defined(EA_REGION_EUROPE)
    return true;
#else
    return false;
#endif
}

// Ninguna de nuestras builds es la europea en ingles ni la asiatica: esta, IsEuropeEngItalSpan, IsEuropeEngFrGer e
// IsAsia devuelven false en las PS2 que las conservan.
bool IsEuropeEng() {
#ifdef EA_PLATFORM_WIN32
    return GetBuildRegion() == BUILD_REGION_UK;
#else
    return false;
#endif
}

bool IsEuropeFr() {
#ifdef EA_PLATFORM_WIN32
    return GetBuildRegion() == BUILD_REGION_FRANCE;
#elif defined(EA_REGION_FRANCE)
    return true;
#else
    return false;
#endif
}

bool IsEuropeGer() {
#ifdef EA_PLATFORM_WIN32
    return GetBuildRegion() == BUILD_REGION_GERMANY;
#elif defined(EA_REGION_GERMANY)
    return true;
#else
    return false;
#endif
}

#ifdef EA_PLATFORM_WIN32 // STRIPPED en GameCube; en PC 0x64A080 y 0x64A090
bool IsEuropeSpan() {
    return GetBuildRegion() == BUILD_REGION_SPAIN;
}

bool IsEuropeItal() {
    return GetBuildRegion() == BUILD_REGION_ITALY;
}
#endif

bool IsEuropeEngItalSpan() {
    return false;
}

#ifndef EA_BUILD_A124
bool IsEuropeEngFrGer() {
    return false;
}
#endif

bool IsAsia() {
#ifdef EA_PLATFORM_WIN32
    eBuildRegion region = GetBuildRegion();
    if (region == BUILD_REGION_KOREA || region == BUILD_REGION_JAPAN || region == BUILD_REGION_TAIWAN ||
        region == BUILD_REGION_CHINA || region == BUILD_REGION_THAILAND) {
        return true;
    }
    return false;
#else
    return false;
#endif
}

bool IsJapan() {
#ifdef EA_PLATFORM_WIN32
    return GetBuildRegion() == BUILD_REGION_JAPAN;
#elif defined(EA_REGION_JAPAN)
    return true;
#else
    return false;
#endif
}

bool IsKorea() {
#ifdef EA_PLATFORM_WIN32
    return GetBuildRegion() == BUILD_REGION_KOREA;
#elif defined(EA_REGION_KOREA)
    return true;
#else
    return false;
#endif
}

bool IsPal() {
    return IsEurope();
}

bool ShowLanguageSelect() {
#ifdef EA_REGION_EUROPE
    return true;
#else
    return false;
#endif
}

// SLUS_21351 es la edicion de coleccionista (Black Edition) y SLUS_21267 la normal; GameCube y PC guardan las dos
// cadenas aunque no usen la funcion.
// La rama europea es la de la A124 (20-sep, la unica PAL que se mide): solo "SLES_53558". Las PAL de venta la
// cambian (decision del usuario, ronda 79: queda como comentario, sin codigo): la GameCube inglesa (GOWP69) y la
// Beta 1.1 de PS2 (12-oct) guardan "SLES_53857" y "SLES_53558", en ese orden (la de coleccionista se anadio entre
// las dos fechas), y las GameCube francesa y alemana (GOWF69, GOWD69) solo "SCED_00000"; o sea, fuera de la A124,
//     if (GetIsCollectorsEdition()) return "SLES_53857";   antes de   return "SLES_53558";
// y "SCED_00000" con EA_REGION_FRANCE o EA_REGION_GERMANY (docs/analisis/2026-10-21_protosmw.md, secciones 5 y 6).
#ifdef EA_PLATFORM_PLAYSTATION2
char *GetSlusCode() {
#else
const char *GetSlusCode() {
#endif
#ifdef EA_REGION_EUROPE
    return "SLES_53558";
#else
    if (GetIsCollectorsEdition()) {
        return "SLUS_21351";
    }
    return "SLUS_21267";
#endif
}

// Ocho bytes sin nombre y sin NUL en .rodata, entre la cadena de GetSlusCode y las de GetPs2ElfFilename (A124
// 0x56c6d8; SLUS guarda la suya en 0x4ed8f0 aunque quito la funcion): un literal compuesto, no una cadena.
#ifdef EA_PLATFORM_PLAYSTATION2
unsigned char *GetDnasPassPhrase() {
#ifdef EA_REGION_EUROPE
    return (unsigned char *)(const unsigned char[8]){0, 0, 0, 0, 0, 0, 0, 0};
#else
    return (unsigned char *)(const unsigned char[8]){0, 0, 0, 0, 0, 0, 0, 0};
#endif
}
#endif

// STRIPPED en GameCube; en PC 0x64A170 ("NfsMWRelease.exe")
void GetExeFilename(char *filename) {
#ifdef EA_PLATFORM_WIN32
    const char *title = "Nfs";
    const char *game = "MW";
    const char *mode = "Release";
    const char *optim = "";
    const char *platform = ".exe";
    bMemSet(filename, 0, 0x20);
    bStrCat(filename, filename, title);
    bStrCat(filename, filename, game);
    bStrCat(filename, filename, mode);
    bStrCat(filename, filename, optim);
    bStrCat(filename, filename, platform);
#endif
}

// Europa sale del idioma elegido; America guarda "EN" y "US" (SLUS y GameCube, en .rodata).
const char *GetLanguageISOCode() {
#ifdef EA_PLATFORM_WIN32
    eBuildRegion region = GetBuildRegion();
    if (region == BUILD_REGION_AMERICA) {
        return "EN";
    }
    if (!BUILD_REGION_IS_EUROPE(region)) {
        if (region == BUILD_REGION_KOREA) {
            return "ko";
        }
        if (region == BUILD_REGION_JAPAN) {
            return "ja";
        }
        if (region == BUILD_REGION_THAILAND) {
            return "en";
        }
        if (region == BUILD_REGION_CHINA) {
            return "en";
        }
        return "en";
    }
#endif
#if defined(EA_REGION_EUROPE) || defined(EA_PLATFORM_WIN32)
    switch (GetCurrentLanguage()) {
        case eLANGUAGE_ENGLISH:
            return "en";
        case eLANGUAGE_FRENCH:
            return "fr";
        case eLANGUAGE_GERMAN:
            return "de";
        case eLANGUAGE_ITALIAN:
            return "it";
        case eLANGUAGE_SPANISH:
            return "es";
        case eLANGUAGE_DUTCH:
            return "nl";
        case eLANGUAGE_SWEDISH:
            return "sv";
        case eLANGUAGE_DANISH:
            return "da";
        case eLANGUAGE_POLISH:
            return "pl";
        case eLANGUAGE_FINNISH:
            return "fi";
        default:
            return nullptr;
    }
#else
    return "EN";
#endif
}

const char *GetCountryISOCode() {
#ifdef EA_PLATFORM_WIN32
    eBuildRegion region = GetBuildRegion();
    if (region == BUILD_REGION_AMERICA) {
        return "US";
    }
    if (!BUILD_REGION_IS_EUROPE(region)) {
        if (region == BUILD_REGION_KOREA) {
            return "KP";
        }
        if (region == BUILD_REGION_JAPAN) {
            return "JP";
        }
        if (region == BUILD_REGION_THAILAND) {
            return "??";
        }
        if (region == BUILD_REGION_CHINA) {
            return "??";
        }
        return "??";
    }
#endif
#if defined(EA_REGION_EUROPE) || defined(EA_PLATFORM_WIN32)
    switch (GetCurrentLanguage()) {
        case eLANGUAGE_ENGLISH:
            return "GB";
        case eLANGUAGE_FRENCH:
            return "FR";
        case eLANGUAGE_GERMAN:
            return "DE";
        case eLANGUAGE_ITALIAN:
            return "IT";
        case eLANGUAGE_SPANISH:
            return "ES";
        case eLANGUAGE_DUTCH:
            return "NL";
        case eLANGUAGE_SWEDISH:
            return "SE";
        case eLANGUAGE_DANISH:
            return "DK";
        case eLANGUAGE_POLISH:
            return "PL";
        case eLANGUAGE_FINNISH:
            return "FI";
        default:
            return nullptr;
    }
#else
    return "US";
#endif
}

#ifdef EA_BUILD_A124
char *GetSkuCode() {
#else
const char *GetSkuCode() {
#endif
#ifdef EA_PLATFORM_WIN32
    if (GetBuildRegion() == BUILD_REGION_AMERICA) {
        return "14705";
    }
    return "SKU_UNKNOWN";
#elif defined(EA_REGION_AMERICA)
    return "14705";
#else
    return "SKU_UNKNOWN";
#endif
}

bool ShowESRBNotice() {
#ifdef EA_PLATFORM_WIN32
    return GetBuildRegion() == BUILD_REGION_AMERICA;
#else
    return false;
#endif
}

// STRIPPED en GameCube; en PC 0x64A3D0
const char *GetAdvertString() {
#ifdef EA_PLATFORM_WIN32
    if (GetBuildRegion() == BUILD_REGION_AMERICA) {
        return "NFSMWNA";
    }
    return "NFSMW";
#endif
}

}; // namespace BuildRegion
