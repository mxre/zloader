#pragma once

#include <efi.h>

#include "externs.h"
#include "config.h"

static inline
efi_status_t stall(efi_size_t microseconds) {
    return BS->stall(microseconds);
}

static inline noreturn
void exit(efi_status_t status) {
#ifdef EFILIB_STALL_ON_EXIT
    stall(EFILIB_STALL_ON_EXIT);
#endif
    BS->exit(EFI_IMAGE, status, 0, NULL);
}
