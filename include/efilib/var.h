#pragma once

#include <efi.h>
#include "debug.h"

static inline
efi_status_t efi_var_get(
    const efi_guid_t guid,
    const char16_t* name,
    uint32_t* attributes,
    efi_size_t* size,
    void* data
) {
    EFILIB_ASSERT(RT);

    efi_status_t err = RT->get_variable(name, guid, attributes, size, data);
    if (EFI_ERROR(err))
        EFILIB_DBG_PRINTF("GetVariable {%g} %ls %r", guid, name, err);
    return err;
}

void* efi_var_get_pool(
    const efi_guid_t guid,
    const char16_t* name,
    uint32_t* attributes,
    efi_size_t* buffer_size
);

static inline
efi_status_t efi_var_set(
    const efi_guid_t guid,
    const char16_t* name,
    uint32_t attributes,
    efi_size_t size,
    const void* data
) {
    EFILIB_ASSERT(RT);

    efi_status_t err = RT->set_variable(name, guid, attributes, size, data);
    if (EFI_ERROR(err))
        EFILIB_DBG_PRINTF("SetVariable {%g} %ls %r", guid, name, err);
    return err;
}

#if EFILIB_PRINTF
efi_size_t efi_var_set_printf(
    const efi_guid_t guid,
    const char16_t* name,
    uint32_t attributes,
    const char16_t* fmt, ...
);
#endif

static inline
uint32_t efi_var_attributes(
    const efi_guid_t guid,
    const char16_t* name
) {
    efi_size_t size = 0; 
    uint32_t attributes = 0;
    efi_status_t err = RT->get_variable(name, guid, &attributes, &size, NULL);
    if (err == EFI_BUFFER_TOO_SMALL)
        return attributes;
    else
        EFILIB_DBG_PRINTF("GetVariable {%g} %ls %r", guid, name, err);
    return 0;
}
