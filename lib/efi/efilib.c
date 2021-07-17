#include <efi.h>
#include <efilib.h>

efi_handle_t EFI_IMAGE = NULL;
efi_file_handle_t EFI_ROOT = NULL;
efi_system_table_t ST = NULL; 
efi_boot_services_table_t BS = NULL; 
efi_runtime_services_table_t RT = NULL; 
efi_memory_t _EFI_POOL_ALLOCATION = EFI_BOOT_SERVICES_DATA;
efi_loaded_image_t EFI_LOADED_IMAGE = NULL;

static inline
uint64_t ticks_read() {
#ifdef __x86_64__
    uint64_t a, d;
    __asm__ volatile ("rdtsc" : "=a" (a), "=d" (d));
    return (d << 32) | a;
#elif defined(__i386__)
    uint64_t val;
    __asm__ volatile ("rdtsc" : "=A" (val));
    return val;
#elif defined(__aarch64__)
    uint64_t val;
    __asm__ volatile ("mrs %0, cntvct_el0" : "=r" (val));
    return val;
#else
    return 0;
#endif
}

static uint64_t freq = 0;

static inline
uint64_t ticks_freq() {
    uint64_t ticks_start, ticks_end;

    ticks_start = ticks_read();
    stall(1000);
    ticks_end = ticks_read();

    EFI_DBG_PRINTF("counter freq: %u", ticks_end - ticks_start);

    return (ticks_end - ticks_start) * UINT64_C(1000);
}

void initialize_library(
    efi_handle_t image,
    efi_system_table_t system_table
) {
    EFI_IMAGE = image;
    
    ST = system_table;
    BS = system_table->boot_services;
    RT = system_table->runtime_services;

    freq = ticks_freq();

    if (EFI_IMAGE) {
        efi_status_t err = BS->handle_protocol(
            EFI_IMAGE, &efi_loaded_image_protocol_guid, (efi_handle_t*) &EFI_LOADED_IMAGE);
        if (EFI_ERROR(err)) {
            EFI_DBG_MESSAGE("Could not get LoadedImageProtocol");
            return;
        } else {
            _EFI_POOL_ALLOCATION = EFI_LOADED_IMAGE->image_data_type;
        }

        efi_simple_file_system_protocol_t fs;
        err = BS->handle_protocol(
            EFI_LOADED_IMAGE->device_handle, &efi_simple_fs_protocol_guid, (efi_handle_t*) &fs);
        if (EFI_ERROR(err)) {
            EFI_DBG_MESSAGE("Could not get SimpleFileSystemProtocol");
            return;
        } else {
            err = fs->open_volume(fs, &EFI_ROOT);
            if (EFI_ERROR(err)) {
                EFI_DBG_MESSAGE("Could not open root directoy");
            }
        }
    } else {
        EFI_DBG_MESSAGE("EFI_IMAGE_HANDLE was empty");
    }
}

efi_file_info_t lib_get_file_info(efi_file_handle_t handle) {
    efi_status_t err;
    size_t size = sizeof(struct efi_file_info);
    efi_file_info_t info = NULL;
    do {
        info = malloc(size);
        err = handle->get_info(handle, &efi_file_info_guid, &size, info);
        if (err == EFI_BUFFER_TOO_SMALL) {
            free(info);
            continue;
        } else if (EFI_ERROR(err)) {
            free(info);
            return NULL;
        }
        break;
    } while(true);

    return info;
}

uint64_t monotonic_time_usec() {
    uint64_t ticks;

    ticks = ticks_read();
    if (ticks == 0)
        return 0;

    if (freq == 0)
        [[ clang::unlikely ]] return 0;

    return UINT64_C(1000) * UINT64_C(1000) * ticks / freq;
}
