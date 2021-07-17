#pragma once

#include <efi.h>

efi_status_t decompress_file(
    efi_file_handle_t handle,
    void** buffer,
    size_t* buffer_size
);
