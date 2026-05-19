// Tested on Linux x86_64 via gcc14 -std=c23 and Windows x86_64 with newest Visual Studio C install as of 20260513
// Licensed under MIT.

#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <locale.h>     // for LC_ALL, setlocale
#include <stddef.h>     // for size_t, wchar_t
#include <stdint.h>     // for WCHAR_WIDTH, UINT16_MAX, uint_fast8_t, uint32_t
#include <stdio.h>      // for printf, putchar
#include <string.h>     // for memset, memcpy, strlen
#include <uchar.h>      // for char8_t, char16_t, char32_t, c16rtomb, c32rtomb, mbrtoc16, mbrtoc32
#include <wchar.h>      // for mbstate_t, mbsrtowcs, wcslen, wcsrtombs, wmemcpy

#ifdef _WIN32
#include <stdbool.h>    // for bool, true
#include <windows.h>    // for CP_UTF8, SSIZE_T, SetConsoleOutputCP

#define WCHAR_WIDTH 16
#define nullptr NULL
typedef SSIZE_T ssize_t;
#else
#include <sys/types.h>  // for ssize_t
#endif

#if !defined(__STDC_VERSION_UCHAR_H__) || __STDC_VERSION_UCHAR_H_ < 202311L
typedef unsigned char char8_t;
#endif

#if (!defined(__STDC_VERSION_WCHAR_H__) || __STDC_VERSION_WCHAR_H_ < 202311L) && defined(__gnu_linux__)
#define WCHAR_WIDTH 32
#endif

#ifndef __STDC_UTF_16__
#error Need UTF-16 encoded char16_t
#endif
#ifndef __STDC_UTF_32__
#error Need UTF-32 encoded char32_t
#endif
#if WCHAR_WIDTH != 16 && WCHAR_WIDTH != 32
#error Need UTF-16 or UTF-32 encoded wchar_t
#endif

static char convBuf8[4];
static char16_t convBuf16[2];
// TODO: Confirm 1 char is fine, all chars are meant to fit in 1 utf-32 code point but mbrtoc32 outputs a "variable-length 32-bit wide character"
static char32_t convBuf32[1];

// utf-16 surrogate values
static const char16_t highSurrogateStart =  0xD800;
static const char16_t highSurrogateEnd   =  0xDBFF;
static const char16_t lowSurrogateStart  =  0xDC00;
static const char16_t lowSurrogateEnd    =  0xDFFF;
static const uint32_t surrogateOffset    = 0x10000;
static const uint_fast8_t highSurrogateShift = 10;


static void printws(const wchar_t *restrict str);
static size_t strlenw(const wchar_t *restrict str);


// These functions returns the number of code units in string, not including the null terminator

static size_t strlen8(const char8_t *restrict str) {
    return strlen((const char *restrict)str);
}

static size_t strlen16(const char16_t *restrict str) {
#if WCHAR_WIDTH == 16
    return strlenw((const wchar_t *restrict)str);
#else
    size_t strOff;
    for (strOff = 0; str[strOff] != u'\0'; ++strOff) {
    }

    return strOff;
#endif
}

static size_t strlen32(const char32_t *restrict str) {
#if WCHAR_WIDTH == 16
    size_t strOff;
    for (strOff = 0; str[strOff] != U'\0'; ++strOff) {
    }

    return strOff;
#else
    return strlenw((const wchar_t *restrict)str);
#endif
}

static size_t strlenw(const wchar_t *restrict str) {
    return wcslen(str);
}


// These functions return the number of code units that have (or would have if str8 or str8Size are 0) been written to str8,
// not including the terminating null byte, or a -1 if an error occurred

// TODO: Make sure compound chars like emoji work (e.g. face + skin colour) for all of these
// TODO: Check if wcsrtombs also has an issue with an undersized output buffer like mbsrtowcs
// TODO: Replace missing mingw-w64 functions: c16rtomb, c32rtomb, mbrtoc16, mbrtoc32

#ifndef __MINGW64__
static ssize_t s16tos8(char8_t *restrict str8, size_t str8Size, const char16_t *restrict str16) {
    mbstate_t state = { 0 };
    size_t codeUnitsWritten = 0;

    while (true) {
        size_t convState = c16rtomb(convBuf8, str16[0], &state);
        if ((ssize_t)convState == -1) { // next utf-16 char is invalid
            return -1;
        }

        if (str8 != nullptr && str8Size != 0) {
            if (codeUnitsWritten >= str8Size) {
                return -1;
            }

            memcpy(str8, convBuf8, convState);
            str8 += convState;
        }

        if (convBuf8[0] == '\0') {
            break;
        }

        ++str16;
        codeUnitsWritten += convState;
    }

    return (ssize_t)codeUnitsWritten;
}

