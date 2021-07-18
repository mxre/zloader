/*
 * Copyright (c) 2016-present, Yann Collet, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of https://github.com/facebook/zstd.
 * An additional grant of patent rights can be found in the PATENTS file in the
 * same directory.
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License version 2 as published by the
 * Free Software Foundation. This program is dual-licensed; you may select
 * either version 2 of the GNU General Public License ("GPL") or BSD license
 * ("BSD").
 */

#pragma once

/*-****************************************
*  Dependencies
******************************************/

#include <stdint.h>
#include <stddef.h>
#include "zstd_deps.h"

#define MEM_STATIC [[ maybe_unused, gnu::always_inline ]] static inline

/*-**************************************************************
*  Basic Types
*****************************************************************/
typedef uint8_t BYTE;
typedef uint16_t U16;
typedef int16_t S16;
typedef uint32_t U32;
typedef int32_t S32;
typedef uint64_t U64;
typedef int64_t S64;
typedef ptrdiff_t iPtrDiff;
typedef uintptr_t uPtrDiff;

/*-**************************************************************
*  Memory I/O
*****************************************************************/
MEM_STATIC bool ZSTD_32bits(void) { return sizeof(size_t) == 4; }
MEM_STATIC bool ZSTD_64bits(void) { return sizeof(size_t) == 8; }

MEM_STATIC bool ZSTD_isLittleEndian(void) { return __LITTLE_ENDIAN__; }

typedef struct { U16 v; } __attribute__((packed)) unalign16;
typedef struct { U32 v; } __attribute__((packed)) unalign32;
typedef struct { U64 v; } __attribute__((packed)) unalign64;
typedef struct { size_t v; } __attribute__((packed)) unalignArch;

MEM_STATIC U16 ZSTD_read16(const void* ptr) { return ((const unalign16*) ptr)->v; }

MEM_STATIC U32 ZSTD_read32(const void* ptr) { return ((const unalign32*) ptr)->v; }

MEM_STATIC U64 ZSTD_read64(const void* ptr) { return ((const unalign64*) ptr)->v; }

MEM_STATIC size_t ZSTD_readST(const void* ptr) { return ((const unalignArch*) ptr)->v; }

MEM_STATIC void ZSTD_write16(void* memPtr, U16 value) { ((unalign16*) memPtr)->v = value; }

MEM_STATIC void ZSTD_write32(void* memPtr, U32 value) { ((unalign32*) memPtr)->v = value; }

MEM_STATIC void ZSTD_write64(void* memPtr, U64 value) { ((unalign64*) memPtr)->v = value; }

#define ZSTD_swap32 __builtin_bswap32

#define ZSTD_swap64 __builtin_bswap64

/*=== Little endian r/w ===*/

MEM_STATIC U16 ZSTD_readLE16(const void* memPtr)
{
    if (ZSTD_isLittleEndian())
        return ZSTD_read16(memPtr);
    else {
        const BYTE* p = (const BYTE*) memPtr;
        return (U16)(p[0] + (p[1]<<8));
    }
}

MEM_STATIC void ZSTD_writeLE16(void* memPtr, U16 val)
{
    if (ZSTD_isLittleEndian()) {
        ZSTD_write16(memPtr, val);
    } else {
        BYTE* p = (BYTE*) memPtr;
        p[0] = (BYTE) val;
        p[1] = (BYTE)(val >> 8);
    }
}

MEM_STATIC U32 ZSTD_readLE24(const void *memPtr)
{
	return ZSTD_readLE16(memPtr) + (((const BYTE*) memPtr)[2] << 16);
}

MEM_STATIC void ZSTD_writeLE24(void* memPtr, U32 val)
{
	ZSTD_writeLE16(memPtr, (U16) val);
	((BYTE*) memPtr)[2] = (BYTE)(val >> 16);
}

MEM_STATIC U32 ZSTD_readLE32(const void* memPtr)
{
    if (ZSTD_isLittleEndian())
        return ZSTD_read32(memPtr);
    else
        return ZSTD_swap32(ZSTD_read32(memPtr));
}

