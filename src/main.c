#include <efi.h>
#include <efilib.h>

#include "decompress.h"
#include "util.h"

efi_status_t efi_main(
    efi_handle_t image,
    efi_system_table_t sys_table
) {
    initialize_library(image, sys_table);
    _MESSAGE("program begin");
    efi_status_t err = EFI_SUCCESS;

    _cleanup(close_file_p) efi_file_handle_t handle = NULL;
    err = open_file(u"\\EFI\\Linux\\Image.zst", &handle, EFI_FILE_MODE_READ, 0);
    if (EFI_ERROR(err)) {
        printf(u"Unable to open file %r\n", err);
        return 0; 
    }

    _cleanup_pool void* buffer = NULL;
    size_t buffer_size;
    uint64_t begin_time = monotonic_time_usec();
    err = decompress_file(handle, &buffer, &buffer_size);
    if (EFI_ERROR(err)) {
        printf(u"Decompress Error: %r\n", err);
        goto end;
    }
    _MESSAGE("decompress end");

    uint64_t end_time = monotonic_time_usec();
    printf(u"decompress took %.3fs\n", (end_time - begin_time) / 1000000.0);

    // begin_time = time_usec();
    // write_buffer_to_file(u"output.txt", buffer, &buffer_size);
    // end_time = time_usec();
    //printf(u"write took %.3fs for %u bytes\n", (end_time - begin_time) / 1000000.0, buffer_size);
    efi_handle_t img = NULL;
    err = image_load_from_memory(buffer, buffer_size, &img);
    if (EFI_ERROR(err)) {
        printf(u"ImageLoad Error: %r\n", err);
        goto end;
    }

    if (img) {
        err = image_start(img, NULL);
        if (EFI_ERROR(err)) {
            printf(u"ImageStart Error: %r\n", err);
        }

        BS->unload_image(img);
    }
end:
    _MESSAGE("program end");
#ifdef SHUTDOWN
    RT->reset_system(EFI_RESET_SHUTDOWN, EFI_SUCCESS, 0, NULL);
#endif
    stall(10 * 1000000);
    return err;
}
