#include "util.h"

#include <efilib.h>

#include "config.h"
#include "pe.h"
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
