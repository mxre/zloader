#pragma once

#include <efi.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#define _cleanup(f) __attribute__((cleanup(f)))

static inline
void close_file_p(efi_file_handle_t* handle) {
    if (*handle) {
        (*handle)->close(*handle);
    }
}

#define _cleanup_file_handle _cleanup(close_file_p)

static inline
void free_p(void** p) {
    if (*p) {
        free(*p);
    }
}

#define _cleanup_pool _cleanup(free_p)

#ifdef DEBUG
#define _MESSAGE(msg, ...) wprintf(u"MSG: " _u(msg) u"\n" __VA_OPT__(, __VA_ARGS__))
#else
#define _MESSAGE(msg, ...)
#endif

#define _ERROR(msg, ...) wprintf(u"ERR: %E" _u(msg) u"%N\n" __VA_OPT__(, __VA_ARGS__))

struct simple_buffer {
    void* buffer;
    size_t length;
    size_t pos;
    size_t allocated;
};

typedef struct simple_buffer* simple_buffer_t;

static __always_inline inline
void* buffer_pos(simple_buffer_t buffer) {
    assert(buffer->buffer);
    assert(buffer->pos < buffer->allocated);

    return (uint8_t*) buffer->buffer + buffer->pos;
}

static __always_inline inline
struct simple_buffer allocate_simple_buffer(size_t length) {
    struct simple_buffer buf = {
        .buffer = malloc(length),
        .length = 0,
        .pos = 0,
        .allocated = length
    };

    return buf;
}

static inline
void free_simple_buffer(simple_buffer_t buffer) {
    free(buffer->buffer);
}

#define _cleanup_buffer _cleanup(free_simple_buffer)

efi_status_t write_simple_buffer_to_file(
    char16_t* filename,
    const simple_buffer_t buffer
);

efi_status_t image_load_from_memory(
    const simple_buffer_t buffer,
    efi_handle_t* image
);

efi_status_t image_start(
    efi_handle_t* image,
    char16_t* options
);
