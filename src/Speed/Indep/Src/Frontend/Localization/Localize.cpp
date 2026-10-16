#include "Speed/Indep/Src/Frontend/Localization/Localize.hpp"
#include "Speed/Indep/Src/Frontend/MemoryCard/MemoryCard.hpp"
#include "Speed/Indep/Src/Frontend/Database/FEDatabase.hpp"
#include "Speed/Indep/Src/EAXSound/EAXSOund.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterface.hpp"
#include "Speed/Indep/Src/Misc/bFile.hpp"
#include "Speed/Indep/bWare/Inc/bPrintf.hpp"
#include "Speed/Indep/Src/FEng/FEWideString.h"
#include "Speed/Indep/bWare/Inc/Strings.hpp"
#include "Speed/Indep/bWare/Inc/bChunk.hpp"
#include "Speed/Indep/bWare/Inc/bWare.hpp"
#include "Speed/Indep/Src/Misc/GameFlow.hpp"
#include "Speed/Indep/Src/Misc/BuildRegion.hpp"
#include "Speed/Indep/Src/Frontend/Localization/WideCharHistogram.hpp"
#include "Speed/GameCube/Src/G.hpp"

#ifdef EA_BUILD_A124
#include <map>
#endif


struct FontNameInfo {
    unsigned int GlobalFonts[8];   // offset 0x0
    unsigned int InGameFonts[8];   // offset 0x20
    unsigned int FrontendFonts[8]; // offset 0x40
    int GlobalFontsLoaded;         // offset 0x60
    int InGameFontsLoaded;         // offset 0x64
    int FrontendFontsLoaded;       // offset 0x68
};
struct LanguageChunkHeader {
    int HistogramTablePos;    // offset 0x0, size 0x4
    int NumStringRecords;     // offset 0x4, size 0x4
    int StringRecordTablePos; // offset 0x8, size 0x4
    int StringTablePos;       // offset 0xC, size 0x4

    void PlatEndianSwap() {
        bPlatEndianSwap(&HistogramTablePos);
        bPlatEndianSwap(&NumStringRecords);
        bPlatEndianSwap(&StringRecordTablePos);
        bPlatEndianSwap(&StringTablePos);
    }
};

struct LanguageInfo {
    eLanguages Language;                   // offset 0x0, size 0x4
    char *Name;                            // offset 0x4, size 0x4
    char *Filename;                        // offset 0x8, size 0x4
    // Ninguna PS2 lo tiene: los stabs de la A124 ponen pFontNameInfo en 0xC, detras de Filename, y SLUS
    // lee pbPrintfLocaleInfo en 0x10.
#ifdef EA_PLATFORM_GAMECUBE
    char *FilenameTextOnly;                // offset 0xC, size 0x4
#endif
    FontNameInfo *pFontNameInfo;           // offset 0x10, size 0x4
    bPrintfLocaleInfo *pbPrintfLocaleInfo; // offset 0x14, size 0x4
};


struct StringRecord {
    uint32 Hash;         // offset 0x0, size 0x4
    uint8 *PackedString; // offset 0x4, size 0x4
};


struct FontSizeInfo {
    unsigned int Hash; // offset 0x0
    int Size;          // offset 0x4
};

int LanguageMemoryPoolNumber = (int)0xFFFFFFFF;
void *pLanguageMemoryPoolMemory = nullptr;
int LanguageMemoryPoolSize = 0;

FontSizeInfo FontSizeInfoTable[9] = {
    {0x545570C6U, 0x0},
    {0x0920075CU, 0x10000},
    {0x5B9D84DBU, 0x10000},
    {0x9583AA1AU, 0x40000},
    {0xA87927BEU, 0x10000},
    {0xAB6215C1U, 0x10000},
    {0xBBBA71C2U, 0x4000},
    {0xDCA5485AU, 0x40000},
    {0x833A8678U, 0x80000},
};

FontNameInfo EuropeanFontNameInfo = {
    {0x545570C6U, 0xA87927BEU, 0x5B9D84DBU, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U},
    {0x01424892U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U},
    {0xAB6215C1U, 0x0920075CU, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U},
    0,
    0,
    0,
};

