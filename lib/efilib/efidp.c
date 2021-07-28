#include <efi.h>
#include <efilib.h>
#include <minmax.h>

#ifdef EFILIB_USE_DEVICE_PATH_TO_TEXT_PROTOCOL

char16_t* device_path_to_string(
    char16_t* buffer,
    efi_device_path_t dp
) {
    EFILIB_ASSERT(dp);
    char16_t* tmp = _EFI_DEVPATH_TO_TEXT->device_path_to_text(dp, true, true);
    buffer = wcsncpy(buffer, tmp, _PRINT_ITEM_BUFFER_LEN);
    *buffer = '\0';
    free(tmp);
    return buffer;
}

#else /* EFILIB_USE_DEVICE_PATH_TO_TEXT_PROTOCOL */

static inline
char16_t* _device_path_pci(char16_t* buffer, efi_pci_device_path_t dp) {
    efi_size_t len = wsprintf(buffer, _PRINT_ITEM_BUFFER_LEN - 1, u"Pci(0x%lx,0x%x)", dp->device, dp->function);
    return buffer + len;
}

static inline
char16_t* _device_path_memmap(char16_t* buffer, efi_memory_device_path_t dp) {
    efi_size_t len = wsprintf(buffer, _PRINT_ITEM_BUFFER_LEN - 1, u"MemMap(%hu,0x%lx,0x%lx)", dp->memory_type, dp->start, dp->end);
    return buffer + len;
}

static struct efi_guid uboot_guid = \
    {{ { 0xe61d73b9, 0xa384, 0x4acc, {0xae, 0xab, 0x82, 0xe8, 0x28, 0xf3, 0x62, 0x8b} } }}; 

static struct efi_guid uboot_virtio_guid = \
    {{ { 0x63293792, 0xadf5, 0x9325, {0xb9, 0x9f, 0x4e, 0x0e, 0x45, 0x5c, 0x1b, 0x1e} } }};

static inline
char16_t* _device_path_vendor(char16_t* buffer, efi_vendor_device_path_t dp) {
    efi_size_t len;
    if (guidcmp(&dp->guid, &uboot_guid)) {
        len = wsprintf(buffer, _PRINT_ITEM_BUFFER_LEN - 1, u"UBoot");
        buffer += len;
    } else if (guidcmp(&dp->guid, &uboot_virtio_guid)) {
        len = wsprintf(buffer, _PRINT_ITEM_BUFFER_LEN - 1, u"Virtio(%u)", dp->data[0]);
        buffer += len;
    } else {
        switch (dp->hdr.type) {
            case HARDWARE_DEVICE_PATH:
                len = wsprintf(buffer, _PRINT_ITEM_BUFFER_LEN - 1, u"VenHw(%g", &dp->guid);
                break;
            case MESSAGING_DEVICE_PATH:
                len = wsprintf(buffer, _PRINT_ITEM_BUFFER_LEN - 1, u"VenMedia(%g", &dp->guid);
                break;
            case MEDIA_DEVICE_PATH:
                len = wsprintf(buffer, _PRINT_ITEM_BUFFER_LEN - 1, u"VenMedia(%g", &dp->guid);
                break;
            default:
                len = wsprintf(buffer, _PRINT_ITEM_BUFFER_LEN - 1, u"Ven?(%g", &dp->guid);
        }
        buffer += len;
        uint32_t n = dp->hdr.length - sizeof(struct efi_vendor_device_path);
        if (n > 0) {
            (*buffer++) = u',';
            for (uint16_t i = 0; i < n; i++) {
                buffer = value_to_hex_string(buffer, dp->data[i]);
            }
        }
        (*buffer++) = u')';
    }
    return buffer;
}

