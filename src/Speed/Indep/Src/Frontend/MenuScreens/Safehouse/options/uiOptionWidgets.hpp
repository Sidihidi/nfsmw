#ifndef _UIOPTIONWIDGETS
#define _UIOPTIONWIDGETS

#include <types.h>

#include "Speed/Indep/Src/Frontend/MenuScreens/Common/feWidget.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Common/FEIconScrollerMenu.hpp"

// ===== OM* classes (extend IconOption) =====

// 0x5C
struct OMAudio : public IconOption {
    OMAudio(unsigned int tex_hash, unsigned int name_hash, unsigned int desc_hash)
        : IconOption(tex_hash, name_hash, desc_hash) {}
    void React(const char* pkg_name, unsigned int data, FEObject* obj, unsigned int param1, unsigned int param2) override;
};

// 0x5C
struct OMVideo : public IconOption {
    OMVideo(unsigned int tex_hash, unsigned int name_hash, unsigned int desc_hash)
        : IconOption(tex_hash, name_hash, desc_hash) {}
    void React(const char* pkg_name, unsigned int data, FEObject* obj, unsigned int param1, unsigned int param2) override;
};

// 0x5C
struct OMGameplay : public IconOption {
    OMGameplay(unsigned int tex_hash, unsigned int name_hash, unsigned int desc_hash)
        : IconOption(tex_hash, name_hash, desc_hash) {}
    void React(const char* pkg_name, unsigned int data, FEObject* obj, unsigned int param1, unsigned int param2) override;
};

// 0x5C
struct OMPlayer : public IconOption {
    OMPlayer(unsigned int tex_hash, unsigned int name_hash, unsigned int desc_hash)
        : IconOption(tex_hash, name_hash, desc_hash) {}
    void React(const char* pkg_name, unsigned int data, FEObject* obj, unsigned int param1, unsigned int param2) override;
};

// 0x5C
struct OMController : public IconOption {
    OMController(unsigned int tex_hash, unsigned int name_hash, unsigned int desc_hash)
        : IconOption(tex_hash, name_hash, desc_hash) {}
    void React(const char* pkg_name, unsigned int data, FEObject* obj, unsigned int param1, unsigned int param2) override;
};

// 0x5C
struct OMEATrax : public IconOption {
    OMEATrax(unsigned int tex_hash, unsigned int name_hash, unsigned int desc_hash)
        : IconOption(tex_hash, name_hash, desc_hash) {}
    void React(const char* pkg_name, unsigned int data, FEObject* obj, unsigned int param1, unsigned int param2) override;
};

// 0x5C
struct OMTrailers : public IconOption {
    OMTrailers(unsigned int tex_hash, unsigned int name_hash, unsigned int desc_hash)
        : IconOption(tex_hash, name_hash, desc_hash) {}
    void React(const char* pkg_name, unsigned int data, FEObject* obj, unsigned int param1, unsigned int param2) override;
};

// 0x5C
struct OMOnline : public IconOption {
    OMOnline(unsigned int tex_hash, unsigned int name_hash, unsigned int desc_hash)
        : IconOption(tex_hash, name_hash, desc_hash) {}
    void React(const char* pkg_name, unsigned int data, FEObject* obj, unsigned int param1, unsigned int param2) override;
};

// 0x5C
struct OMCredits : public IconOption {
    OMCredits(unsigned int tex_hash, unsigned int name_hash, unsigned int desc_hash)
        : IconOption(tex_hash, name_hash, desc_hash) {}
    void React(const char* pkg_name, unsigned int data, FEObject* obj, unsigned int param1, unsigned int param2) override;
};

// ===== AO* slider classes (extend FESliderWidget) =====

// 0xA4
struct AOMasterVol : public FESliderWidget {
    AOMasterVol(bool enabled) : FESliderWidget(enabled) {}
    void Act(const char* parent_pkg, unsigned int data) override;
    void Draw() override;
    void SetInitialValues() override;
};

// 0xA4
struct AOSFXMasterVol : public FESliderWidget {
    AOSFXMasterVol(bool enabled) : FESliderWidget(enabled) {}
    void Act(const char* parent_pkg, unsigned int data) override;
    void Draw() override;
    void SetInitialValues() override;
};

// 0xA4
struct AOCarVol : public FESliderWidget {
    AOCarVol(bool enabled) : FESliderWidget(enabled) {}
    void Act(const char* parent_pkg, unsigned int data) override;
    void Draw() override;
    void SetInitialValues() override;
};

// 0xA4
struct AOEngineVol : public FESliderWidget {
    AOEngineVol(bool enabled) : FESliderWidget(enabled) {}
    void Act(const char* parent_pkg, unsigned int data) override;
    void Draw() override;
    void SetInitialValues() override;
};

// 0xA4
struct AOSpeechVol : public FESliderWidget {
    AOSpeechVol(bool enabled) : FESliderWidget(enabled) {}
    void Act(const char* parent_pkg, unsigned int data) override;
    void Draw() override;
    void SetInitialValues() override;
};

