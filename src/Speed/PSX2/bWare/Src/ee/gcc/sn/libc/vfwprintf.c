/*
 * vfwprintf.c de la libc de SN para PS2 (libc.a(vfwprintf.obj) y, con INTEGER_ONLY, vfwiprintf.obj): el printf
 * de caracter ancho. Es el vfprintf.c de SN (src/libc/vfprintf.c, el mismo que el de GameCube) pasado a wchar_t,
 * escrito a partir del binario: wmemcpy, wcslen, wmemchr y wcscpy en lugar de las de char, las cadenas L"...", y
 * la salida por `_wwrite', que a la consola (descriptores 1 y 2) le manda los caracteres anchos convertidos a
 * char en trozos de 32 y al resto los bytes tal cual. El formato se recorre sin _mbtowc_r.
 *
 * La libc de SN tiene wchar_t de 16 bits (sus cabeceras; las L"..." de 16 bits piden -fshort-wchar); las
 * cabeceras de newlib de esta toolchain lo declaran int.
 */

#ifdef INTEGER_ONLY
#define _VFWPRINTF_R _vfwiprintf_r
#else
#define _VFWPRINTF_R vfwprintf_r
#define FLOATING_POINT
#endif

#define __WCHAR_TYPE__ unsigned short
#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>

extern wchar_t *wmemcpy(wchar_t *, const wchar_t *, size_t);
extern wchar_t *wmemchr(const wchar_t *, wchar_t, size_t);
extern wchar_t *wcscpy(wchar_t *, const wchar_t *);
extern size_t wcslen(const wchar_t *);
extern int write(int, const void *, int);

/* En el EE long es de 64 bits (long long es de 128): los enteros de %ll y %q van en long. */
typedef long quad_t;
typedef unsigned long u_quad_t;
typedef unsigned long u_long;

#ifdef FLOATING_POINT
extern double sn_floor(double);
extern double sn_fmod(double, double);

int wstrround(wchar_t *s, int i)
{
    wchar_t c;

    if (i > 0) {
        i--;
        c = s[i];
        if (c > '4') {
            do {
                s[i] = '0';
                i--;
            } while (i > 0 && s[i] == '9');
            if (s[i] == '9')
                return 0;
            s[i] = s[i] + 1;
        }
    }
    return 1;
}

wchar_t *wstrrev(wchar_t *s)
{
    int i, j;

    for (j = 0; s[j] != '\0'; j++)
        ;
    for (i = 0, j--; i < j; i++, j--) {
        wchar_t c;

        c = s[i];
        s[i] = s[j];
        s[j] = c;
    }
    return s;
}

wchar_t *witoa(int value, wchar_t *string, int radix)
{
    static wchar_t lower[] = L"0123456789abcdefghijklmnopqrstuvwxyz";
    int i;
    int neg;
    unsigned int u;

    neg = value < 0;
    if (neg && radix == 10)
        u = -value;
    else
        u = value;
    i = 0;
    do {
        string[i++] = lower[u % radix];
        u /= radix;
    } while (u != 0);
    if (neg)
        string[i++] = '-';
    string[i] = '\0';
    return wstrrev(string);
}

/* El EE es little endian: la palabra baja va primero (la rama __IEEE_LITTLE_ENDIAN de newlib). */
typedef union {
    double value;
    struct {
        unsigned int lsw;
        unsigned int msw;
    } parts;
} ieee_double_shape_type;

#define EXTRACT_WORDS(ix0, ix1, d)       \
    do {                                 \
        ieee_double_shape_type ew_u;     \
        ew_u.value = (d);                \
        (ix0) = ew_u.parts.msw;          \
        (ix1) = ew_u.parts.lsw;          \
    } while (0)

