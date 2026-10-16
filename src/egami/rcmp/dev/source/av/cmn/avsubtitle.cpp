// RCMP::AV_SUBTITLE (egami rcmp av): subtitulos de video. Layout, tipos y
// valores del enum de symbols/PS2/PS2_types.nothpp (AV_SUBTITLE 0x34 B) y de
// los stabs de la A124; valores del ctor del asm original (avsubtitle 0x4bd950).
// AV_SUBTITLE_ARRAY::Init y sus ayudantes: locales y bloques del DWARF de GameCube
// (estripados alli, Range 0xFFFFFFFF) y de los stabs de la A124; cadenas del .rodata
// de la A124 y de SLUS.

#ifdef EA_PLATFORM_PLAYSTATION2
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#endif

namespace RCMP {

class RCMP_SYSTEM {
  public:
    void *(*AllocMemFunc)(const char *, int, int, int, int);
    void (*FreeMemFunc)(void *);
    int m_DefaultMemDir;

    void *AllocMem(const char *name, unsigned int size, int alignment, int headersize, int type) {
        return AllocMemFunc(name, size, alignment, headersize, type);
    }
    void FreeMem(void *memadr) {
        FreeMemFunc(memadr);
    }
};

extern RCMP_SYSTEM rcmp_sys;

class AV_SUBTITLE {
  public:
    enum JUSTIFY_ENUM {
        JUST_RIGHT = 0,
        JUST_TOP = 1,
        JUST_CENTER = 2,
        JUST_LEFT = 3,
        JUST_BOTTOM = 4,
    };

#ifdef EA_PLATFORM_PLAYSTATION2
    static void *operator new(unsigned int size) {
        return rcmp_sys.AllocMem("", size, 0, 0, rcmp_sys.m_DefaultMemDir);
    }
    static void *operator new[](unsigned int size) {
        return rcmp_sys.AllocMem("", size, 0, 0, rcmp_sys.m_DefaultMemDir);
    }
    static void operator delete(void *ptr) {
        rcmp_sys.FreeMem(ptr);
    }
    static void operator delete[](void *ptr) {
        rcmp_sys.FreeMem(ptr);
    }
#endif

    AV_SUBTITLE();
    ~AV_SUBTITLE() {}

    void SetPosX(int x);
    void SetPosY(int y);
    void SetWidth(unsigned int w);
    void SetHeight(unsigned int h);
    void SetStrID(unsigned int id);
    void SetFrameNumber(unsigned int n);
    void SetJustifyX(JUSTIFY_ENUM jx);
    void SetJustifyY(JUSTIFY_ENUM jy);
    void SetShapeIndex(unsigned int i);
    void SetA(unsigned int a);
    void SetR(unsigned int r);
    void SetG(unsigned int g);
    void SetB(unsigned int b);

    int GetPosX() {
        return mPosX;
    }
    int GetPosY() {
        return mPosY;
    }
    unsigned int GetWidth() {
        return mWidth;
    }
    unsigned int GetHeight() {
        return mHeight;
    }
    unsigned int GetStrID() {
        return mStrID;
    }
    unsigned int GetFrameNumber() {
        return mFrameNumber;
    }
    unsigned char GetA() {
        return mA;
    }
    unsigned char GetR() {
        return mR;
    }
    unsigned char GetG() {
        return mG;
    }
    unsigned char GetB() {
        return mB;
    }
    JUSTIFY_ENUM GetJustifyX() {
        return mJustifyX;
    }
    JUSTIFY_ENUM GetJustifyY() {
        return mJustifyY;
    }
    unsigned int GetShapeIndex() {
        return mShapeIndex;
    }

  private:
    int mPosX;              // offset 0x0
    int mPosY;              // offset 0x4
    unsigned int mWidth;    // offset 0x8
    unsigned int mHeight;   // offset 0xC
    unsigned int mStrID;    // offset 0x10
    unsigned int mFrameNumber; // offset 0x14
    JUSTIFY_ENUM mJustifyX; // offset 0x18
    JUSTIFY_ENUM mJustifyY; // offset 0x1C
    unsigned int mShapeIndex; // offset 0x20
    unsigned char mA;       // offset 0x24
    unsigned char mR;       // offset 0x25
    unsigned char mG;       // offset 0x26
    unsigned char mB;       // offset 0x27
    void *mTexturePtr;      // offset 0x28
    void *mStringPtr;       // offset 0x2C
    void *mWrapStringPtr;   // offset 0x30
};

// total size: 0x8
class AV_SUBTITLE_ARRAY {
  public:
    void Init(void *Data, unsigned int DataSize);

