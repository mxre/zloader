#pragma once

#include <efi.h>
#include "debug.h"

efi_file_info_t lib_get_file_info(efi_file_handle_t handle);
