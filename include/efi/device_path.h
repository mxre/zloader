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

#define NextDevicePathNode(a) ((efi_device_path_t) ((uint8_t*)(a) + (a)->length))

#define IsDevicePathNode(a, t, s) ((a)->type == (t) && (a)->subtype == (s))
#define IsDevicePathEndNode(a) IsDevicePathNode(a, END_DEVICE_PATH_TYPE, END_ENTIRE_DEVICE_PATH_SUBTYPE)

/*==========================================================================*
 *  Hardware Device Path
 *==========================================================================*/
#define HARDWARE_DEVICE_PATH            0x01

/* PCI */
#define HW_PCI_DP                       0x01

struct __packed efi_pci_device_path {
    struct efi_device_path_protocol hdr;
    uint8_t function;
    uint8_t device;
};

typedef struct efi_pci_device_path* efi_pci_device_path_t;

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
    uint8_t data[];
};

typedef struct efi_vendor_device_path* efi_vendor_device_path_t;

/*==========================================================================*
 *  ACPI Device Path
 *==========================================================================*/
#define ACPI_DEVICE_PATH                 0x02

#define ACPI_DP                          0x01
struct __packed efi_acpi_device_path {
    struct efi_device_path_protocol hdr;

    /**
     * @brief Device's PnP hardware ID stored in a numeric 32-bit compressed
     * EISA-type ID. This value must match the corresponding _HID in the
     * ACPI name space
     */
    uint32_t hid;

    /**
     * @brief Unique ID that is required by ACPI if two devices have the same
     * HID. This value must also match the corresponding UID/HID
     * pair in the ACPI name space. Only the 32-bit numeric value type of
     * UID is supported; thus strings must not be used for the UID in
     * the ACPI name space 
     */
    uint32_t uid;
};

#define EISA_PNP_ID_CONST 0x41d0
#define EISA_IS_PNP(id) (((uint16_t) (id)) == EISA_PNP_ID_CONST)
#define EISA_PNP_ID(id) (uint32_t)(((id) << 16) | EISA_PNP_ID_CONST)
#define EISA_PNP_NUM(id) ((uint16_t)((id) >> 16))

typedef struct efi_acpi_device_path* efi_acpi_device_path_t;

/*==========================================================================*
 *  Messaging  Device Path
 *==========================================================================*/
#define MESSAGING_DEVICE_PATH           0x03

#define MSG_SCSI_DP                     0x02

struct __packed efi_scsi_device_path {
    struct efi_device_path_protocol hdr;
    uint16_t target;
    uint16_t lun;
};

typedef struct efi_scsi_device_path* efi_scsi_device_path_t;

#define MSG_USB_DP                      0x05

struct __packed efi_usb_device_path {
    struct efi_device_path_protocol hdr;
    uint8_t port;
    uint8_t endpoint;
};

typedef struct efi_usb_device_path* efi_usb_device_path_t;

#define MSG_SATA_DP                     0x12

struct __packed efi_sata_device_path {
    struct efi_device_path_protocol hdr;
    uint16_t hba_port;
    uint16_t port_multiplier_number;
    uint16_t lun;
};

typedef struct efi_sata_device_path* efi_sata_device_path_t;

/* Devices on the SD Card bus */
#define MSG_SD_DP                       0x1A

struct __packed efi_sd_device_path {
    struct efi_device_path_protocol hdr;
    uint8_t slot_number;
};

typedef struct efi_sd_device_path* efi_sd_device_path_t;

#define MSG_EMMC_DP                     0x1D    ///< use struct efi_sd_device_path

/*==========================================================================*
 *  Media Device Path
 *==========================================================================*/
#define MEDIA_DEVICE_PATH               0x04

/* Disk Partition (EFI specs calls this confusingly a "harddrive") */
#define MEDIA_PARTITION_DP              0x01

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
#define MEDIA_VENDOR_DP                     0x03 ///< use efi_vendor_device_path

/* File Path */
#define MEDIA_FILEPATH_DP                   0x04
struct __packed efi_filepath {
    struct efi_device_path_protocol hdr;
    char16_t pathname[1];
};

typedef struct efi_filepath* efi_filepath_t;
