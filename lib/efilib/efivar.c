#include <efi.h>
#include <efilib.h>

#include <stdargs.h>

void* efi_var_get_pool(
    const efi_guid_t guid,
    const char16_t* name,
    uint32_t* attributes,
    efi_size_t* buffer_size
) {
    efi_size_t size = 0;
    efi_status_t err = RT->get_variable(name, guid, attributes, &size, NULL);
    if (err != EFI_BUFFER_TOO_SMALL) {
        EFILIB_DBG_PRINTF("GetVariable {%g} %ls %r", guid, name, err);
        return NULL;
    }
    void* buffer = malloc(size);
    if (!buffer)
        return NULL;
    err = RT->get_variable(name, guid, attributes, &size, buffer);
    if (EFI_ERROR(err)) {
        EFILIB_DBG_PRINTF("GetVariable {%g} %ls %r", guid, name, err);
        free(buffer);
        return NULL;
    }

    if (buffer_size)
        *buffer_size = size;
    return buffer;
}

#define PRINTF_MAXLEN 100

efi_size_t efi_var_set_printf(
    const efi_guid_t guid,
    const char16_t* name,
    uint32_t attributes,
    const char16_t* fmt, ...
) {
    char16_t buff[PRINTF_MAXLEN];
    va_list args;
    va_start(args, fmt);
    efi_size_t len = vswprintf(buff, PRINTF_MAXLEN, fmt, args) + 1;
    len *= sizeof(char16_t);
    va_end(args);
    return efi_var_set(guid, name, attributes, len, buff);
}