bPrintfLocaleInfo EnglishLocaleInfo = {'.', ',', 3};
bPrintfLocaleInfo FrenchLocaleInfo = {',', ' ', 3};
bPrintfLocaleInfo GermanLocaleInfo = {',', '.', 3};
bPrintfLocaleInfo ItalianLocaleInfo = {',', '.', 3};
bPrintfLocaleInfo SpanishLocaleInfo = {',', ' ', 0};
bPrintfLocaleInfo DutchLocaleInfo = {',', '.', 3};
bPrintfLocaleInfo SwedishLocaleInfo = {',', ' ', 3};
bPrintfLocaleInfo PolishLocaleInfo = {',', ' ', 3};
bPrintfLocaleInfo FinnishLocaleInfo = {',', ' ', 3};
bPrintfLocaleInfo DanishLocaleInfo = {',', '.', 3};

// Ninguna PS2 tiene FilenameTextOnly: ni la A124 ni SLUS traen ningun *_TEXTONLY.BIN. PC y Xbox 360 tampoco (LanguageInfo de 0x14; ni un _TEXTONLY.BIN en sus ejecutables).
LanguageInfo LanguageInfoTable[] = {
    {eLANGUAGE_ENGLISH, "ENGLISH", "LANGUAGES\\ENGLISH.BIN",
#ifdef EA_PLATFORM_GAMECUBE
     "LANGUAGES\\ENGLISH_TEXTONLY.BIN",
#endif
     &EuropeanFontNameInfo, &EnglishLocaleInfo},
    {eLANGUAGE_FRENCH, "FRENCH", "LANGUAGES\\FRENCH.BIN",
#ifdef EA_PLATFORM_GAMECUBE
     "LANGUAGES\\FRENCH_TEXTONLY.BIN",
#endif
     &EuropeanFontNameInfo, &FrenchLocaleInfo},
    {eLANGUAGE_GERMAN, "GERMAN", "LANGUAGES\\GERMAN.BIN",
#ifdef EA_PLATFORM_GAMECUBE
     "LANGUAGES\\GERMAN_TEXTONLY.BIN",
#endif
     &EuropeanFontNameInfo, &GermanLocaleInfo},
    {eLANGUAGE_ITALIAN, "ITALIAN", "LANGUAGES\\ITALIAN.BIN",
#ifdef EA_PLATFORM_GAMECUBE
     "LANGUAGES\\ITALIAN_TEXTONLY.BIN",
#endif
     &EuropeanFontNameInfo, &ItalianLocaleInfo},
    {eLANGUAGE_SPANISH, "SPANISH", "LANGUAGES\\SPANISH.BIN",
#ifdef EA_PLATFORM_GAMECUBE
     "LANGUAGES\\SPANISH_TEXTONLY.BIN",
#endif
     &EuropeanFontNameInfo, &SpanishLocaleInfo},
    {eLANGUAGE_DUTCH, "DUTCH", "LANGUAGES\\DUTCH.BIN",
#ifdef EA_PLATFORM_GAMECUBE
     "LANGUAGES\\DUTCH_TEXTONLY.BIN",
#endif
     &EuropeanFontNameInfo, &DutchLocaleInfo},
    {eLANGUAGE_SWEDISH, "SWEDISH", "LANGUAGES\\SWEDISH.BIN",
#ifdef EA_PLATFORM_GAMECUBE
     "LANGUAGES\\SWEDISH_TEXTONLY.BIN",
#endif
     &EuropeanFontNameInfo, &SwedishLocaleInfo},
    {eLANGUAGE_POLISH, "POLISH", "LANGUAGES\\POLISH.BIN",
#ifdef EA_PLATFORM_GAMECUBE
     "LANGUAGES\\POLISH_TEXTONLY.BIN",
#endif
     &EuropeanFontNameInfo, &PolishLocaleInfo},
    {eLANGUAGE_FINNISH, "FINNISH", "LANGUAGES\\FINNISH.BIN",
#ifdef EA_PLATFORM_GAMECUBE
     "LANGUAGES\\FINNISH_TEXTONLY.BIN",
#endif
     &EuropeanFontNameInfo, &FinnishLocaleInfo},
    {eLANGUAGE_DANISH, "DANISH", "LANGUAGES\\DANISH.BIN",
#ifdef EA_PLATFORM_GAMECUBE
     "LANGUAGES\\DANISH_TEXTONLY.BIN",
#endif
     &EuropeanFontNameInfo, &DanishLocaleInfo},
#if defined(EA_BUILD_A124) || defined(EA_PLATFORM_XENON) // 360: la tabla de NFS.exe (0x8294F408) tiene las 12
    {eLANGUAGE_LARGEST, "LARGEST", "LANGUAGES\\LARGEST.BIN", &EuropeanFontNameInfo, &EnglishLocaleInfo},
    {eLANGUAGE_LABELS, "LABELS", "LANGUAGES\\LABELS.BIN", &EuropeanFontNameInfo, &EnglishLocaleInfo},
#endif
};

