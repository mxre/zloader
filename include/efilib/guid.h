#pragma once

#include <efi.h>

extern struct efi_guid efi_loaded_image_protocol_guid;

extern struct efi_guid efi_simple_fs_protocol_guid;

extern struct efi_guid efi_file_info_guid;

extern struct efi_guid efi_load_file_protocol_guid;

extern struct efi_guid efi_load_file2_protocol_guid;

extern struct efi_guid efi_device_path_protocol_guid;

extern struct efi_guid efi_loaded_image_device_path_guid;

static inline
bool guidcmp(efi_guid_t a, efi_guid_t b) {
#if __SIZE_WIDTH__ == 64
    return (a->u64[0] == b->u64[0] && a->u64[1] == b->u64[1]);
#elif __SIZE_WIDTH__ == 32
    return (a->u32[0] == b->u32[0] && a->u32[1] == b->u32[1] && a->u32[2] == b->u32[2] && a->u32[3] == b->u32[3]);
#else
    return memcpy(a, b, sizeof(struct efi_guid)) == 0;
#endif
}
