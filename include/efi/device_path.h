#pragma once

#include "defs.h"
#include "memory.h"

#define EFI_DEVICE_PATH_PROTOCOL_GUID \
    { 0x09576e91, 0x6d3f, 0x11d2, {0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b} }

struct __packed efi_device_path_protocol {
	uint8_t type;
	uint8_t subtype;
	uint16_t length;
};

typedef struct efi_device_path_protocol* efi_device_path_protocol_t;
typedef struct efi_device_path_protocol* efi_device_path_t;

#define END_DEVICE_PATH_TYPE                0x7f
#define END_INSTANCE_DEVICE_PATH_SUBTYPE    0x01
#define END_ENTIRE_DEVICE_PATH_SUBTYPE      0xff

#define SetDevicePathEndNode(a) { \
		(a)->type = END_DEVICE_PATH_TYPE; \
		(a)->subtype = END_ENTIRE_DEVICE_PATH_SUBTYPE; \
		(a)->length = sizeof(struct efi_device_path_protocol); }

/*==========================================================================*
 *  Hardware Device Path
 *==========================================================================*/
#define HARDWARE_DEVICE_PATH            0x01

/* Memory Device */
#define HW_MEMMAP_DP                    0x03

struct __packed efi_memory_device_path {
    struct efi_device_path_protocol hdr;
    uint32_t memory_type;
    efi_physical_address_t start;
	efi_physical_address_t end;
};

typedef struct efi_memory_device_path* efi_memory_device_path_t;

/* Vendor Device */
#define HW_VENDOR_DP                    0x04

struct __packed efi_vendor_device_path {
    struct efi_device_path_protocol hdr;
    struct efi_guid guid;
};

typedef struct efi_vendor_device_path* efi_vendor_device_path_t;

/*==========================================================================*
 *  ACPI Device Path
 *==========================================================================*/
#define ACPI_DEVICE_PATH                 0x02

/*==========================================================================*
 *  Messaging  Device Path
 *==========================================================================*/
#define MESSAGING_DEVICE_PATH           0x03

/*==========================================================================*
 *  Media Device Path
 *==========================================================================*/
#define MEDIA_DEVICE_PATH               0x04

/* Disk Partition */
#define MEDIA_HARDDRIVE_DP              0x01

struct __packed efi_partition_device_path {
    struct efi_device_path_protocol hdr;
    uint32_t partition_number;
    uint64_t partition_start;
    uint64_t partition_size;
    uint8_t signature[16];
    uint8_t mbr_type;
    uint8_t signature_type;
};

typedef struct efi_partition_device_path* efi_partition_device_path_t;

#define MBR_TYPE_PCAT                       0x01
#define MBR_TYPE_EFI_PARTITION_TABLE_HEADER 0x02

#define SIGNATURE_TYPE_MBR                  0x01
#define SIGNATURE_TYPE_GUID                 0x02

/* Vendor Device */
#define MEDIA_VENDOR_DP                     0x03
/* struct definition in hardware device path section */

/* File Path */
#define MEDIA_FILEPATH_DP                   0x04
struct __packed efi_filepath {
    struct efi_device_path_protocol hdr;
    char16_t pathname[1];
};

typedef struct efi_filepath* efi_filepath_t;




