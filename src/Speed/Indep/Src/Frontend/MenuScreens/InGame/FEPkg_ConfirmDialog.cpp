#include "Speed/Indep/Src/Frontend/MenuScreens/Common/FEMenuScreen.hpp"
#include "Speed/Indep/Src/Frontend/FEPackageData.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterface.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterfaceFEButtons.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterfaceFEObjects.hpp"
#include "Speed/Indep/bWare/Inc/Strings.hpp"
#include "Speed/Indep/bWare/Inc/bWare.hpp"

// total size: 0x128
class DialogConfig {
  public:
    DialogConfig();

    void SetBlurbString(const char *string);

    uint32 BlurbText;               // offset 0x0, size 0x4
    char BlurbString[256];          // offset 0x4, size 0x100
    uint32 Button1Text;             // offset 0x104, size 0x4
    uint32 Button1PressedMessage;   // offset 0x108, size 0x4
    uint32 Button2Text;             // offset 0x10C, size 0x4
    uint32 Button2PressedMessage;   // offset 0x110, size 0x4
    uint32 DialogCancelledMessage;  // offset 0x114, size 0x4
    uint32 HighlightButtonAtStart;  // offset 0x118, size 0x4
    uint32 currentSelected;         // offset 0x11C, size 0x4
    uint32 DetectMissingController; // offset 0x120, size 0x4
    const char *ParentPackage;      // offset 0x124, size 0x4
};

// total size: 0x15C
class ConfirmDialog : public MenuScreen {
  public:
    ConfirmDialog(ScreenConstructorData *sd);
    ~ConfirmDialog() override;

    static ConfirmDialog *OpenModalDialog(DialogConfig *conf, const char *dialog_name, const char *parent_name);

    void NotificationMessage(u32 msg, FEObject *obj, u32 param1, u32 param2) override;

  private:
    bool bButtonInitComplete; // offset 0x2C, size 0x1
    DialogConfig Config;      // offset 0x30, size 0x128
    uint32 PopMeMessage;      // offset 0x158, size 0x4
};

DialogConfig::DialogConfig() {
    BlurbText = 0;
    bMemSet(BlurbString, 0, sizeof(BlurbString));
    Button1Text = 0;
    Button1PressedMessage = 0;
    Button2Text = 0;
    Button2PressedMessage = 0;
    DialogCancelledMessage = 0;
    HighlightButtonAtStart = 0;
    currentSelected = 0;
    DetectMissingController = 0;
    ParentPackage = nullptr;
}

void DialogConfig::SetBlurbString(const char *string) {
    bStrNCpy(BlurbString, string, 255);
}

ConfirmDialog::ConfirmDialog(ScreenConstructorData *sd) : MenuScreen(sd) {
    DialogConfig *temp = reinterpret_cast<DialogConfig *>(sd->Arg);
    Config = *temp;
    delete temp;

    bButtonInitComplete = false;
    Config.currentSelected = 0;
    PopMeMessage = 0;

    if (Config.BlurbText != 0) {
        FEngSetLanguageHash(GetPackageName(), 0x1e2640fa, Config.BlurbText);
    } else if (Config.BlurbString[0] != '\0') {
        FEPrintf(GetPackageName(), 0x1e2640fa, Config.BlurbString);
    }

    FEngSetLanguageHash(GetPackageName(), 0xb8a7c6cc, Config.Button1Text);
    FEngSetLanguageHash(GetPackageName(), 0xb8a7c6cd, Config.Button2Text);
    FEngSetCurrentButton(GetPackageName(), Config.HighlightButtonAtStart);
}

ConfirmDialog::~ConfirmDialog() {
    FEngSetCreateCallback(GetPackageName(), nullptr);
}

MenuScreen *CreateConfirmDialog(ScreenConstructorData *sd) {
    return new ("ConfirmDialog", 0) ConfirmDialog(sd);
}

ConfirmDialog *ConfirmDialog::OpenModalDialog(DialogConfig *conf, const char *dialog_name, const char *parent_name) {
    FEngSetCreateCallback(dialog_name, CreateConfirmDialog);
    conf->ParentPackage = parent_name;

    DialogConfig *dlg = new ("DialogConfig", 0) DialogConfig;
    *dlg = *conf;
    cFEng::Get()->QueuePackagePush(dialog_name, reinterpret_cast<int>(dlg), 0, false);
    return nullptr;
}

void ConfirmDialog::NotificationMessage(u32 msg, FEObject *obj, u32 param1, u32 param2) {
    if (PopMeMessage != 0) {
        if (msg == 0xc98356ba) {
            const char *send_msg_to = Config.ParentPackage;
            unsigned int send_msg = PopMeMessage;

            if (cFEng::Get()->IsPackageInControl(GetPackageName())) {
                cFEng::Get()->QueuePackagePop(1);
            }
            cFEng::Get()->QueueGameMessage(send_msg, send_msg_to, 0xff);
        }
        return;
    }

    switch (msg) {
        case 0x35f8620b: {
            if (obj == FEngGetCurrentButton(GetPackageName())) {
                FEngSetScript(obj, 0x249db7b7, true);
                if (obj->NameHash == 0xb8a7c6cc) {
                    FEngSetScript(GetPackageName(), 0xb8a7c6cd, 0x7ab5521a, true);
                } else if (obj->NameHash == 0xb8a7c6cd) {
                    FEngSetScript(GetPackageName(), 0xb8a7c6cc, 0x7ab5521a, true);
                }
            }
            break;
        }

        case 0x911ab364:
            if (Config.DialogCancelledMessage != 0) {
                PopMeMessage = Config.DialogCancelledMessage;
            }
            break;

        case 0x0c407210:
            switch (obj->NameHash) {
                case 0xb8a7c6cc:
                    PopMeMessage = Config.Button1PressedMessage;
                    break;
                case 0xb8a7c6cd:
                    PopMeMessage = Config.Button2PressedMessage;
                    Config.currentSelected = 1;
                    break;
            }
            break;
    }
}
