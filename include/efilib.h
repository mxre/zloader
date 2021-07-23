#pragma once

#include <efi.h>

#include "efilib/config.h"
#include "efilib/div.h"
#include "efilib/externs.h"
#include "efilib/debug.h"
#include "efilib/rtlib.h"
#include "efilib/string.h"
#include "efilib/file.h"
#include "efilib/guid.h"

void initialize_library(
    efi_handle_t image,
    efi_system_table_t system_table
);

static inline
efi_status_t clear_screen() {
    EFILIB_ASSERT(ST);
    return ST->out->clear_screen(ST->out);
}

static inline
efi_status_t stall(efi_size_t microseconds) {
    EFILIB_ASSERT(BS);
    return BS->stall(microseconds);
}

static inline noreturn
void exit(efi_status_t status) {
    EFILIB_ASSERT(BS);
    EFILIB_ASSERT(EFI_IMAGE);
#ifdef EFILIB_STALL_ON_EXIT
    stall(EFILIB_STALL_ON_EXIT);
#endif
    BS->exit(EFI_IMAGE, status, 0, NULL);
}

/**
 * @brief Get monotonic inreasing microsecond count
 * 
 * @details
 *  This function call a CPU instruction to query a performance
 *  counter this works in 
 */
uint64_t monotonic_time_usec();
