#include "FEngInterfaceFEImages.hpp"

#include "FEngInterfaceFEObjects.hpp"
#if defined(EA_PLATFORM_PLAYSTATION2) && !defined(EA_BUILD_A124)
#include "Speed/Indep/Src/Frontend/Localization/Localize.hpp"
#include "Speed/Indep/Src/Generated/Hash.hpp"
#endif

FEImage *FEngFindImage(const char *pkg_name, int name_hash) {
    FEObject *obj = FEngFindObject(pkg_name, name_hash);
    if (obj != nullptr && obj->Type == FE_Image) {
        return static_cast<FEImage *>(obj);
    }
    return nullptr;
}

unsigned int FEngGetTextureHash(FEImage *image) {
    if (image != nullptr) {
        return image->Handle;
    }
    return 0;
}

void FEngSetTextureHash(FEImage *image, unsigned int hash) {
    if (image == nullptr) {
        return;
    }
    if (image->Handle == hash) {
        return;
    }
    image->Handle = hash;
    // Dos sentencias: EE-GCC no junta las dos mascaras (A124 lineas 39 y 45).
    image->Flags |= FF_DirtyCode;
    image->Flags |= FF_Dirty;
}

void FEngSetButtonTexture(FEImage *image, unsigned int hash) {
#if defined(EA_PLATFORM_PLAYSTATION2) && !defined(EA_BUILD_A124)
    // SLUS: las texturas de los sticks y de R3 llevan texto, y hay version francesa y espanola.
    if (image == nullptr) {
        return;
    }
    switch (hash) {
    case STRINGHASH_LEFT_ANALOG:
        switch (GetCurrentLanguage()) {
            case eLANGUAGE_FRENCH:
                FEngSetTextureHash(image, STRINGHASH_LEFT_ANALOG_FRENCH);
                break;
            case eLANGUAGE_SPANISH:
                FEngSetTextureHash(image, STRINGHASH_LEFT_ANALOG_SPANISH);
                break;
            default:
                FEngSetTextureHash(image, STRINGHASH_LEFT_ANALOG);
                break;
        }
        break;
    case STRINGHASH_RIGHT_ANALOG:
        switch (GetCurrentLanguage()) {
            case eLANGUAGE_FRENCH:
                FEngSetTextureHash(image, STRINGHASH_RIGHT_ANALOG_FRENCH);
                break;
            case eLANGUAGE_SPANISH:
                FEngSetTextureHash(image, STRINGHASH_RIGHT_ANALOG_SPANISH);
                break;
            default:
                FEngSetTextureHash(image, STRINGHASH_RIGHT_ANALOG);
                break;
        }
        break;
    case STRINGHASH_RIGHT_ANALOG_DOWN:
        switch (GetCurrentLanguage()) {
            case eLANGUAGE_FRENCH:
                FEngSetTextureHash(image, STRINGHASH_RIGHT_ANALOG_DOWN_FRENCH);
                break;
            case eLANGUAGE_SPANISH:
                FEngSetTextureHash(image, STRINGHASH_RIGHT_ANALOG_DOWN_SPANISH);
                break;
            default:
                FEngSetTextureHash(image, STRINGHASH_RIGHT_ANALOG_DOWN);
                break;
        }
        break;
    case STRINGHASH_RIGHT_ANALOG_UP:
        switch (GetCurrentLanguage()) {
            case eLANGUAGE_FRENCH:
                FEngSetTextureHash(image, STRINGHASH_RIGHT_ANALOG_UP_FRENCH);
                break;
            case eLANGUAGE_SPANISH:
                FEngSetTextureHash(image, STRINGHASH_RIGHT_ANALOG_UP_SPANISH);
                break;
            default:
                FEngSetTextureHash(image, STRINGHASH_RIGHT_ANALOG_UP);
                break;
        }
        break;
    case STRINGHASH_R3:
        switch (GetCurrentLanguage()) {
            case eLANGUAGE_FRENCH:
                FEngSetTextureHash(image, STRINGHASH_R3_FRENCH);
                break;
            case eLANGUAGE_SPANISH:
                FEngSetTextureHash(image, STRINGHASH_R3_SPANISH);
                break;
            default:
                FEngSetTextureHash(image, STRINGHASH_R3);
                break;
        }
        break;
    default:
        FEngSetTextureHash(image, hash);
        break;
    }
#else
    if (image != nullptr) {
        FEngSetTextureHash(image, hash);
    }
#endif
}
