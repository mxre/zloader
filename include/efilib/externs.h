#pragma once

#include <efi.h>

extern efi_handle_t EFI_IMAGE;
extern efi_file_handle_t EFI_ROOT;
extern efi_system_table_t ST;
extern efi_boot_services_table_t BS;
extern efi_runtime_services_table_t RT;
extern efi_memory_t _EFI_POOL_ALLOCATION;
extern efi_loaded_image_t EFI_LOADED_IMAGE;