// 0xA4
struct AOFEMusicVol : public FESliderWidget {
    AOFEMusicVol(bool enabled) : FESliderWidget(enabled) {}
    void Act(const char* parent_pkg, unsigned int data) override;
    void Draw() override;
    void SetInitialValues() override;
};

// 0xA4
struct AOIGMusicVol : public FESliderWidget {
    AOIGMusicVol(bool enabled) : FESliderWidget(enabled) {}
    void Act(const char* parent_pkg, unsigned int data) override;
    void Draw() override;
    void SetInitialValues() override;
};

// ===== AO* toggle classes (extend FEToggleWidget) =====

// 0x64
struct AOInteractiveMusicMode : public FEToggleWidget {
    AOInteractiveMusicMode(bool enabled) : FEToggleWidget(enabled) {}
    void Act(const char* parent_pkg, unsigned int data) override;
    void Draw() override;
};

// 0x64
struct AOEATraxMusicMode : public FEToggleWidget {
    AOEATraxMusicMode(bool enabled) : FEToggleWidget(enabled) {}
    void Act(const char* parent_pkg, unsigned int data) override;
    void Draw() override;
};

// 0x64
struct AOAudioMode : public FEToggleWidget {
    AOAudioMode(bool enabled) : FEToggleWidget(enabled) {}
    void Act(const char* parent_pkg, unsigned int data) override;
    void Draw() override;
};

#ifdef EA_BUILD_A124
// ===== OO* classes: las opciones online de la A124 (GameCube y SLUS no las tienen) =====
// Los stabs dan a sus constructores cuatro bVector2& delante de enabled; ninguno llega al codigo (el de la base solo
// recibe enabled, como el OOBadConnectionWarning(bool) de PC).

// 0x64
struct OOShowPlayers : public FEToggleWidget {
    OOShowPlayers(bool enabled) : FEToggleWidget(enabled) {}
    void Act(const char* parent_pkg, unsigned int data) override;
    void Draw() override;
};

// 0x64
struct OOAppearOnline : public FEToggleWidget {
    OOAppearOnline(bool enabled) : FEToggleWidget(enabled) {}
    void Act(const char* parent_pkg, unsigned int data) override;
    void Draw() override;
};

// 0x64
struct OOBadConnectionWarning : public FEToggleWidget {
    OOBadConnectionWarning(bool enabled) : FEToggleWidget(enabled) {}
    void Act(const char* parent_pkg, unsigned int data) override;
    void Draw() override;
};

// 0x68
struct OOVoiceSpeaker : public FEToggleWidget {
    OOVoiceSpeaker(bool enabled, FEWidget* v) : FEToggleWidget(enabled) {
        volumewidget = v;
    }
    void Act(const char* parent_pkg, unsigned int data) override;
    void Draw() override;

  private:
    FEWidget* volumewidget; // offset 0x64, size 0x4
};

// 0xA4
struct OOVoiceVolume : public FESliderWidget {
    OOVoiceVolume(bool enabled) : FESliderWidget(enabled) {}
    void Act(const char* parent_pkg, unsigned int data) override;
    void Draw() override;
    void SetInitialValues() override;
    void Enable() override;
    void Disable() override;
};
#endif

// ===== VO* classes (extend FEToggleWidget) =====

// 0x64
struct VOWideScreen : public FEToggleWidget {
    VOWideScreen(bool enabled) : FEToggleWidget(enabled) {}
    void Act(const char* parent_pkg, unsigned int data) override;
    void Draw() override;
};

// Los cuatro deslizadores de video siguientes estan en el DWARF de GameCube (en este orden, detras de VOWideScreen)
// aunque GameCube no los usa; las dos PS2 los compilan y SetupVideo pone los de posicion.

// 0xA4
struct VOScreenPosHoriz : public FESliderWidget {
    VOScreenPosHoriz(bool enabled) : FESliderWidget(enabled) {}
    void Act(const char* parent_pkg, unsigned int data) override;
    void Draw() override;
    void SetInitialValues() override;
};

// 0xA4
struct VOScreenPosVert : public FESliderWidget {
    VOScreenPosVert(bool enabled) : FESliderWidget(enabled) {}
    void Act(const char* parent_pkg, unsigned int data) override;
    void Draw() override;
    void SetInitialValues() override;
};

// 0xA4
struct VOScreenScale : public FESliderWidget {
    VOScreenScale(bool enabled) : FESliderWidget(enabled) {}
    void Act(const char* parent_pkg, unsigned int data) override;
    void Draw() override;
    void SetInitialValues() override;
};

// 0xA4
struct VOTimeOfDay : public FESliderWidget {
    VOTimeOfDay(bool enabled) : FESliderWidget(enabled) {}
    void Act(const char* parent_pkg, unsigned int data) override;
    void Draw() override;
    void SetInitialValues() override;
};

// ===== GO* classes (extend FEToggleWidget) =====

