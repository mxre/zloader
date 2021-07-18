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
    efi_status_t err;
    void* ptr;
    err = BS->allocate_pool(_EFI_POOL_ALLOCATION, num * size, &ptr);
    if (EFI_ERROR(err)) {
        EFILIB_DBG_MESSAGE("Could not allocate pool memory");
        return NULL;
    }
    bzero(ptr, num * size);
    return ptr;
}

void free(void* p) {
    BS->free_pool(p);
}

#ifdef memmove
#undef memmove
#endif
[[ gnu::weak ]]
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

#ifdef memcpy
#undef memcpy
#endif
[[ gnu::weak ]]
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

#ifdef memcmp
#undef memcmp
#endif
[[ gnu::weak ]]
int memcmp (
    const void* a,
    const void* b,
    efi_size_t size   
) {
    const uint8_t* x = a;
    const uint8_t* y = b;
	
    while(size--) {
		int ret;
        if ((ret = *(x++) - *(y++)))
			return (ret > 0) - (ret < 0);
    }
    return 0;
}

#ifdef strlen
#undef strlen
#endif
[[ gnu::weak ]]
efi_size_t strlen (
    const char8_t* s
) {
    efi_size_t len;
    
    for (len = 0; *s; s++, len++) ;
    return len;
}

#ifdef strcmp
#undef strcmp
#endif
[[ gnu::weak ]]
int strcmp (
    const char8_t* a,
    const char8_t* b
) {	
    do {
		int ret;
        if ((ret = *a - *b))
			return (ret > 0) - (ret < 0);
    } while(*(++a) && *(++b));

	return 0;
}

#ifdef strchr
#undef strchr
#endif
[[ gnu::weak ]]
char8_t* strchr (
    const char8_t* str,
    int needle
) {	
    do {
		if (*str == needle)
			return (char8_t*) str;
	} while (*(++str));

	return NULL;
}

#ifdef wcslen
#undef wcslen
#endif
[[ gnu::weak ]]
efi_size_t wcslen (
    const char16_t* s
) {
    efi_size_t len;
    
    for (len = 0; *s; s++, len++) ;
    return len;
}

#ifdef wcscmp
#undef wcscmp
#endif
[[ gnu::weak ]]
int wcscmp (
    const char16_t* a,
    const char16_t* b
) {	
    do {
		int ret;
        if ((ret = *(a++) - *(b++)))
			return (ret > 0) - (ret < 0);
    } while(*a && *b);

	return 0;
}

#ifdef wcschr
#undef wcschr
#endif
[[ gnu::weak ]]
char16_t* wcschr (
    const char16_t* str,
    char16_t needle
) {	
    do {
		if (*str == needle)
			return (char16_t*)str;
	} while (*(++str));

	return NULL;
}