  private:
    AV_SUBTITLE *mSubTitle;          // offset 0x0
    unsigned int mNumberOfSubtitle;  // offset 0x4
};

} // namespace RCMP

// GameCube tiene AV_SUBTITLE_ARRAY::Init y sus ayudantes estripados (Range 0xFFFFFFFF en su DWARF) y su objeto
// original no trae ni un byte de .rodata: sus cadenas no estan en el DOL. Nuestro enlace no puede quitarlas enteras
// (-strip-unused-data deja size & 7 de cada una: +0x48 B en .rodata, DOL roto), asi que solo se compilan en PS2.
#ifdef EA_PLATFORM_PLAYSTATION2
using RCMP::AV_SUBTITLE;

static void SkipWhiteSpace(char **RetString, unsigned int *StrLen) {
    unsigned int stri = 0;
    char *String = *RetString;

    for (; stri < *StrLen && isspace(String[stri]); stri++) {
    }
    *RetString = String + stri;
    *StrLen -= stri;
}

static int MyStrCmpi(const char *s1, const char *s2) {
    int i;
    int ret = 0;

    for (i = 0; s1[i] && s2[i]; i++) {
        ret += tolower(s1[i]) - tolower(s2[i]);
        if (ret) {
            return ret;
        }
    }
    return s1[i] - s2[i];
}

static AV_SUBTITLE::JUSTIFY_ENUM FindJust(char *CurFilePos, unsigned int FileSize) {
    const char Tab = '\t';
    const char cr = '\r';
    unsigned int itab;
    char itabc = Tab;

    for (itab = 0; itab < FileSize; itab++) {
        if (CurFilePos[itab] == Tab || CurFilePos[itab] == cr) {
            itabc = CurFilePos[itab];
            CurFilePos[itab] = 0;
            break;
        }
    }
    char *Test[5] = {"RIGHT", "TOP", "CENTER", "LEFT", "BOTTOM"};
    AV_SUBTITLE::JUSTIFY_ENUM JustTable[5] = {AV_SUBTITLE::JUST_RIGHT, AV_SUBTITLE::JUST_TOP, AV_SUBTITLE::JUST_CENTER,
                                              AV_SUBTITLE::JUST_LEFT, AV_SUBTITLE::JUST_BOTTOM};
    AV_SUBTITLE::JUSTIFY_ENUM Ret = static_cast<AV_SUBTITLE::JUSTIFY_ENUM>(-1);
    for (unsigned int i = 0; i < 5; i++) {
        if (MyStrCmpi(Test[i], CurFilePos) == 0) {
            Ret = JustTable[i];
            break;
        }
    }
    if (itab != FileSize) {
        CurFilePos[itab] = itabc;
    }
    return Ret;
}

static bool ReadNumber(char **pCurFilePos, unsigned int *pFileSize, int *Number) {
    long i;
    char *&CurFilePos = *pCurFilePos;
    unsigned int &FileSize = *pFileSize;
    char *AfterNumber;
    const char Tab = '\t';
    const char LineFeed = '\n';
    unsigned int itab;
    char itabc = Tab;

    for (itab = 0; itab < FileSize; itab++) {
        if (CurFilePos[itab] == Tab || CurFilePos[itab] == LineFeed) {
            itabc = CurFilePos[itab];
            CurFilePos[itab] = 0;
            break;
        }
    }
    i = strtol(CurFilePos, &AfterNumber, 10);
    FileSize -= AfterNumber - CurFilePos;
    if (itab != FileSize) {
        CurFilePos[itab] = itabc;
    }
    if (AfterNumber == CurFilePos) {
        return false;
    }
    *Number = i;
    return true;
}