#ifdef EA_PLATFORM_PLAYSTATION2
int eLanguageToDemoLanguage[] = {0, 1, 2, 4, 3, 5, 11, 8, 12, -1, 7, -1, -1, 10};
#endif

struct VMFile;
static eLanguages CurrentLanguage = eLANGUAGE_NONE;
ResourceFile *pLanguageResourceFile = nullptr;
#ifdef EA_PLATFORM_GAMECUBE
VMFile *pLanguageResourceFile_VM = nullptr;
#endif
static unsigned int NumStringRecords = 0;
static unsigned char *PackedStringTable = nullptr;
static StringRecord *RecordTable = nullptr;
#ifdef EA_BUILD_A124
eLanguages ExcludeCycleLanguages[3] = {eLANGUAGE_KOREAN, eLANGUAGE_CHINESE, eLANGUAGE_JAPANESE};
bool gToggleLocTracking = false;
std::map<unsigned int, bool> lookupMap;
#endif

extern cFrontendDatabase *FEDatabase;

LanguageInfo *GetLanguageInfo(eLanguages language) {
    for (int i = 0; i < static_cast<int>(sizeof(LanguageInfoTable) / sizeof(LanguageInfoTable[0])); i++) {
        if (LanguageInfoTable[i].Language == language) {
            return &LanguageInfoTable[i];
        }
    }
    return nullptr;
}

char *GetLanguageName(eLanguages language) {
    LanguageInfo *info = GetLanguageInfo(language);
    if (info) {
        return info->Name;
    }
    return "UNKNOWN";
}

#ifdef EA_PLATFORM_PLAYSTATION2
int GetDemoLanguageID(eLanguages language) {
    if (language == eLANGUAGE_NONE) {
        return 0;
    }
    return eLanguageToDemoLanguage[language];
}
#endif

#ifdef EA_BUILD_A124
void LanguageChangerCallback(int dir) {
    if (dir <= 0) {
        dir = -1;
    } else {
        dir = 1;
    }
    eLanguages new_language = CurrentLanguage;
    bool exclude_this_language;
    do {
        new_language = static_cast<eLanguages>(new_language + dir);
        if (new_language >= eLANGUAGE_MAX) {
            new_language = eLANGUAGE_FIRST;
        } else if (new_language <= eLANGUAGE_FIRST) {
            new_language = eLANGUAGE_MAX;
        }
        if (new_language == CurrentLanguage) {
            return;
        }
        LanguageInfo *info = GetLanguageInfo(new_language);
        exclude_this_language = false;
        if (!info || !bFileExists(info->Filename)) {
            exclude_this_language = true;
        }
        for (int i = 0; i < 3; i++) {
            if (new_language == ExcludeCycleLanguages[i]) {
                exclude_this_language = true;
            }
        }
    } while (exclude_this_language);
    SetCurrentLanguage(new_language);
}

