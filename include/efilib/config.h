#pragma once

/* library features */

/* enable printf functions */
#define EFILIB_PRINTF 1

/* enable text representation for EFI error codes */
#define EFILIB_ERROR_MESSAGES 1

/* enable floating point marker in runtime library */
#define EFILIB_FLOATING_POINT 1

/* use BootService CopyMem function instead of own implementation */
#define EFLLIB_USE_EFI_COPY_MEM 0

/* use BootService SetMem function instead of own implementation */
#define EFLLIB_USE_EFI_SET_MEM 0
