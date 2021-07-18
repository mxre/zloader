#pragma once

#include <efi.h>
#include "util.h"

efi_status_t decompress_file(
    efi_file_handle_t handle,
    simple_buffer_t buffer
);