static ssize_t s32tos8(char8_t *restrict str8, size_t str8Size, const char32_t *restrict str32) {
    mbstate_t state = { 0 };
    size_t codeUnitsWritten = 0;

    while (true) {
        size_t convState = c32rtomb(convBuf8, str32[0], &state);
        if ((ssize_t)convState == -1) { // next utf-32 char is invalid
            return -1;
        }

        if (str8 != nullptr && str8Size != 0) {
            if (codeUnitsWritten >= str8Size) {
                return -1;
            }

            memcpy(str8, convBuf8, convState);
            str8 += convState;
        }

        if (convBuf8[0] == '\0') {
            break;
        }

        ++str32;
        codeUnitsWritten += convState;
    }

    return (ssize_t)codeUnitsWritten;
}
#endif

static ssize_t wstos8(char8_t *restrict str8, size_t str8Size, const wchar_t *restrict wstr) {
    if (str8Size == 0) {
        str8 = nullptr;
    }

    mbstate_t state = { 0 };
    return (ssize_t)wcsrtombs((char *restrict)str8, (const wchar_t **restrict)&wstr, str8Size, &state);
}


// These functions return the number of code units that have (or would have if strX or strXSize are 0) been written to str,
// not including the terminating null byte, or a -1 if an error occurred
// Note that the size params are in bytes, not code units

// TODO: Make sure compound chars like emoji work (e.g. face + skin colour) for all of these
// TOOD: Detect undersized buf in s8tows, sometimes mbsrtowcs reports success when it shouldn't
// TODO: Make sure dst being nullptr and/or dst size being 0 are handled correctly
// TODO: Make sure dst size is always expected to be in bytes

#ifndef __MINGW64__
static ssize_t s8tos16(char16_t *restrict str16, size_t str16Size, const char8_t *restrict str8) {
    mbstate_t state = { 0 };
    // Add 1 to account for '\0'
    size_t remainingCodeUnit8s = strlen8(str8) + 1;
    size_t codeUnit16sWritten = 0;

    bool writing = str16 != nullptr && str16Size != 0;

    while (true) {
        // This should pass a nullptr instead of convBuf16 when not writing but some versions of glibc crash if nullptr is passed
        size_t convState = mbrtoc16(convBuf16, (const char *restrict)str8, remainingCodeUnit8s, &state);
        if ((ssize_t)convState == -1 || (ssize_t)convState == -2) { // utf-8 char is invalid or truncated
            return -1;
        }

        if (writing) {
            if (codeUnit16sWritten * sizeof *convBuf16 >= str16Size || convBuf16[1] != u'\0') {
                return -1;
            }

            memcpy(str16, convBuf16, sizeof *convBuf16);
            ++str16;
        }

        if (convState == 0) {  // utf-8 char is terminator
            break;
        }

        if ((ssize_t)convState != -3) { // utf-8 code point maps to any code point other than a high surrogate in utf-16
            remainingCodeUnit8s -= convState;
            str8 += convState;
        }

        ++codeUnit16sWritten;
    }

    return (ssize_t)codeUnit16sWritten;
}

static ssize_t s8tos32(char32_t *restrict str32, size_t str32Size, const char8_t *restrict str8) {
    mbstate_t state = { 0 };
    // Add 1 to account for '\0'
    size_t remainingCodeUnit8s = strlen8(str8) + 1;
    size_t codeUnit32sWritten = 0;

    bool writing = str32 != nullptr && str32Size != 0;
    char32_t *restrict buf = writing ? convBuf32 : nullptr;

    while (true) {
        size_t convState = mbrtoc32(buf, (const char *restrict)str8, remainingCodeUnit8s, &state);
        if ((ssize_t)convState < 0) { // utf-8 char is invalid or truncated or requires surrogates in utf-32
            return -1;
        }

        if (writing) {
            if (codeUnit32sWritten * sizeof *buf >= str32Size) {
                return -1;
            }

            memcpy(str32, convBuf32, sizeof *buf);
            ++str32;
        }

        if (convState == 0) {  // utf-8 char is terminator
            break;
        }

        remainingCodeUnit8s -= convState;
        str8 += convState;
        ++codeUnit32sWritten;
    }

    return (ssize_t)codeUnit32sWritten;
}
#endif