static inline
char16_t* _device_path_acpi(char16_t* buffer, efi_acpi_device_path_t dp) {
    efi_size_t len;
    if (EISA_PNP_NUM(dp->hid))
        switch (EISA_PNP_NUM(dp->hid)) {
            case 0x60a:
                len = wsprintf(buffer, _PRINT_ITEM_BUFFER_LEN - 1, u"Floppy(%d)", dp->uid);
                break;
            case 0xa03:
                len = wsprintf(buffer, _PRINT_ITEM_BUFFER_LEN - 1, u"PciRoot(%d)", dp->uid);
                break;
            case 0xa08:
                len = wsprintf(buffer, _PRINT_ITEM_BUFFER_LEN - 1, u"PcieRoot(%d)", dp->uid);
                break;
            default:
                len = wsprintf(buffer, _PRINT_ITEM_BUFFER_LEN - 1, u"Acpi(PNP%04x,%d)", EISA_PNP_NUM(dp->hid), dp->uid);
        }
    else
        len = wsprintf(buffer, _PRINT_ITEM_BUFFER_LEN - 1, u"Acpi(0x%X,%d)", dp->hid, dp->uid);
    return buffer + len;
}

static inline
char16_t* _device_path_scsi(char16_t* buffer, efi_scsi_device_path_t dp) {
    return buffer + wsprintf(buffer, _PRINT_ITEM_BUFFER_LEN - 1, u"Scsi(%hu,%hu)", dp->target, dp->lun);
}

static inline
char16_t* _device_path_usb(char16_t* buffer, efi_usb_device_path_t dp) {
    return buffer + wsprintf(buffer, _PRINT_ITEM_BUFFER_LEN - 1, u"USB(%hu,%hu)", dp->port, dp->endpoint);
}

static inline
char16_t* _device_path_sata(char16_t* buffer, efi_sata_device_path_t dp) {
    return buffer + wsprintf(buffer, _PRINT_ITEM_BUFFER_LEN - 1, u"Sata(%hu,%hu,%hu)", dp->hba_port, dp->port_multiplier_number, dp->lun);
}

static inline
char16_t* _device_path_sd(char16_t* buffer, efi_sd_device_path_t dp) {
    return buffer + wsprintf(buffer, _PRINT_ITEM_BUFFER_LEN - 1, u"%ls(%hu)", dp->hdr.subtype == MSG_SD_DP ? u"SD" : u"eMMC", dp->slot_number);
}

static inline
char16_t* _device_path_partition(char16_t* buffer, efi_partition_device_path_t dp) {
    efi_size_t len;
    switch (dp->signature_type) {
        case SIGNATURE_TYPE_GUID:
            len = wsprintf(buffer, _PRINT_ITEM_BUFFER_LEN - 1, u"HD(%hu,GPT,%g)", dp->partition_number, (efi_guid_t) &dp->signature);
            break;
        case SIGNATURE_TYPE_MBR:
            len = wsprintf(buffer, _PRINT_ITEM_BUFFER_LEN - 1, u"HD(%hu,MBR,0x%X)", dp->partition_number, *((uint32_t*) &dp->signature));
            break;
        default:
            len = wsprintf(buffer, _PRINT_ITEM_BUFFER_LEN - 1, u"HD(%hu,%hu,0)", dp->partition_number, dp->signature_type);
    }
    return buffer + len;
}

static inline
char16_t* _device_path_filepath(char16_t* buffer, efi_filepath_t dp) {
    efi_size_t len = MIN(
        (dp->hdr.length - sizeof(dp->hdr))/sizeof(char16_t),
        _PRINT_ITEM_BUFFER_LEN - 1);
    return wcsncpy(buffer, dp->pathname, len);
}

