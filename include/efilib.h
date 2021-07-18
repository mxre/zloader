#pragma once

#include <efi.h>

#include "efilib/config.h"
#include "efilib/div.h"
#include "efilib/externs.h"
#include "efilib/debug.h"
#include "efilib/rtlib.h"
#include "efilib/file.h"
#include "efilib/guid.h"

void initialize_library(
    efi_handle_t image,
    efi_system_table_t system_table
);

static inline
efi_status_t clear_screen() {
    return ST->out->clear_screen(ST->out);
}

static inline
efi_status_t stall(efi_size_t microseconds) {
    return BS->stall(microseconds);
}

uint64_t monotonic_time_usec();
