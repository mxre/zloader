#include "util.h"

#include <efilib.h>
#include <minmax.h>
#include <xxhash.h>

#include "config.h"

efi_status_t get_part_uuid_from_device_path(efi_device_path_t path, efi_guid_t guid) {
    assert(path);
    assert(guid);

    for (efi_device_path_t dp = path; !IsDevicePathEndNode(dp); dp = NextDevicePathNode(dp)) {
        if (IsDevicePathNode(dp, MEDIA_DEVICE_PATH, MEDIA_PARTITION_DP))
            continue;
        efi_partition_device_path_t part = (efi_partition_device_path_t) dp;
        if (part->signature_type != SIGNATURE_TYPE_GUID)
            continue;
        guidcpy(part->signature, guid);
    }

    return EFI_NOT_FOUND;
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
