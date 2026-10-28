#include "../../../include/common/realmemcard/memcard_interface.h"

#include <stdarg.h>
#include <string.h>

namespace Realmc {

namespace Locale {

static const int TRC_MSG_BUFFER_SIZE = 1024;
static const int NUM_TRC_MSG_BUFFERS = 5;
#if defined(_MSC_VER)
static __declspec(align(32)) unsigned short gTrcMsgBuffer[NUM_TRC_MSG_BUFFERS][TRC_MSG_BUFFER_SIZE];
#else
static unsigned short __attribute__((aligned(32))) gTrcMsgBuffer[NUM_TRC_MSG_BUFFERS][TRC_MSG_BUFFER_SIZE];
#endif
static int gTrcMsgBufferIndex = 0;
static const char *(*gLocaleCallback)(int) = nullptr;


void SetLocaleGetStrCallback(const char *(*cb)(int)) {
    gLocaleCallback = cb;
}

const wchar_t *GetString(int strID, char *parameterTypes, ...) {
    int numParameters = parameterTypes ? strlen(parameterTypes) : 0;
    const int MAX_LOCALE_PARAMETERS = 8;
    va_list val;

    va_start(val, parameterTypes);
    int param[MAX_LOCALE_PARAMETERS];
    char *pType = parameterTypes;
    for (int i = 0; i < numParameters; i++, pType++) {
        switch (*pType) {
        case 'c':
            param[i] = va_arg(val, char);
            break;
        case 'd':
            param[i] = va_arg(val, int);
            break;
        case 'p':
            param[i] = reinterpret_cast<int>(va_arg(val, void *));
            break;
        case 's':
            param[i] = reinterpret_cast<int>(va_arg(val, short *));
            break;
        }
    }
    va_end(val);

    const char *pStr = gLocaleCallback(strID);
    if (pStr == nullptr) {
        return nullptr;
    }

    int bufindex = gTrcMsgBufferIndex;
    short *stri = reinterpret_cast<short *>(const_cast<char *>(pStr));
    short *strd = reinterpret_cast<short *>(gTrcMsgBuffer[bufindex]);
    char numstr[20];
    bool nextcmd = false;
    int pi = 0;

    do {
        if (nextcmd) {
            short c = *stri;
            pi = c - '1';
            if (parameterTypes[pi] != 'd') {
                if (parameterTypes[pi] == 's') {
                    short *s = reinterpret_cast<short *>(param[pi]);

                    while (*s != 0) {
                        *strd++ = *s++;
                    }
                }
            } else {
                int argval = param[pi];

                if (argval < 0) {
                    *strd++ = '-';
                    argval = -argval;
                }

                int ibackup = argval;
                int numdigits = 0;
                do {
                    ibackup /= 10;
                    numdigits++;
                } while (ibackup != 0);
                numstr[numdigits] = 0;

                do {
                    numdigits--;
                    numstr[numdigits] = argval % 10 + '0';
                    argval /= 10;
                } while (numdigits != 0);

                char *cptr = numstr;
                while (*cptr != 0) {
                    *strd++ = *cptr++;
                }
            }

            nextcmd = false;
        } else {
            if (*stri == '%') {
                nextcmd = true;
            } else if (*stri != '"') {
                *strd++ = *stri;
            }
        }
    } while (*stri++ != 0);

    *strd = 0;
#if REALMEMCARD_VERSION < 30401
    gTrcMsgBufferIndex++;
    gTrcMsgBufferIndex %= NUM_TRC_MSG_BUFFERS;
#else
    gTrcMsgBufferIndex = (gTrcMsgBufferIndex + 1) % NUM_TRC_MSG_BUFFERS;
#endif
    return reinterpret_cast<const wchar_t *>(gTrcMsgBuffer[bufindex]);
}

// GameCube tambien la tenia (DWARF: rango 0xFFFFFFFF, con isunicodemarkup, unicode, digit e i) y su enlazador la
// estripa entera; en PS2 se queda. En X360 queda emitida en 0x825E9878 (244 B, la logica de escapes \x).
#if defined(EA_PLATFORM_PLAYSTATION2) || defined(EA_PLATFORM_XENON)
void ConvertAsciiToUnicode(const char *src, unsigned short *dst) {
    while (*src != 0) {
        bool isunicodemarkup = false;
        int unicode = 0;

        if (src[0] == '\\' && src[1] == 'x') {
            int digit;

            isunicodemarkup = true;
            for (int i = 0; i < 4 && src[i + 2] != 0; i++) {
                if (src[i + 2] >= '0' && src[i + 2] <= '9') {
                    digit = static_cast<unsigned char>(src[i + 2]) - '0';
                } else if (src[i + 2] >= 'a' && src[i + 2] <= 'f') {
                    digit = static_cast<unsigned char>(src[i + 2]) - 'a' + 10;
                } else {
                    isunicodemarkup = false;
                    break;
                }
                unicode = unicode * 16 + digit;
            }
        }

        if (isunicodemarkup) {
            *dst++ = unicode;
            src += 6;
        } else {
            *dst++ = static_cast<unsigned char>(*src++);
        }
    }
    *dst = 0;
}
#endif

int GetWstrLength(const wchar_t *str) {
    int len = 0;

    if (str != nullptr) {
        while (*str++ != 0) {
            len++;
        }
    }

    return len;
}

} // namespace Locale

} // namespace Realmc
