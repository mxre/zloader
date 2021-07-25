#pragma once

#include <efi.h>

#include "externs.h"
#include "config.h"

static inline
efi_status_t stall(efi_size_t microseconds) {
    return BS->stall(microseconds);
}

static inline _Noreturn
void exit(efi_status_t status) {
    /* don't use macro here, file included in debug.h */
    ST->out->output_string(ST->out, u"System is now shutting down...\r\n");
#ifdef EFILIB_STALL_ON_EXIT
    stall(EFILIB_STALL_ON_EXIT);
#endif
#ifdef EFILIB_SHUTDOWN
    RT->reset_system(EFI_RESET_SHUTDOWN, status, 0, NULL);
#endif
    BS->exit(EFI_IMAGE, status, 0, NULL);
}
