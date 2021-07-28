#pragma once

#include "defs.h"
#include "device_path.h"

#define EFI_LOAD_FILE_PROTOCOL_GUID \
	{ 0x56ec3091, 0x954c, 0x11d2, {0x8e, 0x3f, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b} }

#define EFI_LOAD_FILE2_PROTOCOL_GUID \
    { 0x4006c0c1, 0xfcb3, 0x403e, {0x99, 0x6d, 0x4a, 0x6c, 0x87, 0x24, 0xe0, 0x6d} }

typedef struct efi_load_file_protocol* efi_load_file_protocol_t;

struct efi_load_file_protocol {
    efi_status_t (efi_api *load_file)(
        efi_load_file_protocol_t this,
        efi_device_path_t file_path,
        bool boot_policy,
        efi_size_t *buffer_size,
        void* buffer);
};
