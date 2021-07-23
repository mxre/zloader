#include "util.h"

#include <efilib.h>

#include <xxhash.h>

#include "config.h"
#include "minmax.h"

# define CHUNKED_WRITE
# ifdef CHUNKED_WRITE
#   define CHUNK ((size_t) (1024 * 1024) * 1)
#   define CHUNK_SIZE(min) MIN(CHUNK, min)
#  else
#   define CHUNK_SIZE(min) (min)
# endif

efi_status_t write_buffer_to_file(
    char16_t* filename,
    simple_buffer_t buffer
) {
    // wprintf_at(0, 5, u"write file:");

    _cleanup(close_file_p) efi_file_handle_t handle = NULL;
    efi_status_t err = open_file(filename, &handle, EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE, 0);
    if (EFI_ERROR(err)) {
        _ERROR("Unable to open file %r", err);
        return err; 
    }

    while (buffer->pos < buffer->length) {
        size_t chunk_size = CHUNK_SIZE(buffer->length - buffer->pos);
        err = handle->write(handle, &chunk_size, buffer_pos(buffer));
        if (EFI_ERROR(err)) {
            _ERROR("Unable to write file %r", err);
            return err; 
        }

        buffer->pos += chunk_size;
        //wprintf_at(12, 5, u"%3.4f%%", (((double) pos) / ((double) *size)) * 100.0);
    }

    //wprintf_at(12, 5, u" done\n");
    return EFI_SUCCESS;
}

uint64_t buffer_xxh64(simple_buffer_t buffer) {
    if (!buffer || !buffer->buffer)
        return (uint64_t) -1;
    struct xxh64_state xs = { 0 };
    xxh64_reset(&xs, 0);
    if (xxh64_update(&xs, buffer_pos(buffer), buffer_len(buffer))) {
        return (uint64_t) -1;
    } else {
        return xxh64_digest(&xs);
    }
}

/* struct to build device path */
efi_device_path_t create_memory_mapped_device_path(
    efi_physical_address_t address,
    efi_size_t size,
    efi_memory_t type
) {
    struct __packed memory_mapped_device_path {
        struct efi_memory_device_path memmap;
        struct efi_device_path_protocol end;
    };

    struct memory_mapped_device_path* dp = malloc(sizeof(struct memory_mapped_device_path));
    if (!dp)
        return NULL;
    struct memory_mapped_device_path _dp = {
        .memmap = {
            .hdr = { HARDWARE_DEVICE_PATH, HW_MEMMAP_DP, sizeof(struct efi_memory_device_path) },
            .memory_type = type,
            .start = address,
	        .end = address + size
        },
        .end = { END_DEVICE_PATH_TYPE, END_ENTIRE_DEVICE_PATH_SUBTYPE, sizeof(struct efi_device_path_protocol) }
    };
    *dp = _dp;

    return (efi_device_path_t) dp;
}