static ssize_t s8tows(wchar_t *restrict wstr, size_t wstrSize, const char8_t *restrict str8) {
    if (wstrSize == 0) {
        wstr = nullptr;
    } else {
        wstrSize /= sizeof *wstr;
    }

    mbstate_t state = { 0 };
    return (ssize_t)mbsrtowcs(wstr, (const char **restrict)&str8, wstrSize, &state);
}

static ssize_t s16tos32(char32_t *restrict str32, size_t str32Size, const char16_t *restrict str16) {
    size_t codeUnit32sWritten = 0;
    bool writing = str32 != nullptr && str32Size != 0;

    while (true) {
        char32_t codeUnit32 = str16[0];

        if (codeUnit32 >= highSurrogateStart && codeUnit32 <= highSurrogateEnd) { // utf-16 code unit is high surrogate
            char16_t lowSurrogate = str16[1];
            if (lowSurrogate < lowSurrogateStart || lowSurrogate > lowSurrogateEnd) { // next utf-16 code unit is not expected low surrogate
                return 1;
            }

            codeUnit32 = ((codeUnit32 - highSurrogateStart) << highSurrogateShift) + lowSurrogate + surrogateOffset - lowSurrogateStart;
            if (codeUnit32 >= highSurrogateStart && codeUnit32 <= lowSurrogateEnd) { // encoded utf-32 code unit is surrogate
                return 1;
            }

            ++str16;
        } else if (codeUnit32 >= lowSurrogateStart && codeUnit32 <= lowSurrogateEnd) { // utf-16 code unit is low surrogate without leading high surrogate
            return -1;
        }

        if (writing) {
            if (codeUnit32sWritten * sizeof *str32 >= str32Size) {
                return -1;
            }

            str32[codeUnit32sWritten] = codeUnit32;
        }

        if (codeUnit32 == u'\0') { // utf-16 code unit is terminator
            break;
        }

        ++str16;
        ++codeUnit32sWritten;
    }

    return (ssize_t)codeUnit32sWritten;
}

static ssize_t s16tows(wchar_t *restrict wstr, size_t wstrSize, const char16_t *restrict str16) {
#if WCHAR_WIDTH == 16
    size_t str16Len = strlen16(str16);
    if (wstr == nullptr || wstrSize == 0) {
        return (ssize_t)str16Len;
    } else if (str16Len >= wstrSize) {
        return -1;
    }

    // Add 1 to account for u'\0'
    wmemcpy(wstr, (const wchar_t *restrict)str16, str16Len + 1);
    return (ssize_t)str16Len;
#else
    return s16tos32((char32_t *restrict)wstr, wstrSize, str16);
#endif
}

static ssize_t s32tos16(char16_t *restrict str16, size_t str16Size, const char32_t *restrict str32) {
    size_t codeUnit16sWritten = 0;
    bool writing = str16 != nullptr && str16Size != 0;

    while (true) {
        char32_t codeUnit32 = str32[0];
        bool needSurrogatePair = codeUnit32 > UINT16_MAX;
        size_t newWritten = needSurrogatePair ? 2 : 1;

        if (writing) {
            if ((codeUnit16sWritten + newWritten - 1) * sizeof *str16 >= str16Size) {
                return -1;
            }

            char16_t codeUnit16;
            if (needSurrogatePair) { // utf-32 code unit needs utf-16 surrogate pair
                codeUnit32 -= surrogateOffset;
                // high surrogate = codeUnit32 / 0x400 + 0xD800 = codeUnit32 >> 10 + 0xD800
                // low surrogate = codeUnit32 % 0x400 + 0xDC00 = codeUnit32 ^ ((codeUnit32 >> 10) << 10) + 0xDC00
                codeUnit16 = (char16_t)(codeUnit32 >> highSurrogateShift);
                str16[codeUnit16sWritten + 1] = (char16_t)(codeUnit32 ^ (char32_t)(codeUnit16 << highSurrogateShift)) + lowSurrogateStart;
                codeUnit16 += highSurrogateStart;
            } else {
                codeUnit16 = (char16_t)codeUnit32;
            }

            str16[codeUnit16sWritten] = codeUnit16;
        }

        if (codeUnit32 == U'\0') { // utf-32 code unit is terminator
            break;
        }

        ++str32;
        codeUnit16sWritten += newWritten;
    }

    return (ssize_t)codeUnit16sWritten;
}

