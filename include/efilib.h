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
#include "efilib/misc.h"
#include "efilib/var.h"
#include "efilib/print.h"

void initialize_library(
    efi_handle_t image,
    efi_system_table_t system_table
);

static inline
efi_status_t clear_screen() {
    EFILIB_ASSERT(ST);
    return ST->out->clear_screen(ST->out);
}

/**
 * @brief Get monotonic inreasing microsecond count
 * 
 * @details
 *  This function call a CPU instruction to query a performance
 *  counter this works in 
 */
uint64_t monotonic_time_usec();
