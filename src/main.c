#include <efilib.h>

#include "decompress.h"
#include "util.h"

#ifdef DEBUG
#include <xxhash.h>

static inline void print_hash(simple_buffer_t buffer) {
    struct xxh64_state xs = { 0 };
    xxh64_reset(&xs, 0);
    if (xxh64_update(&xs, buffer_pos(buffer), buffer->length - buffer->pos)) {
        _ERROR("XXH64_ERR");
    }
    _MESSAGE("checksum %blX", xxh64_digest(&xs));
}
#endif

efi_status_t efi_main(
    efi_handle_t image,
    efi_system_table_t sys_table
) {
    initialize_library(image, sys_table);
    _MESSAGE("program begin");
    efi_status_t err = EFI_SUCCESS;

    _cleanup_file_handle efi_file_handle_t handle = NULL;
    err = open_file(u"\\EFI\\Linux\\Image", &handle, EFI_FILE_MODE_READ, 0);
    if (EFI_ERROR(err)) {
        _ERROR("Unable to open file %r", err);
        return 0; 
    }

    _cleanup_buffer struct simple_buffer buffer = { 0 };
    uint64_t begin_time = monotonic_time_usec();
    err = decompress_file(handle, &buffer);
    if (EFI_ERROR(err)) {
        _ERROR("Decompress Error: %r", err);
        goto end;
    }

    uint64_t end_time = monotonic_time_usec();
    _MESSAGE(
        "decompress took %b.3f ms %b.3f MiB/s",
        (end_time - begin_time) / 1000.0,
        (buffer.length * 1024 * 1024) / ((end_time - begin_time) / 1000000.0));

#ifdef DEBUG
    print_hash(&buffer);
#endif

    efi_handle_t img = NULL;
    err = image_load_from_memory(&buffer, &img);
    if (EFI_ERROR(err)) {
        _ERROR("ImageLoad Error: %r", err);
        goto end;
    }

    if (img) {
        err = image_start(img, u"initrd=\\initrd.img  debug earlyprintk=efi");
        if (EFI_ERROR(err)) {
            _ERROR("ImageStart Error: %r", err);
        }

        BS->unload_image(img);
    }
end:
    _MESSAGE("program end");
#ifdef SHUTDOWN
    _MESSAGE("system is now shutting down");
    RT->reset_system(EFI_RESET_SHUTDOWN, EFI_SUCCESS, 0, NULL);
#endif
#ifdef DEBUG
    stall(10 * 1000000);
#endif
    return err;
}