static inline
char16_t* _device_path_unknown(char16_t* buffer, efi_device_path_t dp) {
    efi_size_t len;
    switch (dp->type) {
        case HARDWARE_DEVICE_PATH:
            len = wsprintf(buffer, _PRINT_ITEM_BUFFER_LEN -1, u"HardwarePath(%hu)", dp->subtype);
            break;
        case ACPI_DEVICE_PATH:
            len = wsprintf(buffer, _PRINT_ITEM_BUFFER_LEN -1, u"AcpiPath(%hu)", dp->subtype);
            break;
        case MESSAGING_DEVICE_PATH:
            len = wsprintf(buffer, _PRINT_ITEM_BUFFER_LEN -1, u"MsgPath(%hu)", dp->subtype);
            break;
        case MEDIA_DEVICE_PATH:
            len = wsprintf(buffer, _PRINT_ITEM_BUFFER_LEN -1, u"MediaPath(%hu)", dp->subtype);
            break;
        default:
            len = wsprintf(buffer, _PRINT_ITEM_BUFFER_LEN -1, u"Path(%hu,%hu)", dp->type, dp->subtype);
    }
    return buffer + len;
}

static inline
char16_t* _device_instance_end(char16_t* buffer, efi_device_path_t dp) {
    *(buffer++) = u',';
    return buffer;
}

char16_t* device_path_to_string(
    char16_t* buffer,
    efi_device_path_t dp
) {
    EFILIB_ASSERT(dp);
    for(; !IsDevicePathEndNode(dp); dp = NextDevicePathNode(dp)) {
        EFILIB_ASSERT(dp->type != 0);
        *(buffer++) = u'/';
        if (dp->type == HARDWARE_DEVICE_PATH) {
            if (dp->subtype == HW_PCI_DP)
                buffer = _device_path_pci(buffer, (efi_pci_device_path_t) dp);
            else if (dp->subtype == HW_MEMMAP_DP)
                buffer = _device_path_memmap(buffer, (efi_memory_device_path_t) dp);
            else if (dp->subtype == HW_VENDOR_DP)
                buffer = _device_path_vendor(buffer, (efi_vendor_device_path_t) dp);
            else
                buffer = _device_path_unknown(buffer, dp);
        } else if (dp->type == ACPI_DEVICE_PATH) {
            if (dp->subtype == ACPI_DP)
                buffer = _device_path_acpi(buffer, (efi_acpi_device_path_t) dp);
            else
                buffer = _device_path_unknown(buffer, dp);
        } else if (dp->type == MEDIA_DEVICE_PATH) {
            if (dp->subtype == MEDIA_PARTITION_DP)
                buffer = _device_path_partition(buffer, (efi_partition_device_path_t) dp);
            else if (dp->subtype == MEDIA_VENDOR_DP)
                buffer = _device_path_vendor(buffer, (efi_vendor_device_path_t) dp);
            else if (dp->subtype == MEDIA_FILEPATH_DP)
                buffer = _device_path_filepath(buffer, (efi_filepath_t) dp);
            else
                buffer = _device_path_unknown(buffer, dp);
        } else if (dp->type == MESSAGING_DEVICE_PATH) {
            if (dp->subtype == MSG_SCSI_DP) {
                buffer = _device_path_scsi(buffer, (efi_scsi_device_path_t) dp);
            } else if (dp->subtype == MSG_SATA_DP) {
                buffer = _device_path_sata(buffer, (efi_sata_device_path_t) dp);
            } else if (dp->subtype == MSG_USB_DP) {
                buffer = _device_path_usb(buffer, (efi_usb_device_path_t) dp);
            } else if (dp->subtype == MSG_SD_DP)
                buffer = _device_path_sd(buffer, (efi_sd_device_path_t) dp);
            else
                buffer = _device_path_unknown(buffer, dp);
        } else if (dp->type == END_DEVICE_PATH_TYPE) {
            if (dp->subtype == END_INSTANCE_DEVICE_PATH_SUBTYPE)
                buffer = _device_instance_end(buffer, dp);
        } else {
            buffer = _device_path_unknown(buffer, dp);
        }
        *buffer = u'\0';
    }
    return buffer;
}

#endif /* EFILIB_USE_DEVICE_PATH_TO_TEXT_PROTOCOL */
