#include <efilib.h>

#include "decompress.h"
#include "util.h"
#include "pe.h"
#include "initrd.h"

#include <xxhash.h>

[[ maybe_unused ]]
static inline
void print_hash(simple_buffer_t buffer) {
    if (!buffer->buffer)
        return;
    struct xxh64_state xs = { 0 };
    xxh64_reset(&xs, 0);
    if (xxh64_update(&xs, buffer_pos(buffer), buffer_len(buffer))) {
        _ERROR("XXH64_ERR");
    }
    _MESSAGE("checksum %blX", xxh64_digest(&xs));
}

#if USE_EFI_LOAD_IMAGE
static inline
efi_status_t image_start(efi_handle_t* image, simple_buffer_t options) {
    efi_status_t err;

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
    struct __packed memory_mapped_device_path {
        struct efi_memory_device_path memmap;
        struct efi_device_path_protocol end;
    };

    struct memory_mapped_device_path* dp = malloc(sizeof(struct memory_mapped_device_path));
    struct memory_mapped_device_path _dp = {
        .memmap = {
            .hdr = { HARDWARE_DEVICE_PATH, HW_MEMMAP_DP, sizeof(struct efi_memory_device_path) },
            .memory_type = EFI_LOADER_CODE,
            .start = (efi_physical_address_t) buffer_pos(buffer),
	        .end = (efi_physical_address_t) buffer_pos(buffer) + buffer_len(buffer)
        },
        .end = { END_DEVICE_PATH_TYPE, END_ENTIRE_DEVICE_PATH_SUBTYPE, sizeof(struct efi_device_path_protocol) }
    };
    *dp = _dp;
    
    efi_handle_t image;
    err = BS->load_image(false, EFI_IMAGE, (efi_device_path_t) dp, buffer_pos(buffer), buffer_len(buffer), &image);

    if (image) {
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
        /* may not return */
        err = entry_point(image, ST);

        loaded_image->unload(image);
    }
    
#endif
    return err;
}

efi_status_t efi_main(
    efi_handle_t image,
    efi_system_table_t sys_table
) {
    initialize_library(image, sys_table);
    _MESSAGE("program begin");
    efi_status_t err = EFI_SUCCESS;

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
        return EFI_UNSUPPORTED;
    }

    if (!sections[SECTION_LINUX].load_address || !sections[SECTION_LINUX].size) {
        _ERROR("No kernel embedded");
        return EFI_UNSUPPORTED;
    }

    _cleanup_buffer struct simple_buffer options = { 0 };
    if (sections[SECTION_CMDLINE].load_address) {
        options = allocate_simple_buffer((sections[SECTION_CMDLINE].size + 1)* sizeof(char16_t));
        efi_size_t length = mbstowcs((char16_t*) options.buffer, (const char*) EFI_LOADED_IMAGE->image_base + sections[SECTION_CMDLINE].load_address, sections[SECTION_CMDLINE].size);
        options.length = length * sizeof(char16_t);
        _MESSAGE("embedded cmdline found: %.*ls", length, (char16_t*) options.buffer);
    }

    if (sections[SECTION_INITRD].load_address) {
        struct simple_buffer initrd = {
            .buffer = (uint8_t*) EFI_LOADED_IMAGE->image_base + sections[SECTION_INITRD].load_address,
            .length = sections[SECTION_INITRD].size, 0
        };

        _MESSAGE("embedded initrd found: size: %zu", initrd.length);
#if DEBUG
        print_hash(&initrd);
#endif
        err = initrd_register(&initrd);
        if (EFI_ERROR(err)) {
            _ERROR("Failed to register initrd handler");
        }
    }

    struct simple_buffer linux_section = {
        .buffer = (uint8_t*) EFI_LOADED_IMAGE->image_base + sections[SECTION_LINUX].load_address,
        .length = sections[SECTION_LINUX].size,
        .pos = 0,
        .allocated = 0
    };    
    _cleanup_buffer struct simple_buffer decompressed_kernel = { 0 };

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
#if DEBUG
    print_hash(&decompressed_kernel);
#endif
    err = execute_image_from_memory(&decompressed_kernel, &options);
    if (EFI_ERROR(err)) {
        _ERROR("ImageLoad Error: %r", err);
        goto end;
    }
end:
    _MESSAGE("program end");
    initrd_deregister();
#ifdef SHUTDOWN
    _MESSAGE("system is now shutting down");
    RT->reset_system(EFI_RESET_SHUTDOWN, EFI_SUCCESS, 0, NULL);
#endif
#ifdef EFILIB_STALL_ON_EXIT
    stall(EFILIB_STALL_ON_EXIT);
#endif
    return err;
}
