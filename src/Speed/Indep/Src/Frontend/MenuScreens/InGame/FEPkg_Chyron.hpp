#ifndef FEPKG_CHYRON_H
#define FEPKG_CHYRON_H

#include "Speed/Indep/Src/Frontend/MenuScreens/Common/FEMenuScreen.hpp"
#include "Speed/Indep/Src/Misc/Timer.hpp"

static MenuScreen *ChyronScreenPtr = nullptr;

class Chyron : public MenuScreen {
  public:
    void *operator new(size_t size);
    void *operator new(size_t size, char *file, int line) {}
    void *operator new(size_t size, char *msg) {}
    void operator delete(void *ptr) {}

    Chyron(ScreenConstructorData *sd);
    ~Chyron() override {};
    void NotificationMessage(u32 msg, FEObject *pobject, u32 param1, u32 param2) override;
    void Start();

    static char *mTitle;
    static char *mArtist;
    static char *mAlbum;

    // La A124 no lo tiene: sus stabs dan a Chyron 0x2C, lo mismo que MenuScreen.
#ifndef EA_BUILD_A124
    Timer mDelayTimer; // offset 0x2C
#endif
};

void SummonChyron(char *title, char *artist, char *album);
void DismissChyron();
void InitChyron();

#endif
