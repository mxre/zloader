#include "util.h"

#include <efilib.h>

[[ gnu::pure, maybe_unused ]] static inline uint16_t minU16(uint16_t a, uint16_t b) { return a < b ? a : b; }
[[ gnu::pure, maybe_unused ]] static inline uint32_t minU32(uint32_t a, uint32_t b) { return a < b ? a : b; }
[[ gnu::pure, maybe_unused ]] static inline uint64_t minU64(uint64_t a, uint64_t b) { return a < b ? a : b; }

#define MIN(a, b) _Generic((a), \
    uint16_t: minU16(a, b), \
    uint32_t: minU32(a, b), \
    uint64_t: minU64(a, b)  )

#define CHUNKED_WRITE
#ifdef CHUNKED_WRITE
#define CHUNK ((size_t) (1024 * 1024) * 1)
#define CHUNK_SIZE(min) MIN(CHUNK, min)
#else
#define CHUNK_SIZE(min) (min)
#endif

efi_status_t write_buffer_to_file(
    char16_t* filename,
    const void* buffer,
    size_t* size
) {
    printf_at(0, 5, u"write file:");

    _cleanup(close_file_p) efi_file_handle_t handle = NULL;
    efi_status_t err = open_file(filename, &handle, EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE, 0);
    if (EFI_ERROR(err)) {
        printf(u"Unable to open file %r\n", err);
        return err; 
    }

    size_t pos = 0;
    while (pos < *size) {
        size_t out_size = CHUNK_SIZE((*size) - pos);
        err = handle->write(handle, &out_size, (uint8_t*) buffer + pos);
        if (EFI_ERROR(err)) {
            printf(u"Unable to write file %r\n", err);
            return err; 
        }

        pos += out_size;
        printf_at(12, 5, u"%3.4f%%", (((double) pos) / ((double) *size)) * 100.0);
    }

    printf_at(12, 5, u" done\n");

    *size = pos;

    return EFI_SUCCESS;
}

efi_status_t image_load_from_memory(void* buffer, size_t length, efi_handle_t* image) {
    efi_status_t err;

    efi_memory_device_path_t mdp;
    efi_device_path_t p;
    _cleanup_pool void* dp = malloc(sizeof(*mdp) + sizeof(*p));
    mdp = dp;

    p = dp;
    p->type = HARDWARE_DEVICE_PATH;
    p->subtype = HW_MEMMAP_DP;
    SetDevicePathNodeLength(p, sizeof(*mdp));
    mdp->memory_type = EFI_RESERVED_MEMORY_TYPE;
    mdp->start = (efi_physical_address_t) buffer;
    mdp->end = ((efi_physical_address_t) buffer) + length;

    p = (efi_device_path_t) (++mdp);
    SetDevicePathEndNode(p);
    
    err = BS->load_image(false, EFI_IMAGE, dp, buffer, length, image);
    return err;
}

efi_status_t image_start(efi_handle_t* image, char16_t* options) {
    efi_status_t err;

    efi_loaded_image_t loaded_image = NULL;
    if (options) {
        err = BS->open_protocol(image, &efi_loaded_image_protocol_guid, (void**) &loaded_image, EFI_IMAGE, NULL, EFI_OPEN_PROTOCOL_GET_PROTOCOL);
        if (EFI_ERROR(err)) {
            printf(u"Error getting a LoadedImageProtocol handle: %r\n", err);
            return err;
        }

        loaded_image->load_options = options;
        loaded_image->load_options_size = (strlen(loaded_image->load_options) + 1) * sizeof(char16_t);
        
        _MESSAGE("start with options: %s", options);
    }

    err = BS->start_image(image, NULL, NULL);
    if (EFI_ERROR(err)) {
        printf(u"Unable to start image: %r\n", err);
    }

    BS->close_protocol((efi_handle_t) loaded_image, &efi_loaded_image_protocol_guid, loaded_image, NULL);

    return err;
}
