#include <efilib.h>

#include "config.h"
#include "decompress.h"
#include "util.h"
#include "pe.h"
#include "initrd.h"
#include "systemd.h"

#if USE_EFI_LOAD_IMAGE
static inline
efi_status_t image_start(efi_handle_t* image, simple_buffer_t options) {
    efi_status_t err;
    assert(BS);
    assert(EFI_IMAGE);

    efi_loaded_image_t loaded_image = NULL;
    if (buffer_len(options)) {
        err = BS->open_protocol(image, &efi_loaded_image_protocol_guid, (void**) &loaded_image, EFI_IMAGE, NULL, EFI_OPEN_PROTOCOL_GET_PROTOCOL);
        if (EFI_ERROR(err)) {
            _ERROR("Error getting a LoadedImageProtocol handle: %r", err);
            return err;
        }

        loaded_image->load_options = buffer_pos(options);
        loaded_image->load_options_size = buffer_len(options);
    }

    if (BOOT_TIME_USECS)
        efi_var_set_printf(&loader_guid, u"LoaderTimeExecUSec",
            EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
            u"%lu", monotonic_time_usec());
    _MESSAGE("StartImage: %D", loaded_image->file_path);
    err = BS->start_image(image, NULL, NULL);
    if (EFI_ERROR(err)) {
        _ERROR("Unable to start image: %r", err);
    }

    BS->close_protocol((efi_handle_t) loaded_image, &efi_loaded_image_protocol_guid, loaded_image, NULL);

    return err;
}
#endif

efi_status_t execute_image_from_memory(
    simple_buffer_t buffer,
    simple_buffer_t options
) {
    efi_status_t err;

#if USE_EFI_LOAD_IMAGE
    assert(BS);
    assert(EFI_IMAGE);

    efi_device_path_t dp = create_memory_mapped_device_path(buffer_pos(buffer), buffer_len(buffer), _EFI_POOL_ALLOCATION);
    
    efi_handle_t image;
    err = BS->load_image(false, EFI_IMAGE, (efi_device_path_t) dp, buffer_pos(buffer), buffer_len(buffer), &image);

    if (image) {
        _MESSAGE("StartImage: %D", dp);
        err = image_start(image, options);
        if (EFI_ERROR(err)) {
            _ERROR("ImageStart Error: %r", err);
        }

        BS->unload_image(image);
    }
    free(dp);
#else
    efi_entry_point_t entry_point;
    efi_handle_t image;
    efi_loaded_image_t loaded_image;
    err = PE_handle_image(buffer, &image, &loaded_image, &entry_point);

    if (!EFI_ERROR(err))  {
        if (options && buffer_len(options) > 0) {
            loaded_image->load_options = buffer_pos(options);
            loaded_image->load_options_size = buffer_len(options);
        }
        // ST->out->reset(ST->out, false);
        if (BOOT_TIME_USECS)
            efi_var_set_printf(&loader_guid, u"LoaderTimeExecUSec",
                EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                u"%lu", monotonic_time_usec());
        _MESSAGE("StartImage: %D", loaded_image->file_path);
        err = entry_point(image, ST);

        loaded_image->unload(image);
    }
    
#endif
    return err;
}

