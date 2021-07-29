#include <efi.h>
#include <efilib.h>

#include "util.h"

#define EFI_IMAGE_SECURITY_DATABASE1_GUID \
    { 0xd719b2cb, 0x3d3a, 0x4596, {0xa3, 0xbc, 0xda, 0xd0, 0x0e, 0x67, 0x65, 0x6f} }

static inline
char16_t* get_dirname(const char16_t* path) {
    assert(path);

    const char16_t* last_pos;
    for(const char16_t* pos = path; pos; pos = wcschr(pos+1, u'\\')) {
        last_pos = pos;
    }
    efi_size_t len = last_pos - path;
    char16_t* dir = malloc(len + 1);
    if (!dir)
        return NULL;
    char16_t* end = wcsncpy(dir, path, len);
    *end = u'\0';

    return dir;
}

static inline
efi_status_t read_file_to_buffer(
    efi_file_handle_t cwd,
    const char16_t* filename,
    simple_buffer_t buffer
) {
    assert(cwd);
    assert(filename);
    assert(buffer);

    efi_status_t err;
    _cleanup_file_handle efi_file_handle_t handle;
    err = cwd->open(cwd, &handle, filename, EFI_FILE_MODE_READ, 0);
    if (EFI_ERROR(err))
        return err;
    
    _cleanup_pool efi_file_info_t info = lib_get_file_info(handle);
    if (!info)
        return EFI_OUT_OF_RESOURCES;
    
    efi_size_t size = info->file_size;
    if (size == 0)
        return EFI_INVALID_PARAMETER;
    
    allocate_simple_buffer(size, buffer);
    err = handle->read(handle, &size, buffer_pos(buffer));
    if (EFI_ERROR(err)) {
        buffer->free(buffer);
        memset(buffer, 0, sizeof(struct simple_buffer)); 
        return err;
    }

    buffer->length = size;

    return EFI_SUCCESS;
}

static struct efi_guid efi_image_securiy_database1_guid = {{ EFI_IMAGE_SECURITY_DATABASE1_GUID }};

efi_api
efi_status_t efi_main(efi_handle_t image, efi_system_table_t systable) {
    initialize_library(image, systable);
    efi_status_t err;

    {
        uint8_t setup_mode; efi_size_t size = sizeof(setup_mode);
        err = efi_var_get(&efi_global_variable_guid, u"SetupMode", NULL, &size, &setup_mode);
        if (EFI_ERROR(err)) {
            _ERROR("GetVariable SetupMode: %r", err);
            return err;
        }

        if (setup_mode != 1) {
            _ERROR("Platform not in SetupMode.");
            return EFI_UNSUPPORTED;
        }
    }

    _MESSAGE("Platform in SetupMode");

    efi_device_path_t filepath = EFI_LOADED_IMAGE->file_path;
    if (!IsDevicePathNode(filepath, MEDIA_DEVICE_PATH, MEDIA_FILEPATH_DP)) {
        _ERROR("Image does not have a valid file path");
        return EFI_UNSUPPORTED;
    }

    _cleanup_pool char16_t* path = get_dirname(((efi_filepath_t) filepath)->pathname);
    if (!path)
        return EFI_OUT_OF_RESOURCES;
    
    _MESSAGE("Using %ls as Working Directoy", path);
    efi_file_handle_t cwd;
    err = EFI_ROOT->open(EFI_ROOT, &cwd, path, EFI_FILE_MODE_READ, 0);
    if (EFI_ERROR(err)) {
        _ERROR("Could not set Working Directory: %ls", path);
        return err;
    }

    _cleanup_buffer struct simple_buffer kek = { 0 };
    err = read_file_to_buffer(cwd, u"KEK.auth", &kek);
    if (EFI_ERROR(err)) {
        _ERROR("Could not load KEK certificate");
        return err;
    }

    _cleanup_buffer struct simple_buffer db = { 0 };
    err = read_file_to_buffer(cwd, u"db.auth", &db);
    if (EFI_ERROR(err)) {
        _ERROR("Could not load db certificate");
        return err;
    }

    _cleanup_buffer struct simple_buffer pk = { 0 };
    err = read_file_to_buffer(cwd, u"PK.auth", &pk);
    if (EFI_ERROR(err)) {
        _ERROR("Could not load PK certificate");
        return err;
    }

    _MESSAGE("All files loaded, setting variables...");

    err = efi_var_set(&efi_global_variable_guid, u"KEK",
        EFI_VARIABLE_NON_VOLATILE
        | EFI_VARIABLE_RUNTIME_ACCESS
        | EFI_VARIABLE_BOOTSERVICE_ACCESS
        | EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS,
        buffer_len(&kek), buffer_pos(&kek));
    if(EFI_ERROR(err)) {
        _ERROR("Failed to enroll KEK: %r", err);
        return err;
    }
    
    err = efi_var_set(&efi_image_securiy_database1_guid, u"db",
        EFI_VARIABLE_NON_VOLATILE
        | EFI_VARIABLE_RUNTIME_ACCESS
        | EFI_VARIABLE_BOOTSERVICE_ACCESS
        | EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS,
        buffer_len(&db), buffer_pos(&db));
    if(EFI_ERROR(err)) {
        _ERROR("Failed to enroll db: %r", err);
        return err;
    }

    err = efi_var_set(&efi_global_variable_guid, u"PK",
        EFI_VARIABLE_NON_VOLATILE
        | EFI_VARIABLE_RUNTIME_ACCESS
        | EFI_VARIABLE_BOOTSERVICE_ACCESS
        | EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS,
        buffer_len(&pk), buffer_pos(&pk));
    if(EFI_ERROR(err)) {
        _ERROR("Failed to enroll PK: %r", err);
        return err;
    }

    {
        uint8_t secure_boot; efi_size_t size = sizeof(secure_boot);
        err = efi_var_get(&efi_global_variable_guid, u"SecureBoot", NULL, &size, &secure_boot);
        if (EFI_ERROR(err)) {
            _ERROR("GetVariable SecureBoot: %r", err);
            return err;
        }

        if (secure_boot != 1) {
            _ERROR("Platform not in SecureBoot.");
            return EFI_UNSUPPORTED;
        }

        if (secure_boot == 1)
            _MESSAGE("Platform is in SecureBoot mode");
    }

    return EFI_SUCCESS;
}
