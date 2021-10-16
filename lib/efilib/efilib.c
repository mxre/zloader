#include <efi.h>
#include <efilib.h>

#include <minmax.h>
#include "efi/pe.h"

efi_handle_t EFI_IMAGE = NULL;
efi_file_handle_t EFI_ROOT = NULL;
efi_system_table_t ST = NULL;
efi_boot_services_table_t BS = NULL;
efi_runtime_services_table_t RT = NULL;
efi_memory_t _EFI_POOL_ALLOCATION = EFI_BOOT_SERVICES_DATA;
efi_loaded_image_t EFI_LOADED_IMAGE = NULL;
uint64_t BOOT_TIME_USECS = 0;

#ifdef EFILIB_USE_DEVICE_PATH_TO_TEXT_PROTOCOL
efi_device_path_to_text_t _EFI_DEVPATH_TO_TEXT = NULL;
#endif

/**
 * @brief Use assembly instruction to read CPU ticks counter
 */
static inline
uint64_t ticks_read() {
    /* x64 and some i686 may know rdtscp which could be more accurate
       but this requires feature testing with cpu id */
#ifdef __x86_64__
    uint64_t a, d;
    __asm__ volatile ("rdtsc" : "=a"(a), "=d"(d));
    return (d << 32) | a;
#elif defined(__i386__)
    uint64_t val;
    __asm__ volatile ("rdtsc" : "=A"(val));
    return val;
#elif defined(__aarch64__)
    uint64_t val;
    __asm__ volatile ("mrs %0, cntpct_el0" : "=r"(val));
    return val;
#else
    return 0;
#endif
}

static uint64_t freq = 0;

/**
 * @brief measure 1ms in cpu ticks
 */
static inline
uint64_t ticks_freq() {
    uint64_t freq;
#if defined(__aarch64__)
    __asm__ volatile ("mrs %0, cntfrq_el0": "=r"(freq));
#else
    uint64_t ticks_start;
    ticks_start = ticks_read();
    stall(500);
    freq = (ticks_read() - ticks_start) / UINT64_C(500);
#endif
    EFILIB_DBG_PRINTF("boottime: %.4fms counter freq: %lu", UINT64_C(1000) * BOOT_TIME_USECS / (double) freq, freq);
    return freq;
}

static inline
uint32_t get_pe_subsystem_version() {
    PE_image_headers_t pe = (PE_image_headers_t) ((uint8_t*) EFI_LOADED_IMAGE->image_base
        + *(uint32_t*)((uint8_t*) EFI_LOADED_IMAGE->image_base + DOS_PE_OFFSET_LOCATION));
    if (pe->file_header.signature != PE_HEADER_SIGNATURE) {
        EFILIB_ERROR("LoadedImageProtocol::ImageBase is not a valid executable");
        exit(EFI_UNSUPPORTED);
    }
    return (uint32_t) pe->optional_header.subsystem_version.major << 16 | pe->optional_header.subsystem_version.minor;

    EFILIB_ERROR("LoadedImageProtocol::ImageBase is not a valid executable");
    exit(EFI_UNSUPPORTED);
}

