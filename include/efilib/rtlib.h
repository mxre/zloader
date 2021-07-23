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
void* memset (
    void* buffer,
    uint8_t value,
    efi_size_t size    
) {
#if EFLLIB_USE_EFI_SET_MEM
    EFILIB_ASSERT(BS);
    BS->set_mem(buffer, size, value);
#else
    uint8_t* ptr = buffer;
    while (size--) {
        *(ptr++) = value;
    }
#endif
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
