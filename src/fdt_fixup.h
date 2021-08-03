/**
 * @file fdt-fixup.h
 * @author Max Resch
 * @brief UBoot EFI DeviceTree fixup
 * @version 0.1
 * @date 2021-08-03
 *
 * @copyright Copyright (c) 2021
 *
 * @see https://github.com/u-boot/u-boot/include/efi_dt_fixup.h
 */
#pragma once

#include <efi.h>
#include <stdint.h>

#define EFI_DT_FIXUP_PROTOCOL_GUID \
	{ 0xe617d64c, 0xfe08, 0x46da, {0xf4, 0xdc, 0xbb, 0xd5, 0x87, 0x0c, 0x73, 0x00} }

#define EFI_FDT_GUID \
    { 0xb1b621d5, 0xf19c, 0x41a5, {0x83, 0x0b, 0xd9, 0x15, 0x2c, 0x69, 0xaa, 0xe0} }

#define EFI_DT_FIXUP_PROTOCOL_REVISION 0x00010000

/* Add nodes and update properties */
#define EFI_DT_APPLY_FIXUPS    0x00000001
/*
 * Reserve memory according to the /reserved-memory node
 * and the memory reservation block
 */
#define EFI_DT_RESERVE_MEMORY  0x00000002

#define EFI_DT_ALL ( EFI_DT_APPLY_FIXUPS | EFI_DT_RESERVE_MEMORY )

typedef struct efi_dt_fixup_protocol* efi_dt_fixup_protocol_t;

struct efi_dt_fixup_protocol {
	uint64_t revision;
	efi_status_t (efi_api *fixup) (
        efi_dt_fixup_protocol_t self,
        void* dtb,
        efi_size_t* buffer_size,
        uint32_t flags);
};

extern struct efi_guid efi_fdt_guid;

extern struct efi_guid efi_dt_fixup_protocol_guid;