// 0x64
struct GODamage : public FEToggleWidget {
    GODamage(bool enabled) : FEToggleWidget(enabled) {}
    void Act(const char* parent_pkg, unsigned int data) override;
    void Draw() override;
};

// 0x64
struct GOAutoSave : public FEToggleWidget {
    GOAutoSave(bool enabled) : FEToggleWidget(enabled) {}
    void Act(const char* parent_pkg, unsigned int data) override;
    void Draw() override;
};

// 0x64
struct GOJumpCams : public FEToggleWidget {
    GOJumpCams(bool enabled) : FEToggleWidget(enabled) {}
    void Act(const char* parent_pkg, unsigned int data) override;
    void Draw() override;
};

// 0x64
struct GORearview : public FEToggleWidget {
    GORearview(bool enabled) : FEToggleWidget(enabled) {}
    void Act(const char* parent_pkg, unsigned int data) override;
    void Draw() override;
};

// 0x64
struct GOSpeedoUnits : public FEToggleWidget {
    GOSpeedoUnits(bool enabled) : FEToggleWidget(enabled) {}
    void Act(const char* parent_pkg, unsigned int data) override;
    void Draw() override;
};

// 0x64
struct GORacingMiniMap : public FEToggleWidget {
    GORacingMiniMap(bool enabled) : FEToggleWidget(enabled) {}
    void Act(const char* parent_pkg, unsigned int data) override;
    void Draw() override;
};

// 0x64
struct GOExploringMiniMap : public FEToggleWidget {
    GOExploringMiniMap(bool enabled) : FEToggleWidget(enabled) {}
    void Act(const char* parent_pkg, unsigned int data) override;
    void Draw() override;
};

// ===== PO* classes (extend FEToggleWidget) =====

// 0x64
struct POTransmission : public FEToggleWidget {
    POTransmission(bool enabled) : FEToggleWidget(enabled) {}
    void Act(const char* parent_pkg, unsigned int data) override;
    void Draw() override;
};

// 0x64
struct POTransPrompt : public FEToggleWidget {
    POTransPrompt(bool enabled) : FEToggleWidget(enabled) {}
    void Act(const char* parent_pkg, unsigned int data) override;
    void Draw() override;
};

// 0x64
struct POHandling : public FEToggleWidget {
    POHandling(bool enabled) : FEToggleWidget(enabled) {}
    void Act(const char* parent_pkg, unsigned int data) override;
    void Draw() override;
};

// 0x64
struct PODriveCam : public FEToggleWidget {
    PODriveCam(bool enabled) : FEToggleWidget(enabled) {}
    void Act(const char* parent_pkg, unsigned int data) override;
    void Draw() override;
};

// 0x64
struct POGauges : public FEToggleWidget {
    POGauges(bool enabled) : FEToggleWidget(enabled) {}
    void Act(const char* parent_pkg, unsigned int data) override;
    void Draw() override;
};

// 0x64
struct POPosition : public FEToggleWidget {
    POPosition(bool enabled) : FEToggleWidget(enabled) {}
    void Act(const char* parent_pkg, unsigned int data) override;
    void Draw() override;
};

// 0x64
struct POLapInfo : public FEToggleWidget {
    POLapInfo(bool enabled) : FEToggleWidget(enabled) {}
    void Act(const char* parent_pkg, unsigned int data) override;
    void Draw() override;
};

// 0x64
struct POScore : public FEToggleWidget {
    POScore(bool enabled) : FEToggleWidget(enabled) {}
    void Act(const char* parent_pkg, unsigned int data) override;
    void Draw() override;
};

// 0x64
struct POSplitTime : public FEToggleWidget {
    POSplitTime(bool enabled) : FEToggleWidget(enabled) {}
    void Act(const char* parent_pkg, unsigned int data) override;
    void Draw() override;
};

// 0x64
struct POLeaderBoard : public FEToggleWidget {
    POLeaderBoard(bool enabled) : FEToggleWidget(enabled) {}
    void Act(const char* parent_pkg, unsigned int data) override;
    void Draw() override;
};

// ===== CO* classes (extend FEToggleWidget) =====

// 0x64
struct COVibration : public FEToggleWidget {
    COVibration(int player_num, bool enabled) : FEToggleWidget(enabled) {}
    void Act(const char* parent_pkg, unsigned int data) override;
    void Draw() override;
    void SetFocus(const char* parent_pkg) override;
    void UnsetFocus() override;
};

// 0x64
struct CODriveWithAnalog : public FEToggleWidget {
    CODriveWithAnalog(bool enabled) : FEToggleWidget(enabled) {}
    void Act(const char* parent_pkg, unsigned int data) override;
    void Draw() override;
};

// 0x64
struct COConfig : public FEToggleWidget {
    COConfig(bool enabled) : FEToggleWidget(enabled) {}
    void Act(const char* parent_pkg, unsigned int data) override;
    void Draw() override;
};

#endif
