#pragma once

#include "defs.h"
#include "device_path.h"

#define EFI_DEVICE_PATH_UTILITIES_PROTOCOL_GUID \
    { 0x379be4e, 0xd706, 0x437d, {0xb0, 0x37, 0xed, 0xb8, 0x2f, 0xb7, 0x72, 0xa4} }

struct efi_device_path_utilities_protocol {
    /**
     * @brief Returns the size of the device path, in bytes.
     */
    efi_size_t (efi_api *size) (
        const efi_device_path_t);
    
    /**
     * @brief Create a duplicate of the specified path.
     */
    efi_device_path_t (efi_api *duplicate) (
        const efi_device_path_t);
    
    efi_device_path_t (efi_api *append) (
        const efi_device_path_t,
        const efi_device_path_t
    );

    efi_device_path_t (efi_api *append_node) (
        const efi_device_path_t,
        const efi_device_path_t node
    );

    efi_device_path_t (efi_api *append_instance) (
        const efi_device_path_t,
        const efi_device_path_t instance
    );

    /**
     * @brief Creates a copy of the current device path instance and returns a
     * pointer to the next device path instance.
     * 
     * @param[in,out] instance
     * @param[out] instance_size 
     */
    efi_device_path_t (efi_api *next_instance) (
        efi_device_path_t** instance,
        efi_size_t* instance_size
    );

    efi_device_path_t (efi_api *create_node) (
        uint8_t type,
        uint8_t subtype,
        uint16_t length
    );

    bool (efi_api *is_multi_instance) (
        const efi_device_path_t
    );
};

typedef struct efi_device_path_utilities_protocol* efi_device_path_utilities_t;