static ssize_t s32tows(wchar_t *restrict wstr, size_t wstrSize, const char32_t *restrict str32) {
#if WCHAR_WIDTH == 16
    return s32tos16((char16_t *restrict)wstr, wstrSize, str32);
#else
    size_t str32Len = strlen32(str32);
    if (wstr == nullptr || wstrSize == 0) {
        return (ssize_t)str32Len;
    } else if (str32Len >= wstrSize) {
        return -1;
    }

    // Add 1 to account for U'\0'
    wmemcpy(wstr, (wchar_t *restrict)str32, str32Len + 1);
    return (ssize_t)str32Len;
#endif
}

static ssize_t wstos16(char16_t *restrict str16, size_t str16Size, const wchar_t *restrict wstr) {
#if WCHAR_WIDTH == 16
    return s16tows((wchar_t *restrict)str16, str16Size, (const char16_t *restrict)wstr);
#else
    return s32tos16(str16, str16Size, (const char32_t *restrict)wstr);
#endif
}

static ssize_t wstos32(char32_t *restrict str32, size_t str32Size, const wchar_t *restrict wstr) {
#if WCHAR_WIDTH == 16
    return s16tos32(str32, str32Size, (char16_t *restrict)wstr);
#else
    return s32tows((wchar_t *restrict)str32, str32Size, (const char32_t *restrict)wstr);
#endif
}


static void prints8(const char8_t *restrict str) {
    printf("%s", str);
}

static void prints16(const char16_t *restrict str) {
#if WCHAR_WIDTH == 16
    printws((const wchar_t *restrict)str);
#else
    ssize_t str8Size = s16tos8(nullptr, 0, str);
    if (str8Size < 0) {
        return;
    }

    // Add 1 to account for '\0'
    char8_t str8[str8Size + 1];
    ssize_t str8Len = s16tos8(str8, sizeof str8, str);
    if (str8Len < 0) {
        return;
    }

    prints8(str8);
#endif
}

#ifndef __MINGW64__
static void prints32(const char32_t *restrict str) {
#if WCHAR_WIDTH == 16
    ssize_t str8Size = s32tos8(nullptr, 0, str);
    if (str8Size < 0) {
        return;
    }

    // Add 1 to account for '\0'
    str8Size += 1;
    char8_t *str8 = malloc(str8Size);
    if (str8 == nullptr) {
        return;
    }

    ssize_t str8Len = s32tos8(str8, str8Size, str);
    if (str8Len < 0) {
        free(str8);
        return;
    }

    prints8(str8);
    free(str8);
#else
    printws((const wchar_t *restrict)str);
#endif
}
#endif

static void printws(const wchar_t *restrict str) {
    // TODO: Figure out why this is needed for any output, even if still wrong
#ifdef __MINGW64__
    wprintf(L"%ls", str);
#else
    printf("%ls", str);
#endif
}


