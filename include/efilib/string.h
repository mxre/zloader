/**
 * @file rtstring.h
 * @author Max Resch
 * @brief runtime string library
 * @version 0.1
 * @date 2021-07-20
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#pragma once

#include "efi/defs.h"
#include "efilib/debug.h"

static_assert(
    sizeof(wchar_t) == sizeof(char16_t),
    "Compiler needs -fshort-wchar for builtins to work");

/**
 * @brief strlen for ASCII
 */
#if __has_builtin(__builtin_strlen)
# define strlen __builtin_strlen
#else
efi_size_t strlen (
    const char8_t* s
);
#endif

#if __has_builtin(__builtin_strcmp)
# define strcmp __builtin_strcmp
#else
static inline
int strcmp (
    const char8_t* a,
    const char8_t* b
);
#endif

#if __has_builtin(__builtin_strncmp)
# define strncmp __builtin_strncmp
#else
static inline
int strncmp (
    const char8_t* a,
    const char8_t* b,
    size_t length
);
#endif

#if __has_builtin(__builtin_strchr)
# define strchr __builtin_strchr
#else
char8_t* strchr (
    const char8_t* str,
    int needle
);
#endif

/**
 * @brief strcpy for ASCII
 */
static inline
char8_t* strcpy (
    char8_t* restrict dst,
    const char8_t* restrict src
) {
    while (*src) {
        *(dst++) = *(src++);
    }
    *dst = 0;
    return dst;
}

/**
 * @brief strchr but returns pointer to end
 */
static inline
char8_t* strchrnul (
    const char8_t* str,
    int needle
) {	
    do {
		if (*str == needle)
			return (char8_t*) str;
	} while (*(++str));

	return (char8_t*) str - 1;
}

/**
 * @brief strlen for UTF16
 */
#if __has_builtin(__builtin_wcslen)
# define wcslen __builtin_wcslen
#else
efi_size_t wcslen (
    const char16_t* s
);
#endif

#if __has_builtin(__builtin_wcscmp)
# define wcscmp __builtin_wcscmp
#else
int wcscmp (
    const char16_t* a,
    const char16_t* b
);
#endif

#if __has_builtin(__builtin_wcschr)
# define wcschr __builtin_wcschr
#else
char16_t* wcschr (
    const char16_t* str,
    char16_t needle
);
#endif

/**
 * @brief strcpy for UTF16
 */
static inline
char16_t* wcscpy (
    char16_t* restrict dst,
    const char16_t* restrict src
) {
    while (*src)
        *(dst++) = *(src++);
    return dst;
}

static inline
char16_t* wcsncpy (
    char16_t* restrict dst,
    const char16_t* restrict src,
    efi_size_t n
) {
    while (n-- && *src) {
        *(dst++) = *(src++);
    }
    return dst;
}

static inline
char16_t* wcsncat(
    char16_t* restrict dst,
    const char16_t* restrict src,
    efi_size_t n
) {
    while (*dst) dst++;
    return wcsncpy(dst, src, n);
}

/**
 * @brief wcschr but returns pointer to end
 */
static inline
char16_t* wcschrnul (
    const char16_t* str,
    char16_t needle
) {	
    do {
		if (*str == needle)
			return (char16_t*) str;
	} while (*(++str));

	return (char16_t*) str - 1;
}

static inline
int mbtowc(
    char16_t* restrict pwc,
    const char8_t* restrict s,
    size_t n
) {
    if (n == 0)
        return -1;

    int len;
    /* assure unsinged char */
    const uint8_t* a = (const uint8_t*) s;

    if (a[0] < 0x80)
        len = 1;
    else if ((a[0] & 0xe0) == 0xc0)
        len = 2;
    else if ((a[0] & 0xf0) == 0xe0)
        len = 3;
    else if ((a[0] & 0xf8) == 0xf0)
        len = 4;
    else if ((a[0] & 0xfc) == 0xf8)
        len = 5;
    else if ((a[0] & 0xfe) == 0xfc)
        len = 6;
    else
        return -1;

    if (len > n)
        return -1;

    char16_t unichar;
    switch (len) {
        case 1:
            unichar = a[0];
            break;
        case 2:
            unichar = a[0] & 0x1f;
            break;
        case 3:
            unichar = a[0] & 0x0f;
            break;
        case 4:
            unichar = a[0] & 0x07;
        break;
        case 5:
            unichar = a[0] & 0x03;
            break;
        case 6:
            unichar = a[0] & 0x01;
            break;
    }

    for (int i = 1; i < len; i++) {
        if ((a[i] & 0xc0) != 0x80)
            return -1;
        unichar <<= 6;
        unichar |= a[i] & 0x3f;
    }

    *pwc = unichar;
    return len;
}

/**
 * @brief Convert UTF8 string to UTF16
 * 
 * @param dest
 *  pointer with enough space for the string
 * @param src
 *  pointer to UTF8 string
 * @param len
 *  length of the source string
 * @return efi_size_t
 *  number of characters in the resulting string
 */
efi_size_t mbstowcs(
    char16_t* restrict dest,
    const char8_t* restrict src,
    efi_size_t len
);

