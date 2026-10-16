#include "Speed/Indep/Src/Frontend/MenuScreens/Common/FEMenuScreen.hpp"
#include "Speed/Indep/Src/Frontend/FEPackageData.hpp"
#include "Speed/Indep/Src/Frontend/FEPackageManager.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterface.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterfaceFEButtons.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterfaceFEObjects.hpp"
#include "Speed/Indep/Src/Misc/Timer.hpp"
#include "Speed/Indep/bWare/Inc/Strings.hpp"
#include "Speed/Indep/bWare/Inc/bWare.hpp"

// total size: 0x10
struct OKDialogConfig {
    uint32 BlurbText;              // offset 0x0, size 0x4
    uint32 Button1Text;            // offset 0x4, size 0x4
    uint32 Button1PressedMessage;  // offset 0x8, size 0x4
    uint32 DialogCancelledMessage; // offset 0xC, size 0x4
};

// total size: 0x4C
class OKDialog : public MenuScreen {
  public:
    static FEPackage *pParentPackage;

    OKDialog(ScreenConstructorData *sd);
    ~OKDialog() override;

    static OKDialog *OpenModalDialog(OKDialogConfig *conf, const char *dialog_name, const char *parent_name);

    void NotificationMessage(u32 msg, FEObject *obj, u32 param1, u32 param2) override;

    void SetBlurbOverride(const char *pBlurbText);
    void PostProduce(const char *parent_pkg, OKDialogConfig *conf);

    static FEPackage *GetParent();

  private:
    Timer CreateTimer;         // offset 0x2C, size 0x4
    char *m_pBlurbOverride;    // offset 0x30, size 0x4
    bool bButtonInitComplete;  // offset 0x34, size 0x1
    const char *ParentPackage; // offset 0x38, size 0x4
    OKDialogConfig Config;     // offset 0x3C, size 0x10
};

FEPackage *OKDialog::pParentPackage = nullptr;

OKDialog::OKDialog(ScreenConstructorData *sd) : MenuScreen(sd) {
    CreateTimer = RealTimer;
    m_pBlurbOverride = nullptr;
    bButtonInitComplete = false;
}

OKDialog::~OKDialog() {
    if (m_pBlurbOverride) {
        bFree(m_pBlurbOverride);
    }
    FEngSetCreateCallback(GetPackageName(), nullptr);
}

MenuScreen *CreateOKDialog(ScreenConstructorData *sd) {
    return new ("OKDialog", 0) OKDialog(sd);
}

OKDialog *OKDialog::OpenModalDialog(OKDialogConfig *conf, const char *dialog_name, const char *parent_name) {
    FEngSetCreateCallback(dialog_name, CreateOKDialog);
    FEPackage *parent_package = cFEng::Get()->FindPackage(parent_name);
    cFEng::Get()->QueuePackagePush(dialog_name, 0, 0, false);

    OKDialog *dialog = static_cast<OKDialog *>(FEngFindScreen(dialog_name));
    if (dialog) {
        dialog->PostProduce(parent_name, conf);
    }
    pParentPackage = parent_package;
    return dialog;
}

void OKDialog::SetBlurbOverride(const char *pBlurbText) {
    if (m_pBlurbOverride == nullptr) {
        m_pBlurbOverride = static_cast<char *>(bMalloc(0x100, 0));
    }
    bStrNCpy(m_pBlurbOverride, pBlurbText, 0x100);

    if (m_pBlurbOverride == nullptr) {
        FEngSetLanguageHash(GetPackageName(), 0x1e2640fa, Config.BlurbText);
    } else {
        FEPrintf(GetPackageName(), 0x1e2640fa, m_pBlurbOverride);
    }
}

void OKDialog::PostProduce(const char *parent_pkg, OKDialogConfig *conf) {
    ParentPackage = parent_pkg;
    Config = *conf;

    if (m_pBlurbOverride == nullptr) {
        FEngSetLanguageHash(GetPackageName(), 0x1e2640fa, Config.BlurbText);
    } else {
        FEPrintf(GetPackageName(), 0x1e2640fa, m_pBlurbOverride);
    }

    FEngSetLanguageHash(GetPackageName(), 0xb8a7c6cc, Config.Button1Text);
    FEngSetCurrentButton(GetPackageName(), 0xb8a7c6cc);
}

void OKDialog::NotificationMessage(u32 msg, FEObject *obj, u32 param1, u32 param2) {
    if (CreateTimer == RealTimer) {
        return;
    }

    switch (msg) {
        case 0x35f8620b: {
            if (obj == FEngGetCurrentButton(GetPackageName())) {
                FEngSetScript(obj, 0x249db7b7, true);
            }
            break;
        }

        case 0x911ab364: {
            cFEng::Get()->QueuePackagePop(1);
            cFEng::Get()->QueueGameMessage(Config.DialogCancelledMessage, ParentPackage, 0xff);
            cFEng::Get()->QueuePackageMessage(0x7f6c5142, ParentPackage, nullptr);
            break;
        }

        case 0x0c407210: {
            if (obj->NameHash == 0xb8a7c6cc) {
                // Bloque de mas: los stabs de la A124 lo anidan dentro del if y el mapa de lineas trae dos '{'.
                {
                    cFEng::Get()->QueuePackagePop(1);
                    cFEng::Get()->QueueGameMessage(Config.Button1PressedMessage, ParentPackage, 0xff);
                    cFEng::Get()->QueuePackageMessage(0x7f6c5142, ParentPackage, nullptr);
                }
            }
            break;
        }
    }
}

FEPackage *OKDialog::GetParent() {
    return pParentPackage;
}