// TODO: Support wchar_t blocks
#define _cat(a, b) a ## b
#define cat(a, b) _cat(a, b)
#define convert(startWidth, endWidth) \
    printf("utf-" #startWidth " as utf-" #endWidth ": "); \
    memset(cat(cat(str, endWidth), Buf), 0xFF, sizeof cat(cat(str, endWidth), Buf)); \
    if (cat(cat(cat(s, startWidth), tos), endWidth)(cat(cat(str, endWidth), Buf), sizeof cat(cat(str, endWidth), Buf), cat(str, startWidth)) == -1) { \
        printf("Failed"); \
    } else { \
        cat(prints, endWidth)(cat(cat(str, endWidth), Buf)); \
    } \
    putchar('\n')

int main(void) {
    char8_t  str8[]  = u8"这是一次测试, Ā, 🞀, a\u0304, 👨🏻‍👩🏽‍👧🏼‍👦🏾 (8)";
    char16_t str16[] =  u"这是一次测试, Ā, 🞀, a\u0304, 👨🏻‍👩🏽‍👧🏼‍👦🏾 (16)";
    char32_t str32[] =  U"这是一次测试, Ā, 🞀, a\u0304, 👨🏻‍👩🏽‍👧🏼‍👦🏾 (32)";
    wchar_t  wstr[]  =  L"这是一次测试, Ā, 🞀, a\u0304, 👨🏻‍👩🏽‍👧🏼‍👦🏾 (w)";

#ifdef _WIN32
    // Only supported offically since Windows 10 1903 but may work before that since CP_UTF8 is defined on older systems,
    // _setmode may allows wprintf to handle char16_t/wchar_t on more systems but can make printf crash
    SetConsoleOutputCP(CP_UTF8);
    // (void)_setmode(_fileno(stdout), _O_U16TEXT);
#endif
    setlocale(LC_ALL, "en_AU.utf8");

    printf("utf-8:  ");
    prints8(str8);
    putchar('\n');

    printf("utf-16: ");
    prints16(str16);
    putchar('\n');

#ifndef __MINGW64__
    printf("utf-32: ");
    prints32(str32);
    putchar('\n');
#endif

    printf("wchar:  ");
    printws(wstr);
    putchar('\n');

    char8_t str8Buf[82];
    printf("\n\"Correct\" utf-8 size is 80/81\n");

    putchar('\n');
#ifndef __MINGW64__
    printf("s16tos8: %zd\n", s16tos8(nullptr, 0, str16));
    convert(16, 8);
    printf("s32tos8: %zd\n", s32tos8(nullptr, 0, str32));
    convert(32, 8);
#endif

    // TODO: Fix -1 with mingw-w64
    printf("wstos8: %zd\n", wstos8(nullptr, 0, wstr));
    printf("wchar as utf-8:  ");
    memset(str8Buf, 0xFF, sizeof str8Buf);
    if (wstos8(str8Buf, sizeof str8Buf, wstr) == -1) {
        printf("Failed");
    } else {
        prints8(str8Buf);
    }
    putchar('\n');


    char16_t str16Buf[44];
    printf("\n\"Correct\" utf-16 size is 42/43\n");

    putchar('\n');
#ifndef __MINGW64__
    printf("s8tos16: %zd\n", s8tos16(nullptr, 0, str8));
    convert(8, 16);
#endif

    printf("s32tos16: %zd\n", s32tos16(nullptr, 0, str32));
    convert(32, 16);

    printf("wstos16: %zd\n", wstos16(nullptr, 0, wstr));
    printf("wchar as utf-16:  ");
    memset(str16Buf, 0xFF, sizeof str16Buf);
    if (wstos16(str16Buf, sizeof str16Buf, wstr) == -1) {
        printf("Failed");
    } else {
        prints16(str16Buf);
    }
    putchar('\n');


#ifndef __MINGW64__
    char32_t str32Buf[35];
    printf("\n\"Correct\" utf-32 size is 33/34\n");

    putchar('\n');
    printf("s8tos32: %zd\n", s8tos32(nullptr, 0, str8));
    convert(8, 32);
    printf("s16tos32: %zd\n", s16tos32(nullptr, 0, str16));
    convert(16, 32);

    printf("wstos32: %zd\n", wstos32(nullptr, 0, wstr));
    printf("wchar as utf-32:  ");
    memset(str32Buf, 0xFF, sizeof str32Buf);
    if (wstos32(str32Buf, sizeof str32Buf, wstr) == -1) {
        printf("Failed");
    } else {
        prints32(str32Buf);
    }
    putchar('\n');
#endif


    wchar_t wstrBuf[80];
    printf("\n\"Correct\" wchar size is 33/34(Unix-likes) and 42/43(Windows)\n");

    putchar('\n');
    // TODO: Figure out why this returns 80 with mingw-w64
    printf("s8tows: %zd\n", s8tows(nullptr, 0, str8));
    printf("utf-8 as wchar:  ");
    memset(wstrBuf, 0xFF, sizeof wstrBuf);
    if (s8tows(wstrBuf, sizeof wstrBuf, str8) == -1) {
        printf("Failed");
    } else {
        printws(wstrBuf);
    }
    putchar('\n');

    printf("s16tows: %zd\n", s16tows(nullptr, 0, str16));
    printf("utf-16 as wchar: ");
    memset(wstrBuf, 0xFF, sizeof wstrBuf);
    if (s16tows(wstrBuf, sizeof wstrBuf, str16) == -1) {
        printf("Failed");
    } else {
        printws(wstrBuf);
    }
    putchar('\n');

    printf("s32tows: %zd\n", s32tows(nullptr, 0, str32));
    printf("utf-32 as wchar: ");
    memset(wstrBuf, 0xFF, sizeof wstrBuf);
    if (s32tows(wstrBuf, sizeof wstrBuf, str32) == -1) {
        printf("Failed");
    } else {
        printws(wstrBuf);
    }
    putchar('\n');
}
