// Tested on Linux x86_64 via gcc14 -std=c23 and Windows x86_64 with newest visual studio C install as of 20260511
// Licensed under MIT.

#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <locale.h>     // for LC_ALL, setlocale
#include <stddef.h>     // for size_t, wchar_t
#include <stdio.h>      // for printf, putchar
#include <string.h>     // for memset, memcpy, strlen
#include <uchar.h>      // for char8_t, char16_t, char32_t, c16rtomb, c32rtomb, mbrtoc16, mbrtoc32
#include <wchar.h>      // for mbstate_t, mbsrtowcs, wcslen, wcsrtombs

#ifdef _WIN32
#include <stdbool.h>    // for bool, true
#include <Windows.h>    // for CP_UTF8, SSIZE_T, SetConsoleOutputCP

#define nullptr NULL
typedef SSIZE_T ssize_t;
typedef unsigned char char8_t;
#else
#include <sys/types.h>  // for ssize_t
#endif

#ifndef __STDC_UTF_16__
#error Need UTF-16 support
#endif
#ifndef __STDC_UTF_32__
#error Need UTF-32 support
#endif

static char convBuf8[4];
static char16_t convBuf16[2];
// TODO: Confirm 1 char is fine, all chars are meant to fit in 1 utf-32 code point but mbrtoc32 outputs a "variable-length 32-bit wide character"
static char32_t convBuf32[1];

static void printws(const wchar_t *restrict str);
static size_t strlenw(const wchar_t *restrict str);


// These functions returns the number of bytes in string

static size_t strlen8(const char8_t *restrict str) {
    return strlen((const char *)str);
}

static size_t strlen16(const char16_t *restrict str) {
#ifdef _WIN32
    return strlenw((const wchar_t *restrict)str);
#else
    size_t strOff;
    for (strOff = 0; str[strOff] != u'\0'; ++strOff) {
    }

    return strOff;
#endif
}

