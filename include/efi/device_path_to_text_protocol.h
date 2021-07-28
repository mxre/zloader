#pragma once

#include "defs.h"
#include "device_path.h"

#define EFI_DEVICE_PATH_TO_TEXT_PROTOCOL_GUID \
    { 0x8b843e20, 0x8132, 0x4852, {0x90, 0xcc, 0x55, 0x1a, 0x4e, 0x4a, 0x7f, 0x1c} }

struct efi_device_path_to_text_protocol {
    char16_t* (efi_api *device_node_to_text) (
        const efi_device_path_t node,
        bool display_only,
        bool allow_shortcuts);
    
    char16_t* (efi_api *device_path_to_text) (
        const efi_device_path_t device_path,
        bool display_only,
        bool allow_shortcuts);
};

typedef struct efi_device_path_to_text_protocol* efi_device_path_to_text_t;
