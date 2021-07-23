/**
 * @file systemd.h
 * @author Max Resch
 * @brief systemd bootloader interface
 * @version 0.1
 * @date 2021-07-23
 * 
 * @copyright Copyright (c) 2021
 * 
 * @see https://systemd.io/BOOT_LOADER_INTERFACE/
 */
#pragma once

#define LOADER_GUID \
    { 0x4a67b082, 0x0a4c, 0x41cf, {0xb6, 0xc7, 0x44, 0x0b, 0x29, 0xbb, 0x8c, 0x4f} }

extern struct efi_guid loader_guid;
