#pragma once

#include <efi.h>

#include "externs.h"
#include "config.h"

static inline
efi_status_t stall(efi_size_t microseconds) {
    return BS->stall(microseconds);
}

static inline _Noreturn
void exit(efi_status_t status) {
    /* don't use macro here, file included in debug.h */
    ST->out->output_string(ST->out, u"System is now shutting down...\r\n");
#ifdef EFILIB_STALL_ON_EXIT
    stall(EFILIB_STALL_ON_EXIT);
#endif
#ifdef EFILIB_SHUTDOWN
    RT->reset_system(EFI_RESET_SHUTDOWN, status, 0, NULL);
#endif
    BS->exit(EFI_IMAGE, status, 0, NULL);
}

/**
 * @brief Get a list of handles implementing the given GUID
 * 
 * @param[in] guid
 * @param[out] length
 *  number of handles returned
 * @param[out] handles 
 * @return efi_status_t 
 */
efi_status_t lib_find_handles(
    efi_guid_t guid,
    efi_size_t* length,
    efi_handle_t** handles
);

/**
 * @brief Get the protocol interface for the first handle found
 * 
 * @param[in] guid 
 * @param[out] handle
 *  Can be NULL
 * @param[out] interface 
 * @return efi_status_t 
 */
efi_status_t lib_get_protocol_interface(
    efi_guid_t guid,
    efi_handle_t* handle,
    void** interface
);
