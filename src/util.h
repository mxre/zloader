#pragma once

#include <efilib.h>

#define _cleanup(f) __attribute__((cleanup(f)))

static inline
void close_file_p(efi_file_handle_t* handle) {
    if (*handle) {
        (*handle)->close(*handle);
    }
}

static inline
void free_p(void** p) {
    if (*p) {
        free(*p);
    }
}

#define _cleanup_pool _cleanup(free_p)

efi_status_t write_buffer_to_file(
    char16_t* filename,
    const void* buffer,
    size_t* size
);

#ifdef DEBUG
#define _MESSAGE(msg, ...) printf(u"MSG: " _u(msg) u"\n" __VA_OPT__(, __VA_ARGS__))
#else
#define _MESSAGE(msg, ...)
#endif

efi_status_t image_load_from_memory(void* buffer, size_t length, efi_handle_t* image);

efi_status_t image_start(efi_handle_t* image, char16_t* options);
