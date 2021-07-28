#include "decompress.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <efilib.h>
#include <assert.h>
#include <minmax.h>

#include "util.h"
#include "pe.h"

#ifdef USE_LZ4
# include <lz4.h>
# include <lz4frame.h>
#endif

#ifdef USE_ZSTD
# include <zstd.h>
# include <zstd_errors.h>

/* Check that our buffer implementation is ABI compatible with ZSTD*/
static_assert(
    offsetof(struct simple_buffer, buffer) == offsetof(ZSTD_inBuffer, src) &&
    offsetof(struct simple_buffer, length) == offsetof(ZSTD_inBuffer, size) &&
    offsetof(struct simple_buffer, pos)    == offsetof(ZSTD_inBuffer, pos) &&
    sizeof(struct simple_buffer) >= sizeof(ZSTD_inBuffer),
    "'struct simple_buffer' is not compatible with ZSTD_inBuffer"
);

static_assert(
    offsetof(struct simple_buffer, buffer) == offsetof(ZSTD_outBuffer, dst) &&
    offsetof(struct simple_buffer, length) == offsetof(ZSTD_outBuffer, size) &&
    offsetof(struct simple_buffer, pos)    == offsetof(ZSTD_outBuffer, pos) &&
    sizeof(struct simple_buffer) >= sizeof(ZSTD_outBuffer),
    "'struct simple_buffer' is not compatible with ZSTD_outBuffer"
);
#endif

#ifdef USE_LZ4
static inline
efi_status_t decompress_lz4(
    simple_buffer_t in,
    simple_buffer_t out
) {
    efi_status_t err;

    LZ4F_dctx* ctx;
    err = LZ4F_createDecompressionContext(&ctx, LZ4F_VERSION);
    if (LZ4F_isError(err)) {
        _ERROR("LZ4 (%zu): %s", -err, LZ4F_getErrorName(err));
        return EFI_OUT_OF_RESOURCES;
    }

    {
        /* retrieve uncompressed size
         * NOTE: This only works if lz4 was invoked with --content-size */
        LZ4F_frameInfo_t frame_info = { 0 };
        size_t in_pos = in->length;
        err = LZ4F_getFrameInfo(ctx, &frame_info, buffer_pos(in), &in_pos);
        if (LZ4F_isError(err)) {
            _ERROR("LZ4 (%zu): %s", -err, LZ4F_getErrorName(err));
            err = EFI_UNSUPPORTED;
            goto end;
        }

        in->pos = in_pos;

        if (!frame_info.contentSize) {
            _ERROR("LZ4 does not contain uncompressed size");
            err = EFI_UNSUPPORTED;
            goto end;
        }

        if (!allocate_simple_buffer(frame_info.contentSize, out)) {
            err = EFI_OUT_OF_RESOURCES;
            goto end;
        }
    }

    size_t result = 0;
    while (in->pos < in->length) {
        size_t out_end = out->allocated - out->pos;
        size_t in_end = buffer_len(in);
        result = LZ4F_decompress(ctx, buffer_pos((simple_buffer_t) out), &out_end, buffer_pos(in), &in_end, NULL);
        if (LZ4F_isError(result)) {
            _ERROR("LZ4 (%zu): %s", -result, LZ4F_getErrorName(result));
            err = EFI_UNSUPPORTED;
            goto end;
        }
        in->pos += in_end;
        out->length = out->pos += out_end;
    }
    
    if (result)
        _ERROR("EOF before end of stream: %zu", result);
    
    _MESSAGE("in = %zu out = %zu", in->pos, out->pos);
    err = EFI_SUCCESS;
end:
    LZ4F_freeDecompressionContext(ctx);

    if (EFI_ERROR(err)) {
        out->free(out);
        out->allocated = 0;
        out->buffer = NULL;
    } else {
        out->pos = 0;
    }

    return err;
}
#endif /* USE_LZ4 */

#ifdef USE_ZSTD
static inline
efi_status_t decompress_zstd(
    simple_buffer_t in,
    simple_buffer_t out
) {
    efi_status_t err;

    /* retrieve uncompressed size */
    out->allocated = ZSTD_getFrameContentSize(buffer_pos(in), buffer_len(in));
    if (out->allocated == ZSTD_CONTENTSIZE_ERROR || out->allocated == ZSTD_CONTENTSIZE_UNKNOWN) {
        _ERROR("ZSTD can't determine content size: %zu", -out->allocated);
        out->allocated = 0;
        return EFI_UNSUPPORTED;
    }

    if (!allocate_simple_buffer(out->allocated, out)) {
        return EFI_OUT_OF_RESOURCES;
    }

    ZSTD_DStream* zstream = ZSTD_createDStream();
    if (!zstream) {
        free(out->buffer);
        return EFI_OUT_OF_RESOURCES;
    }

    size_t result = 0;
    out->length = out->allocated;
    while (in->pos < in->length) {
        result = ZSTD_decompressStream(zstream, (ZSTD_outBuffer*) out, (ZSTD_inBuffer*) in);
        if (ZSTD_isError(result)) {
            _ERROR("ZSTD (%zu): %s", ZSTD_getErrorCode(result), ZSTD_getErrorName(result));
            err = EFI_COMPROMISED_DATA;
            goto end;
        }
    }

    if (result)
        _ERROR("EOF before end of stream: %zu", result);
    
    _MESSAGE("in = %zu out = %zu", in->pos, out->pos);
    err = EFI_SUCCESS;
end:
    ZSTD_freeDStream(zstream);

    if (EFI_ERROR(err)) {
        out->free(out);
        out->allocated = 0;
        out->buffer = NULL;
    } else {
        out->pos = 0;
    }

    return err;
}
#endif /* USE_ZSTD */

efi_status_t decompress(
    simple_buffer_t in,
    simple_buffer_t out
) {
    if (!in || !out)
        return EFI_INVALID_PARAMETER;
    if (!in->buffer || !in->length)
        return EFI_INVALID_PARAMETER;
    if (out->buffer || out->length)
        return EFI_INVALID_PARAMETER;
    
#ifdef USE_ZSTD
    if ((*(uint32_t*) buffer_pos(in)) == ZSTD_MAGICNUMBER) {
        _MESSAGE("detected ZSTD compressed data");
        return decompress_zstd(in, out);
    } else
#endif
#ifdef USE_LZ4
    if ((*(uint32_t*) buffer_pos(in)) == LZ4_MAGICNUMBER) {
        _MESSAGE("detected LZ4 compressed data");
        return decompress_lz4(in, out);
    } else
#endif
    /* directly pass on an uncompressed executable */
    if (PE_header(in) > 0) {
        _MESSAGE("detected EFI executable");
        out->buffer = in->buffer;
        out->allocated = out->length = in->length;
        out->pos = in->pos;
        out->free = NULL;
        return EFI_SUCCESS;
    } else {
        _MESSAGE("unsupported file format: %X", (*(uint32_t*) buffer_pos(in)));
        return EFI_UNSUPPORTED;
    }
}
