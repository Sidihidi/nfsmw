#include "Speed/Indep/Src/Frontend/MenuScreens/MemCard/uiMemcardBase.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/MemCard/uiMemcard.hpp"
#include "Speed/Indep/Src/EAXSound/CARSFX/SFXObj_Pathfinder.hpp"
#include "Speed/Indep/Src/EAXSound/EAXSOund.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/career/uiRepSheetRivalFlow.hpp"
#include "Speed/Indep/Src/Generated/Events/EQuitToFE.hpp"
#ifdef EA_BUILD_A124
#include "Speed/Indep/Src/Generated/Events/EPauseForMemcard.hpp"
#endif
#include "Speed/Indep/Src/Misc/GameFlow.hpp"
#include "Speed/Indep/Src/FEng/FEObject.h"
#include "Speed/Indep/Src/Frontend/Database/FEDatabase.hpp"
#include "Speed/Indep/Src/Frontend/FEngFont.hpp"
#include "Speed/Indep/Src/Frontend/FEngHashes/SoundHashes.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterface.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterfaceFEButtons.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterfaceFEImages.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterfaceFEObjects.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterfaceFEStrings.hpp"
#include "Speed/Indep/Src/Frontend/Localization/Localize.hpp"
#include "Speed/Indep/Src/Frontend/MemoryCard/MemoryCard.hpp"
#include "Speed/Indep/Src/Frontend/MemoryCard/MemoryCardHelper.hpp"
#include "Speed/Indep/bWare/Inc/Strings.hpp"
#include "Speed/Indep/bWare/Inc/bPrintf.hpp"
#include "Speed/Indep/bWare/Inc/bWare.hpp"
#include "Speed/Indep/Src/Frontend/FEngHashes/ScriptHashes.hpp"
#include "Speed/Indep/Src/Generated/LanguageHashes.hpp"

const unsigned int gButtonIDs[3] = {FEHASH_BUTTON1, FEHASH_BUTTON2, FEHASH_BUTTON3};
const unsigned int gButtonTextIDs[3] = {0xF9363F30, 0xFB8B67D1, 0xFDE09072};

inline void FEMemWidget::Act(const char *parent_pkg, unsigned int data) {
    MemoryCard::GetInstance()->GetScreen()->DoSelect(Strings.GetNode(0)->String);
}

UIMemcardKeyboard::UIMemcardKeyboard(ScreenConstructorData *sd) : MenuScreen(sd) {
    const u32 FEObj_MESSAGE_BLURB = 0x1E2640FA;
    const u32 FEObj_HEADER_TEXT_02 = 0x426C7B4D;

    m_pDisplayMsg = reinterpret_cast<FEString *>(FEngFindObject(GetPackageName(), FEObj_MESSAGE_BLURB));
    m_pDisplayMsgShadow = reinterpret_cast<FEString *>(FEngFindObject(GetPackageName(), FEHashUpper("message_blurb_shadow")));
    m_pTitleMaster = reinterpret_cast<FEString *>(FEngFindObject(GetPackageName(), FEObj_HEADER_TEXT_02));
    m_pOK = reinterpret_cast<FEString *>(FEngFindObject(GetPackageName(), gButtonIDs[0]));
    m_pCancel = reinterpret_cast<FEString *>(FEngFindObject(GetPackageName(), gButtonIDs[1]));
}

void UIMemcardKeyboard::Setup() {
    const u32 Hahs_FADEIN = FEHASH_FADEIN;

    FEngSetScript(GetPackageName(), gButtonIDs[0], Hahs_FADEIN, true);

    FEngSetScript(GetPackageName(), gButtonIDs[1], Hahs_FADEIN, true);
    FEngSetVisible(GetPackageName(), gButtonIDs[0]);
    FEngSetVisible(GetPackageName(), gButtonIDs[1]);

    FEngSetCurrentButton(GetPackageName(), gButtonIDs[1]);
}

void UIMemcardKeyboard::ShowKeyboard() {
    const u32 FEObj_DIM = 0x47FF4E7C;
    const u32 FEObj_SHOW = FEHASH_DIM;

    FEngSetScript(GetPackageName(), FEObj_DIM, FEObj_SHOW, true);

    FEngBeginTextInput(0, 6, GetLocalizedString(0x70513BD4), GetLocalizedString(0x0D48D95F), 7);

    FEDatabase->mFEKeyboardSettings.Mode = 5;
}

void UIMemcardKeyboard::NotificationMessage(u32 msg, FEObject *obj, u32 param1, u32 param2) {
    const u32 FEObj_UNDIM = 0x47FF4E7C;
    const u32 FEObj_HIDE = FEHASH_UNDIM;

    if (msg == 0xC9D30688) {
        FEngSetScript(GetPackageName(), FEObj_UNDIM, FEObj_HIDE, true);
    }
}

UIMemcardBase::UIMemcardBase(ScreenConstructorData *sd) : UIMemcardKeyboard(sd), mIndex(1) {
    m_ExpectingInput = false;
    m_LoadedNetConfig = 0;
    m_nMsgOptions = 0;
    m_bVisible = false;
    m_bDelayedFailed = false;
    m_bInButtonAnimation = false;
    m_pChild = nullptr;
#ifndef EA_PLATFORM_PLAYSTATION2 // las dos PS2 se quedan con la lista que deja el constructor de bTList
    m_Items.InitList();
#endif
    m_SimPausedForMemcard = false;
}

UIMemcardBase::~UIMemcardBase() {
    m_pDisplayMsg = nullptr;
    MemoryCard::GetInstance()->FEngLinkObjects(nullptr);

    if (gMemcardSetup.mOp & MCE_PostOnDestroy) {
        gMemcardSetup.Complete(gMemcardSetup.mLastMessage);
    }
}

void UIMemcardBase::Abort() {
    cFEng::Get()->QueueGameMessage(0x8867412D, GetPackageName(), 0xFF);
}

bool UIMemcardBase::AddItem(const char *pName, const char *pDate, int size, int flag) {
    Item *pNode = new ("FEPkgMemcardFileItem", 0) Item();

    bStrNCpy(pNode->m_Name, pName, 31);
    pNode->m_Name[31] = 0;
    bStrCpy(pNode->m_Data, pDate);
    pNode->m_Size = size;
    pNode->m_Flag = static_cast<MemCardFileFlag>(flag);

    m_Items.AddTail(pNode);

    return true;
}

bool UIMemcardBase::IsProfile(const char *pName) {
    return bStrLen(pName) <= 7;
}

void UIMemcardBase::EmptyFileList() {
    m_Items.DeleteAllElements();
}

void UIMemcardBase::InitCompleteDoList() {
    m_Items.DeleteAllElements();

    SetStringCheckingCard();

    MemoryCard::GetInstance()->RequestTask(7, nullptr);

#if !defined(EA_PLATFORM_PLAYSTATION2) && !defined(EA_PLATFORM_WIN32) // PC tampoco (speed.exe 0x559D60)
    cFEng::Get()->QueuePackageMessage(FEHashUpper("SHOW LOADER"), GetPackageName(), nullptr);
#endif
}

