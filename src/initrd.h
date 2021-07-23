/**
 * @file initrd.h
 * @author Max Resch
 * @brief initrd loader via LoadFile2 protocol
 * @version 0.1
 * @date 2021-07-18
 * 
 * @copyright Copyright (c) 2021
 * 
 * @see https://github.com/u-boot/u-boot/commit/ec80b4735a593961fe701cc3a5d717d4739b0fd0
 */
#pragma once

#include <efi.h>
#include "util.h"

/**
 * @brief Initrd device path media GUID
 *
 * @see https://github.com/torvalds/linux/blob/v5.13/include/linux/efi.h
 * @see https://github.com/u-boot/u-boot/blob/v2021.07/include/efi_load_initrd.h
 */
#define LINUX_INITRD_MEDIA_GUID \
    { 0x5568e427, 0x68fc, 0x4f3d, {0xac, 0x74, 0xca, 0x55, 0x52, 0x31, 0xcc, 0x68} }

efi_status_t initrd_register(
    simple_buffer_t initrd
);

efi_status_t initrd_deregister();