void DumpMap() {
    std::map<unsigned int, bool>::iterator it = lookupMap.begin();
    while (it != lookupMap.end()) {
        ++it;
    }
}
#endif

const char *GetLocalizedPercentSign() {
    const char *szPercentUnit = "%";
    eLanguages currLang = GetCurrentLanguage();
    if (currLang == eLANGUAGE_DANISH || currLang == eLANGUAGE_FINNISH || currLang == eLANGUAGE_FRENCH || currLang == eLANGUAGE_GERMAN ||
        currLang == eLANGUAGE_SWEDISH) {
        szPercentUnit = " %";
    }
    return szPercentUnit;
}

void InitLocalization() {
    LanguageInfo *info;
    if (BuildRegion::IsKorea()) {
        info = GetLanguageInfo(eLANGUAGE_KOREAN);
    } else {
        info = GetLanguageInfo(eLANGUAGE_ENGLISH);
    }
    int total_font_size = 0;
    int n = 0;
    while (info->pFontNameInfo->GlobalFonts[n] != 0) {
        int font_size = -1;
        for (int i = 0; i < 9; i++) {
            if (FontSizeInfoTable[i].Hash == info->pFontNameInfo->GlobalFonts[n]) {
                font_size = FontSizeInfoTable[i].Size;
                break;
            }
        }
        if (font_size >= 0) {
            total_font_size = total_font_size + font_size;
        }
        n++;
    }
    LanguageMemoryPoolSize = (total_font_size + 0x4080) & 0xFFFFFFC0;
    if (LanguageMemoryPoolNumber != 0) {
        LanguageMemoryPoolNumber = bGetFreeMemoryPoolNum();
        pLanguageMemoryPoolMemory = bMalloc(LanguageMemoryPoolSize, "LanguageMemoryPool", 0, 0);
        bInitMemoryPool(LanguageMemoryPoolNumber, pLanguageMemoryPoolMemory, LanguageMemoryPoolSize, "LanguageMemoryPool");
    }
    eLoadStreamingTexturePack("LANGUAGES\\LANGUAGETEXTURES.BIN");
    eWaitForStreamingTexturePackLoading("LANGUAGES\\LANGUAGETEXTURES.BIN");
}

void LanguageHasChanged(eLanguages new_language) {
    if (FEDatabase) {
        eLanguages lang = GetCurrentLanguage();
        if (lang != eLANGUAGE_ENGLISH) {
            FEDatabase->GetGameplaySettings()->SpeedoUnits = 1;
        } else {
            FEDatabase->GetGameplaySettings()->SpeedoUnits = 0;
        }
    }
    cFEng::Get()->MakeLoadedPackagesDirty();
}