void UIMemcardBase::InitComplete() {

    if (IsMemcardEnabled == 0) {

        cFEng::Get()->QueueGameMessage(0x461A18EE, GetPackageName(), 0xff);
        return;
    }
#ifndef EA_BUILD_A124 // la A124 no limpia el texto
    SetMessageBlurbText(" ");
#endif
#if !(defined(EA_PLATFORM_PLAYSTATION2) || defined(EA_PLATFORM_WIN32)) // PC (speed.exe 0x562C42) y las PS2 no esconden el "Button"
    FEngSetInvisible(GetPackageName(), FEHashUpper("Button"));
#endif





    m_pDisplayMsg->Flags |= 0x80;
#ifdef EA_BUILD_A124 // la A124 avisa tambien en la serie de desafios y enciende la tarjeta solo con el rival
    if ((gMemcardSetup.GetExtraOptions() & MCE_SendInitComplete) != 0 || (gMemcardSetup.GetExtraOptions() & MCE_ChallengeSeries) != 0) {
#else
    if ((gMemcardSetup.GetExtraOptions() & MCE_SendInitComplete) != 0) {
#endif
        cFEng::Get()->QueueGameMessage(0x5AFE12F4, gMemcardSetup.mFromScreen, 0xff);
    }

#ifdef EA_BUILD_A124
    if ((gMemcardSetup.GetExtraOptions() & MCE_RivalFlow) != 0) {
#else
    if ((gMemcardSetup.GetExtraOptions() & MCE_RivalFlow) != 0 ||
        ((gMemcardSetup.GetExtraOptions() & MCE_ChallengeSeries) != 0 && gMemcardSetup.GetCommand() == MCO_AutoSave)) {
#endif

        cFEng::Get()->QueuePackageMessage(FEHashUpper("MEMCARD_ON"), GetPackageName(), nullptr);
    }

    switch (MemcardGetCurrentUIOperation()) {




        case MCO_LoadList:
        case MCO_CarLotLoad:
#ifdef EA_BUILD_A124 // la A124 no mira el segundo jugador
            if (FEDatabase->bProfileLoaded) {
#else
            if (FEDatabase->bProfileLoaded && (gMemcardSetup.GetExtraOptions() & MCE_Player2) == 0) {
#endif



                ShowYesNo(0x87C7577E, 0x06000000);

            } else {

                InitCompleteDoList();
            }
            break;










        case MCO_BootList:
#if defined(EA_PLATFORM_PLAYSTATION2) || defined(EA_PLATFORM_WIN32) // speed.exe 0x562D8F
            if (MemoryCard::IsCardAvailable() && IsMemcardEnabled != 0) {
                InitCompleteDoList();
            } else {
                MemcardExit(0x8867412D);
            }
#else
            MemcardExit(0x8867412D);
#endif
            break;


        case MCO_DeleteList:
            SetStringCheckingCard();




            InitCompleteDoList();
            break;










        case MCO_SaveCreate:
        case MCO_CreateNew:
            cFEng::Get()->QueueGameMessage(0x5A051729, nullptr, 0xff);
            break;








        case MCO_Save:
#ifdef EA_BUILD_A124 // la A124: sin nombre de perfil sale; con el rival guarda sin preguntar
            if (FEDatabase->GetUserProfile(0)->IsProfileNamed()) {
                bStrCpy(m_FileName, FEDatabase->GetUserProfile(0)->GetProfileName());
                if ((gMemcardSetup.GetExtraOptions() & MCE_RivalFlow) != 0) {
                    HideAllButtons();
                    DoSaveFlow(MCSF_DoSave);
                } else {
                    DoSaveFlow(MCSF_PromptForConfirm);
                }
            } else {
                cFEng::Get()->QueueGameMessage(0x461A18EE, GetPackageName(), 0xff);
            }
#else
            bStrCpy(m_FileName, FEDatabase->GetUserProfile(0)->GetProfileName());
            DoSaveFlow(MCSF_PromptForConfirm);
#endif
            break;

        case MCO_CheckCard:
            MemoryCard::GetInstance()->CheckCard(0);
            break;

#ifdef EA_PLATFORM_PLAYSTATION2
        case MCO_ShowTRCMessage:
            ShowMessage(MemoryCard::GetInstance()->GetPendingMessage());
            MemoryCard::GetInstance()->ReleasePendingMessage();
            break;
#endif














        case MCO_EnableAutoSave:
            if ((gMemcardSetup.GetExtraOptions() & MCE_Online) != 0) {

                MemoryCard::GetInstance()->SetAutoSaveEnabled(true);

            } else {

                SetStringCheckingCard();
                ShowYesNo(0x750EB45C, 0x0C000000);
            }
            break;

        case MCO_AutoSaveError:
#ifndef EA_BUILD_A124 // la A124 no marca la pausa aqui ni en el caso siguiente
            m_SimPausedForMemcard = true;
#endif
            HandleAutoSaveError();
            break;

        case MCO_AutoSaveOverwrite:
#ifndef EA_BUILD_A124
            m_SimPausedForMemcard = true;
#endif
            HandleAutoSaveOverwriteMessage();
            break;


        case MCO_AutoSave:
            if (FEDatabase->bProfileLoaded) {

                if (MemoryCard::GetInstance()->ShouldDoAutoSave(false)) {

#ifdef EA_PLATFORM_WIN32 // speed.exe 0x562E93: el aviso de autoguardado con el nombre del perfil
                    FEPrintf(m_pDisplayMsg, GetLocalizedString(0x50D23EBD), FEDatabase->GetUserProfile(0)->GetProfileName());
#endif
                    SetScreenVisible(true, 0);
                    SetStringCheckingCard();
#ifndef EA_BUILD_A124 // la A124 no copia el nombre del perfil
                    bStrCpy(m_FileName, FEDatabase->GetUserProfile(0)->GetProfileName());
#endif
                    MemoryCard::GetInstance()->StartAutoSave(true);

                } else {

                    gMemcardSetup.ClearCommand();
                    gMemcardSetup.SetCommand(MCO_Save);
                    InitComplete();
                }

            } else {

                gMemcardSetup.ClearCommand();
                gMemcardSetup.SetCommand(MCO_CreateNew);
                InitComplete();
            }
            break;



#if !(defined(EA_PLATFORM_PLAYSTATION2) || defined(EA_PLATFORM_WIN32))
        case 0xF0:
            if (MemoryCard::IsCardAvailable() && IsMemcardEnabled != 0) {
                InitCompleteDoList();

            } else {


                MemcardExit(0x8867412D);
            }
            break;
#endif
    }
}

void UIMemcardBase::ExitComplete() {

    gMemcardSetup.SendTermMessage(gMemcardSetup.mLastMessage);


    if (FEDatabase->IsCareerManagerMode() && TheGameFlowManager.IsInFrontend()) {


        FEDatabase->ResetGameMode();


        if (FEDatabase->bProfileLoaded &&
            !(gMemcardSetup.GetCommand() == MCO_LoadList && gMemcardSetup.mLastMessage == 0x8867412D) &&
            gMemcardSetup.mPreviousPrompt != MCP_Create && gMemcardSetup.mPreviousPrompt != MCP_DismissMe &&
            gMemcardSetup.mPreviousPrompt != MCP_ConfirmDestoryCreate) {

            if (FEDatabase->GetCareerSettings()->HasCareerStarted()) {

                FEDatabase->GetCareerSettings()->ResumeCareer();


            } else {

                FEDatabase->GetCareerSettings()->StartNewCareer(true);
            }

        } else {

            gMemcardSetup.ClearMethod();
            gMemcardSetup.SetMethod(MCF_Modal);
            FEDatabase->RestoreFromBackupDB();
            FEDatabase->SetGameMode(eFE_GAME_MODE_CAREER_MANAGER);
        }
    }


    if ((gMemcardSetup.GetExtraOptions() & MCE_RivalFlow) != 0) {

        uiRepSheetRivalFlow::Get()->Next();

    } else if ((gMemcardSetup.GetExtraOptions() & MCE_ChallengeSeries) != 0) {
#ifdef EA_BUILD_A124 // la A124 sale al FE desde el guardado automatico y lo mira primero
        if (gMemcardSetup.GetCommand() == MCO_AutoSave) {
            new EQuitToFE(static_cast<eGarageType>(1), nullptr);
        } else
#else
        if (TheGameFlowManager.IsInFrontend())
#endif
        {
            cFEng::Get()->QueuePackagePop(1);
            if (FEDatabase->bProfileLoaded) {

                FEDatabase->ResetGameMode();
                FEDatabase->SetGameMode(eFE_GAME_MODE_CHALLENGE);

                cFEng::Get()->QueuePackageSwitch(gMemcardSetup.mToScreen, 0, 0, false);
            }
#ifndef EA_BUILD_A124
        } else {
            new EQuitToFE(static_cast<eGarageType>(1), nullptr);
#endif
        }

    } else {




        switch (gMemcardSetup.GetMethod()) {

            case MCF_Modal:
                if (m_SimPausedForMemcard) {
#ifndef EA_BUILD_A124
                    m_SimPausedForMemcard = false;
                    cFEng::Get()->QueuePackagePop(cFEng::Get()->IsPackagePushed("SMS_Mailboxes.fng") ? 1 : 0);
#else
                    new EUnPause(); // la A124 quita la pausa con el evento y baja la bandera al final
#endif
                } else {
                    cFEng::Get()->QueuePackagePop(1);
                }

                break;

            case MCF_Switch:
                cFEng::Get()->QueuePackageSwitch(gMemcardSetup.mToScreen, MemoryCard::GetInstance()->GetPlayerNum(), 0, false);
                break;





            case MCF_ModalSwitch:
                cFEng::Get()->QueuePackagePop(1);
                cFEng::Get()->QueuePackageSwitch(gMemcardSetup.mToScreen, MemoryCard::GetInstance()->GetPlayerNum(), 0, false);
                break;
        }
    }

#ifndef EA_BUILD_A124
    if (m_SimPausedForMemcard) {
        m_SimPausedForMemcard = false;
    }
#endif


    eSndAudioMode eaudiomode = g_pEAXSound->SetAudioModeFromMemoryCard(
        static_cast<eSndAudioMode>(FEDatabase->GetOptionsSettings()->TheAudioSettings.AudioMode));

    FEDatabase->GetOptionsSettings()->TheAudioSettings.AudioMode = eaudiomode;
    g_pEAXSound->UpdateVolumes(&FEDatabase->GetOptionsSettings()->TheAudioSettings, -1.0f);




    InitializeEATrax(
#ifndef EA_BUILD_A124
        true
#endif
    );







    FEPackage *pPackage = cFEng::Get()->FindPackage(gMemcardSetup.mMemScreen);

    if (pPackage != nullptr) {

        FEPackage *pParentPackage = pPackage->GetParentPackage();

        if (pParentPackage != nullptr) {


            pParentPackage->SetInputEnabled(true);
        }
    }




    gMemcardSetup.Complete(gMemcardSetup.mLastMessage);


    if (MemoryCard::GetInstance()->InBootSequence()) {





        BootFlowManager::Get()->ChangeToNextBootFlowScreen(0xff);

        MemoryCard::GetInstance()->EndBootSequence();
    }
#ifdef EA_BUILD_A124
    if (m_SimPausedForMemcard) m_SimPausedForMemcard = false;
#endif
    cFEng::Get()->QueueGameMessage(0x7E998E5E, nullptr, 0xff);

    MemoryCard::GetInstance()->SetMemcardScreenShowing(false);
#ifdef EA_PLATFORM_GAMECUBE
    MemoryCard::GetInstance()->SetMemcardScreenExiting(false);
#endif

    // El prototipo no sabe si el monitor esta encendido: lo apaga siempre.
#ifndef EA_BUILD_A124
    if (MemoryCard::GetInstance()->IsMonitorOn())
#endif
    {
        MemoryCard::GetInstance()->SetMonitor(false);
    }
}

eMenuSoundTriggers UIMemcardBase::NotifySoundMessage(u32 msg, eMenuSoundTriggers maybe) {
    if (!m_bAnyButtonVisible && (msg == FEHASH_SOUND_LEFT || msg == FEHASH_SOUND_RIGHT)) {
        return UISND_NONE;
    }

    return maybe;
}

void UIMemcardBase::NotificationMessage(u32 msg, FEObject *obj, u32 param1, u32 param2) {




    if (msg != 0x0C407210 && MemoryCard::GetInstance()->GetOp() == 0) {


        UIMemcardKeyboard::NotificationMessage(msg, obj, param1, param2);
    }


    switch (msg) {




        case 0xE1FDE1D1:

            ExitComplete();
            break;



        case 0x35F8620B:
        case 0x3A2BE557:

            InitComplete();
            break;

        case 0xDA5B8712:
            bStrCpy(m_FileName, FEngGetEditedString());
            FEDatabase->GetUserProfile(0)->SetProfileName(m_FileName, true);
            FEDatabase->DeallocBackupDB();
            FEDatabase->bProfileLoaded = true;
            DoSaveFlow(MCSF_PromptForSave);
            break;

        case 0xC9D30688:
            if (gMemcardSetup.GetCommand() == MCO_CreateNew && !FEDatabase->bProfileLoaded) {

                DoSaveFlow(MCSF_PromptForCreate);

            } else if ((gMemcardSetup.GetCommand() & 0x60) != 0 && FEDatabase->bProfileLoaded) {

                DoSaveFlow(MCSF_PromptForDestoryCreate);

            } else {











                FEPrintf(m_pDisplayMsg, "");
                m_bDelayedFailed = true;
            }
            break;

        case 0xC98356BA:
            if (m_bDelayedFailed) {

                m_bDelayedFailed = false;
                cFEng::Get()->QueueGameMessage(0x8867412D, GetPackageName(), 0xff);
            }
            break;

        case 0xC502DF5D:
            m_bInButtonAnimation = true;
            TranslateButton(obj);
            break;

        case 0x0C407210:
            m_bInButtonAnimation = false;
            gMemcardSetup.mLastController = param1;
            HandleButtonPressed(0x0C407210, obj, param1, param2, false);
            break;

        case 0xF35D144E:
            SetupPromptCorruptProfile();
            break;

        case 0x54B3AC6C:
            SetScreenVisible(false, 0);
            cFEng::Get()->QueuePackagePush("MC_List.fng", 0, 0, false);
            break;

        // Case vacio: no emite una sola instruccion, pero cuenta en el arbol
        // del switch y es lo unico que mueve el pivote del subarbol izquierdo
        // de 0x35F8620B a 0x3A2BE557, como en el original.
        case 0x8867412D:
            break;
    }
}

void UIMemcardBase::HandleButtonPressed(u32 msg, FEObject *obj, u32 param1, u32 param2, bool bPadBack) {
    bool bYes = obj->NameHash == gButtonIDs[1] && !bPadBack;


    int nPrompt = gMemcardSetup.GetPrompt();

    gMemcardSetup.ClearPrompt();
    HideAllButtons();


















    switch (nPrompt) {


        case MCP_Create: {

            MemoryCardMessage *pMsg;

            if (bYes) {

                FEDatabase->AllocBackupDB(true);
                if ((gMemcardSetup.GetExtraOptions() & MCE_PromptSaveOptions) == 0) {

                    if ((gMemcardSetup.GetExtraOptions() & MCE_PromptSaveStable) == 0) {
                        FEDatabase->DefaultProfile();
                    }
                }
                if ((gMemcardSetup.mOp & MCE_NewCareer) != 0) {

                    FEDatabase->GetCareerSettings()->StartNewCareer(false);
                }




                if (gMemcardSetup.GetCommand() == MCO_BootList) {


                    gMemcardSetup.ClearCommand();
                    gMemcardSetup.SetCommand(MCO_CreateNew);
                }

                DoSaveFlow(MCSF_AutoSaveWarning);

            } else {

                if ((gMemcardSetup.mOp & MCE_NewCareer) != 0) {

                    FEDatabase->RestoreFromBackupDB();
                }



                cFEng::Get()->QueueGameMessage(0x8867412D, GetPackageName(), 0xff);
            }

            break;
        }

        case MCP_ConfirmSave:
            if (bYes) {

                DoSaveFlow(MCSF_DisableAutoSave);


            } else {




                if (gMemcardSetup.GetCommand() == MCO_CreateNew) {

                    FEDatabase->GetGameplaySettings()->AutoSaveOn = bYes;
                }
                cFEng::Get()->QueueGameMessage(0xDC12AF2E, GetPackageName(), 0xff);
            }
            break;

        case MCP_ConfirmDestoryCreate:
            if (bYes) {

                FEDatabase->AllocBackupDB(true);
                if ((gMemcardSetup.GetExtraOptions() & MCE_PromptSaveOptions) == 0) {

                    if ((gMemcardSetup.GetExtraOptions() & MCE_PromptSaveStable) == 0) {
                        FEDatabase->DefaultProfile();
                    }
                }
                DoSaveFlow(MCSF_AutoSaveWarning);


            } else {

                MemcardExit(0x8867412D);
            }
            break;


        case MCP_ConfirmDestoryLoad:
            if (bYes) {

                InitCompleteDoList();

            } else {




                cFEng::Get()->QueueGameMessage(0x8867412D, GetPackageName(), 0xff);
            }
            break;

        case MCP_OK:
            cFEng::Get()->QueueGameMessage(0x461A18EE, GetPackageName(), 0xff);
            break;


        case MCP_AutoSaveWarning:
            DoSaveFlow(MCSF_AutoSaveWarning2);
            break;


        case MCP_AutoSaveWarning2:
            cFEng::Get()->QueuePackageMessage(0x40E73793, GetPackageName(), nullptr);
            DoSaveFlow(MCSF_PromptForKeyboard);
            break;




















        case MCP_ConfirmAutoSave:
            if (bYes) {




                FEDatabase->GetGameplaySettings()->AutoSaveOn = false;
                cFEng::Get()->QueueGameMessage(0x8867412D, GetPackageName(), 0xff);


            } else {

                MemoryCard::GetInstance()->SetRetryAutoSave(true);

                FEDatabase->GetGameplaySettings()->AutoSaveOn = true;

                gMemcardSetup.ClearCommand();
                MemoryCard::GetInstance()->ShowMessages(true);

                gMemcardSetup.SetCommand(MCO_Save);
                DoSaveFlow(MCSF_DisableAutoSave);
            }





            break;


        case MCP_ConfirmAutoSaveEnableFailed:
            if (gMemcardSetup.GetCommand() == MCO_EnableAutoSave && (gMemcardSetup.mOp & MCE_Online) == 0) {



                gMemcardSetup.ClearMethod();
                gMemcardSetup.SetMethod(MCF_Modal);
            }


            cFEng::Get()->QueueGameMessage(0x7E998E5E, nullptr, 0xff);
            cFEng::Get()->QueueGameMessage(0x461A18EE, nullptr, 0xff);
            break;


        case MCP_EnableAutoSave:
            if (bYes) {




                MemoryCard::GetInstance()->SetAutoSaveEnabled(true);


            } else {




                FEDatabase->GetGameplaySettings()->AutoSaveOn = bYes;
                cFEng::Get()->QueueGameMessage(0x7E998E5E, nullptr, 0xff);
                cFEng::Get()->QueueGameMessage(0x461A18EE, nullptr, 0xff);
            }

            break;

        case MCP_DismissMe:
        case MCP_CorruptProfile:
            cFEng::Get()->QueueGameMessage(0x8867412D, GetPackageName(), 0xff);
            break;




















        default:
            SetStringCheckingCard();

            if (MemoryCard::GetInstance()->GetPendingMessage() != nullptr) {

                ShowMessage(MemoryCard::GetInstance()->GetPendingMessage());
            }

            if (MemoryCard::GetInstance()->GetOp() == 7) {
                cFEng::Get()->QueuePackageMessage(FEHashUpper("SHOW LOADER"), GetPackageName(), nullptr);
            }
            break;
    }
}

void UIMemcardBase::HideAllButtons() {
    m_bAnyButtonVisible = false;

    for (int i = 0; i < 3; i++) {
        ShowButton(i, false, nullptr);
    }

    FEngSetScript(GetPackageName(), 0x07F9DCA9, FEHASH_HIDE, true);
}

void UIMemcardBase::ShowButton(int idx, bool bShow, short *pText) {
    if (bShow) {
        m_bAnyButtonVisible = true;

        if (pText != nullptr) {
            FEString *pObj = reinterpret_cast<FEString *>(FEngFindObject(GetPackageName(), gButtonTextIDs[idx]));

            FESetString(pObj, pText);
        }

        FEngEnableButton(GetPackageName(), gButtonIDs[idx]);
        FEngSetVisible(GetPackageName(), gButtonIDs[idx]);
        FEngSetVisible(GetPackageName(), gButtonTextIDs[idx]);
        FEngSetScript(GetPackageName(), 0x57689FDD, FEHASH_FORWARD, true);
    } else {
        FEngDisableButton(GetPackageName(), gButtonIDs[idx]);
        FEngSetInvisible(GetPackageName(), gButtonIDs[idx]);
        FEngSetInvisible(GetPackageName(), gButtonTextIDs[idx]);
    }
}

void UIMemcardBase::SetButtonText(short *b1, short *b2, short *b3) {
    int active = 0;

    if (b3 != nullptr) {
        m_nMsgOptions = 3;
        ShowButton(0, true, b1);
        ShowButton(1, true, b2);
        ShowButton(2, true, b3);
    } else if (b2 != nullptr) {
        m_nMsgOptions = 2;
        ShowButton(0, true, b1);
        ShowButton(1, true, b2);
        ShowButton(2, false, nullptr);
    } else if (b1 != nullptr) {
        m_nMsgOptions = 1;
#if defined(EA_PLATFORM_PLAYSTATION2) || defined(EA_PLATFORM_WIN32) // PC como PS2: un solo boton, el central
        ShowButton(0, false, nullptr);
        active = 1;
        ShowButton(1, true, b1);
#else
        ShowButton(0, true, b1);
        ShowButton(1, false, nullptr);
#endif
        ShowButton(2, false, nullptr);
    }

    FEngSetCurrentButton(GetPackageName(), gButtonIDs[active]);
    m_ExpectingInput = true;

    gMemcardSetup.ClearPrompt();
}

void UIMemcardBase::SetMessage(short *pMsg) {
    if (pMsg == nullptr) {
        SetMessageBlurbText("");
        HideAllButtons();
    } else {
        const u32 FEObj_MEMCARD = 0x47FF4E7C;

        SetMessageBlurbText(pMsg);
        m_pDisplayMsg->Flags |= 2;
        FEngSetScript(GetPackageName(), FEObj_MEMCARD, 0xE18DA018, true);
    }
}

void UIMemcardBase::ShowOK(unsigned int language_main, unsigned int flag) {
#if defined(EA_PLATFORM_PLAYSTATION2) || defined(EA_PLATFORM_WIN32) // PC como PS2: el OK en el boton central
    SetMessageBlurbText(language_main);

    gMemcardSetup.SetPrompt(flag);

    ShowButton(0, false, nullptr);
    ShowButton(1, true, nullptr);
    FEngSetLanguageHash(GetPackageName(), gButtonTextIDs[1], LANGUAGE_COMMON_OK);
    FEngSetCurrentButton(GetPackageName(), gButtonIDs[1]);

    ShowButton(2, false, nullptr);
    m_ExpectingInput = true;
    SetScreenVisible(true, 1);
#else
    cFEng::Get()->QueuePackageMessage(FEHashUpper("HIDE LOADER"), GetPackageName(), nullptr);

    SetMessageBlurbText(language_main);

    gMemcardSetup.SetPrompt(flag);

    ShowButton(0, true, nullptr);
    FEngSetLanguageHash(GetPackageName(), gButtonTextIDs[0], LANGUAGE_COMMON_OK);
    FEngSetCurrentButton(GetPackageName(), gButtonIDs[0]);
    ShowButton(1, false, nullptr);

    ShowButton(2, false, nullptr);
    m_ExpectingInput = true;
    SetScreenVisible(true, 1);
#endif
}

void UIMemcardBase::ShowYesNo(unsigned int language_main, unsigned int flag) {
#if !defined(EA_PLATFORM_PLAYSTATION2) && !defined(EA_PLATFORM_WIN32) // el indicador de carga es de GameCube
    cFEng::Get()->QueuePackageMessage(FEHashUpper("HIDE LOADER"), GetPackageName(), nullptr);
#endif

    SetMessageBlurbText(language_main);

    gMemcardSetup.SetPrompt(flag);

    ShowButton(0, true, nullptr);
    FEngSetLanguageHash(GetPackageName(), gButtonTextIDs[0], LANGUAGE_COMMON_NO);

    ShowButton(1, true, nullptr);
    FEngSetLanguageHash(GetPackageName(), gButtonTextIDs[1], LANGUAGE_COMMON_YES);

    FEngSetCurrentButton(GetPackageName(), gButtonIDs[0]);

    ShowButton(2, false, nullptr);
    m_ExpectingInput = true;
    SetScreenVisible(true, 2);
}

void UIMemcardBase::SetScreenVisible(bool visible, int nButtons) {
    if (m_bVisible != visible) {

        m_bVisible = visible;
        cFEng::Get()->QueuePackageMessage(visible ? 0xC0F2AE7C : 0x4F3559B5, GetPackageName(), nullptr);
        if (visible) {

            cFEng::Get()->QueuePackageMessage(FEHashUpper("INITIALIZE_SCREEN"), GetPackageName(), nullptr);
        }

        MemoryCard::GetInstance()->SetMemcardScreenInitialized(m_bVisible);
    }

#if !defined(EA_PLATFORM_PLAYSTATION2) && !defined(EA_PLATFORM_WIN32) // PC tampoco manda el N_BUTTONS
    if (visible) {
        char script[32];

        bSPrintf(script, "%d_BUTTONS", nButtons);
        cFEng::Get()->QueuePackageMessage(FEHashUpper(script), GetPackageName(), nullptr);
    }
#endif
}

void UIMemcardBase::SetIcon(unsigned int icon) {
    FEngSetTextureHash(GetPackageName(), 0x0D4F4069, icon);
    FEngSetTextureHash(GetPackageName(), 0xFAC88427, icon);
}

void UIMemcardBase::TranslateButton(FEObject *obj) {
    if (obj->Flags & 1) {
        return;
    }

    if (obj->NameHash == gButtonIDs[0]) {
        MemoryCard::GetInstance()->MessageDone(RealmcIface::CHOICE_OPTION1);
    } else if (obj->NameHash == gButtonIDs[1]) {
#if defined(EA_PLATFORM_PLAYSTATION2) || defined(EA_PLATFORM_WIN32)
        if (m_nMsgOptions == 1) {
            MemoryCard::GetInstance()->MessageDone(RealmcIface::CHOICE_OPTION1);
        } else
#endif
        {
            MemoryCard::GetInstance()->MessageDone(RealmcIface::CHOICE_OPTION2);
        }
    } else if (obj->NameHash == gButtonIDs[2]) {
        MemoryCard::GetInstance()->MessageDone(RealmcIface::CHOICE_OPTION3);
    }

    m_ExpectingInput = false;
}

// STRIPPED en GameCube (Range 0xFFFFFFFF en el DWARF); las dos PS2 la tienen
void UIMemcardBase::SetupPromptBootloadOK() {
    char fmt[512];
    char msg[512];

    ShowOK(0x253FF67A, MCP_OK);
    GetLocalizedString(fmt, 512, 0x253FF67A);
    bSPrintf(msg, fmt, m_FileName);
    SetMessageBlurbText(msg);
}

void UIMemcardBase::SetupPromptNoProfileFound() {
    ShowOK(0xBA373453, MCP_DismissMe);
}

void UIMemcardBase::SetupPromptSaveConfirm() {
    char text[512];
    const char *fmt;

    if (gMemcardSetup.GetExtraOptions() & MCE_Online) {
        fmt = GetLocalizedString(0x391A0AAC);
    } else {
        unsigned int fmtHash;

        if (gMemcardSetup.GetExtraOptions() & MCE_PromptSaveOptions) {
            fmtHash = 0xB0AF33A5;
        } else if (gMemcardSetup.GetExtraOptions() & MCE_PromptSaveStable) {
            fmtHash = 0xD80818F8;
        } else {
            fmtHash = 0x39B3CCBA;
        }

        fmt = GetLocalizedString(fmtHash);
    }

    ShowYesNo(0x39B3CCBA, MCP_ConfirmSave);

    bSPrintf(text, fmt, m_FileName, m_FileName);
    SetMessageBlurbText(text);
}

void UIMemcardBase::SetupAutoSaveOverwritePrompt() {
    char text[512];
    const char *fmt = GetLocalizedString(0xBA7A8C09);

    ShowYesNo(0xBA7A8C09, MCP_AutoSaveOverwrite);
    bSPrintf(text, fmt, m_FileName, m_FileName);
    SetMessageBlurbText(text);
}

void UIMemcardBase::SetupAutoSaveConfirmPrompt() {
    gMemcardSetup.SetPrompt(MCP_ConfirmAutoSave);

    SetMessageBlurbText(GetLocalizedString(0xA0B434A2));
    FEngEnableButton(GetPackageName(), gButtonIDs[0]);
    FEngSetVisible(GetPackageName(), gButtonIDs[0]);
    FEngSetVisible(GetPackageName(), gButtonTextIDs[0]);
    FEPrintf(GetPackageName(), gButtonTextIDs[0], GetLocalizedString(LANGUAGE_COMMON_NO));
    FEngEnableButton(GetPackageName(), gButtonIDs[1]);
    FEngSetVisible(GetPackageName(), gButtonIDs[1]);
    FEngSetVisible(GetPackageName(), gButtonTextIDs[1]);
    FEPrintf(GetPackageName(), gButtonTextIDs[1], GetLocalizedString(0x2B07A03D));
    FEngDisableButton(GetPackageName(), gButtonIDs[2]);
    FEngSetInvisible(GetPackageName(), gButtonIDs[2]);
    FEngSetInvisible(GetPackageName(), gButtonTextIDs[2]);
    FEngSetCurrentButton(GetPackageName(), gButtonIDs[0]);
    FEngSetScript(GetPackageName(), FEHashUpper("HANDLER"), FEHashUpper("FORWARD"), true);
    SetScreenVisible(true, 2);
}

void UIMemcardBase::SetupPromptForSave() {
    char text[512];
    const char *fmt;

    ShowYesNo(0x83F4BB3E, MCP_ConfirmSave);

    fmt = GetLocalizedString(gMemcardSetup.GetExtraOptions() & MCE_PromptSaveStable ? 0xD80818F8 : 0x83F4BB3E);

    bSPrintf(text, fmt, m_FileName, m_FileName);
    SetMessageBlurbText(text);
}

void UIMemcardBase::SetupPromptCorruptProfile() {
#ifdef EA_PLATFORM_PLAYSTATION2
    ShowOK(0x821E4E41, MCP_CorruptProfile); // MC_CORRUPT_PROFILE_PS2
#else
    char text[512];
    const char *fmt;

    ShowOK(0x821E4444, MCP_CorruptProfile);

    fmt = GetLocalizedString(0x821E4444);
    bSPrintf(text, fmt, m_FileName);
    SetMessageBlurbText(text);
#endif
}

void UIMemcardBase::SetupPromptAutoSaveEnableFailedNoCard() {
#ifdef EA_PLATFORM_PLAYSTATION2
    ShowOK(0x9E85C5A5, MCP_ConfirmAutoSaveEnableFailed);
#else
    ShowOK(0x9E85BBA8, MCP_ConfirmAutoSaveEnableFailed);
#endif
}

void UIMemcardBase::Setup() {
#ifdef EA_PLATFORM_PLAYSTATION2 // titulo propio de PS2 y sin el texto de la tarjeta
    FEngSetLanguageHash(GetPackageName(), 0x42ADB44C, 0x61183184);
#elif defined(EA_PLATFORM_WIN32) // PC: su titulo, tampoco el texto de la tarjeta (speed.exe 0x5442D0)
    FEngSetLanguageHash(GetPackageName(), 0x42ADB44C, 0x774E4F02);
#else
    FEngSetLanguageHash(GetPackageName(), 0x42ADB44C, 0x774E4DD9);
    FEngSetLanguageHash(m_pDisplayMsg, 0x99054304);
#endif

    MemoryCard::GetInstance()->FEngLinkObjects(this);
    SetIcon(0x6948E2B3);
}

void UIMemcardBase::SetStringCheckingCard() {
#if !defined(EA_PLATFORM_PLAYSTATION2) && !defined(EA_PLATFORM_WIN32) // PC: solo lo de abajo (MSVC la expande)
    SetScreenVisible(true, 0);
    SetMessageBlurbText(0x99054304);

    cFEng::Get()->QueuePackageMessage(FEHashUpper("0_BUTTONS"), GetPackageName(), nullptr);
#elif defined(EA_BUILD_A124)
    SetMessageBlurbText(" ");
#endif

    HideAllButtons();

    m_ExpectingInput = false;
}

void UIMemcardBase::ShowKeyboard() {
#ifdef EA_PLATFORM_WIN32 // PC deja a la vista el boton 1 y cambia su texto (speed.exe 0x544310)
    ShowButton(1, true, nullptr);
    FEngSetLanguageHash(GetPackageName(), gButtonTextIDs[1], 0x4F876B6F);
    FEngSetCurrentButton(GetPackageName(), gButtonIDs[1]);
    ShowButton(0, false, nullptr);
    ShowButton(2, false, nullptr);
#else
    SetScreenVisible(false, 0);
    HideAllButtons();
#endif

    UIMemcardKeyboard::ShowKeyboard();
}

void UIMemcardBase::DoSaveFlow(int flow) {
    if (flow != 0) {

        m_Flow = flow;
    } else {

        if (!FEDatabase->GetUserProfile(0)->IsProfileNamed()) {

            m_Flow = MCSF_PromptForCreate;
        }
    }

    switch (m_Flow) {
        case MCSF_PromptForExceedingLimit:
            ShowOK(0xD9783C57, MCP_DismissMe);
            break;

        case MCSF_PromptForDestoryCreate:
            ShowYesNo(0x7209349F, MCP_ConfirmDestoryCreate);
            break;

        case MCSF_PromptForCreate: {
            unsigned int msg;

            if (gMemcardSetup.GetExtraOptions() & MCE_NewCareer) {
                msg = 0xBADD522C;
            } else if (gMemcardSetup.GetExtraOptions() & MCE_ChallengeSeries) {
                msg = 0x93C25B3D;
            } else if (gMemcardSetup.GetExtraOptions() & MCE_Online) {
                msg = 0xF8448956;
            } else {
                msg = 0xBE97590F;
            }

            ShowYesNo(msg, MCP_Create);
            break;
        }

        case MCSF_PromptForKeyboard:
            ShowKeyboard();
            break;

        case MCSF_PromptForConfirm:
            SetupPromptSaveConfirm();
            break;

        case MCSF_PromptForSave:
            SetupPromptForSave();
            break;

        case MCSF_DisableAutoSave:
            MemoryCard::GetInstance()->SetAutoSaveEnabled(false);
            break;

        case MCSF_DoSave:
            FEDatabase->GetUserProfile(0)->SetProfileName(m_FileName, true);
            MemoryCard::GetInstance()->Save(m_FileName);

#if !defined(EA_PLATFORM_PLAYSTATION2) && !defined(EA_PLATFORM_WIN32)
            SetStringCheckingCard();
#endif

            break;

        case MCSF_AutoSaveWarning:
            cFEng::Get()->QueuePackageMessage(0x001C8ACE, GetPackageName(), nullptr);

#if defined(EA_PLATFORM_PLAYSTATION2) || defined(EA_PLATFORM_WIN32)
            ShowOK(GetAutoSaveWarning(), MCP_AutoSaveWarning);
#else
            ShowOK(GetAutoSaveWarning(), MCP_AutoSaveWarning2);
#endif

            break;

        case MCSF_AutoSaveWarning2:
            ShowOK(GetAutoSaveWarning2(), MCP_AutoSaveWarning2);
            break;
    }
}

void UIMemcardBase::SetMessageBlurbText(short *pText) {
    FESetString(m_pDisplayMsg, pText);
    if (m_pDisplayMsgShadow != nullptr) {
        FESetString(m_pDisplayMsgShadow, pText);
    }
    FindScreenSize(reinterpret_cast<const wchar_t *>(pText));
}

void UIMemcardBase::SetMessageBlurbText(char *pText) {
    wchar_t wText[1024];

    FEPrintf(m_pDisplayMsg, pText);
    if (m_pDisplayMsgShadow != nullptr) {
        FEPrintf(m_pDisplayMsgShadow, pText);
    }

    bStrCpy(reinterpret_cast<unsigned short *>(wText), pText);
    FindScreenSize(wText);
}

void UIMemcardBase::SetMessageBlurbText(unsigned int textHash) {
    wchar_t wText[1024];

    FEngSetLanguageHash(m_pDisplayMsg, textHash);
    if (m_pDisplayMsgShadow != nullptr) {
        FEngSetLanguageHash(m_pDisplayMsgShadow, textHash);
    }
    char *pText = GetLocalizedString(textHash);

    bStrCpy(reinterpret_cast<unsigned short *>(wText), pText);
    FindScreenSize(wText);
}

void UIMemcardBase::FindScreenSize(const wchar_t *msg) {
    const float MAX_SIZE_MED = 4400.0f;
    FEngFont *font = FindFont(0x545570C6);
    float numLines = bStrLen(reinterpret_cast<const unsigned short *>(msg)) * font->GetHeight();
    const float MAX_SIZE_SMALL = 2200.0f;
    const u32 FEObj_dialogsmall = 0x79B0C1C7;
    const u32 FEObj_dialogmedium = 0xA13ADCAF;
    const u32 FEObj_dialoglarge = 0x792BC959;

    if (numLines < MAX_SIZE_SMALL) {
        cFEng::Get()->QueuePackageMessage(FEObj_dialogsmall, GetPackageName(), nullptr);
    } else if (numLines < MAX_SIZE_MED) {
        cFEng::Get()->QueuePackageMessage(FEObj_dialogmedium, GetPackageName(), nullptr);
    } else {
        cFEng::Get()->QueuePackageMessage(FEObj_dialoglarge, GetPackageName(), nullptr);
    }
}

unsigned int UIMemcardBase::GetAutoSaveWarning() {
#ifdef EA_PLATFORM_PLAYSTATION2
    return LANGUAGE_MC_AUTOSAVE_WARNING_PS2;
#elif defined(EA_PLATFORM_WIN32)
    return LANGUAGE_MC_AUTOSAVE_WARNING_PC;
#else
    return LANGUAGE_MC_AUTOSAVE_WARNING_NGC;
#endif
}

unsigned int UIMemcardBase::GetAutoSaveWarning2() {
#ifdef EA_PLATFORM_PLAYSTATION2
    return LANGUAGE_MC_AUTOSAVE_WARNING2_PS2;
#elif defined(EA_PLATFORM_WIN32)
    return LANGUAGE_MC_AUTOSAVE_WARNING2_PC;
#else
    return 0x2386F454;
#endif
}

void UIMemcardBase::ShowMessage(MemoryCardMessage *msg) {
    ShowMessage(msg->mMsg, msg->mnOptions, msg->mOptions[0], msg->mOptions[1], msg->mOptions[2]);
    MemoryCard::GetInstance()->ReleasePendingMessage();
}

void UIMemcardBase::ShowMessage(const wchar_t *msg, unsigned int nOptions, const wchar_t *option1, const wchar_t *option2,
                                const wchar_t *option3) {

    PopChild();

    HideAllButtons();
    SetMessage(reinterpret_cast<short *>(const_cast<wchar_t *>(msg)));

    switch (nOptions) {

        case 1:
            SetButtonText(reinterpret_cast<short *>(const_cast<wchar_t *>(option1)), nullptr, nullptr);
            break;

        case 2:
            SetButtonText(reinterpret_cast<short *>(const_cast<wchar_t *>(option1)), reinterpret_cast<short *>(const_cast<wchar_t *>(option2)),
                          nullptr);
            break;

        case 3:
            SetButtonText(reinterpret_cast<short *>(const_cast<wchar_t *>(option1)), reinterpret_cast<short *>(const_cast<wchar_t *>(option2)),
                          reinterpret_cast<short *>(const_cast<wchar_t *>(option3)));
            break;

        // Los dos comparten etiqueta con el `default`, asi que no emiten ni una
        // instruccion, pero cuentan en el arbol del switch: con cinco nodos el
        // pivote cae en 2 y el nodo 1 lleva su comprobacion de cota inferior,
        // como en el original.
        case 0:
        case 4:
        default:
            MemoryCard::GetInstance()->SetWaitingForResponse(false);
            break;
    }

    SetScreenVisible(true, nOptions);

#ifndef EA_PLATFORM_PLAYSTATION2
    cFEng::Get()->QueuePackageMessage(FEHashUpper(nOptions == 0 ? "SHOW LOADER" : "HIDE LOADER"), GetPackageName(), nullptr);
#endif
}

void UIMemcardBase::ActivateChild() {
    MemoryCard::GetInstance()->SetMonitor(true);
}

void UIMemcardBase::PopChild() {
    if (m_pChild != nullptr) {

        if (cFEng::Get()->IsPackagePushed("MC_List.fng")) {
            cFEng::Get()->QueuePackagePop(1);
        }
    }

    m_pChild = nullptr;
}

void UIMemcardBase::HandleAutoSaveError() {
#ifdef EA_BUILD_A124
    // La A124 pausa la simulacion aqui y siempre pasa a modal.
    new EPauseForMemcard();
    m_SimPausedForMemcard = true;
#endif
    if (!MemoryCard::GetInstance()->IsCheckingCardForAutoSave() && !MemoryCard::GetInstance()->IsCheckingCardForOverwrite()) {

#ifdef EA_BUILD_A124
        gMemcardSetup.ClearMethod();
        gMemcardSetup.ClearCommand();
        gMemcardSetup.SetMethod(MCF_Modal);
#else
        if (gMemcardSetup.GetCommand() != MCO_AutoSave) {
            gMemcardSetup.ClearMethod();
            gMemcardSetup.SetMethod(MCF_Modal);
        }

        gMemcardSetup.ClearCommand();
#endif
        gMemcardSetup.SetCommand(MCO_Save);
    }

    bStrCpy(m_FileName, FEDatabase->GetUserProfile(0)->GetProfileName());
    if (MemoryCard::GetInstance()->IsCheckingCardForAutoSave() || MemoryCard::GetInstance()->IsCheckingCardForOverwrite() ||
        MemoryCard::GetInstance()->WasCardRemovedWithAutoSaveEnabled()) {

        MemoryCard::GetInstance()->ReleasePendingMessage();

#ifdef EA_BUILD_A124
        MemoryCard::GetInstance()->SetCardRemovedWithAutoSaveEnabled(false);
        ShowYesNo(0xA0B43E9F, MCP_ConfirmAutoSave);
#else
#if defined(EA_PLATFORM_PLAYSTATION2) || defined(EA_PLATFORM_WIN32) // PS2 y PC preguntan con el de tarjeta quitada o el de autoguardado
        ShowYesNo(MemoryCard::GetInstance()->WasCardRemovedWithAutoSaveEnabled() ? 0xA0B43E9F : 0xF1F3B4BA, MCP_ConfirmAutoSave);
#else
        SetupAutoSaveConfirmPrompt();
#endif
        MemoryCard::GetInstance()->SetCardRemovedWithAutoSaveEnabled(false);
#endif

    } else {

        MemoryCard::GetInstance()->SetRetryAutoSave(true);

        ShowMessage(MemoryCard::GetInstance()->GetPendingMessage());
    }

    MemoryCard::GetInstance()->EndAutoSave();
}

void UIMemcardBase::HandleAutoSaveOverwriteMessage() {
#ifdef EA_BUILD_A124
    new EPauseForMemcard();
    m_SimPausedForMemcard = true;
    bStrCpy(m_FileName, FEDatabase->GetUserProfile(0)->GetProfileName());
    SetupAutoSaveOverwritePrompt();
#else
    bStrCpy(m_FileName, FEDatabase->GetUserProfile(0)->GetProfileName());

    MemoryCard::GetInstance()->EndAutoSave();

#if !defined(EA_PLATFORM_PLAYSTATION2) && !defined(EA_PLATFORM_WIN32) // solo GameCube (speed.exe 0x544510)
    FEDatabase->bAutoSaveOverwriteConfirmed = true;
#endif

    gMemcardSetup.ClearCommand();
    gMemcardSetup.SetCommand(MCO_Save);

    MemoryCard::GetInstance()->ShowMessages(true);
    DoSaveFlow(MCSF_DisableAutoSave);
#endif
}

static unsigned int sOpName[] = {0x841C21AF, 0xE85326E2};

UIMemcardList::UIMemcardList(ScreenConstructorData *sd)
    : MenuScreen(sd), m_SaveGameList(GetPackageName(), "", "Scrollbar", true, true, false, false), m_Initialized(0) {

    FEPrintf(GetPackageName(), 0xEB406FEC, FEDatabase->GetUserProfile(0)->GetProfileName());

    for (int i = 1; i <= 8; i++) {

        ScrollerSlot *pSlot = new ("ScrollerSlot", 0) ScrollerSlot;

        char name[32];

        FEngSNPrintf(name, 32, "option_name_%d", i);
        pSlot->AddData(FEngFindString(GetPackageName(), FEHashUpper(name)));

        FEngSNPrintf(name, 32, "option_data_%d", i);
        pSlot->AddData(FEngFindString(GetPackageName(), FEHashUpper(name)));

        FEngSNPrintf(name, 32, "option_mouse_%d", i);
        pSlot->SetBacking(FEngFindObject(GetPackageName(), FEHashUpper(name)));

        pSlot->Hide();
        m_SaveGameList.AddSlot(pSlot);
    }

    m_ListOp = gMemcardSetup.GetCommand() == MCO_DeleteList;
    FEngSetLanguageHash(GetPackageName(), 0x48D4FCAE, sOpName[m_ListOp]);
    FEngSetLanguageHash(GetPackageName(), 0x426C7B4D, sOpName[m_ListOp]);
}

UIMemcardList::~UIMemcardList() {}

MenuScreen *CreateMemcardListFiles(ScreenConstructorData *sd) {
    UIMemcardList *pRes = new ("UIMemcardList", 0) UIMemcardList(sd);

    static_cast<UIMemcardMain *>(MemoryCard::GetInstance()->GetScreen())->SetPopupWindow(pRes);

    return pRes;
}

void UIMemcardList::NotificationMessage(u32 msg, FEObject *obj, u32 param1, u32 param2) {
    switch (msg) {

        case 0x35F8620B: {

            m_SaveGameList.SetSelected(m_SaveGameList.Slots.GetHead());
            m_SaveGameList.HighlightSelected();

            UIMemcardBase *pParent = MemoryCard::GetInstance()->GetScreen();

            pParent->m_ExpectingInput = true;

            m_Initialized++;

            if (MemoryCard::GetInstance()->InBootSequence()) {

                FEngSetLanguageHash(GetPackageName(), FEHASH_BUTTON2, LANGUAGE_COMMON_CANCEL);
            }

            break;
        }

        case 0xC98356BA:
            if (m_Initialized == 0) {

                m_Initialized = 1;

                UIMemcardBase *pParent = MemoryCard::GetInstance()->GetScreen();

                UIMemcardBase::Item *pNode = pParent->m_Items.GetHead();

                while (pNode != pParent->m_Items.EndOfList()) {

                    const char *pName = pNode->m_Name + MemoryCard::GetInstance()->GetPrefixLength();

                    if (pParent->IsProfile(pName)) {

                        AddItem(pName, pNode->m_Data, pNode->m_Size, pNode->m_Flag);
                    }

                    pNode = pNode->GetNext();
                }


                FEngSetScript("MC_List.fng", 0x47FF4E7C, FEHASH_CALL, true);
            }
            break;

        case 0x911AB364:
            if (MemoryCard::GetInstance()->InBootSequence()) {

                cFEng::Get()->QueueGameMessage(0x8D0CC9F9, "MC_Main_GC.fng", 0xff);




            } else {

                cFEng::Get()->QueueGameMessage(0x8867412D, MemoryCard::GetInstance()->GetScreen()->GetPackageName(), 0xff);
            }


            gMemcardSetup.mLastController = param1;
            break;

        case 0x72619778:
            gMemcardSetup.mLastController = param1;
            m_SaveGameList.ScrollPrev();
            break;

        case 0x911C0A4B:
            gMemcardSetup.mLastController = param1;
            m_SaveGameList.ScrollNext();
            break;

        case 0x406415E3:
            gMemcardSetup.mLastController = param1;
            if (!FEDatabase->IsSplitScreenMode()) {

                FEDatabase->SetPlayersJoystickPort(MemoryCard::GetInstance()->GetPlayerNum(), FEngMapJoyParamToJoyport(param1));
            }
            MemoryCard::GetInstance()->SetMonitor(false);

            break;

        case 0xEB29392A:
            if (m_LastMsg == 0x406415E3) {
                static_cast<FEMemWidget *>(m_SaveGameList.GetSelectedDatum())->Act(GetPackageName(), 0);
            }
            break;
    }

    m_LastMsg = msg;
}


FEMemWidget *UIMemcardList::AddItem(const char *pName, const char *pDate, int size, int flag) {
    FEMemWidget *pWidget = new ("FEMemWidget", 0) FEMemWidget(pName, pDate, size, flag);

    m_SaveGameList.AddData(pWidget);
    m_SaveGameList.Enable(pWidget);
    m_SaveGameList.Update(true);

    return pWidget;
}
