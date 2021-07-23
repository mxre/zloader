#pragma once

#include "defs.h"
#include "file.h"

#define EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID \
    { 0x964e5b22, 0x6459, 0x11d2, {0x8e, 0x39, 0x0, 0xa0, 0xc9, 0x69, 0x72, 0x3b} }

#define EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_REVISION  UINT64_C(0x00010000)

typedef struct efi_simple_file_system_protocol* efi_simple_file_system_protocol_t; 

struct efi_simple_file_system_protocol {
    uint64_t revision;

    efi_api
    efi_status_t (*open_volume) (
        efi_simple_file_system_protocol_t self,
        efi_file_handle_t *root
    );
};
