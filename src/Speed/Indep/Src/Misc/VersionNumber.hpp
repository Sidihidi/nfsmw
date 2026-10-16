//
//
//
//
//
//
//
//
//
#ifndef __VERSIONNUMBER_HPP__
#define __VERSIONNUMBER_HPP__

static const int GAME_VERSION_NUMBER_MAJOR = 1; // Decl: 17
#ifdef EA_PLATFORM_WIN32 // speed.exe es la 1.3 (push 3 en 0x64BAD0 y en 0x64BB20)
static const int GAME_VERSION_NUMBER_MINOR = 3;
#else
static const int GAME_VERSION_NUMBER_MINOR = 2; // Decl: 18
#endif

void GetGameVersionNumberString(char *buffer, int bufsize);

#endif