void LoadLanguageResources(bool load_global, bool load_frontend, bool load_ingame, bool blocking) {
    LanguageInfo *info = GetLanguageInfo(CurrentLanguage);
    if (!load_global) {
        UnloadResourceFile(pLanguageResourceFile);
        pLanguageResourceFile = nullptr;
        // En PS2 no hay fichero de solo texto en memoria virtual: ni se descarga ni se carga.
#ifdef EA_PLATFORM_GAMECUBE
        UnloadFileFromVirtualMemory(pLanguageResourceFile_VM);
        pLanguageResourceFile_VM = nullptr;
#endif
        if (info->pFontNameInfo->GlobalFontsLoaded) {
            eUnloadStreamingTexture(info->pFontNameInfo->GlobalFonts, 8);
            info->pFontNameInfo->GlobalFontsLoaded = 0;
        }
    }
    if (!load_frontend && info->pFontNameInfo->FrontendFontsLoaded) {
        eUnloadStreamingTexture(info->pFontNameInfo->FrontendFonts, 8);
        info->pFontNameInfo->FrontendFontsLoaded = 0;
    }
    if (!load_ingame && info->pFontNameInfo->InGameFontsLoaded) {
        eUnloadStreamingTexture(info->pFontNameInfo->InGameFonts, 8);
        info->pFontNameInfo->InGameFontsLoaded = 0;
    }
    if (load_global) {
        if (!pLanguageResourceFile) {
#ifdef EA_PLATFORM_GAMECUBE
            pLanguageResourceFile_VM = LoadFileIntoVirtualMemory(info->FilenameTextOnly, false, false);
#endif
            {
            } // empty anonymous block (DWARF)
            int pool = 0;
            pLanguageResourceFile = CreateResourceFile(info->Filename, static_cast<ResourceFileType>(7), 0, 0, 0);
            int file_size = bFileSize(info->Filename);
            if (bLargestMalloc(LanguageMemoryPoolNumber) >= file_size + 0x80) {
                pool = LanguageMemoryPoolNumber;
            }
            pLanguageResourceFile->SetAllocationParams((pool & 0xF) | 0x2000, info->Filename);
#ifdef EA_BUILD_A124
            pLanguageResourceFile->BeginLoading(nullptr, 0);
#else
            pLanguageResourceFile->BeginLoading();
#endif
            if (blocking) {
                WaitForResourceLoadingComplete();
            }
        }
        if (load_global && !info->pFontNameInfo->GlobalFontsLoaded) {
#ifdef EA_BUILD_A124
            eLoadStreamingTexture(info->pFontNameInfo->GlobalFonts, 8, nullptr, 0, LanguageMemoryPoolNumber);
#else
            eLoadStreamingTexture(info->pFontNameInfo->GlobalFonts, 8, static_cast<void (*)(void *)>(nullptr), static_cast<void *>(nullptr),
                                  LanguageMemoryPoolNumber);
#endif
            info->pFontNameInfo->GlobalFontsLoaded = 1;
        }
    }
    if (load_frontend && !info->pFontNameInfo->FrontendFontsLoaded) {
#ifdef EA_BUILD_A124
        eLoadStreamingTexture(info->pFontNameInfo->FrontendFonts, 8, nullptr, 0, LanguageMemoryPoolNumber);
#else
        eLoadStreamingTexture(info->pFontNameInfo->FrontendFonts, 8, static_cast<void (*)(void *)>(nullptr), static_cast<void *>(nullptr),
                              LanguageMemoryPoolNumber);
#endif
        info->pFontNameInfo->FrontendFontsLoaded = 1;
    }
    if (load_ingame && !info->pFontNameInfo->InGameFontsLoaded) {
#ifdef EA_BUILD_A124
        eLoadStreamingTexture(info->pFontNameInfo->InGameFonts, 8, nullptr, 0, LanguageMemoryPoolNumber);
#else
        eLoadStreamingTexture(info->pFontNameInfo->InGameFonts, 8, static_cast<void (*)(void *)>(nullptr), static_cast<void *>(nullptr),
                              LanguageMemoryPoolNumber);
#endif
        info->pFontNameInfo->InGameFontsLoaded = 1;
    }
    if (blocking) {
        eWaitForStreamingTexturePackLoading("LANGUAGES\\LANGUAGETEXTURES.BIN");
    }
}

void SetCurrentLanguage(eLanguages new_language) {
    if (new_language != CurrentLanguage) {
        if (CurrentLanguage != static_cast<eLanguages>(-1)) {
            LoadLanguageResources(false, false, false, false);
        }
        CurrentLanguage = new_language;
        if (new_language != static_cast<eLanguages>(-1)) {
            LoadLanguageResources(true, TheGameFlowManager.IsInFrontend(), TheGameFlowManager.IsInGame(), true);
            LanguageInfo *langInfo = GetLanguageInfo(CurrentLanguage);
            if (langInfo) {
                bPrintfLocaleInfo locInfo = *langInfo->pbPrintfLocaleInfo;
                bPrintfSetLocaleInfo(locInfo.decimal_char, locInfo.group_char, locInfo.group_len);
            }
            LanguageHasChanged(CurrentLanguage);
        }
        if (FEDatabase && !FEDatabase->GetUserProfile(0)->IsProfileNamed()) {
            FEDatabase->GetUserProfile(0)->SetProfileName(nullptr, true);
        }
        MemoryCard::LoadLocale(CurrentLanguage);
    }
}

