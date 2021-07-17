#pragma once

#include "defs.h"
#include "memory.h"

struct efi_capsule_block_descriptor {
    uint64_t length;
    union {
        efi_physical_address_t data_block;
        efi_physical_address_t continuation_pointer;
    };
};

typedef struct efi_capsule_block_descriptor* efi_capsule_block_descriptor_t;

struct efi_capsule_header {
    efi_guid_t capsule_guid;
    uint32_t header_size;
    uint32_t flags;
    uint32_t capsule_image_size;
};

typedef struct efi_capsule_header* efi_capsule_header_t;

#define EFI_CAPSULE_FLAGS_PERSIST_ACROSS_RESET  UINT32_C(0x00010000)
#define EFI_CAPSULE_FLAGS_POPULATE_SYSTEM_TABLE UINT32_C(0x00020000)
#define EFI_CAPSULE_FLAGS_INITIATE_RESET        UINT32_C(0x00040000)
