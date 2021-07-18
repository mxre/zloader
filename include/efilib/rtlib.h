/**
 * @file rtlib.h
 * @author Max Resch
 * @brief EFI runtime support library
 * @version 0.1
 * @date 2021-07-18
 * 
 * @copyright Copyright (c) 2021
 * 
 * @details
 *  Define runtime memory and string support functions
 *  using either inline or builtin functions.
 * 
 *  malloc, calloc, free, memcpy and memmove are symbols
 */
#pragma once

#include "efi/defs.h"
#include "efilib/debug.h"

[[ gnu::malloc, nodiscard ]]
void* malloc(
    efi_size_t size
);

[[ gnu::malloc, nodiscard ]]
void* calloc(
    efi_size_t num,
    efi_size_t size
);

void free(
    void* p
);

static inline
void bzero(
    void* buffer,
    efi_size_t size
) {
    uint8_t* ptr = buffer;
    while(size--) {
        *(ptr++) = 0;
    }
}

static inline
void* memset (
    void* buffer,
    uint8_t value,
    efi_size_t size    
) {
    uint8_t* ptr = buffer;
    while(size--) {
        *(ptr++) = value;
    }
    return buffer;
}

#if __has_builtin(__builtin_memcmp)
# define memcpy __builtin_memcpy
#else
void* memcpy (
    void* dst,
    const void* src,
    efi_size_t size   
);
#endif

#if __has_builtin(__builtin_memcmp)
# define memmove __builtin_memmove
#else
void* memmove (
    void* dst,
    const void* src,
    efi_size_t size  
);
#endif

#if __has_builtin(__builtin_memcmp)
# define memcmp __builtin_memcmp
#else
int memcmp (
    const void* a,
    const void* b,
    efi_size_t size   
);
#endif

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
    char8_t* dst,
    const char8_t* src
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
    char16_t* dst,
    const char16_t* src
) {
    while (*src) {
        *(dst++) = *(src++);
    }
    *dst = 0;
    return dst;
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
