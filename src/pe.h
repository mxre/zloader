/**
 * @file pe.h
 * @author Max Resch
 * @brief PE image header
 * @version 0.1
 * @date 2021-07-18
 * 
 * @copyright Copyright (c) 2021
 *
 * @see https://docs.microsoft.com/windows/win32/debug/pe-format
 */
#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <assert.h>

#include <efi/pe.h>
#include "util.h"

/**
 * @brief relocate PE file from buffer and prepare for execution
 * 
 * @param[in] data
 *  buffer containing PE image
 * @param[out] image
 *  Handle for the new image
 * @param[out] loaded_image
 *  LoadedImage protocol registerd for image handle
 * @param[out] entry_point
 *  pointer to the `efi_main` function of the image, can be called, when
 *  this function returned `EFI_SUCCESS`
 */
efi_status_t PE_handle_image(
    simple_buffer_t data,
    efi_handle_t* image,
    efi_loaded_image_t* loaded_image,
    efi_entry_point_t* entry_point
);

/**
 * @brief check if the data in buffer is a PE image
 * 
 * @param[in] buffer
 * @param[in] buffer_size 
 * 
 * @returns 0 on error
 * @returns offset to PE header
 */
static inline
efi_size_t PE_header(simple_buffer_t buffer) {
    assert(buffer);
    assert(buffer->buffer);

    /* has MZ magic and is large enough for MSDOS header */
    if (*(uint16_t*) buffer_pos(buffer) == MZ_DOS_SIGNATURE && buffer_len(buffer) > 0x40) {
        uint32_t pe_offset = *(uint32_t*)(buffer_pos(buffer) + DOS_PE_OFFSET_LOCATION);
        /* check if large enough for PE header */
        if (buffer_len(buffer) > (pe_offset + sizeof(struct PE_image_headers))) {
            if(*(uint32_t*)(buffer_pos(buffer) + pe_offset) == PE_HEADER_SIGNATURE) {
                return pe_offset;
            }
        }
    }
    return 0;
}

struct PE_locate_sections {
    /**
     * @brief section short name
     */
    char8_t name[PE_SECTION_SIZE_OF_SHORT_NAME]; 

    /**
     * @brief virtual address where section is loaded
     */
    size_t load_address;

    /**
     * @brief offset from the image base address
     */
    size_t offset;

    /**
     * @brief length of the sections data
     */
    size_t size;
};

typedef struct PE_locate_sections* PE_locate_sections_t;

/**
 * @brief locate sections in current loaded image
 * 
 * @param[in,out] sections
 *  NULL terminated array of sections, filled with the section names
 * @returns true
 *  sections was filled with available section information, sections that
 *  where not found in image were not touched in the array
 * @returns false 
 *  could not parse the image correctly, sections was not filled
 */
bool PE_locate_sections(
    PE_locate_sections_t sections
);