wchar_t *wfftoa(double value, int ndigits, wchar_t fmt, int trim, wchar_t *sign)
{
    static wchar_t str[164];
    static wchar_t *s;
    unsigned int hi, mant, t, neg, expo;
    unsigned int lo;
    int i, e, total, prec, digit;
    double eps;

    EXTRACT_WORDS(hi, lo, value);
    mant = hi & 0xfffff;
    t = hi >> 20;
    neg = t & 0x800;
    expo = t & 0x7ff;
    if (expo == 0x7ff) {
        if (mant == 0 && lo == 0) {
            if (neg)
                wcscpy(str, L"-Inf");
            else
                wcscpy(str, L"Inf");
        } else
            wcscpy(str, L"NaN");
        return str;
    }

    prec = ndigits;
    i = 0;
    if (value < 0.0) {
        if (sign != 0) {
            *sign = '-';
            s = str;
        } else {
            str[0] = '-';
            s = str + 1;
        }
        value = -value;
    } else {
        s = str;
    }

    if (value >= 1.0) {
        double intpart = sn_floor(value);
        int k;

        value -= intpart;
        do {
            if (i > 162)
                break;
            digit = (int)sn_fmod(intpart, 10.0);
            s[i] = digit + '0';
            i++;
            intpart -= digit;
            intpart /= 10.0;
        } while (intpart >= 1.0);

        for (digit = 0, k = i - 1; digit < k; digit++, k--) {
            wchar_t c;

            c = s[digit];
            s[digit] = s[k];
            s[k] = c;
        }
    }

    e = i;
    if (e == 0) {
        if (fmt != 'f') {
            if (value != 0.0) {
                while ((value *= 10.0) < 1.0 && e > -1021)
                    e--;
                e--;
            }
            if (value >= 1.0) {
                value /= 10.0;
                prec = ndigits + 1;
            }
        } else {
            s[e] = '0';
            i++;
        }
    }

    if (e < 0)
        total = prec;
    else if (fmt == 'f')
        total = prec + (e > 0 ? e : 1);
    else
        total = prec + 1;

    eps = 5.551115123125783e-17;
    for (;;) {
        value *= 10.0;
        digit = (int)value;
        value -= digit;
        eps *= 10.0;
        if (i < total && value >= eps && value <= 1.0 - eps) {
            s[i] = digit + '0';
            i++;
        } else
            break;
    }

    if (value >= 0.5)
        digit++;
    s[i] = digit + '0';
    i++;

    while (i <= total) {
        s[i] = '0';
        i++;
    }

    if (wstrround(s, total + 1) == 0) {
        s[0] = '1';
        total = total + 1;
        e++;
    }

    if (ndigits != 0) {
        int point;

        if (fmt == 'f') {
            if (e > 0)
                point = e;
            else
                point = 1;
        } else {
            point = 1;
        }
        for (i = total; i > point; i--)
            s[i] = s[i - 1];
        s[point] = '.';
    } else {
        total--;
    }

    if (trim) {
        while (total != 0 && s[total] == '0')
            total--;
        if (s[total] == '.')
            total--;
    }

    if (fmt != 'f') {
        total++;
        s[total] = fmt;
        if (e >= 0) {
            total++;
            s[total] = '+';
            if (e != 0)
                e--;
            if (e <= 9) {
                total++;
                s[total] = '0';
            }
        } else {
            total++;
            s[total] = '-';
            if (e > -10) {
                total++;
                s[total] = '0';
            }
            e = -e;
        }
        total++;
        witoa(e, &s[total], 10);
    } else {
        s[total + 1] = '\0';
    }

    return str;
}
#endif /* FLOATING_POINT */

#ifndef INTEGER_ONLY
int vfwprintf_r(struct _reent *, FILE *, const wchar_t *, va_list);
extern int _vfwiprintf_r(struct _reent *, FILE *, const wchar_t *, va_list);
wchar_t *wadd_separators(wchar_t *, wchar_t *, wchar_t *, int *);