MEM_STATIC void ZSTD_writeLE32(void* memPtr, U32 val32)
{
    if (ZSTD_isLittleEndian())
        ZSTD_write32(memPtr, val32);
    else
        ZSTD_write32(memPtr, ZSTD_swap32(val32));
}

MEM_STATIC U64 ZSTD_readLE64(const void* memPtr)
{
    if (ZSTD_isLittleEndian())
        return ZSTD_read64(memPtr);
    else
        return ZSTD_swap64(ZSTD_read64(memPtr));
}

MEM_STATIC void ZSTD_writeLE64(void* memPtr, U64 val64)
{
    if (ZSTD_isLittleEndian())
        ZSTD_write64(memPtr, val64);
    else
        ZSTD_write64(memPtr, ZSTD_swap64(val64));
}

MEM_STATIC size_t ZSTD_readLEST(const void *memPtr)
{
	if (ZSTD_32bits())
		return (size_t) ZSTD_readLE32(memPtr);
	else
		return (size_t) ZSTD_readLE64(memPtr);
}

MEM_STATIC void ZSTD_writeLEST(void *memPtr, size_t val)
{
	if (ZSTD_32bits())
		ZSTD_writeLE32(memPtr, (U32) val);
	else
		ZSTD_writeLE64(memPtr, (U64) val);
}

/*=== Big endian r/w ===*/


MEM_STATIC U32 ZSTD_readBE32(const void* memPtr)
{
    if (ZSTD_isLittleEndian())
        return ZSTD_swap32(ZSTD_read32(memPtr));
    else
        return ZSTD_read32(memPtr);
}

MEM_STATIC void ZSTD_writeBE32(void* memPtr, U32 val32)
{
    if (ZSTD_isLittleEndian())
        ZSTD_write32(memPtr, ZSTD_swap32(val32));
    else
        ZSTD_write32(memPtr, val32);
}

MEM_STATIC U64 ZSTD_readBE64(const void* memPtr)
{
    if (ZSTD_isLittleEndian())
        return ZSTD_swap64(ZSTD_read64(memPtr));
    else
        return ZSTD_read64(memPtr);
}

MEM_STATIC void ZSTD_writeBE64(void* memPtr, U64 val64)
{
    if (ZSTD_isLittleEndian())
        ZSTD_write64(memPtr, ZSTD_swap64(val64));
    else
        ZSTD_write64(memPtr, val64);
}

MEM_STATIC size_t ZSTD_readBEST(const void *memPtr)
{
	if (ZSTD_32bits())
		return (size_t) ZSTD_readBE32(memPtr);
	else
		return (size_t) ZSTD_readBE64(memPtr);
}

MEM_STATIC void ZSTD_writeBEST(void *memPtr, size_t val)
{
	if (ZSTD_32bits())
		ZSTD_writeBE32(memPtr, (U32) val);
	else
		ZSTD_writeBE64(memPtr, (U64) val);
}

/* function safe only for comparisons */
MEM_STATIC U32 ZSTD_readMINMATCH(const void *memPtr, U32 length)
{
	switch (length) {
		default:
		case 4: return ZSTD_read32(memPtr);
		case 3:
			if (ZSTD_isLittleEndian())
				return ZSTD_read32(memPtr) << 8;
			else
				return ZSTD_read32(memPtr) >> 8;
	}
}

#define MEM_isLittleEndian  ZSTD_isLittleEndian
#define MEM_32bits          ZSTD_32bits
#define MEM_64bits          ZSTD_64bits

#define MEM_read16          ZSTD_read16
#define MEM_read32          ZSTD_read32
#define MEM_read64          ZSTD_read64
#define MEM_readLE16        ZSTD_readLE16
#define MEM_readLE24        ZSTD_readLE24
#define MEM_readLE32        ZSTD_readLE32
#define MEM_readLE64        ZSTD_readLE64
#define MEM_readLEST        ZSTD_readLEST

#define MEM_write16         ZSTD_write16
#define MEM_write32         ZSTD_write32
#define MEM_write64         ZSTD_write64
#define MEM_writeLE16       ZSTD_writeLE16
#define MEM_writeLE32       ZSTD_writeLE32
#define MEM_writeLE64       ZSTD_writeLE64
#define MEM_writeLEST       ZSTD_writeLEST
