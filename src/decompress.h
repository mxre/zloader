#pragma once

#include <efi.h>
#include "util.h"

efi_status_t decompress(
    simple_buffer_t in,
    aligned_buffer_t out
);
