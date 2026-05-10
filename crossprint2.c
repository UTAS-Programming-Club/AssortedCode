// Tested on Linux x86_64 via gcc14 -std=c23 and Windows x86_64 with newest visual studio C install as of 20260511
// Licensed under MIT.

#include <locale.h>     // for LC_ALL, setlocale
#include <stddef.h>     // for size_t, wchar_t, ptrdiff_t
#include <stdio.h>      // for printf, putchar
#include <stdlib.h>     // for MB_CUR_MAX
#include <string.h>     // for memchr
#include <uchar.h>      // for char16_t, char8_t, char32_t, c16rtomb
#include <wchar.h>      // for mbstate_t, wprintf

#ifdef _WIN32
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


static void wsprint(const wchar_t *restrict str);
static size_t wstrlen(const wchar_t *restrict str);


static size_t strlen8(const char8_t *restrict str) {
    return strlen((const char *)str);
}

static size_t strlens16(const char16_t *restrict str) {
#ifdef _WIN32
    return wstrlen((const wchar_t *)str);
#else
    size_t strOff;
    for (strOff = 0; str[strOff] != u'\0'; ++strOff) {
    }

    return strOff;
#endif
}

static size_t strlens32(const char32_t *restrict str) {
#ifdef _WIN32
    size_t strOff;
    for (strOff = 0; str[strOff] != U'\0'; ++strOff) {
    }

    return strOff;
#else
    return wstrlen((const wchar_t *)str);
#endif
}

static size_t wstrlen(const wchar_t *restrict str) {
    return wcslen(str);
}


static ssize_t s16tos8(char8_t *restrict buf, size_t bufSize, const char16_t *restrict str) {
    size_t strLen = strlens16(str);
    // May be larger than necessary
    size_t expectedLen = MB_CUR_MAX * strLen;
    if (buf == nullptr || bufSize == 0) {
        return (ssize_t)expectedLen;
    }
    // Add one to account for '\0'
    if (expectedLen + 1 > bufSize) {
        return -1;
    }

    mbstate_t state = { 0 };
    ssize_t totalBytesWritten = 0;

    for (size_t i = 0; i <= strLen; ++i) {
        size_t bytesWritten = c16rtomb((char *)buf, str[i], &state);
        if (bytesWritten == (size_t)-1) {
            break;
        }

        buf += bytesWritten;
        totalBytesWritten += (ssize_t)bytesWritten;
    }

    return totalBytesWritten;
}

static ssize_t s32tos8(char8_t *restrict buf, size_t bufSize, const char32_t *restrict str) {
    size_t strLen = strlens32(str);
    // May be larger than necessary
    size_t expectedLen = MB_CUR_MAX * strLen;
    if (buf == nullptr || bufSize == 0) {
        return (ssize_t)expectedLen;
    }
    // Add one to account for '\0'
    if (expectedLen + 1 > bufSize) {
        return -1;
    }

    mbstate_t state = { 0 };
    ssize_t totalBytesWritten = 0;

    for (size_t i = 0; i <= strLen; ++i) {
        size_t bytesWritten = c32rtomb((char *)buf, str[i], &state);
        if (bytesWritten == (size_t)-1) {
            break;
        }

        buf += bytesWritten;
        totalBytesWritten += (ssize_t)bytesWritten;
    }

    return totalBytesWritten;
}

// TODO: Add wstos8, s8tos16, s8tos32 & s8tows
// TODO: Add s16tos32, s16tows, s32tos16, s32tows, wstos16 & wstos32?


static void prints8(const char8_t *restrict str) {
    printf("%s", str);
}

static void prints16(const char16_t *restrict str) {
#ifdef _WIN32
    wsprint((const wchar_t *)str);
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
    wsprint((const wchar_t *)str);
#endif
}

static void wsprint(const wchar_t *restrict str) {
#ifdef _WIN32
    // printf("%s", str) prints junk chars instead of str
    wprintf(L"%ls", str);
#else
    // wprintf("%s", str) causes some later printing to be ignored, mode related?
    printf("%ls", str);
#endif
}


int main(void) {
    char8_t  str8[] = u8"这是一次测试";
    char16_t str16[] = u"这是一次测试";
    char32_t str32[] = U"这是一次测试";
    wchar_t  wstr[] = L"这是一次测试";

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
    wsprint(wstr);
    putchar('\n');
}
