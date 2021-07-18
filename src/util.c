#include "util.h"

#include <efilib.h>

#include "minmax.h"

#ifdef DEBUG
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

#endif /* DEBUG */

efi_status_t image_load_from_memory(simple_buffer_t buffer, efi_handle_t* image) {
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
    mdp->start = (efi_physical_address_t) buffer_pos(buffer);
    mdp->end = ((efi_physical_address_t) buffer_pos(buffer->buffer) + buffer->length - buffer->pos);

    p = (efi_device_path_t) (++mdp);
    SetDevicePathEndNode(p);
    
    err = BS->load_image(false, EFI_IMAGE, dp, buffer_pos(buffer), buffer->length - buffer->pos, image);
    return err;
}

efi_status_t image_start(efi_handle_t* image, char16_t* options) {
    efi_status_t err;

    efi_loaded_image_t loaded_image = NULL;
    if (options) {
        err = BS->open_protocol(image, &efi_loaded_image_protocol_guid, (void**) &loaded_image, EFI_IMAGE, NULL, EFI_OPEN_PROTOCOL_GET_PROTOCOL);
        if (EFI_ERROR(err)) {
            _ERROR("Error getting a LoadedImageProtocol handle: %r", err);
            return err;
        }

        loaded_image->load_options = options;
        loaded_image->load_options_size = (wcslen(loaded_image->load_options) + 1) * sizeof(char16_t);
        
        _MESSAGE("start with options: %ls", options);
    }

    err = BS->start_image(image, NULL, NULL);
    if (EFI_ERROR(err)) {
        _ERROR("Unable to start image: %r", err);
    }

    BS->close_protocol((efi_handle_t) loaded_image, &efi_loaded_image_protocol_guid, loaded_image, NULL);

    return err;
}
