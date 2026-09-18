#ifndef __SUBTITLE_HPP__
#define __SUBTITLE_HPP__

#include <types.h>
#include "Speed/Indep/Src/FEng/FEString.h"
#include "Speed/Indep/Src/Frontend/SubtitleInfo.hpp"

// File: speed/indep/src/frontend/Subtitle.hpp
// total size: 0x24
// Decl: speed/indep/src/frontend/Subtitle.hpp:15
class SubTitler {
  private:
    unsigned int next_;    // offset 0x0, size 0x4
    SubtitleInfo *data_;   // offset 0x4, size 0x4
    FEString *str_;        // offset 0x8, size 0x4
    // La A124 no lo tiene: sus stabs pasan de str_ (0x8) a back_ (0xC).
#ifndef EA_BUILD_A124
    FEString *str2_;       // offset 0xC, size 0x4
#endif
    FEObject *back_;       // offset 0x10, size 0x4
    float timeElapsed;     // offset 0x14, size 0x4
    unsigned int lastTime; // offset 0x18, size 0x4
    // La A124 no lo tiene: sus stabs pasan de lastTime (0x14) a mSubtitlePaused (0x18).
#ifndef EA_BUILD_A124
    bool mIsTutorial;      // offset 0x1C, size 0x1
#endif
    bool mSubtitlePaused;  // offset 0x20, size 0x1

    static SubTitler *gCurrentSubtitler_; // size: 0x4, address: 0x8041BBB4

  public:
    SubTitler();
    ~SubTitler();

    void BeginningMovie(const char *moviename, const char *packagename);
    void Load(const char *movieName, const char *packageName);
    void Unload();
    void Update(uint32 msg);
    bool ShouldShowSubTitles(const char *movie_name);
    static void NotifyFirstFrame();
    float GetElapsedTime();
    void RefreshText();
    // La A124 no la tiene: ni en sus stabs ni en su tabla de simbolos; solo fija mIsTutorial.
#ifndef EA_BUILD_A124
    void SetIsTutorialMovie(const char *movieName);
#endif

  private:
    void Start();
};

#endif
