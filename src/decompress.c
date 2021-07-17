#include "decompress.h"

#include <efilib.h>
#include <lz4.h>
#include <lz4frame.h>
#include <zstd.h>
#include <zstd_errors.h>
#include "util.h"

static inline
efi_status_t decompress_lz4(
    efi_file_handle_t handle,
    void* in_buffer,
    size_t in_buffer_pos,
    size_t in_buffer_size,
    size_t in_total_size,
    void** buffer,
    size_t* buffer_size
) {
    efi_status_t err;

    LZ4F_dctx* ctx;
    err = LZ4F_createDecompressionContext(&ctx, LZ4F_VERSION);
    if (LZ4F_isError(err)) {
        printf(u"LZ4 error (%u): %a\n", -err, LZ4F_getErrorName(err));
        return EFI_OUT_OF_RESOURCES;
    }

    size_t total_in = in_buffer_pos;
    size_t in_pos = in_buffer_pos;
    size_t out_size;
    {
        LZ4F_frameInfo_t frame_info = { 0 };
        err = LZ4F_getFrameInfo(ctx, &frame_info, in_buffer, &in_pos);
        if (LZ4F_isError(err)) {
            printf(u"LZ4 error (%u): %a\n", -err, LZ4F_getErrorName(err));
            err = EFI_COMPROMISED_DATA;
            goto end;
        }

        if (!frame_info.contentSize) {
            printf(u"LZ4 does not contain uncompressed size\n");
            err = EFI_COMPROMISED_DATA;
            goto end;
        }

        out_size = frame_info.contentSize;
    }

    void* out_buf = malloc(out_size);
    _MESSAGE("allocated %u", out_size);

    size_t result = 0;
    size_t out_pos = 0;

    ST->out->set_attribute(ST->out, EFI_TEXT_ATTR(EFI_LIGHTGRAY, EFI_BLUE));
    clear_screen();
    ST->out->set_cursor_position(ST->out, 0, 0);
    print(u"decompress:");

    do {
        while (in_pos < in_buffer_pos) {
            size_t out_end = out_size - out_pos;
            size_t in_end = in_buffer_pos - in_pos;
            result = LZ4F_decompress(ctx, (uint8_t*) out_buf + out_pos, &out_end, (uint8_t*) in_buffer + in_pos, &in_end, NULL);
            if (LZ4F_isError(result)) {
                printf(u"LZ4 error (%u): %a\n", -result, LZ4F_getErrorName(result));
                err = EFI_COMPROMISED_DATA;
                goto end;
            }
            in_pos += in_end;
            out_pos += out_end;
            printf_at(12, 0, u"%3.4f%%", (((double) out_pos) / ((double) out_size)) * 100.0);
        }

        if (total_in == in_total_size) {
            break;
        } else {
            size_t size = in_buffer_size;
            err = handle->read(handle, &size, (uint8_t*) in_buffer);
            if (EFI_ERROR(err)) {
                printf(u"Unable to read file: %r\n", err);
                goto end;
            }

            /* reached input end */
            if (size == 0) {
                break;
            }

            in_pos = 0;
            total_in += size;
            in_buffer_pos = size;
        }
    } while(1);

    printf_at(12, 0, u" done\n");
    
    if (result)
        printf(u"EOF before end of stream: %u\n", result);
    
    printf(u"in = %u out = %u\n", total_in, out_pos);
end:
    LZ4F_freeDecompressionContext(ctx);

    if (EFI_ERROR(err)) {
        free(out_buf);
        *buffer = NULL;
    } else {
        *buffer = out_buf;
        *buffer_size = out_pos;
    }

    return err;
}

