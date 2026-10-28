#include "SubTitle.hpp"
#include "MoviePlayer/MoviePlayer.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterface.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterfaceFEObjects.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterfaceFEStrings.hpp"
#include "Speed/Indep/Src/Frontend/Localization/Localize.hpp"
#include "Speed/Indep/Src/Misc/Timer.hpp"
#include "Speed/Indep/Src/Misc/bFile.hpp"
#include "Speed/Indep/bWare/Inc/bPrintf.hpp"
#include "Speed/Indep/bWare/Inc/bWare.hpp"
#ifdef EA_BUILD_A124
void ScreenPrintf(int x, int y, const char *fmt, ...);
#endif

extern bool IsMovieTimerPrintf;

SubTitler *SubTitler::gCurrentSubtitler_ = nullptr;

SubTitler::SubTitler() {
    next_ = 0;
    data_ = nullptr;
    str_ = nullptr;
#ifndef EA_BUILD_A124
    str2_ = nullptr;
#endif
    back_ = nullptr;
    gCurrentSubtitler_ = this;
    timeElapsed = 0.0f;
    lastTime = 0;
    mSubtitlePaused = false;
}

SubTitler::~SubTitler() {
    Unload();
    gCurrentSubtitler_ = nullptr;
}

bool SubTitler::ShouldShowSubTitles(const char *movie_name) {
#ifndef EA_BUILD_A124
    if (GetCurrentLanguage() == 0 && !mIsTutorial) {
#else
    if (GetCurrentLanguage() == 0) {
#endif
        return false;
    }
    return true;
}

void SubTitler::BeginningMovie(const char *moviename, const char *packagename) {
#ifndef EA_BUILD_A124
    SetIsTutorialMovie(moviename);
#endif
    if (ShouldShowSubTitles(moviename)) {
        Load(moviename, packagename);
    }
}

void SubTitler::Load(const char *movieName, const char *packageName) {
    char filename[64];

    Unload();
    if (movieName != nullptr) {
        bSNPrintf(filename, 64, "SUBTITLES\\%s", movieName);
        data_ = static_cast<SubtitleInfo *>(bGetFile(filename, 0, 0));
        if (data_ != nullptr) {
            next_ = 0;
            timeElapsed = 0.0f;
            lastTime = 0;
            for (int i = 0; data_[i].startTime != 0xFFFF; i++) {
                bPlatEndianSwap(&data_[i].startTime);
                bPlatEndianSwap(&data_[i].stringHash);
            }
            str_ = FEngFindString(packageName, 0x599B8442);
#ifndef EA_BUILD_A124
            str2_ = FEngFindString(packageName, 0x2E8DA933);
#endif
            back_ = FEngFindObject(packageName, 0x8BD49BCC);
            FEPrintf(str_, "");
            FEngSetScript(str_, FEngHashString("SHOWSUBS"), true);
#ifndef EA_BUILD_A124
            FEPrintf(str2_, "");
            FEngSetScript(str2_, FEngHashString("SHOWSUBS"), true);
#endif
        }
    }
}

void SubTitler::Unload() {
    if (data_ != nullptr) {
        bFree(data_);
        data_ = nullptr;
    }
}

float SubTitler::GetElapsedTime() {
    unsigned int timenow;
    float thetime_ms;
    if (mSubtitlePaused) {
        lastTime = bGetTicker();
        return timeElapsed;
    }
    timenow = bGetTicker();
    thetime_ms = bGetTickerDifference(lastTime, timenow);
    timeElapsed += thetime_ms * 0.001f;
    lastTime = timenow;
    return timeElapsed;
}

void SubTitler::Update(uint32 msg) {
    if (gMoviePlayer != nullptr) {
        if (gMoviePlayer->IsMoviePaused()) {
            mSubtitlePaused = true;
        } else {
            mSubtitlePaused = false;
        }
        if (msg == 0xC98356BA) {
            if (data_ != nullptr && lastTime != 0) {
                float timenow = GetElapsedTime();
                if (IsMovieTimerPrintf) {
                    Timer timer(bFloatToInt(timenow * 4000.0f + 0.5f));
                    char timer_str[100];
                    timer.PrintToString(timer_str, 0);
#ifdef EA_BUILD_A124
                    ScreenPrintf(0, -220, "TIME : %s", timer_str);
#endif
                }
                unsigned short delta = static_cast<unsigned short>(timenow * 10.0f);
                if (data_[next_].startTime <= delta) {
#ifdef EA_BUILD_A124
                    if (data_[next_].stringHash != 0x1A20BA && bStrCmp("", GetLocalizedString(data_[next_].stringHash)) != 0) {
                        FEngSetLanguageHash(str_, data_[next_].stringHash);
                        FEngSetTopLeftY(back_, FEngGetTopLeftY(str_));
                    } else {
                        FEngSetTopLeftY(back_, 6000.0f);
                        FEPrintf(str_, "");
                    }
#else
                    RefreshText();
#endif
                    next_++;
                }
            }
        } else if (msg == 0xC3960EB9) {
            Unload();
        }
    }
}

void SubTitler::Start() {
    lastTime = bGetTicker();
}

void SubTitler::NotifyFirstFrame() {
    if (gCurrentSubtitler_ != nullptr) {
        gCurrentSubtitler_->Start();
    }
}

#ifndef EA_BUILD_A124
void SubTitler::RefreshText() {
#ifndef EA_BUILD_A124
    if (!mIsTutorial)
#endif
    {
        if (data_[next_].stringHash != 0x1A20BA && bStrCmp("", GetLocalizedString(data_[next_].stringHash)) != 0) {
            FEngSetLanguageHash(str_, data_[next_].stringHash);
            FEngSetTopLeftY(back_, FEngGetTopLeftY(str_));
        } else {
            FEngSetTopLeftY(back_, 6000.0f);
            FEPrintf(str_, "");
        }
    }
#ifndef EA_BUILD_A124
    else {
        if (data_[next_].stringHash != 0x1A20BA) {
            FEngSetScript(str_, 0x16A259, true);
            FEngSetScript(str2_, 0x16A259, true);
            unsigned int text_hash = bStringHash("_A", data_[next_].stringHash);
            if (DoesStringExist(text_hash)) {
                FEngSetLanguageHash(str_, text_hash);
                FEngSetScript(str_, 0xBCBF0306, true);
            }
            text_hash = bStringHash("_B", data_[next_].stringHash);
            if (DoesStringExist(text_hash)) {
                FEngSetLanguageHash(str2_, text_hash);
                FEngSetScript(str2_, 0xBCBF0306, true);
            }
        } else {
            cFEng::Get()->QueuePackageMessage(0xDBDF2888, nullptr, nullptr);
        }
    }
#endif
}

#endif

#ifndef EA_BUILD_A124
void SubTitler::SetIsTutorialMovie(const char *movieName) {
    if (bStrCmp(movieName, "drag_tutorial") == 0 || bStrCmp(movieName, "speedtrap_tutorial") == 0 || bStrCmp(movieName, "tollbooth_tutorial") == 0 ||
        bStrCmp(movieName, "bounty_tutorial") == 0 || bStrCmp(movieName, "pursuit_tutorial") == 0) {
        mIsTutorial = true;
    } else {
        mIsTutorial = false;
    }
}
#endif