void initialize_library(
    efi_handle_t image,
    efi_system_table_t system_table
) {
    BOOT_TIME_USECS = ticks_read();
    EFI_IMAGE = image;

    ST = system_table;
    BS = system_table->boot_services;
    RT = system_table->runtime_services;

    EFILIB_DBG_PRINTF("%ls %hX.%hX UEFI %hu.%hu",
        ST->firmware_vendor,
        ST->firmware_revision >> 16, ST->firmware_revision,
        ST->hdr.revision >> 16, ST->hdr.revision );
    EFILIB_DBG_PRINTF("SystemTable    : %.8s", (char8_t*) &ST->hdr.signature);
    EFILIB_DBG_PRINTF("BootServices   : %.8s", (char8_t*) &BS->hdr.signature);
    EFILIB_DBG_PRINTF("RuntimeServices: %.8s", (char8_t*) &RT->hdr.signature);

    /* UEFI Docs suggest that version 1.0 is sufficient for our api usage */
    if (ST->hdr.revision < 0x0001000) {
        EFILIB_ERROR("UEFI implementation too old");
        exit(EFI_INCOMPATIBLE_VERSION);
    }

    EFILIB_DBG_PRINTF("Console: { mode: %u attr: %X (%d, %d) }",
        ST->out->mode->mode, ST->out->mode->attribute,
        ST->out->mode->cursor_column, ST->out->mode->cursor_row );

    freq = ticks_freq();
    BOOT_TIME_USECS = UINT64_C(1000000) * BOOT_TIME_USECS / freq;

    if (!EFI_IMAGE) {
        EFILIB_ERROR("EFI_IMAGE_HANDLE was empty");
        exit(EFI_UNSUPPORTED);
    }

    efi_status_t err = BS->open_protocol(
        EFI_IMAGE, &efi_loaded_image_protocol_guid, (efi_handle_t*) &EFI_LOADED_IMAGE,
        EFI_IMAGE, NULL, EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
    if (EFI_ERROR(err)) {
        EFILIB_ERROR("Could not get LoadedImageProtocol");
        exit(err);
    } else {
        _EFI_POOL_ALLOCATION = EFI_LOADED_IMAGE->image_data_type;
    }

    if (!EFI_LOADED_IMAGE->image_base) {
        EFILIB_ERROR("LoadedImageProtocol::ImageBase is NULL");
        exit(EFI_UNSUPPORTED);
    }

    uint32_t subsystem_version = get_pe_subsystem_version();
    if (ST->hdr.revision < subsystem_version) {
        EFILIB_ERROR_PRINTF("Executable requires at least UEFI %hu.%hu", subsystem_version >> 16, subsystem_version);
        exit(EFI_INCOMPATIBLE_VERSION);
    }

    efi_simple_file_system_protocol_t fs;
    err = BS->open_protocol(
        EFI_LOADED_IMAGE->device_handle, &efi_simple_fs_protocol_guid, (efi_handle_t*) &fs,
        EFI_IMAGE, NULL, EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
    if (EFI_ERROR(err)) {
        EFILIB_ERROR("Could not get SimpleFileSystemProtocol");
        exit(EFI_INCOMPATIBLE_VERSION);
    } else {
        err = fs->open_volume(fs, &EFI_ROOT);
        if (EFI_ERROR(err)) {
            EFILIB_ERROR("Could not open root directoy");
            exit(err);
        }
    }

#ifdef EFILIB_USE_DEVICE_PATH_TO_TEXT_PROTOCOL
    err = lib_get_protocol_interface(&efi_device_path_to_text_guid, NULL, (void**) &_EFI_DEVPATH_TO_TEXT);
    if (EFI_ERROR(err))
        EFILIB_ERROR("DevicePathToTextProtocol not found");
#endif
    EFILIB_DBG_MESSAGE("Initalization Done");
}

efi_file_info_t lib_get_file_info(efi_file_handle_t handle) {
    efi_status_t err;
    size_t size = sizeof(struct efi_file_info);
    efi_file_info_t info = NULL;
    do {
        info = malloc(size);
        if (!info)
            return NULL;
        err = handle->get_info(handle, &efi_file_info_guid, &size, info);
        if (err == EFI_BUFFER_TOO_SMALL) {
            free(info);
            continue;
        }
    } while(err == EFI_BUFFER_TOO_SMALL);
    if (EFI_ERROR(err)) {
        free(info);
        return NULL;
    }

    return info;
}

efi_status_t lib_find_handles(
    efi_guid_t guid,
    efi_size_t* length,
    efi_handle_t** handles
) {
    efi_status_t err;
    efi_size_t sz = sizeof(efi_handle_t);
    do {
        *handles = malloc(sz);
        if (!*handles)
            return EFI_OUT_OF_RESOURCES;
        err = BS->locate_handle(EFI_SEARCH_BY_PROTOCOL, guid, NULL, &sz, *handles);
        if (err == EFI_BUFFER_TOO_SMALL)
            free(*handles);
    } while (err == EFI_BUFFER_TOO_SMALL);
    if (EFI_ERROR(err)) {
        free(*handles);
        return err;
    }
    *length = sz / sizeof(efi_handle_t);
    return EFI_SUCCESS;
}

efi_status_t lib_get_protocol_interface(
    efi_guid_t guid,
    efi_handle_t* handle,
    void** interface
) {
    efi_status_t err;
    efi_size_t length;
    efi_handle_t* handles;
    err = lib_find_handles(guid, &length, &handles);
    if (EFI_ERROR(err))
        return err;
    for (efi_size_t i = 0; i < length; i++) {
        err = BS->open_protocol(handles[i], guid, interface,
            EFI_IMAGE, NULL, EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
        if (err == EFI_SUCCESS) {
            if (*handle)
                *handle = handles[i];
            break;
        }
        if (EFI_ERROR(err))
            EFILIB_DBG_PRINTF("OpenProtocol {%g}: %r", guid, err);
    }
    free(handles);
    if (EFI_ERROR(err))
        return err;

    return EFI_SUCCESS;
}

uint64_t monotonic_time_usec() {
    uint64_t ticks;

    ticks = ticks_read();
    if (ticks == 0)
        __unlikely__ return 0;

    if (freq == 0)
        __unlikely__ return 0;

    return UINT64_C(1000000) * ticks / freq;
}