static size_t strlen32(const char32_t *restrict str) {
#ifdef _WIN32
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

static ssize_t wstos8(char8_t *restrict str8, size_t str8Size, const wchar_t *restrict wstr) {
    if (str8Size == 0) {
        str8 = nullptr;
    }

    mbstate_t state = { 0 };
    return (ssize_t)wcsrtombs((char *restrict)str8, (const wchar_t **restrict)&wstr, str8Size, &state);
}


// These functions return the number of code units that have (or would have if strX or strXSize are 0) been written to str,
// not including the terminating null byte, or a -1 if an error occurred

// TODO: Make sure compound chars like emoji work (e.g. face + skin colour) for all of these
// TODO: Add s16tos32, s16tows, s32tos16, s32tows, wstos16 & wstos32
// TOOD: Detect undersized buf in s8tows, sometimes mbsrtowcs reports success when it shouldn't

static ssize_t s8tos16(char16_t *restrict str16, size_t str16Size, const char8_t *restrict str8) {
    mbstate_t state = { 0 };
    size_t remainingCodeUnits = strlen8(str8) + 1;
    size_t codeUnitsWritten = 0;

    bool writing = str16 != nullptr && str16Size != 0;
    char16_t *restrict buf = writing ? convBuf16 : nullptr;

    while (true) {
        size_t convState = mbrtoc16(buf, (const char *restrict)str8, remainingCodeUnits, &state);
        if ((ssize_t)convState == -1 || (ssize_t)convState == -2) { // utf-8 char is invalid or truncated
            return -1;
        }

        if (writing) {
            if (codeUnitsWritten * sizeof *buf >= str16Size || convBuf16[1] != u'\0') {
                return -1;
            }

            memcpy(str16, convBuf16, sizeof *buf);
            ++str16;
        }

        if (convState == 0) {  // utf-8 char is terminator
            break;
        }

        if ((ssize_t)convState != -3) { // utf-8 code point maps to any code point other than a high surrogate in utf-16
            remainingCodeUnits -= convState;
            str8 += convState;
        }

        ++codeUnitsWritten;
    }

    return (ssize_t)codeUnitsWritten;
}

static ssize_t s8tos32(char32_t *restrict str32, size_t str32Size, const char8_t *restrict str8) {
    mbstate_t state = { 0 };
    size_t remainingCodeUnits = strlen8(str8) + 1;
    size_t codeUnitsWritten = 0;

    bool writing = str32 != nullptr && str32Size != 0;
    char32_t *restrict buf = writing ? convBuf32 : nullptr;

    while (true) {
        size_t convState = mbrtoc32(buf, (const char *restrict)str8, remainingCodeUnits, &state);
        if ((ssize_t)convState < 0) { // utf-8 char is invalid or truncated or requires surrogates in utf-32
            return -1;
        }

        if (writing) {
            if (codeUnitsWritten * sizeof *buf >= str32Size) {
                return -1;
            }

            memcpy(str32, convBuf32, sizeof *buf);
            ++str32;
        }

        if (convState == 0) {  // utf-8 char is terminator
            break;
        }

        remainingCodeUnits -= convState;
        str8 += convState;
        ++codeUnitsWritten;
    }

    return (ssize_t)codeUnitsWritten;
}

static ssize_t s8tows(wchar_t *restrict wstr, size_t wstrSize, const char8_t *restrict str8) {
    if (wstrSize == 0) {
        wstr = nullptr;
    }

    mbstate_t state = { 0 };
    return (ssize_t)mbsrtowcs(wstr, (const char **restrict)&str8, wstrSize, &state);
}


static void prints8(const char8_t *restrict str) {
    printf("%s", str);
}

static void prints16(const char16_t *restrict str) {
#ifdef _WIN32
    printws((const wchar_t *restrict)str);
#else
    ssize_t str8Size = s16tos8(nullptr, 0, str);
    if (str8Size < 0) {
        return;
    }

    // Add one to account for '\0'
    char8_t str8[str8Size + 1];
    ssize_t str8Len = s16tos8(str8, sizeof str8, str);
    if (str8Len < 0) {
        return;
    }

    prints8(str8);
#endif
}

static void prints32(const char32_t *restrict str) {
#ifdef _WIN32
    ssize_t str8Size = s32tos8(nullptr, 0, str);
    if (str8Size < 0) {
        return;
    }

    // Add one to account for '\0'
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

static void printws(const wchar_t *restrict str) {
    printf("%ls", str);
}


int main(void) {
    char8_t  str8[]  = u8"这是一次测试(8), Ā, 🞀";
    char16_t str16[] =  u"这是一次测试(16), Ā, 🞀";
    char32_t str32[] =  U"这是一次测试(32), Ā, 🞀";
    wchar_t  wstr[]  =  L"这是一次测试(w), Ā, 🞀";

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

    printf("utf-32: ");
    prints32(str32);
    putchar('\n');

    printf("wchar:  ");
    printws(wstr);
    putchar('\n');

    char8_t str8Buf[33];

    printf("\nutf-16 as utf-8: ");
    memset(str8Buf, 0xFF, sizeof str8Buf);
    if (s16tos8(str8Buf, sizeof str8Buf, str16) == -1) {
        printf("Failed");
    }
    else {
        prints8(str8Buf);
    }
    putchar('\n');

    printf("utf-32 as utf-8: ");
    memset(str8Buf, 0xFF, sizeof str8Buf);
    if (s32tos8(str8Buf, sizeof str8Buf, str32) == -1) {
        printf("Failed");
    }
    else {
        prints8(str8Buf);
    }
    putchar('\n');

    printf("wchar as utf-8:  ");
    memset(str8Buf, 0xFF, sizeof str8Buf);
    if (wstos8(str8Buf, sizeof str8Buf, wstr) == -1) {
        printf("Failed");
    }
    else {
        prints8(str8Buf);
    }
    putchar('\n');


    char16_t str16Buf[18];

    printf("\nutf-8 as utf-16:  ");
    memset(str16Buf, 0xFF, sizeof str16Buf);
    if (s8tos16(str16Buf, sizeof str16Buf, str8) == -1) {
        printf("Failed");
    }
    else {
        prints16(str16Buf);
    }
    putchar('\n');

    printf("utf-32 as utf-16: ");
    memset(str8Buf, 0xFF, sizeof str8Buf);
    memset(str16Buf, 0xFF, sizeof str16Buf);
    if (s32tos8(str8Buf, sizeof str8Buf, str32) == -1 || s8tos16(str16Buf, sizeof str16Buf, str8Buf) == -1) {
        printf("Failed");
    }
    else {
        prints16(str16Buf);
    }
    putchar('\n');

    printf("wchar as utf-16:  ");
    memset(str8Buf, 0xFF, sizeof str8Buf);
    memset(str16Buf, 0xFF, sizeof str16Buf);
    if (wstos8(str8Buf, sizeof str8Buf, wstr) == -1 || s8tos16(str16Buf, sizeof str16Buf, str8Buf) == -1) {
        printf("Failed");
    }
    else {
        prints16(str16Buf);
    }
    putchar('\n');


    char32_t str32Buf[17];

    printf("\nutf-8 as utf-32:  ");
    memset(str32Buf, 0xFF, sizeof str32Buf);
    if (s8tos32(str32Buf, sizeof str32Buf, str8) == -1) {
        printf("Failed");
    }
    else {
        prints32(str32Buf);
    }
    putchar('\n');

    printf("utf-16 as utf-32: ");
    memset(str8Buf, 0xFF, sizeof str8Buf);
    memset(str32Buf, 0xFF, sizeof str32Buf);
    if (s16tos8(str8Buf, sizeof str8Buf, str16) == -1 || s8tos32(str32Buf, sizeof str32Buf, str8Buf) == -1) {
        printf("Failed");
    }
    else {
        prints32(str32Buf);
    }
    putchar('\n');

    printf("wchar as utf-32:  ");
    memset(str8Buf, 0xFF, sizeof str8Buf);
    memset(str32Buf, 0xFF, sizeof str32Buf);
    if (wstos8(str8Buf, sizeof str8Buf, wstr) == -1 || s8tos32(str32Buf, sizeof str32Buf, str8Buf) == -1) {
        printf("Failed");
    }
    else {
        prints32(str32Buf);
    }
    putchar('\n');


    wchar_t wstrBuf[18];

    printf("\nutf-8 as wchar:  ");
    memset(wstrBuf, 0xFF, sizeof wstrBuf);
    if (s8tows(wstrBuf, sizeof wstrBuf, str8) == -1) {
        printf("Failed");
    }
    else {
        printws(wstrBuf);
    }
    putchar('\n');

    printf("utf-16 as wchar: ");
    memset(str8Buf, 0xFF, sizeof str8Buf);
    memset(wstrBuf, 0xFF, sizeof wstrBuf);
    if (s16tos8(str8Buf, sizeof str8Buf, str16) == -1 || s8tows(wstrBuf, sizeof wstrBuf, str8Buf) == -1) {
        printf("Failed");
    }
    else {
        printws(wstrBuf);
    }
    putchar('\n');

    printf("utf-32 as wchar: ");
    memset(str8Buf, 0xFF, sizeof str8Buf);
    memset(wstrBuf, 0xFF, sizeof wstrBuf);
    if (s32tos8(str8Buf, sizeof str8Buf, str32) == -1 || s8tows(wstrBuf, sizeof wstrBuf, str8Buf) == -1) {
        printf("Failed");
    }
    else {
        printws(wstrBuf);
    }
    putchar('\n');
}
