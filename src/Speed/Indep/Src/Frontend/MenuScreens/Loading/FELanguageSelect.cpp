#include "Speed/Indep/Src/Frontend/MenuScreens/Loading/FELanguageSelect.hpp"
#ifdef EA_BUILD_A124
#include "Speed/Indep/Src/Frontend/Localization/Localize.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Loading/FEBootFlowManager.hpp"

// Solo la A124: la pantalla ofrece los idiomas como iconos (GameCube y SLUS no tienen la clase).
// total size: 0x60
struct ChooseLanguage : public IconOption {
    ChooseLanguage(eLanguages language, unsigned int tex_hash, unsigned int name_hash) : IconOption(tex_hash, name_hash, 0) {
        Language = language;
    }

    void React(const char *pkg_name, unsigned int data, FEObject *obj, unsigned int param1, unsigned int param2) override {
        if (data == 0x0C407210) {
            SetCurrentLanguage(Language);
            BootFlowManager::Get()->ChangeToNextBootFlowScreen(0xFF);
        }
    }

  private:
    eLanguages Language; // offset 0x5C, size 0x4
};
#endif

LanguageSelectScreen::LanguageSelectScreen(ScreenConstructorData *sd) : IconScrollerMenu(sd) {
#ifdef EA_BUILD_A124
    AddOption(new ChooseLanguage(eLANGUAGE_ENGLISH, 0xD73D6172, 0xC628BECA));
    AddOption(new ChooseLanguage(eLANGUAGE_FRENCH, 0x75BD5D7E, 0xAB866FD6));
    AddOption(new ChooseLanguage(eLANGUAGE_GERMAN, 0x772E6402, 0xACF7765A));
    AddOption(new ChooseLanguage(eLANGUAGE_DUTCH, 0xEC294100, 0x796D6858));
    AddOption(new ChooseLanguage(eLANGUAGE_SWEDISH, 0x21C28D9F, 0x10ADEAF7));
    AddOption(new ChooseLanguage(eLANGUAGE_DANISH, 0x6FE447DF, 0xA5AD5A37));
    AddOption(new ChooseLanguage(eLANGUAGE_FINNISH, 0x190D9757, 0x07F8F4AF));
    AddOption(new ChooseLanguage(eLANGUAGE_POLISH, 0x8CDE6DB7, 0xC2A7800F));
#endif
    SetInitialOption(0);
    RefreshHeader();
    StartedTimer = RealTimer;
}

LanguageSelectScreen::~LanguageSelectScreen() {}

void LanguageSelectScreen::NotificationMessage(u32 msg, FEObject *obj, u32 param1, u32 param2) {
    IconScrollerMenu::NotificationMessage(msg, obj, param1, param2);
}
