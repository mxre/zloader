/**
 * @file initrd.c
 * @author Max Resch
 * @brief initrd loader via LoadFile2 protocol
 * @version 0.1
 * @date 2021-07-18
 * 
 * @copyright Copyright (c) 2021
 * 
 * @see https://github.com/u-boot/u-boot/blob/v2021.07/lib/efi_loader/efi_load_initrd.c
 * @see https://github.com/torvalds/linux/blob/v5.13/drivers/firmware/efi/libstub/efi-stub-helper.c
 */

#include "initrd.h"

#include <string.h>
#include <assert.h>
#include <efilib.h>
#include "util.h"

/**
 * @brief static device path for initrd
 * 
 * @see https://github.com/torvalds/linux/blob/v5.13/drivers/firmware/efi/libstub/efi-stub-helper.c
 */
static const struct __packed efi_initrd_device_path {
    struct efi_vendor_device_path vendor;
    struct efi_device_path_protocol end;
} efi_initrd_device_path = {
    .vendor = {
        { MEDIA_DEVICE_PATH, MEDIA_VENDOR_DP, sizeof(efi_initrd_device_path.vendor) },
        {{ LINUX_INITRD_MEDIA_GUID }}
    },
    .end = {  END_DEVICE_PATH_TYPE, END_ENTIRE_DEVICE_PATH_SUBTYPE, sizeof(efi_initrd_device_path.end) }
};

efi_api
efi_status_t fl2_load_file(
    efi_load_file_protocol_t this,
    efi_device_path_t file_path,
    bool boot_policy,
    efi_size_t *buffer_size,
    void* buffer
);

static const struct efi_load_file_protocol load_file2_protocol = {
    .load_file = fl2_load_file
};

static struct simple_buffer initrd = { 0 };

efi_status_t fl2_load_file(
    efi_load_file_protocol_t this,
    efi_device_path_t file_path,
    bool boot_policy,
    efi_size_t *buffer_size,
    void* buffer
) {
    if (!this || this != &load_file2_protocol || !buffer_size || !file_path) {
        _MESSAGE("Invalid parameter");
        return EFI_INVALID_PARAMETER;
    }

    if (boot_policy)
        return EFI_UNSUPPORTED;

    if (!(file_path->type == END_DEVICE_PATH_TYPE && file_path->subtype == END_ENTIRE_DEVICE_PATH_SUBTYPE)) {
        return EFI_UNSUPPORTED;
    }

    if (!initrd.buffer || buffer_len(&initrd) == 0) {
        _MESSAGE("Empty initrd");
        return EFI_NOT_FOUND;
    }

    if (!buffer || *buffer_size < buffer_len(&initrd)) {
        *buffer_size = buffer_len(&initrd);
        return EFI_BUFFER_TOO_SMALL;
    }

    _MESSAGE("Copy initrd to buffer");

    memcpy(buffer, buffer_pos(&initrd), buffer_len(&initrd));
    *buffer_size = buffer_len(&initrd);
    return EFI_SUCCESS;
}

static efi_handle_t initrd_handle = NULL;

#if 0 /* use this too hook into the kernel procedure used for finding the initrd handle */
typedef efi_api efi_status_t (*locate_device_path_t)(efi_guid_t,efi_device_path_t*, efi_handle_t*);
static locate_device_path_t __locate_device_path;

efi_api efi_status_t locate_device_path(efi_guid_t guid, efi_device_path_t* dp, efi_handle_t* h) {
    efi_status_t err =  __locate_device_path(guid, dp, h);
    _MESSAGE("{%g} %r", guid, err);
    return err;
}
#endif

efi_status_t initrd_register(
    simple_buffer_t _initrd
) {
    assert(_initrd);
    assert(BS);
    
    efi_status_t err;
    
    if (!_initrd->buffer || _initrd->length == 0) /* don't install initrd if empty */
        return EFI_SUCCESS;

    /* check if a previous stage already registered an initrd */
    efi_device_path_protocol_t dp = (efi_device_path_t) &efi_initrd_device_path;
    efi_handle_t handle;
    err = BS->locate_device_path(&efi_load_file2_protocol_guid, &dp, &handle);
    if (err != EFI_NOT_FOUND) {
        _MESSAGE("initrd media device path already registered");
    }

#if 0 /* override locate_device_path */
    __locate_device_path = BS->locate_device_path;
    BS->locate_device_path = locate_device_path;
#endif
    
    memcpy(&initrd, _initrd, sizeof(struct simple_buffer));
    err = BS->install_multiple_protocol_interfaces(
        &initrd_handle,
        &efi_device_path_protocol_guid, &efi_initrd_device_path,// initrd media device path
        &efi_load_file2_protocol_guid, &load_file2_protocol,    // load file2
        NULL);
    return err;
}

efi_status_t initrd_deregister() {
    if (initrd_handle) {
        efi_status_t err = BS->uninstall_multiple_protocol_interfaces(
            &initrd_handle,
            &efi_device_path_protocol_guid, &efi_initrd_device_path,
            &efi_load_file2_protocol_guid, &load_file2_protocol,
            NULL);
        if (err == EFI_SUCCESS)
            initrd_handle = NULL;

        return err;
    }

    return EFI_SUCCESS;
}
