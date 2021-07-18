#pragma once

#include "defs.h"
#include "memory.h"

struct efi_device_path_protocol {
	uint8_t type;
	uint8_t subtype;
	uint8_t length[2];
};

typedef struct efi_device_path_protocol* efi_device_path_protocol_t;
typedef struct efi_device_path_protocol* efi_device_path_t;

#define SetDevicePathNodeLength(a,l) { \
		(a)->length[0] = (uint8_t) (l); \
		(a)->length[1] = (uint8_t) ((l) >> 8); \
	}

#define END_DEVICE_PATH_TYPE                0x7f
#define END_ENTIRE_DEVICE_PATH_SUBTYPE      0xff

#define SetDevicePathEndNode(a) { \
		(a)->type = END_DEVICE_PATH_TYPE; \
		(a)->subtype = END_ENTIRE_DEVICE_PATH_SUBTYPE; \
		(a)->length[0] = sizeof(struct efi_device_path_protocol); \
		(a)->length[1] = 0; \
	}

/*==========================================================================*
 *  Hardware Device Path
 *==========================================================================*/

#define HARDWARE_DEVICE_PATH            0x01

/* Memory Device */

#define HW_MEMMAP_DP                    0x03
struct efi_memory_device_path {
    struct efi_device_path_protocol hdr;
    uint32_t memory_type;
    efi_physical_address_t start;
	efi_physical_address_t end;
};

typedef struct efi_memory_device_path* efi_memory_device_path_t;

/* Vendor Device */

#define HW_VENDOR_DP                    0x04
struct efi_vendor_device_path {
    struct efi_device_path_protocol hdr;
    struct efi_guid guid;
};

typedef struct efi_vendor_device_path* efi_vendor_device_path;