int vfwprintf(FILE *fp, const wchar_t *fmt0, va_list ap)
{
    const wchar_t *p;

    for (p = fmt0; *p != '\0'; p++) {
        if (*p == '%' && p[1] != '\0') {
            p++;
            while (*p <= '@' && p[1] != '\0')
                p++;
            switch (*p) {
            case 'E':
            case 'G':
            case 'L':
            case 'e':
            case 'f':
            case 'g':
                return vfwprintf_r(fp->_data, fp, fmt0, ap);
            }
        }
    }

    return _vfwiprintf_r(fp->_data, fp, fmt0, ap);
}

int _vfwwrite(int fd, const char *buf, unsigned int len, int flush)
{
    static char pch[128];
    static int cumulative_written = 0;
    static char *pch_pointer = pch;
    unsigned int i;

    if (flush == 1) {
        int done = write(fd, pch, cumulative_written);
        cumulative_written = 0;
        pch_pointer = pch;
        return done;
    }

    for (i = 0; i < len; i++) {
        int done;

        *pch_pointer = buf[i];
        pch_pointer++;
        cumulative_written++;
        if (cumulative_written > 127) {
            done = write(fd, pch, cumulative_written);
            pch_pointer = pch;
            cumulative_written = 0;
            if (done == 0)
                return 0;
        }
    }

    return i;
}

int _wwrite(int fd, const wchar_t *buf, unsigned int len)
{
    char tmp[32];
    int i, n, j, done, ret = 0;

    if (fd == 1 || fd == 2) {
        n = 32;
        if (len < 32)
            n = len;
        i = 0;
        for (;;) {
            for (j = 0; i < n; i++)
                tmp[j++] = buf[i];
            done = _vfwwrite(fd, tmp, j, 0);
            ret += done;
            if (n == len || done <= 0)
                break;
            n += 32;
            if (n > len)
                n = len;
        }
    } else {
        ret = _vfwwrite(fd, (const char *)buf, len * sizeof(wchar_t), 0) / len * sizeof(wchar_t);
    }
    return ret;
}
#else
extern int _vfwwrite(int, const char *, unsigned int, int);
extern int _wwrite(int, const wchar_t *, unsigned int);
extern wchar_t *wadd_separators(wchar_t *, wchar_t *, wchar_t *, int *);
int _vfwiprintf_r(struct _reent *, FILE *, const wchar_t *, va_list);
#endif /* !INTEGER_ONLY */

#ifdef FLOATING_POINT
extern double sn_log10(double);
extern void *localeconv(void);

#define MAXEXP 308
#define MAXFRACT 39
#define BUF (MAXEXP + MAXFRACT + 1) /* + decimal point */
#else
#define BUF 40
#endif

#define ALT 0x001 /* alternate form */
#define HEXPREFIX 0x002 /* add 0x or 0X prefix */
#define LADJUST 0x004 /* left adjustment */
#define LONGDBL 0x008 /* long double */
#define LONGINT 0x010 /* long integer */
#define QUADINT 0x020 /* quad integer */
#define SHORTINT 0x040 /* short integer */
#define ZEROPAD 0x080 /* zero (as opposed to blank) pad */
#define FPT 0x100 /* Floating point number */
#define GROUPING 0x200 /* thousands separators */

#define to_digit(c) ((c) - '0')
#define is_digit(c) ((unsigned)to_digit(c) <= 9)
#define to_char(n) ((n) + '0')

#define PADSIZE 16 /* pad chunk size */

#define PRINT(ptr, len)                                          \
    {                                                            \
        if (fp->_flags & __SSTR) {                               \
            if (fp->_w > (len) * sizeof(wchar_t)) {              \
                wmemcpy((wchar_t *)fp->_p, (ptr), (len));        \
                fp->_p += (len) * sizeof(wchar_t);               \
                fp->_w -= (len);                                 \
            } else {                                             \
                wmemcpy((wchar_t *)fp->_p, (ptr), fp->_w);       \
                fp->_p += fp->_w * sizeof(wchar_t);              \
                fp->_w = 0;                                      \
            }                                                    \
        } else {                                                 \
            if (fp->_flags == 0 && fp->_file == 0)               \
                fp->_file = 1;                                   \
            _wwrite(fp->_file, (ptr), (len));                    \
        }                                                        \
    }

