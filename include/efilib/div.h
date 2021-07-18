#pragma once

#include <efi.h>

typedef struct { int quot; int  rem; } div_t;

[[ gnu::pure ]]
static inline
div_t div(int numerator, int denominator) {
    div_t r = {
        .quot = numerator / denominator,
        .rem  = numerator % denominator
    };
    return r;
}


typedef struct { long quot; long  rem; } ldiv_t;

[[ gnu::pure ]]
static inline
ldiv_t ldiv(long numerator, long denominator) {
    ldiv_t r = {
        .quot = numerator / denominator,
        .rem  = numerator % denominator
    };
    return r;
}


typedef struct { long long quot; long long  rem; } lldiv_t;

[[ gnu::pure ]]
static inline
lldiv_t lldiv(long long numerator, long long denominator) {
   lldiv_t r = {
        .quot = numerator / denominator,
        .rem  = numerator % denominator
    };
    return r;
}