static inline
efi_status_t decompress_zstd(
    efi_file_handle_t handle,
    void* in_buffer,
    size_t in_buffer_pos,
    size_t in_buffer_size,
    size_t in_total_size,
    void** buffer,
    size_t* buffer_size
) {
    efi_status_t err;

    size_t out_size = ZSTD_getFrameContentSize(in_buffer, in_buffer_pos);
    if (out_size == ZSTD_CONTENTSIZE_ERROR || out_size == ZSTD_CONTENTSIZE_UNKNOWN) {
        printf(u"ZSTD can't determine content size: %u\n", -out_size);
        return EFI_COMPROMISED_DATA;
    }

    void* out_buf = malloc(out_size);
    if (!out_buf) {
        return EFI_OUT_OF_RESOURCES;
    }

    ZSTD_DStream* zstream = ZSTD_createDStream();
    if (!zstream) {
        free(out_buf);
        return EFI_OUT_OF_RESOURCES;
    }

    ST->out->set_attribute(ST->out, EFI_TEXT_ATTR(EFI_LIGHTGRAY, EFI_BLUE));
    clear_screen();
    ST->out->set_cursor_position(ST->out, 0, 0);
    print(u"decompress:");

    ZSTD_inBuffer in = { .src = in_buffer, .size = in_buffer_pos, .pos = 0 };
    ZSTD_outBuffer out = { .dst = out_buf, .size = out_size, .pos = 0 };

    size_t result = 0;
    size_t total_in = in_buffer_pos;
    do {
        while (in.pos < in.size) {
            result = ZSTD_decompressStream(zstream, &out, &in);
            if (ZSTD_isError(result)) {
                printf(u"ZSTD error (%u): %a\n", ZSTD_getErrorCode(result), ZSTD_getErrorName(result));
                err = EFI_COMPROMISED_DATA;
                goto end;
            }

            printf_at(12, 0, u"%3.4f%%", (((double) out.pos) / ((double) out.size)) * 100.0);
        }

        /* input end */
        if (total_in == in_total_size) {
            break;
        } else {
            size_t size = in_buffer_size;
            err = handle->read(handle, &size, in_buffer);
            if (EFI_ERROR(err)) {
                printf(u"Unable to read file: %r\n", err);
                goto end;
            }

            /* reached input end */
            if (size == 0) {
                break;
            }
            
            total_in += size;
            in.pos = 0;
            in.size = size;
        }
    } while(true);

    printf_at(12, 0, u" done\n");

    if (result)
        printf(u"EOF before end of stream: %u\n", result);
    
    printf(u"in = %u out = %u\n", total_in, out.pos);
end:
    ZSTD_freeDStream(zstream);

    if (EFI_ERROR(err)) {
        free(out_buf);
        *buffer = NULL;
    } else {
        *buffer = out_buf;
        *buffer_size = out.pos;
    }

    return err;
}

efi_status_t decompress_file(
    efi_file_handle_t handle,
    void** buffer,
    size_t* buffer_size
) {
    efi_status_t err = EFI_SUCCESS;

    if (!buffer || !buffer_size)
        return EFI_INVALID_PARAMETER;
    *buffer_size = 0;
    if (*buffer)
        return EFI_INVALID_PARAMETER;

    efi_file_info_t info = lib_get_file_info(handle);
    if (!info)
        return EFI_OUT_OF_RESOURCES;
    efi_size_t file_size = info->file_size;
    free(info);

    size_t in_size = ZSTD_DStreamInSize();
    in_size = (in_size < file_size) ? in_size : file_size;
    _MESSAGE("buffer: %u file: %u", in_size, file_size);

    _cleanup_pool void* buf = malloc(in_size);
    size_t size = in_size;
    err = handle->read(handle, &size, buf);
    if (EFI_ERROR(err)) {
        printf(u"Unable to read file: %r\n", err);
        return err;
    }

    _MESSAGE("read: %u", size);

    if ((*(uint32_t*) buf) == ZSTD_MAGICNUMBER) {
        return decompress_zstd(
            handle, buf, size, in_size, file_size,
            buffer, buffer_size
        );
    } else if ((*(uint32_t*) buf) == LZ4_MAGICNUMBER) {
        return decompress_lz4(
            handle, buf, size, in_size, file_size,
            buffer, buffer_size
        );
    } else {
        return EFI_UNSUPPORTED;
    }
}
