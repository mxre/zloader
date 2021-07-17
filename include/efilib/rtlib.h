#pragma once

#include "efi/defs.h"

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


void bzero(
    void* buffer,
    efi_size_t size
);

void* memset(
    void* buffer,
    uint8_t value,
    efi_size_t size
);

#define memcpy __builtin_memcpy
#define memmove __builtin_memmove

efi_size_t strlen (
    const char16_t* s
);

char16_t* strcpy (
    char16_t* dst,
    const char16_t* src
);
