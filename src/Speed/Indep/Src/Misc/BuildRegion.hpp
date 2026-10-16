//
//
//
//
//
//
#ifndef BUILD_REGION_HPP
#define BUILD_REGION_HPP

// Decl: 45
enum eBuildRegion {
    BUILD_REGION_AMERICA = 0,
    BUILD_REGION_EU = 1,
    BUILD_REGION_KOREA = 2,
    BUILD_REGION_JAPAN = 3,
    BUILD_REGION_TAIWAN = 4,
    BUILD_REGION_UK = 5,
    BUILD_REGION_FRANCE = 6,
    BUILD_REGION_GERMANY = 7,
    BUILD_REGION_ITALY = 8,
    BUILD_REGION_SPAIN = 9,
    BUILD_REGION_ROA = 10,
    BUILD_REGION_CHINA = 11,
    BUILD_REGION_THAILAND = 12,
    BUILD_REGION_SOUTHAMERICA = 13,
    BUILD_REGION_ASIA = 10,
    BUILD_REGION_EUROPE = 1,
};

namespace BuildRegion {

typedef enum {
    BUILD_REGION_AMERICA = 0,
    BUILD_REGION_EU = 1,
    BUILD_REGION_KOREA = 2,
    BUILD_REGION_JAPAN = 3,
    BUILD_REGION_TAIWAN = 4,
    BUILD_REGION_UK = 5,
    BUILD_REGION_FRANCE = 6,
    BUILD_REGION_GERMANY = 7,
    BUILD_REGION_ITALY = 8,
    BUILD_REGION_SPAIN = 9,
    BUILD_REGION_ROA = 10,
    BUILD_REGION_CHINA = 11,
    BUILD_REGION_THAILAND = 12,
    BUILD_REGION_SOUTHAMERICA = 13,
    BUILD_REGION_ASIA = 10,
    BUILD_REGION_EUROPE = 1
} eBuildRegion;

eBuildRegion GetBuildRegion();

const char *GetName();

const char *GetAbbreviation();

bool IsAmerica();
bool IsEurope();
bool IsEuropeEng();
bool IsEuropeFr();
bool IsEuropeGer();
bool IsEuropeSpan();
bool IsEuropeItal();
bool IsEuropeEngItalSpan();
#ifndef EA_BUILD_A124
// Solo en las builds de tienda: los stabs de la A124 no la tienen y PS2_GetOSLanguage de SLUS la llama.
bool IsEuropeEngFrGer();
#endif
bool IsAsia();
bool IsJapan();
bool IsKorea();
bool IsPal();
bool ShowLanguageSelect();
#ifdef EA_PLATFORM_PLAYSTATION2
// PS2 only (the online code and SLUS's MemoryCardImp::GetTitleId use them); there they return non-const strings.
char *GetSlusCode();
#endif
#ifdef EA_PLATFORM_PLAYSTATION2
// La frase de paso de DNAS: SLUS guarda la suya en .rodata aunque el enlazador quito la funcion.
unsigned char *GetDnasPassPhrase();
#endif
#ifdef EA_BUILD_A124
char *GetSkuCode();
#endif
// ShowESRBNotice__11BuildRegionv (A124 0x346868; el DWARF de GC la lista tras GetSkuCode; el 0x312a48 de SLUS es
// ShowLanguageSelect): UIEALogin::UIEALogin la llama.
bool ShowESRBNotice();

inline const char *GetCarBadgingSuffix() {
    if (IsEurope()) {
        return "_EU";
    } else {
        return nullptr;
    }
}

}; // namespace BuildRegion

#endif