void LoadCurrentLanguage() {
#ifdef EA_PLATFORM_GAMECUBE
    eLanguages ngc_language = GC_GetOSLanguage();
    SetCurrentLanguage(ngc_language);
#elif defined(EA_PLATFORM_PLAYSTATION2)
    if (!BuildRegion::IsAmerica()) {
        SetCurrentLanguage(PS2_GetOSLanguage());
    } else {
        SetCurrentLanguage(eLANGUAGE_ENGLISH);
    }
#endif
}

eLanguages GetCurrentLanguage() {
    return CurrentLanguage;
};

void WideToCharString(char *dest, uint32 destlen, const int16 *src) {
    if (dest == nullptr) {
        return;
    }
    if (src == nullptr) {
        return;
    }

    bool error;
    uint32 bytes = 0;

    while (*src != 0 && bytes < destlen - 1) {
        if (*reinterpret_cast<const uint16 *>(src) < 0x100) {
            bytes++;
            *dest = *src;
            src++;
            dest++;
        } else {
            src++;
        }
    }

    *dest = 0;
}

void PackedStringToWideString(uint16 *wide_string, int wide_string_buffer_size, const char *packed_string) {
    if (!pWideCharHistogram) {
        bStrCpy(wide_string, packed_string);
    } else {
        pWideCharHistogram->UnpackString(wide_string, wide_string_buffer_size, packed_string);
    }
}

void WideStringToPackedString(char *packed_string, int packed_string_buffer_size, const unsigned short *wide_string) {
    pWideCharHistogram->PackString(packed_string, packed_string_buffer_size, wide_string);
}

// TODO: why is this here????
FEWideString &FEWideString::operator=(const char *pcString) {
    short wide_string[1024];
    PackedStringToWideString(reinterpret_cast<unsigned short *>(wide_string), 0x800, pcString);
    *this = wide_string;
    return *this;
}

static const uint8 *SearchForString(uint32 string_label) {
    if (!RecordTable) {
        return nullptr;
    }
#ifdef EA_BUILD_A124
    if (gToggleLocTracking) {
        lookupMap[string_label] = true;
    }
#endif
    unsigned int top;
    unsigned int bot;
    int step;
    bot = 0;
    top = NumStringRecords - 1;
    while (true) {
        unsigned int mid = (bot + top) >> 1;
        if (RecordTable[mid].Hash == string_label) {
            return RecordTable[mid].PackedString;
        }
        if (top - bot <= 2) {
            if (RecordTable[bot].Hash == string_label) {
                return RecordTable[bot].PackedString;
            }
            if (RecordTable[top].Hash == string_label) {
                return RecordTable[top].PackedString;
            }
        }
        if (mid == bot) {
            break;
        }
        if (RecordTable[mid].Hash > string_label) {
            top = mid;
        }
        if (RecordTable[mid].Hash < string_label) {
            bot = mid;
        }
    }
#ifdef EA_BUILD_A124
    if (gToggleLocTracking) {
        lookupMap[string_label] = false;
    }
#endif
    return nullptr;
}

bool DoesStringExist(uint32 label) {
    if (!SearchForString(label)) {
        return false;
    }
    return true;
}

char *GetLocalizedString(uint32 id) {
    const uint8 *str = SearchForString(id);
    if (!str) {
        str = SearchForString(0x9bb9ccc3);
    }
    return reinterpret_cast<char *>(const_cast<uint8 *>(str));
}

void GetLocalizedString(char *buffer, uint32 bufsize, uint32 string_label) {
    char *str = const_cast<char *>(GetLocalizedString(string_label));
    bStrNCpy(buffer, str, static_cast<int>(bufsize));
}

char *GetTranslatedString(int label_hash) {
    return const_cast<char *>(GetLocalizedString(static_cast<unsigned int>(label_hash)));
}

