#pragma once

#include <efi.h>
#include "debug.h"

efi_file_info_t lib_get_file_info(efi_file_handle_t handle);

static inline
efi_status_t open_file(
    const char16_t* filename,
    efi_file_handle_t* handle,
    uint64_t mode,
    uint64_t attributes
) {
    EFILIB_ASSERT(EFI_ROOT);
    return EFI_ROOT->open(EFI_ROOT, handle, filename, mode, attributes);
}
