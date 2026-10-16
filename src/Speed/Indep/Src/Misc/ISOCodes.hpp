//
//
//
//
//
//
//
//
//
#ifndef __ISOCODES_HPP__
#define __ISOCODES_HPP__

// total size: 0x1
// Decl: 20
class ISOCodes {
  public:
#ifdef EA_BUILD_A124
    // PS2: non-const, as the online code (WebOfferUG2) stores them in char pointers
    static char *GetLanguageISOCode();
    static char *GetCountryISOCode();
#else
    static const char *GetLanguageISOCode(); // Decl: 22
    static const char *GetCountryISOCode();  // Decl: 23
#endif
};

#endif
