#pragma once

#include <efi.h>

extern efi_handle_t EFI_IMAGE;
extern efi_file_handle_t EFI_ROOT;
extern efi_system_table_t ST;
extern efi_boot_services_table_t BS;
extern efi_runtime_services_table_t RT;
extern efi_memory_t _EFI_POOL_ALLOCATION;
extern efi_loaded_image_t EFI_LOADED_IMAGE;
extern uint64_t BOOT_TIME_USECS;

#ifdef EFILIB_USE_DEVICE_PATH_TO_TEXT_PROTOCOL
extern efi_device_path_to_text_t _EFI_DEVPATH_TO_TEXT;
#endif
