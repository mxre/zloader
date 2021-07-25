/**
 * @file efirtlib.c
 * @author Max Resch
 * @brief EFI runtime support library
 * @version 0.1
 * @date 2021-07-18
 * 
 * @copyright Copyright (c) 2021
 * 
 * @details
 *  Defines support funtions required by compiler builtins,
 *  some funtions are defined weak, so that they can be
 *  replaced by more efficient ones.
 */
#include <efi.h>
#include <efilib.h>

void* malloc(efi_size_t size) {
    EFILIB_ASSERT(BS);
    efi_status_t err;
    void* ptr;
    err = BS->allocate_pool(_EFI_POOL_ALLOCATION, size, &ptr);
    if (EFI_ERROR(err)) {
        EFILIB_DBG_MESSAGE("Could not allocate pool memory");
        return NULL;
    }
    return ptr;
}

void* calloc(efi_size_t num, efi_size_t size) {
    EFILIB_ASSERT(BS);
    efi_status_t err;
    void* ptr;
    err = BS->allocate_pool(_EFI_POOL_ALLOCATION, num * size, &ptr);
    if (EFI_ERROR(err)) {
        EFILIB_DBG_MESSAGE("Could not allocate pool memory");
        return NULL;
    }
    memset(ptr, 0, num * size);
    return ptr;
}

void free(void* p) {
    EFILIB_ASSERT(BS);
    BS->free_pool(p);
}

__weak__
inline
void* memset (
    void* buffer,
    uint8_t value,
    efi_size_t size    
) {
#if EFLLIB_USE_EFI_SET_MEM
    EFILIB_ASSERT(BS);
    BS->set_mem(buffer, size, value);
#else
    uint8_t* ptr = buffer;
    while (size--) {
        *(ptr++) = value;
    }
#endif
    return buffer;
}

#ifdef memmove
#undef memmove
#endif
__weak__
void* memmove (
    void* dst,
    const void* src,
    efi_size_t size  
) {
#if EFLLIB_USE_EFI_COPY_MEM
    EFILIB_ASSERT(BS);
    BS->copy_mem(dst, src, size);
    return dst;
#else
    if ((void*)((uint8_t*) dst + size) < src || (void*)((uint8_t*) src + size) < dst)
        return memcpy(dst, src, size);
    else {
        void* tmp = malloc(size);
        EFILIB_ASSERT(tmp); // OOM

        memcpy(tmp, src, size);
        memcpy(dst, tmp, size);
        free(tmp);
        return dst;
    }
#endif
}

#ifdef memcpy
#undef memcpy
#endif
__weak__
void* memcpy (
    void* dst,
    const void* src,
    efi_size_t size   
) {
#if EFLLIB_USE_EFI_COPY_MEM
    EFILIB_ASSERT(BS);
    BS->copy_mem(dst, src, size);
#else
    uint8_t* d = dst;
    const uint8_t* s = src;
    while (size--) {
        *(d++) = *(s++);
    }
#endif
    return dst;
}

#ifdef memcmp
#undef memcmp
#endif
__weak__
int memcmp (
    const void* a,
    const void* b,
    efi_size_t size   
) {
    const uint8_t* x = a;
    const uint8_t* y = b;
    
    while (size--) {
        int ret;
        if ((ret = *(x++) - *(y++)))
            return (ret > 0) - (ret < 0);
    }
    return 0;
}
