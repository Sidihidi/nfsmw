#include "Speed/Indep/Src/Frontend/MenuScreens/Common/UIDateMenu.hpp"
#include "Speed/Indep/Src/FEng/FEList.h"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterface.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterfaceFEObjects.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterfaceFEStrings.hpp"
#include "Speed/Indep/bWare/Inc/bPrintf.hpp"

UIDateMenu *UIDateMenu::pCurrentDate = nullptr;

UIDateMenu::UIDateMenu(ScreenConstructorData *sd)
    : MenuScreen(sd),  //
      pDay(nullptr),   //
      pMonth(nullptr), //
      pYear(nullptr),  //
      Day(31),         //
      Month(12),       //
      Year(1980),      //
      m_MaxYear(2015) {
    pWidgets[0] = static_cast<FEString *>(FEngFindObject(GetPackageName(), FEHashUpper("DATA_1")));
    pWidgets[1] = static_cast<FEString *>(FEngFindObject(GetPackageName(), FEHashUpper("DATA_2")));
    pWidgets[2] = static_cast<FEString *>(FEngFindObject(GetPackageName(), FEHashUpper("DATA_3")));
    Index = 0;

    pMonth = pWidgets[0];
    pDay = pWidgets[1];
    pYear = pWidgets[2];

    pWidget = sd ? reinterpret_cast<FEDateWidget *>(sd->Arg) : nullptr;
    if (pWidget) {
        Day = pWidget->mDay;
        Month = pWidget->mMonth;
        Year = pWidget->mYear;
        m_MaxYear = pWidget->GetMaxYear();

        FEString *pTitle = static_cast<FEString *>(FEngFindObject(GetPackageName(), 0x42adb44c));
        FEPrintf(pTitle, pWidget->GetHeaderText());
    }

    for (int i = 0; i < 6; i++) {
        char ButtonName[64];
        bSPrintf(ButtonName, "ScrollBar_Arrow_%d", i + 1);
        pBlinkies[i] = FEngFindObject(GetPackageName(), FEHashUpper(ButtonName));
    }

    UpdateDate(-1);
    pCurrentDate = this;
}

void UIDateMenu::UpdateDate(int ArrowIndex) {
    switch (Month) {
        case 4:
        case 6:
        case 9:
        case 11:
            if (Day > 30) {
                Day = 30;
            }
            break;
        case 2:
            if (Day > 28) {
                if (Year % 4 == 0 && (Year % 100 != 0 || Year % 400 == 0)) {
                    Day = 29;
                } else {
                    Day = 28;
                }
            }
            break;
    }

    FEPrintf(pDay, "%d", Day);
    FEPrintf(pMonth, "%d", Month);
    FEPrintf(pYear, "%d", Year);

    if (ArrowIndex >= 0 && ArrowIndex <= 6) {
        FEngSetScript(pBlinkies[ArrowIndex], 0x0280164f, true);
    }
}

void UIDateMenu::NotificationMessage(u32 msg, FEObject *pobj, u32 param1, u32 param2) {
    switch (msg) {
        case 0x35f8620b:
            FEngSetScript(pWidgets[Index], 0x249db7b7, true);
            break;

        case 0x0c407210:
        case 0x406415e3:
            if (pWidget && pWidget->mPkgName) {
                pWidget->SetDate(Day, Month, Year);
                cFEng::Get()->QueueGameMessage(0xda52af6b, pWidget->mPkgName, 0xff);
            }
            cFEng::Get()->QueuePackagePop(1);
            break;

        case 0x911ab364:
            if (pWidget && pWidget->mPkgName) {
                cFEng::Get()->QueueGameMessage(0xaadd0187, pWidget->mPkgName, 0xff);
            }
            cFEng::Get()->QueuePackagePop(1);
            break;

        case 0x9120409e:
            ScrollHorizontal(eSD_PREV);
            break;

        case 0xb5971bf1:
            ScrollHorizontal(eSD_NEXT);
            break;

        case 0x72619778:
            ScrollVertical(eSD_NEXT);
            break;

        case 0x911c0a4b:
            ScrollVertical(eSD_PREV);
            break;

        case 0x84378bef:
            break;
    }
}

void UIDateMenu::ScrollHorizontal(eScrollDir dir) {
    if (dir == eSD_NEXT) {
        FEngSetScript(pWidgets[Index], 0x7ab5521a, true);
        Index++;
        if (Index > 2) {
            Index = 0;
        }
        FEngSetScript(pWidgets[Index], 0x249db7b7, true);
    } else if (dir == eSD_PREV) {
        FEngSetScript(pWidgets[Index], 0x7ab5521a, true);
        Index--;
        if (Index < 0) {
            Index = 2;
        }
        FEngSetScript(pWidgets[Index], 0x249db7b7, true);
    }
}

void UIDateMenu::ScrollVertical(eScrollDir dir) {
    if (dir == eSD_NEXT) {
        if (pWidgets[Index] == pMonth) {
            Month++;
            if (Month > 12) {
                Month = 1;
            }
        } else if (pWidgets[Index] == pDay) {
            Day++;
            switch (Month) {
                case 1:
                case 3:
                case 5:
                case 7:
                case 8:
                case 10:
                case 12:
                    if (Day > 31) {
                        Day = 1;
                    }
                    break;
                case 4:
                case 6:
                case 9:
                case 11:
                    if (Day > 30) {
                        Day = 1;
                    }
                    break;
                case 2:
                    if (Day == 29) {
                        if (!(Year % 4 == 0 && (Year % 100 != 0 || Year % 400 == 0))) {
                            Day = 1;
                        }
                    } else if (Day > 29) {
                        Day = 1;
                    }
                    break;
            }
        } else {
            Year++;
            if (Year > m_MaxYear) {
                Year = 1900;
            }
        }
        UpdateDate(Index * 2);
    } else if (dir == eSD_PREV) {
        if (pWidgets[Index] == pMonth) {
            Month--;
            if (Month < 1) {
                Month = 12;
            }
        } else if (pWidgets[Index] == pDay) {
            Day--;
            if (Day < 1) {
                switch (Month) {
                    case 1:
                    case 3:
                    case 5:
                    case 7:
                    case 8:
                    case 10:
                    case 12:
                        Day = 31;
                        break;
                    case 4:
                    case 6:
                    case 9:
                    case 11:
                        Day = 30;
                        break;
                    case 2:
                        if (Year % 4 == 0 && (Year % 100 != 0 || Year % 400 == 0)) {
                            Day = 29;
                        } else {
                            Day = 28;
                        }
                        break;
                }
            }
        } else {
            Year--;
            if (Year < 1900) {
                Year = m_MaxYear;
            }
        }
        UpdateDate(Index * 2 + 1);
    }
}

MenuScreen *CreateUIDateMenu(ScreenConstructorData *sd) {
    return new ("UIDateMenu", 0) UIDateMenu(sd);
}
