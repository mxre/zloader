#include <efi.h>
#include <efilib.h>

#include <assert.h>
#include "util.h"
#include "systemd.h"

static efi_device_path_utilities_t device_path_utils;

static
efi_status_t run_image_from_file(
    const efi_device_path_t dp,
    const uint8_t* options,
    const efi_size_t options_length
) {
    efi_status_t err;
    efi_handle_t image;
    efi_loaded_image_protocol_t loaded_image;

    err = BS->load_image(true, EFI_IMAGE, dp, NULL, 0, &image);
    if (EFI_ERROR(err)) {
        _MESSAGE("LoadImage %D: %r", dp, err);
        return err;
    }

    err = BS->open_protocol(image, &efi_loaded_image_protocol_guid, (void**) &loaded_image,
        EFI_IMAGE, NULL, EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
    if (EFI_ERROR(err)) {
        _MESSAGE("OpenProtocol: %r", err);
        BS->unload_image(image);
        return err;
    }

    if (options && options_length > 0) {
        loaded_image->load_options = (void*) options;
        loaded_image->load_options_size = options_length;
    }

    err = BS->start_image(image, NULL, NULL);
    if (EFI_ERROR(err)) {
        _MESSAGE("StartImage %D: %r", dp, err);
    }

    BS->close_protocol(image, &efi_loaded_image_protocol_guid, EFI_IMAGE, NULL);
    BS->unload_image(image);

    return err;
}

static inline
efi_status_t boot_entry_get_device_path(
    const efi_load_option_t boot_entry,
    efi_device_path_t* dp
) {
    assert(boot_entry);
    assert(dp);

    if (boot_entry->file_path_list_length < sizeof(struct efi_device_path_protocol))
        return EFI_UNSUPPORTED;

    efi_size_t pos = sizeof(char16_t) * (wcslen(boot_entry->description) + 1) + sizeof(struct efi_load_option);
    *dp = (efi_device_path_t) ((uint8_t*) boot_entry + pos);

    if (IsDevicePathEndNode(*dp))
        return EFI_NOT_FOUND;
    return EFI_SUCCESS;
}

static
efi_status_t boot_entry(
    uint16_t entry_num
) {
    efi_status_t err;
    efi_size_t boot_entry_length;
    char16_t boot_order_name[9];
    wsprintf(boot_order_name, 9, u"Boot%04hx", entry_num);
    _cleanup_pool efi_load_option_t boot_entry = (efi_load_option_t) efi_var_get_pool(&efi_global_variable_guid, boot_order_name, NULL, &boot_entry_length);
    if (!boot_entry) {
        _MESSAGE("Boot%04x does not exist", entry_num);
        return EFI_SUCCESS;
    }
    if (!(boot_entry->attributes && LOAD_OPTION_ACTIVE)) {
        _MESSAGE("Boot%04x is marked inactive", entry_num);
        return EFI_SUCCESS;
    }

    efi_device_path_t dp;
    err = boot_entry_get_device_path(boot_entry, &dp);
    if (err == EFI_NOT_FOUND) {
        _MESSAGE("Boot%04x has no device path", entry_num);
        return EFI_SUCCESS;
    } else if (EFI_ERROR(err)) {
        return err;
    }
    _MESSAGE("Boot%04hx %ls %D", entry_num, boot_entry->description, dp);

    efi_var_set(&efi_global_variable_guid, u"BootCurrent", EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS, sizeof(uint16_t), &entry_num);

    uint8_t* options = (uint8_t*) dp + boot_entry->file_path_list_length;
    efi_size_t options_length = options - ((uint8_t*) boot_entry + boot_entry_length);
    _MESSAGE("Execute %D", dp);
    err = run_image_from_file(dp, options, options_length);
    efi_var_set(&efi_global_variable_guid, u"BootCurrent", EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS, 0, NULL);
    if (EFI_ERROR(err)) {
        return err;
    }

    return EFI_SUCCESS;
}

static
efi_status_t boot_file(efi_device_path_t efi_device, const char16_t* filename) {
    efi_status_t err;
    efi_size_t len = wcslen(filename);
    efi_filepath_t fl = (efi_filepath_t) malloc(sizeof(struct efi_filepath) + sizeof(char16_t) * (len + 1));
    if (!fl) {
        _MESSAGE("Could not allocate filepath");
        return EFI_OUT_OF_RESOURCES;
    }
    fl->hdr.type = MEDIA_DEVICE_PATH;
    fl->hdr.subtype = MEDIA_FILEPATH_DP;
    fl->hdr.length = sizeof(struct efi_filepath) + sizeof(char16_t) * (len + 1);
    wcsncpy(fl->pathname, filename, len); fl->pathname[len] = u'\0';
    efi_device_path_t d = device_path_utils->append_node(efi_device, (efi_device_path_t) fl);
    free(fl);

    _MESSAGE("Execute %D", d);
    err = run_image_from_file(d, NULL, 0);
    if (EFI_ERROR(err))
        _ERROR("%D failed: %r", d, err);
    free(d);

    return err;
}

efi_api
efi_status_t efi_main(efi_handle_t image, efi_system_table_t systable) {
    initialize_library(image, systable);

    if (!efi_var_attributes(&loader_guid, u"LoaderTimeInitUSec") && BOOT_TIME_USECS) {
        efi_var_set_printf(&loader_guid, u"LoaderTimeInitUSec",
            EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
            u"%lu", BOOT_TIME_USECS);
    }

    efi_status_t err;
    err = lib_get_protocol_interface(&efi_device_path_utilities_guid, NULL, (void**) &device_path_utils);
    if (EFI_ERROR(err)) {
        _MESSAGE("Could not open DevicePathUtilities: %r");
        return err;
    }

    efi_size_t sz = sizeof(uint16_t);
    uint16_t next;
    err = efi_var_get(&efi_global_variable_guid, u"BootNext", NULL, &sz, &next);
    if (err == EFI_SUCCESS) {
        boot_entry(next);
        if (EFI_ERROR(err)) {
            _ERROR("BootNext failed: %r", err);
        }
    }

    _cleanup_pool uint16_t* boot_order = efi_var_get_pool(&efi_global_variable_guid, u"BootOrder", NULL, &sz);
    if (boot_order) {
        for (efi_size_t i = 0; i < sz / sizeof(char16_t); i++) {
            err = boot_entry(boot_order[i]);
            if (EFI_ERROR(err)) {
                _ERROR("Boot%04hx failed: %r", boot_order[i], err);
                continue;
            }
        }
    }

    efi_device_path_t dp;
    err = BS->open_protocol(EFI_LOADED_IMAGE->device_handle, &efi_device_path_protocol_guid, (void**) &dp,
        EFI_IMAGE, NULL, EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
    if (EFI_ERROR(err)) {
        _ERROR("Can't open current root directory: %r", err);
        return err;
    }

    err = boot_file(dp, u"\\efi\\boot\\boot" EFI_ARCH ".efi");
    err = boot_file(dp, u"\\shell" EFI_ARCH ".efi");
    return err;
}
