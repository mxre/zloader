#pragma once

#include <efi.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "config.h"
#define ALIGN_VALUE(v, a) ((v) + (((a) - (v)) & ((a) - 1)))

#if __has_c_attribute(gnu::cleanup)
#  define _cleanup(f) [[ gnu::cleanup(f) ]]
#else
#  define _cleanup(f) __attribute__((cleanup(f)))
#endif

static inline
void close_file_p(efi_file_handle_t* handle) {
    if (*handle) {
        (*handle)->close(*handle);
    }
}

#define _cleanup_file_handle _cleanup(close_file_p)

static inline
void free_p(void* pointer) {
    void** p = (void**) pointer;
    if (*p) {
        free(*p);
    }
}

#define _cleanup_pool _cleanup(free_p)

#ifdef PRINT_MESSAGES
#define _MESSAGE(msg, ...) wprintf(u"MSG: " _u(msg) u"\n" __VA_OPT__(, __VA_ARGS__))
#else
#define _MESSAGE(msg, ...)
#endif

#define _ERROR(msg, ...) wprintf(u"ERR: %E" _u(msg) u"%N\n" __VA_OPT__(, __VA_ARGS__))

typedef struct simple_buffer* simple_buffer_t;

/* don't move fields buffer, lengthand pos they are identical to ZSTD buffer */
struct simple_buffer {
    void* buffer;       ///< pointer to base of the buffer
    size_t length;      ///< length of data set in the buffer
    size_t pos;         ///< position of the cursor
    size_t allocated;   ///< number of bytes allocated, this is the maximum size of the buffer
    void (*free) (simple_buffer_t);
};

typedef struct aligned_buffer* aligned_buffer_t;

struct aligned_buffer {
    void* buffer;
    size_t length;
    size_t pos;
    size_t allocated;
    void (*free) (aligned_buffer_t);
    size_t pages;
};

static __always_inline inline
uint8_t* buffer_pos(simple_buffer_t buffer) {
    assert(buffer->buffer);
    assert(buffer->pos < buffer->allocated);

    return (uint8_t*) buffer->buffer + buffer->pos;
}

static __always_inline inline
size_t buffer_len(simple_buffer_t buffer) {
    assert(buffer->buffer);
    assert(buffer->pos < buffer->length);

    return buffer->length - buffer->pos;
}

static inline
void free_buffer(void* buffer) {
    simple_buffer_t b = (simple_buffer_t) buffer;
    if (b && b->buffer && b->free) b->free(b);
}

#define _cleanup_buffer _cleanup(free_buffer)

static inline
void free_simple_buffer(simple_buffer_t buffer) {
    /* only free buffers that were actually allocated */
    if (buffer->allocated)
        free(buffer->buffer);
}

static inline
void free_aligned_buffer(aligned_buffer_t buffer) {
    if (buffer->allocated && buffer->buffer)
        BS->free_pages((efi_physical_address_t) buffer->buffer, buffer->pages);
}

static __always_inline inline
bool allocate_simple_buffer(size_t length, simple_buffer_t buffer) {
    buffer->buffer = malloc(length);
    if (!buffer->buffer)
        return false;

    buffer->length = buffer->pos = 0;
    buffer->allocated = length;
    buffer->free = free_simple_buffer;

    return true;
}

static __always_inline inline
bool allocate_aligned_buffer(size_t length, efi_memory_t type, aligned_buffer_t buffer) {
    buffer->allocated = (ALIGN_VALUE(length + PAGE_SIZE, PAGE_SIZE));
    buffer->pages = buffer->allocated / PAGE_SIZE;
    buffer->pos = buffer->length = 0;
    buffer->free = free_aligned_buffer;

    if (EFI_SUCCESS != BS->allocate_pages(EFI_ALLOCATE_ANY_PAGES, type,
        buffer->pages, (efi_physical_address_t*) &buffer->buffer)
    ) {
        return false;
    }

    return true;
}

/**
 * @brief Create XXH64 hash for buffer contents
 *
 * @param buffer
 * @returns XXH64 hash
 * @returns -1 on ERROR
 */
uint64_t buffer_xxh64(simple_buffer_t buffer);

efi_device_path_t create_memory_mapped_device_path(
    efi_physical_address_t address,
    efi_size_t size,
    efi_memory_t type
);

efi_status_t get_part_uuid_from_device_path(efi_device_path_t path, efi_guid_t guid);