static void FindFirstCommand(char **ppString, unsigned int *pStrLen, char **Commands, unsigned int *CommandsStrLen,
                             unsigned int NumberOfCommands, unsigned int *pFoundCommand) {
    char *String = *ppString;
    unsigned int stri;
    unsigned int comi;
    unsigned int letteri;

    if (String) {
        SkipWhiteSpace(&String, pStrLen);
        for (stri = 0; stri < *pStrLen; stri++) {
            for (comi = 0; comi < NumberOfCommands; comi++) {
                for (letteri = 0; letteri < CommandsStrLen[comi] && stri + letteri < *pStrLen; letteri++) {
                    if (Commands[comi][letteri] != String[stri + letteri]) {
                        break;
                    }
                }
                if (letteri == CommandsStrLen[comi]) {
                    *ppString = String + stri + letteri;
                    *pStrLen -= stri + letteri;
                    *pFoundCommand = comi;
                    return;
                }
            }
        }
    }
    *ppString = 0;
    *pStrLen = 0;
    *pFoundCommand = 0xFFFFFFFF;
}

namespace RCMP {

void AV_SUBTITLE_ARRAY::Init(void *Data, unsigned int DataSize) {
    char *File = static_cast<char *>(Data);
    unsigned int i;
    int FirstRow = -1;
    int MaxRows = 0;
    const char Tab = '\t';
    const char LineFeed = '\n';
    int FrameColumn = -1;
    int StringIDColumn = -1;
    int WidthColumn = -1;
    int HeightColumn = -1;
    int JustifyColumnX = -1;
    int JustifyColumnY = -1;
    int ShapeColumn = -1;
    int XColumn = -1;
    int YColumn = -1;
    int AColumn = -1;
    int RColumn = -1;
    int GColumn = -1;
    int BColumn = -1;
    unsigned int FoundCommandID;
    char *Commands[13] = {"FRAME", "STRINGID", "X", "Y", "WIDTH", "HEIGHT", "JUSTIFY_X", "JUSTIFY_Y", "SHAPE",
                          "A", "R", "G", "B"};
    unsigned int CommandsStrLen[13];
    bool FirstCol;
    char backup;
    int j;
    int CurColumn;
    int CurRow;

    for (i = 0; i < 13; i++) {
        CommandsStrLen[i] = strlen(Commands[i]);
    }
    FirstCol = true;
    backup = 0;

    for (i = 0; i < DataSize; i++) {
        if (File[i] == Tab || FirstCol == true) {
            FirstCol = false;
            if (FirstRow == -1) {
                unsigned int DataLeft = DataSize - i;
                char *CurFilePos = File + i;
                FindFirstCommand(&CurFilePos, &DataLeft, Commands, CommandsStrLen, 13, &FoundCommandID);
                if (FoundCommandID != 0xFFFFFFFF) {
                    FirstRow = MaxRows;
                }
            }
        }
        if (File[i] == LineFeed) {
            MaxRows++;
            FirstCol = true;
        }
    }

    if (FirstRow == -1) {
        mNumberOfSubtitle = 0;
        mSubTitle = 0;
        return;
    }
    MaxRows = MaxRows - FirstRow - 1;
    mSubTitle = new AV_SUBTITLE[MaxRows];
    mNumberOfSubtitle = MaxRows;

    CurColumn = 0;
    CurRow = 0;
    for (i = 0; i < DataSize; i++) {
        if (File[i] == LineFeed) {
            CurRow++;
        }
        if (CurRow == FirstRow) {
            break;
        }
    }

    FirstCol = true;
    while (i < DataSize) {
        if (File[i] == Tab || FirstCol == true) {
            unsigned int DataLeft = DataSize - i;
            char *CurFilePos = File + i;
            FirstCol = false;
            FindFirstCommand(&CurFilePos, &DataLeft, Commands, CommandsStrLen, 13, &FoundCommandID);
            switch (FoundCommandID) {
                case 0:
                    FrameColumn = CurColumn;
                    break;
                case 1:
                    StringIDColumn = CurColumn;
                    break;
                case 2:
                    XColumn = CurColumn;
                    break;
                case 3:
                    YColumn = CurColumn;
                    break;
                case 4:
                    WidthColumn = CurColumn;
                    break;
                case 5:
                    HeightColumn = CurColumn;
                    break;
                case 6:
                    JustifyColumnX = CurColumn;
                    break;
                case 7:
                    JustifyColumnY = CurColumn;
                    break;
                case 8:
                    ShapeColumn = CurColumn;
                    break;
                case 9:
                    AColumn = CurColumn;
                    break;
                case 10:
                    RColumn = CurColumn;
                    break;
                case 11:
                    GColumn = CurColumn;
                    break;
                case 12:
                    BColumn = CurColumn;
                    break;
            }
            CurColumn++;
        }
        if (File[i] == LineFeed) {
            i++;
            FirstCol = true;
            break;
        }
        i++;
    }

    CurColumn = 0;
    CurRow = 0;
    while (i < DataSize) {
        if (File[i] == Tab || FirstCol == true) {
            if (FirstCol == false) {
                i++;
            }
            for (j = 0; i + j < DataSize; j++) {
                if (File[i + j] == Tab || File[i + j] == LineFeed) {
                    break;
                }
            }
            if (i + j != DataSize) {
                backup = File[i + j];
                File[i + j] = 0;
            }
            FirstCol = false;
            if (CurColumn == FrameColumn) {
                unsigned int DataLeft = DataSize - i;
                char *CurFilePos = File + i;
                int frame = mSubTitle[CurRow - 1 > -1 ? CurRow - 1 : 0].GetFrameNumber();
                ReadNumber(&CurFilePos, &DataLeft, &frame);
                mSubTitle[CurRow].SetFrameNumber(frame);
            } else if (CurColumn == StringIDColumn) {
                unsigned int DataLeft = DataSize - i;
                char *CurFilePos = File + i;
                int id = mSubTitle[CurRow - 1 > -1 ? CurRow - 1 : 0].GetStrID();
                ReadNumber(&CurFilePos, &DataLeft, &id);
                mSubTitle[CurRow].SetStrID(id);
            } else if (CurColumn == WidthColumn) {
                unsigned int DataLeft = DataSize - i;
                char *CurFilePos = File + i;
                int Width = mSubTitle[CurRow - 1 > -1 ? CurRow - 1 : 0].GetWidth();
                ReadNumber(&CurFilePos, &DataLeft, &Width);
                mSubTitle[CurRow].SetWidth(Width);
            } else if (CurColumn == HeightColumn) {
                unsigned int DataLeft = DataSize - i;
                char *CurFilePos = File + i;
                int Height = mSubTitle[CurRow - 1 > -1 ? CurRow - 1 : 0].GetHeight();
                ReadNumber(&CurFilePos, &DataLeft, &Height);
                mSubTitle[CurRow].SetHeight(Height);
            } else if (CurColumn == JustifyColumnX) {
                unsigned int DataLeft;
                char *CurFilePos;
                AV_SUBTITLE::JUSTIFY_ENUM Ret = FindJust(File + i, DataSize - i);
                if (Ret != -1) {
                    mSubTitle[CurRow].SetJustifyX(Ret);
                } else {
                    mSubTitle[CurRow].SetJustifyX(mSubTitle[CurRow - 1 > -1 ? CurRow - 1 : 0].GetJustifyX());
                }
            } else if (CurColumn == JustifyColumnY) {
                unsigned int DataLeft;
                char *CurFilePos;
                AV_SUBTITLE::JUSTIFY_ENUM Ret = FindJust(File + i, DataSize - i);
                if (Ret != -1) {
                    mSubTitle[CurRow].SetJustifyY(Ret);
                } else {
                    mSubTitle[CurRow].SetJustifyY(mSubTitle[CurRow - 1 > -1 ? CurRow - 1 : 0].GetJustifyY());
                }
            } else if (CurColumn == ShapeColumn) {
                unsigned int DataLeft = DataSize - i;
                char *CurFilePos = File + i;
                int Index = mSubTitle[CurRow - 1 > -1 ? CurRow - 1 : 0].GetShapeIndex();
                ReadNumber(&CurFilePos, &DataLeft, &Index);
                mSubTitle[CurRow].SetShapeIndex(Index);
            } else if (CurColumn == XColumn) {
                unsigned int DataLeft = DataSize - i;
                char *CurFilePos = File + i;
                int x = mSubTitle[CurRow - 1 > -1 ? CurRow - 1 : 0].GetPosX();
                ReadNumber(&CurFilePos, &DataLeft, &x);
                mSubTitle[CurRow].SetPosX(x);
            } else if (CurColumn == YColumn) {
                unsigned int DataLeft = DataSize - i;
                char *CurFilePos = File + i;
                int y = mSubTitle[CurRow - 1 > -1 ? CurRow - 1 : 0].GetPosY();
                ReadNumber(&CurFilePos, &DataLeft, &y);
                mSubTitle[CurRow].SetPosY(y);
            } else if (CurColumn == AColumn) {
                unsigned int DataLeft = DataSize - i;
                char *CurFilePos = File + i;
                int a = mSubTitle[CurRow - 1 > -1 ? CurRow - 1 : 0].GetA();
                ReadNumber(&CurFilePos, &DataLeft, &a);
                mSubTitle[CurRow].SetA(a);
            } else if (CurColumn == RColumn) {
                unsigned int DataLeft = DataSize - i;
                char *CurFilePos = File + i;
                int r = mSubTitle[CurRow - 1 > -1 ? CurRow - 1 : 0].GetR();
                ReadNumber(&CurFilePos, &DataLeft, &r);
                mSubTitle[CurRow].SetR(r);
            } else if (CurColumn == GColumn) {
                unsigned int DataLeft = DataSize - i;
                char *CurFilePos = File + i;
                int g = mSubTitle[CurRow - 1 > -1 ? CurRow - 1 : 0].GetG();
                ReadNumber(&CurFilePos, &DataLeft, &g);
                mSubTitle[CurRow].SetG(g);
            } else if (CurColumn == BColumn) {
                unsigned int DataLeft = DataSize - i;
                char *CurFilePos = File + i;
                int b = mSubTitle[CurRow - 1 > -1 ? CurRow - 1 : 0].GetB();
                ReadNumber(&CurFilePos, &DataLeft, &b);
                mSubTitle[CurRow].SetB(b);
            }
            CurColumn++;
            if (i + j != DataSize) {
                File[i + j] = backup;
            }
        } else if (File[i++] == LineFeed) {
            CurColumn = 0;
            CurRow++;
            FirstCol = true;
            mSubTitle[CurRow < mNumberOfSubtitle - 1 ? CurRow : mNumberOfSubtitle - 1] = mSubTitle[CurRow - 1];
        }
    }
}

} // namespace RCMP
#endif