#define PAD(howmany, with)            \
    {                                 \
        if ((n = (howmany)) > 0) {    \
            while (n > PADSIZE) {     \
                PRINT(with, PADSIZE); \
                n -= PADSIZE;         \
            }                         \
            PRINT(with, n);           \
        }                             \
    }

#define FLUSH()                            \
    {                                      \
        if (!(fp->_flags & __SSTR))        \
            _vfwwrite(fp->_file, 0, 0, 1); \
    }

#define SARG()                                                    \
    (flags & QUADINT ? va_arg(ap, quad_t)                         \
                     : flags & LONGINT ? (quad_t)va_arg(ap, long) \
                                       : flags & SHORTINT         \
                     ? (quad_t)(short)va_arg(ap, int)             \
                     : (quad_t)va_arg(ap, int))
#define UARG()                                                         \
    (flags & QUADINT ? va_arg(ap, u_quad_t)                            \
                     : flags & LONGINT ? (u_quad_t)va_arg(ap, u_long) \
                                       : flags & SHORTINT              \
                     ? (u_quad_t)(unsigned short)va_arg(ap, int)       \
                     : (u_quad_t)va_arg(ap, unsigned int))

int _VFWPRINTF_R(struct _reent *data, FILE *fp, const wchar_t *fmt0, va_list ap)
{
    static const wchar_t blanks[PADSIZE]
        = { ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
              ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ' };
    static const wchar_t zeroes[PADSIZE]
        = { '0', '0', '0', '0', '0', '0', '0', '0',
              '0', '0', '0', '0', '0', '0', '0', '0' };
    register const wchar_t *fmt; /* format string */
    register int ch; /* character from fmt */
    register int n, m; /* handy integers (short term usage) */
    register wchar_t *cp; /* handy char pointer (short term usage) */
    register int flags; /* flags as above */
    int ret; /* return value accumulator */
    int width; /* width from format (%8d), or 0 */
    int prec; /* precision from format (%.3d), or -1 */
    wchar_t sign; /* sign prefix (' ', '+', '-', or \0) */

#ifdef FLOATING_POINT
    void *decimal_point = localeconv();
    double _fpvalue; /* floating point arguments %[eEfgG] */
    int trim;
#endif
    u_quad_t _uquad; /* integer arguments %[diouxX] */
    enum { OCT, DEC, HEX } base; /* base for [diouxX] conversion */
    int dprec; /* a copy of prec if [diouxX], 0 otherwise */
    int realsz; /* field size expanded by dprec */
    int size; /* size of converted field or string */
    const wchar_t *xdigs; /* digits for [xX] conversion */
    wchar_t buf[BUF]; /* space for %c, %[diouxX], %[eEfgG] */
    wchar_t ox[2]; /* space for 0x hex-prefix */

    fmt = fmt0;
    ret = 0;

    /*
     * Scan the format for conversions (`%' character).
     */
    for (;;) {
        cp = (wchar_t *)fmt;
        for (n = 0; fmt[n] != '\0' && fmt[n] != '%'; n++)
            ;
        fmt += n;
        if (*fmt == '\0')
            n = 0;
        else if (*fmt == '%' && n == 0)
            n = 1;
        if ((m = fmt - cp) != 0) {
            PRINT(cp, m);
            ret += m;
        }
        if (n <= 0)
            goto done;
        if (fp->_w <= 0 && (fp->_flags & __SSTR))
            goto error;
        fmt++; /* skip over '%' */

        flags = 0;
        dprec = 0;
        width = 0;
        prec = -1;
        sign = '\0';
    nextch:
        ch = *fmt++;
    reswitch:
        switch (ch) {
        case ' ':
            if (!sign)
                sign = ' ';
            goto nextch;
        case '#':
            flags |= ALT;
            goto nextch;
        case '\'':
            flags |= GROUPING;
            goto nextch;
        case '*':
            if ((width = va_arg(ap, int)) >= 0)
                goto nextch;
            width = -width;
            /* FALLTHROUGH */
        case '-':
            flags |= LADJUST;
            goto nextch;
        case '+':
            sign = '+';
            goto nextch;
        case '.':
            ch = *fmt++;
            if (ch == '*') {
                n = va_arg(ap, int);
                prec = n < 0 ? -1 : n;
                goto nextch;
            }
            n = 0;
            while (is_digit(ch)) {
                n = 10 * n + to_digit(ch);
                ch = *fmt++;
            }
            prec = n;
            if (prec < -1)
                prec = -1;
            goto reswitch;
        case '0':
            flags |= ZEROPAD;
            goto nextch;
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            n = 0;
            do {
                n = 10 * n + to_digit(ch);
                ch = *fmt++;
            } while (is_digit(ch));
            width = n;
            goto reswitch;
#ifdef FLOATING_POINT
        case 'L':
            flags |= LONGDBL;
            goto nextch;
#endif
        case 'h':
            flags |= SHORTINT;
            goto nextch;
        case 'l':
            if (*fmt == 'l') {
                fmt++;
                flags |= QUADINT;
            } else {
                flags |= LONGINT;
            }
            goto nextch;
        case 'q':
            flags |= QUADINT;
            goto nextch;
        case 'c':
            *(cp = buf) = va_arg(ap, int);
            size = 1;
            sign = '\0';
            break;
        case 'D':
            flags |= LONGINT;
            /*FALLTHROUGH*/
        case 'd':
        case 'i':
            _uquad = SARG();
            if ((quad_t)_uquad < 0) {
                _uquad = -_uquad;
                sign = '-';
            }
            base = DEC;
            goto number;
#ifdef FLOATING_POINT
        case 'e':
        case 'E':
        case 'f':
        case 'g':
        case 'G':
            trim = 0;
            if (prec == -1)
                prec = 6;
            _fpvalue = va_arg(ap, double);
            if (ch == 'g' || ch == 'G') {
                double expt;

                if (_fpvalue != 0.0)
                    expt = sn_log10(_fpvalue < 0.0 ? -_fpvalue : _fpvalue);
                else
                    expt = 1.0;
                if (expt < -4.0 || expt >= prec) {
                    ch = ch == 'g' ? 'e' : 'E';
                } else
                    ch = 'f';
                trim = 1;
            }
            if (flags & ALT) {
                if (prec == 0)
                    prec = 1;
                trim = 0;
            }
            cp = wfftoa(_fpvalue, prec, ch, trim, &sign);
            size = wcslen(cp);
            if (flags & GROUPING)
                cp = wadd_separators(cp, cp, cp + size, &size);
            break;
#endif /* FLOATING_POINT */
        case 'n':
            if (flags & QUADINT)
                *va_arg(ap, quad_t *) = ret;
            else if (flags & LONGINT)
                *va_arg(ap, long *) = ret;
            else if (flags & SHORTINT)
                *va_arg(ap, short *) = ret;
            else
                *va_arg(ap, int *) = ret;
            continue; /* no output */
        case 'O':
            flags |= LONGINT;
            /*FALLTHROUGH*/
        case 'o':
            _uquad = UARG();
            base = OCT;
            goto nosign;
        case 'p':
            _uquad = (u_long)va_arg(ap, void *);
            base = HEX;
            xdigs = L"0123456789abcdef";
            flags |= HEXPREFIX;
            ch = 'x';
            goto nosign;
        case 's':
            if ((cp = va_arg(ap, wchar_t *)) == 0)
                cp = L"(null)";
            if (prec >= 0) {
                wchar_t *p = wmemchr(cp, 0, prec);

                if (p != 0) {
                    size = p - cp;
                    if (size > prec)
                        size = prec;
                } else
                    size = prec;
            } else
                size = wcslen(cp);
            sign = '\0';
            break;
        case 'U':
            flags |= LONGINT;
            /*FALLTHROUGH*/
        case 'u':
            _uquad = UARG();
            base = DEC;
            goto nosign;
        case 'X':
            xdigs = L"0123456789ABCDEF";
            goto hex;
        case 'x':
            xdigs = L"0123456789abcdef";
        hex:
            _uquad = UARG();
            base = HEX;
            if (flags & ALT && _uquad != 0)
                flags |= HEXPREFIX;
        nosign:
            sign = '\0';
        number:
            if ((dprec = prec) >= 0)
                flags &= ~ZEROPAD;

            cp = buf + BUF;
            if (_uquad != 0 || dprec != 0) {
                switch (base) {
                case OCT:
                    do {
                        *--cp = to_char(_uquad & 7);
                        _uquad >>= 3;
                    } while (_uquad);
                    if (flags & ALT && *cp != '0')
                        *--cp = '0';
                    break;

                case DEC:
                    while (_uquad >= 10) {
                        *--cp = to_char(_uquad % 10);
                        _uquad /= 10;
                    }
                    *--cp = to_char(_uquad);
                    break;

                case HEX:
                    do {
                        *--cp = xdigs[_uquad & 15];
                        _uquad >>= 4;
                    } while (_uquad);
                    break;

                default:
                    cp = L"bug in vfprintf: bad base";
                    size = wcslen(cp);
                    goto skipsize;
                }
            }
            if (base == DEC && (flags & GROUPING))
                cp = wadd_separators(buf, cp, buf + BUF, &size);
            else
                size = buf + BUF - cp;
        skipsize:
            break;
        default: /* "%?" prints ?, unless ? is NUL */
            if (ch == '\0')
                goto done;
            cp = buf;
            *cp = ch;
            size = 1;
            sign = '\0';
            break;
        }

        realsz = dprec > size ? dprec : size;
        if (sign)
            realsz++;
        else if (flags & HEXPREFIX)
            realsz += 2;

        if ((flags & (LADJUST | ZEROPAD)) == 0)
            PAD(width - realsz, blanks);

        if (sign) {
            PRINT(&sign, 1);
        } else if (flags & HEXPREFIX) {
            ox[0] = '0';
            ox[1] = ch;
            PRINT(ox, 2);
        }

        if ((flags & (LADJUST | ZEROPAD)) == ZEROPAD)
            PAD(width - realsz, zeroes);

        PAD(dprec - size, zeroes);

        PRINT(cp, size);

        if (flags & LADJUST)
            PAD(width - realsz, blanks);

        ret += width > realsz ? width : realsz;
    }
done:
    FLUSH();
error:
    return ret;
}

#ifndef INTEGER_ONLY
wchar_t *wadd_separators(wchar_t *dst, wchar_t *start, wchar_t *end, int *lenp)
{
    int n = end - start + 10;
    wchar_t *buf = alloca(n * sizeof(wchar_t));
    wchar_t *p = buf + n - 1;
    wchar_t *q;
    struct {
        int state;
        wchar_t *mark;
    } scan;
    register int count = 0;

    if (dst == 0 || start == 0 || end == 0)
        return 0;

    scan.state = 0;
    for (q = start, scan.mark = end; q <= end && scan.state != 2; q++) {
        switch (*q) {
        case '.':
            scan.mark = q;
            if (scan.state == 0)
                scan.state = 1;
            break;
        case 'e':
        case 'E':
            scan.state = 2;
            break;
        }
    }

    if (scan.state == 2)
        return start;

    for (q = end; q > start; q--) {
        *p-- = *q;
        if (q <= scan.mark) {
            if (count % 3 == 0 && count != 0) {
                *p-- = ',';
                n++;
            }
            count++;
        }
    }
    *p = *q;
    n -= 10;
    wmemcpy(dst, p, n);
    *lenp = n;
    return dst;
}
#endif /* !INTEGER_ONLY */
