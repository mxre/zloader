#include <efi.h>
#include <efilib.h>

#undef memcpy
#undef memmove

void* malloc(efi_size_t size) {
    efi_status_t err;
    void* ptr;
    err = BS->allocate_pool(_EFI_POOL_ALLOCATION, size, &ptr);
    if (EFI_ERROR(err)) {
        EFI_DBG_MESSAGE("Could not allocate pool memory");
        return NULL;
    }
    return ptr;
}

void* calloc(efi_size_t num, efi_size_t size) {
    efi_status_t err;
    void* ptr;
    err = BS->allocate_pool(_EFI_POOL_ALLOCATION, num * size, &ptr);
    if (EFI_ERROR(err)) {
        EFI_DBG_MESSAGE("Could not allocate pool memory");
        return NULL;
    }
    bzero(ptr, num * size);
    return ptr;
}

void free(void* p) {
    BS->free_pool(p);
}

inline
void bzero(
    void* buffer,
    efi_size_t size
) {
    uint8_t* ptr = buffer;
    while(size--) {
        *(ptr++) = 0;
    }
}

void* memset (
    void* buffer,
    uint8_t value,
    efi_size_t size    
) {
    uint8_t* ptr = buffer;
    while(size--) {
        *(ptr++) = value;
    }
    return buffer;
}

void* memcpy (
    void* dst,
    const void* src,
    efi_size_t size   
) {
    uint8_t* d = dst;
    const uint8_t* s = src;
    while(size--) {
        *(d++) = *(s++);
    }
    return d;
}

void* memmove (
    void* dst,
    const void* src,
    efi_size_t size  
) {
    if ((void*)((uint8_t*) dst + size) < src || (void*)((uint8_t*) src + size) < dst)
        return memcpy(dst, src, size);
    else {
        void* tmp = malloc(size);
        if (tmp) {
            memcpy(tmp, src, size);
            memcpy(dst, tmp, size);
            free(tmp);
            return dst;
        } else {
            /* just hope for the best */
            return memcpy(dst, src, size);
        }
    }
}

efi_size_t strlen (
    const char16_t* s
) {
    efi_size_t len;
    
    for (len = 0; *s; s++, len++) ;
    return len;
}

char16_t* strcpy (
    char16_t* dst,
    const char16_t* src
) {
    while (*src) {
        *(dst++) = *(src++);
    }
    *dst = 0;
    return dst;
}