static inline
void set_systemd_variables() {
    efi_var_set_printf(&loader_guid, u"StubInfo",
        EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
        u"%s %s", LOADER_NAME, LOADER_VERSION);

    if (!efi_var_attributes(&loader_guid, u"LoaderTimeInitUSec") && BOOT_TIME_USECS) {
        efi_var_set_printf(&loader_guid, u"LoaderTimeInitUSec",
            EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
            u"%lu", BOOT_TIME_USECS);
    }

    efi_var_set_printf(&loader_guid, u"LoaderFirmwareInfo",
        EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
        u"%ls %hx.%02hx", ST->firmware_vendor, ST->firmware_revision >> 16, ST->firmware_revision);

    efi_var_set_printf(&loader_guid, u"LoaderFirmwareType",
        EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
        u"UEFI %hu.%02hu", ST->hdr.revision >> 16, ST->hdr.revision);
    
    if (!efi_var_attributes(&loader_guid, u"LoaderImageIdentifier")) {
        efi_var_set_printf(&loader_guid, u"LoaderImageIdentifier",
            EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
            u"%D", EFI_LOADED_IMAGE->file_path);
    }
    
    efi_device_path_protocol_t dp;
    BS->open_protocol(EFI_LOADED_IMAGE->device_handle, &efi_device_path_protocol_guid, (void**) &dp,
        EFI_IMAGE, NULL, EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
    _MESSAGE("EFI Root %D", dp);
    struct efi_guid part_guid;
    if (EFI_SUCCESS == get_part_uuid_from_device_path(dp, &part_guid)
        && !efi_var_attributes(&loader_guid, u"LoaderDevicePartUUID")
    ) {
        efi_var_set_printf(&loader_guid, u"LoaderDevicePartUUID",
            EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
            u"%g", part_guid);
    }   
}

efi_api
efi_status_t efi_main(
    efi_handle_t image,
    efi_system_table_t sys_table
) {
    initialize_library(image, sys_table);

    assert(EFI_LOADED_IMAGE);

    efi_status_t err = EFI_SUCCESS;
    bool secure_boot = false;
    {
        efi_size_t data_size = 1; uint8_t data[data_size];
        if (EFI_SUCCESS == efi_var_get(&efi_global_variable_guid, u"SecureBoot", NULL, &data_size, data))
            secure_boot = data[0] == 1;
    }
    if (secure_boot)
        _MESSAGE("Running in %TSECURE%N mode", EFI_GREEN);

    set_systemd_variables();

    /* get the relevant sections from the image */
    struct PE_locate_sections sections[] = {
        { .name = ".osrel",   0 },
        { .name = ".cmdline", 0 },
        { .name = ".linux",   0 },
        { .name = ".initrd",  0 },
        { 0 }
    };

    enum {
        SECTION_OSREL, SECTION_CMDLINE, SECTION_LINUX, SECTION_INITRD
    };

    if (!PE_locate_sections(sections)) {
        _ERROR("Could not read section table");
        exit(EFI_UNSUPPORTED);
    }

    if (!sections[SECTION_LINUX].load_address || !sections[SECTION_LINUX].size) {
        _ERROR("No kernel embedded");
        exit(EFI_UNSUPPORTED);
    }

    _cleanup_buffer struct simple_buffer options = { 0 };
    
    /* get cmdline from arguments or from internal cmdline section */
    if (EFI_LOADED_IMAGE->load_options_size > 0 && !secure_boot) {
        options.buffer = EFI_LOADED_IMAGE->load_options;
        options.allocated = options.length = EFI_LOADED_IMAGE->load_options_size;
        _MESSAGE("use supplied cmdline: %.*ls", options.length / (sizeof(char16_t)), (char16_t*) options.buffer);
    } else if (sections[SECTION_CMDLINE].load_address) {
        options = allocate_simple_buffer((sections[SECTION_CMDLINE].size + 1)* sizeof(char16_t));
        efi_size_t length = mbstowcs((char16_t*) options.buffer, (const char*) EFI_LOADED_IMAGE->image_base + sections[SECTION_CMDLINE].load_address, sections[SECTION_CMDLINE].size);
        options.length = length * sizeof(char16_t);
        _MESSAGE("embedded cmdline found: %.*ls", length, (char16_t*) options.buffer);
    }

    if (sections[SECTION_INITRD].load_address) {
        struct simple_buffer initrd = {
            .buffer = (uint8_t*) EFI_LOADED_IMAGE->image_base + sections[SECTION_INITRD].load_address,
            .length = sections[SECTION_INITRD].size,
            .allocated = sections[SECTION_INITRD].size,
            0
        };

        _MESSAGE("embedded initrd found: size: %zu", buffer_len(&initrd));
        _MESSAGE("initrd hash %blX", buffer_xxh64(&initrd));

        err = initrd_register(&initrd);
        if (EFI_ERROR(err)) {
            _ERROR("Failed to register initrd handler");
        }
    }

    struct simple_buffer linux_section = {
        .buffer = (uint8_t*) EFI_LOADED_IMAGE->image_base + sections[SECTION_LINUX].load_address,
        .length = sections[SECTION_LINUX].size,
        .allocated = sections[SECTION_LINUX].size,
        0
    };
    
    _cleanup_buffer struct aligned_buffer decompressed_kernel = { 0 };

    uint64_t time = monotonic_time_usec();
    err = decompress(&linux_section, &decompressed_kernel);
    if (EFI_ERROR(err)) {
        _ERROR("Decompress Error: %r", err);
        goto end;
    }

    time = monotonic_time_usec() - time;
    assert(time > 0);

    _MESSAGE(
        "decompress took %b.3f ms %b.3f MiB/s",
        time / 1000.0,
        (decompressed_kernel.length * 1024 * 1024) / (time / 1000000.0));
    _MESSAGE("kernel hash %blX", buffer_xxh64((simple_buffer_t) &decompressed_kernel));

    err = execute_image_from_memory((simple_buffer_t) &decompressed_kernel, &options);
    if (EFI_ERROR(err)) {
        _ERROR("ImageLoad Error: %r", err);
        goto end;
    }
end:
    initrd_deregister();
    exit(err);
}
