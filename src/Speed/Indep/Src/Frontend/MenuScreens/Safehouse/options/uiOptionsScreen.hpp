#ifndef _UIOPTIONSSCREEN
#define _UIOPTIONSSCREEN

#include <types.h>

#include "Speed/Indep/Src/Frontend/MenuScreens/Common/feUIWidgetMenu.hpp"

struct AudioSettings;
struct VideoSettings;
struct GameplaySettings;
struct PlayerSettings;

// total size: 0x15C
struct UIOptionsScreen : public UIWidgetMenu {
    void SetupDebug();

    void SetupDebugAI();

    void SetupDebugPhysics();

    void SetupDebugOnline();

    void SetupDebugRendering();

    static int PlayerToEdit;
#ifdef EA_BUILD_A124
    static bool mTempAppearOffline;
#endif

    bool mCalledFromPauseMenu;                    // offset 0x138, size 0x1
    bool NeedsColorCal;                           // offset 0x13C, size 0x1
    bool mInitialAutoSaveValue;                   // offset 0x140, size 0x1
    FEToggleWidget* speakeroption;                // offset 0x144, size 0x4
    FESliderWidget* volumeoption;                 // offset 0x148, size 0x4
    AudioSettings* OriginalAudioSettings;         // offset 0x14C, size 0x4
    VideoSettings* OriginalVideoSettings;         // offset 0x150, size 0x4
    GameplaySettings* OriginalGameplaySettings;   // offset 0x154, size 0x4
    PlayerSettings* OriginalPlayerSettings;       // offset 0x158, size 0x4
#ifdef EA_BUILD_A124
    // Solo la A124 (stabs: UIOptionsScreen de 0x16c; CreateOptionsScreen hace new de 0x16c): los ajustes online y el
    // altavoz del VOIP.
    struct cOnlineSettings* OriginalOnlineSettings; // offset 0x15C, size 0x4
    bool OriginalSpeakerState;                    // offset 0x160, size 0x1
    float OriginalSpeakerVolume;                  // offset 0x164, size 0x4
    float OriginalPlaybackVolume;                 // offset 0x168, size 0x4
#endif

    UIOptionsScreen(ScreenConstructorData* sd);
    ~UIOptionsScreen() override;

    void NotificationMessage(u32 msg, FEObject* pobj, u32 param1, u32 param2) override;
  private:
    void Setup() override;

    void SetupAudio();
    void SetupVideo();
    void SetupGameplay();
    void SetupPlayer();
    void SetupOnline();
#ifdef EA_BUILD_A124
    void UpdateOnlineVoipOptions();
#endif
    void RestoreDefaults();
    bool OptionsDidNotChange();
    void RestoreOriginals();
    void TogglePlayer(bool revert_changes);
    bool ShouldShowAutoSave();
};


// [fe3] declaraciones que HEAD tenia en esta cabecera y la de la rama no:
int GetPlayerToEditForOptions();
void SetPlayerToEditForOptions(int player);

#endif