namespace RCMP {

// En el orden del original.
AV_SUBTITLE::AV_SUBTITLE() {
    mPosX = 0x20;
    mPosY = 0x15E;
    mWidth = 0x258;
    mHeight = 0x32;
    mStrID = -1;
    mA = 0xFF;
    mR = 0xFF;
    mFrameNumber = -1;
    mG = 0xFF;
    mJustifyX = JUST_CENTER;
    mB = 0xFF;
    mJustifyY = JUST_CENTER;
    mShapeIndex = 0;
    mTexturePtr = 0;
    mStringPtr = 0;
    mWrapStringPtr = 0;
}

void AV_SUBTITLE::SetPosX(int PosX) {
    mPosX = PosX;
}

void AV_SUBTITLE::SetPosY(int y) {
    mPosY = y;
}

void AV_SUBTITLE::SetWidth(unsigned int w) {
    mWidth = w;
}

void AV_SUBTITLE::SetHeight(unsigned int h) {
    mHeight = h;
}

void AV_SUBTITLE::SetStrID(unsigned int id) {
    mStrID = id;
}

void AV_SUBTITLE::SetFrameNumber(unsigned int n) {
    mFrameNumber = n;
}

void AV_SUBTITLE::SetJustifyX(JUSTIFY_ENUM jx) {
    mJustifyX = jx;
}

void AV_SUBTITLE::SetJustifyY(JUSTIFY_ENUM jy) {
    mJustifyY = jy;
}

void AV_SUBTITLE::SetShapeIndex(unsigned int i) {
    mShapeIndex = i;
}

void AV_SUBTITLE::SetA(unsigned int a) {
    mA = static_cast<unsigned char>(a);
}

void AV_SUBTITLE::SetR(unsigned int r) {
    mR = static_cast<unsigned char>(r);
}

void AV_SUBTITLE::SetG(unsigned int g) {
    mG = static_cast<unsigned char>(g);
}

void AV_SUBTITLE::SetB(unsigned int b) {
    mB = static_cast<unsigned char>(b);
}

} // namespace RCMP
