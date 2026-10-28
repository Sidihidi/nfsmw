#ifndef SUPPORT_MISCELLANEOUS_STRINGREGISTRY_H
#define SUPPORT_MISCELLANEOUS_STRINGREGISTRY_H

#include <vector>

class StringKey;
class StringStore;
#ifdef EA_PLATFORM_PLAYSTATION2
// total size: 0x1000
class StringStoreBlock {
  public:
    ~StringStoreBlock();

    bool RmvRef() {
        return --mRefCount == 0;
    }

    unsigned int mRefCount;     // offset 0x0, size 0x4
    unsigned int mNextFreeByte; // offset 0x4, size 0x4
    char mData[4088];           // offset 0x8, size 0xFF8
};
#else
class StringStoreBlock;
#endif

// total size: 0x4
class StringStoreBlockPtr {
  public:
    StringStoreBlockPtr() {}
    StringStoreBlockPtr(const StringStoreBlockPtr &src) : mData(src.mData) {}
#ifdef EA_PLATFORM_PLAYSTATION2
    ~StringStoreBlockPtr();

    static void operator delete(void *mem, unsigned int size);
#else
    ~StringStoreBlockPtr() {}
#endif

    bool operator<(const StringStoreBlockPtr &rhs) const {
        return mData < rhs.mData;
    }

    static std::vector<StringStoreBlockPtr> sStringStoreBlockPtrCache; // size: 0x10, address: 0x80473ED4

  private:
    StringStoreBlock *mData; // offset 0x0, size 0x4
};

// total size: 0x14
class StringPool {
  private:
    StringKey *mStrings;         // offset 0x0, size 0x4
    unsigned int mHashMask;      // offset 0x4, size 0x4
    unsigned int mNumStrings;    // offset 0x8, size 0x4
    unsigned int mNumCollisions; // offset 0xC, size 0x4
    StringStore *mStorage;       // offset 0x10, size 0x4

  public:
    StringPool();
#ifdef EA_PLATFORM_PLAYSTATION2
    ~StringPool();
#endif
    void Startup();
    void Shutdown();
    void RegisterStaticStrings();
    void GetMetrics();
    StringKey &GetStringKey(const char *string);
    StringKey &GetStringKey(const char *string, unsigned int hash);
    char *GetString(const char *string);
    char *GetString(const char *string, unsigned int hash);
    char *Search(const char *string);
    char *Search(const char *string, unsigned int hash);
    StringKey &GetLowerCaseKey(const char *string);
    StringKey &GetLowerCaseKey(const char *string, unsigned int hash);

  private:
    unsigned int Find(const char *string, unsigned int hash);
};

// total size: 0x14
class StringRegistry : public StringPool {
  private:
    static StringRegistry fgThis;

  public:
    StringRegistry();
#ifdef EA_PLATFORM_PLAYSTATION2
    ~StringRegistry();
#endif
};

#endif
