/**
 * @file efirtlib.c
 * @author Max Resch
 * @brief runtime string library
 * @version 0.1
 * @date 2021-07-18
 * 
 * @copyright Copyright (c) 2021
 */

#include <efilib.h>

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
    while(*a && (*a == *b)) {
        a++;
        b++;
    }

    return *a - *b;
}

#ifdef strncmp
#undef strncmp
#endif
int strncmp (
    const char8_t* a,
    const char8_t* b,
    size_t length
) {
    while(*a && (*a == *b) && length) {
        if (*a != *b)
            break;
        a++;
        b++;
        length--;
    }

    if (length == 0)
        return 0;
    else
        return *a - *b;
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

/**
 * @see https://github.com/systemd/systemd/blob/main/src/boot/efi/util.c
 */
efi_size_t mbstowcs(
    char16_t* restrict dest,
    const char8_t* restrict src,
    efi_size_t len
) {
    efi_size_t strlen;

    strlen = 0;
    efi_size_t i = 0;
    while (i < len) {
        int utf8len = mbtowc(dest + strlen, src + i, len - i);
        if (utf8len <= 0) {
            /* invalid utf8 sequence, skip the garbage */
            i++;
            continue;
        }

        strlen++;
        i += utf8len;
    }
    dest[strlen] = u'\0';
    return strlen;
}
