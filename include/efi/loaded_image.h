#pragma once

#include "defs.h"
#include "system_table.h"
#include "device_path.h"

#define EFI_LOADED_IMAGE_PROTOCOL_GUID \
    { 0x5b1b31a1, 0x9562, 0x11d2, {0x8e, 0x3f, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b} }

#define EFI_LOADED_IMAGE_DEVICE_PATH_PROTOCOL_GUID \
    { 0xbc62157e, 0x3e33, 0x4fec, {0x99 ,0x20, 0x2d ,0x3b, 0x36, 0xd7, 0x50, 0xdf} }

#define EFI_LOADED_IMAGE_PROTOCOL_REVISION UINT32_C(0x1000)

typedef struct efi_loaded_image_protocol* efi_loaded_image_protocol_t;
typedef struct efi_loaded_image_protocol* efi_loaded_image_t;

struct efi_loaded_image_protocol {
    uint32_t revision;
    efi_handle_t parent_handle;
    efi_system_table_t system_table;

    efi_handle_t device_handle;
    efi_device_path_t file_path;
    void *reserved;

    uint32_t load_options_size;
    void *load_options;

    void *image_base;
    efi_size_t image_size;
    efi_memory_t image_code_type;
    efi_memory_t image_data_type;

    efi_status_t (efi_api *unload) (
        efi_handle_t image_handle
    );
};