const bool GetLocalizedWideString(int16 *wide_string, int wide_string_buffer_size, uint32 string_label) {
    const uint8 *str = SearchForString(string_label);
    if (str) {
        PackedStringToWideString(reinterpret_cast<uint16 *>(wide_string), wide_string_buffer_size, reinterpret_cast<const char *>(str));
        return true;
    }
    return false;
}

int LoaderLanguage(bChunk *chunk) {
    if (chunk->GetID() == 0x39000) {
        LanguageChunkHeader *header = reinterpret_cast<LanguageChunkHeader *>(chunk->GetData());
        header->PlatEndianSwap();
        NumStringRecords = header->NumStringRecords;
        pWideCharHistogram = reinterpret_cast<WideCharHistogram *>(reinterpret_cast<char *>(header) + header->HistogramTablePos);
        RecordTable = reinterpret_cast<StringRecord *>(reinterpret_cast<char *>(header) + header->StringRecordTablePos);
        PackedStringTable = reinterpret_cast<unsigned char *>(header) + header->StringTablePos;
        pWideCharHistogram->PlatEndianSwap();
        for (unsigned int i = 0; i < NumStringRecords; i++) {
            bPlatEndianSwap(reinterpret_cast<unsigned int *>(&RecordTable[i]));
            unsigned int offset = reinterpret_cast<unsigned int *>(&RecordTable[i])[1];
            bPlatEndianSwap(&offset);
            reinterpret_cast<unsigned int *>(&RecordTable[i])[1] = reinterpret_cast<unsigned int>(PackedStringTable) + offset;
        }
        return 1;
    }
    return 0;
}

int UnloaderLanguage(bChunk *chunk) {
    if (chunk->GetID() == 0x39000) {
        NumStringRecords = 0;
        PackedStringTable = nullptr;
        RecordTable = nullptr;
        pWideCharHistogram = nullptr;
        return 1;
    }
    return 0;
}

// La de la A124. La de GameCube traia ademas un caso de cuatro bytes (su DWARF declara data3 y un wdata
// unsigned int) que la A124 no tiene: sus lineas entre el caso de tres bytes y el error no dejan codigo.
bool ConvertUTF8ToUCS2(unsigned short *ucs2_data, int ucs2_data_len, const unsigned char *utf8_data) {
    int ucs2_data_pos = 0;
    int utf8_data_pos = 0;
    bool error = false;
    int utf8_data_len = bStrLen(reinterpret_cast<const char *>(utf8_data));

    while (utf8_data_pos < utf8_data_len && ucs2_data_pos < ucs2_data_len) {
        unsigned char data0 = utf8_data[utf8_data_pos++];
        if ((data0 & 0x80) == 0) {
            ucs2_data[ucs2_data_pos++] = data0;
        } else if ((data0 & 0xE0) == 0xC0) {
            unsigned char data1 = utf8_data[utf8_data_pos++];
            if ((data1 & 0xC0) != 0x80) {
                error = true;
                utf8_data_pos--;
                ucs2_data[ucs2_data_pos++] = data0;
            } else {
                unsigned short wdata = ((data0 & 0x1F) << 6) | (data1 & 0x3F);
                ucs2_data[ucs2_data_pos++] = wdata;
            }
        } else if ((data0 & 0xF0) == 0xE0) {
            unsigned char data1 = utf8_data[utf8_data_pos++];
            unsigned char data2 = utf8_data[utf8_data_pos++];
            if ((data1 & 0xC0) != 0x80 || (data2 & 0xC0) != 0x80) {
                error = true;
                utf8_data_pos -= 2;
                ucs2_data[ucs2_data_pos++] = data0;
            } else {
                unsigned short wdata = (data2 & 0x3F) | ((data1 & 0x3F) << 6) | ((data0 & 0x0F) << 12);
                if (wdata > 0xFF7F) {
                    wdata &= 0xFF;
                }
                ucs2_data[ucs2_data_pos++] = wdata;
            }
        } else {
            error = true;
            ucs2_data[ucs2_data_pos++] = data0;
        }
    }
    ucs2_data[ucs2_data_pos] = 0;
    return !error;
}
