/*
 * decoder_flac.c
 *
 * FLAC decoder using dr_flac (single-header, pure C, no assembly).
 * Uses shared static pool for all allocations (heap-free).
 */
#include "decoder.h"
#include "decoder_pool.h"
#include "main.h"
#include <string.h>

#define DRFLAC_MALLOC(sz)    decoder_pool_malloc(sz)
#define DRFLAC_REALLOC(p,sz) decoder_pool_realloc(p,sz)
#define DRFLAC_FREE(p)       decoder_pool_free(p)

#define DR_FLAC_BUFFER_SIZE  8192
#define DR_FLAC_IMPLEMENTATION
#define DR_FLAC_NO_STDIO
#define DR_FLAC_NO_OGG
#include "dr_flac.h"

typedef struct {
    FIL    *fp;
    drflac *flac;
} flac_state_t;

static size_t on_read(void *pUserData, void *pDst, size_t bytes)
{
    // WARNING:dr_wav decoder uses variables assigned at RTOS task stack as pDst to accmodate file contents.
    // WARNING: so pDst may not within the AXI SRAM section, which is the only place SDMMC can access
    static uint8_t  __attribute__((section(".DMA_no_cache"))) __attribute__((aligned(32))) bufFileOnRead[DR_FLAC_BUFFER_SIZE];
    // pDst must point to AXI SRAM, or the SDMMC will not have access to.
    FIL *fp = (FIL *)pUserData;
    UINT br;
    if (f_read(fp, bufFileOnRead, bytes, &br) != FR_OK) return 0;
    /* Invalidate D-cache after SDMMC DMA read into cacheable pool */
//    SCB_InvalidateDCache_by_Addr((uint32_t *)pDst, (int32_t)br);
    memcpy(pDst, bufFileOnRead, br);
    return br;
}

static drflac_bool32 on_seek(void *pUserData, int offset, drflac_seek_origin origin)
{
    FIL *fp = (FIL *)pUserData;
    FSIZE_t pos;
    if (origin == DRFLAC_SEEK_SET)      pos = offset;
    else if (origin == DRFLAC_SEEK_CUR) pos = f_tell(fp) + offset;
    else return DRFLAC_FALSE;
    return (f_lseek(fp, pos) == FR_OK) ? DRFLAC_TRUE : DRFLAC_FALSE;
}

static drflac_bool32 on_tell(void *pUserData, drflac_int64 *pPos)
{
    FIL *fp = (FIL *)pUserData;
    *pPos = (drflac_int64)f_tell(fp);
    return DRFLAC_TRUE;
}


static int flac_open(FIL *fp, void **state, decoder_info_t *info)
{
    decoder_pool_reset();

    flac_state_t *s = (flac_state_t *)decoder_pool_malloc(sizeof(flac_state_t));
    if (!s) return DECODER_ERR_OPEN;

    f_rewind(fp);
    s->fp = fp;

    s->flac = drflac_open(on_read, on_seek, on_tell, fp, NULL);
    if (!s->flac) {
        return DECODER_ERR_PARSE;
    }

    info->sample_rate    = s->flac->sampleRate;
    info->bits_per_sample = 16;
    info->num_channels   = (uint8_t)s->flac->channels;
    info->total_samples  = (uint32_t)s->flac->totalPCMFrameCount;
    info->duration_ms    = (uint32_t)((s->flac->totalPCMFrameCount / s->flac->sampleRate) * 1000);

    /* System only supports stereo, 44.1k / 48k / 96k and 16/24-bit */
    if (info->num_channels != 2 ||
        (info->sample_rate != 44100 && info->sample_rate != 48000 && info->sample_rate != 96000)) {
        drflac_close(s->flac);
        return DECODER_ERR_PARSE;
    }

    *state = s;
    return DECODER_OK;
}

static int flac_decode(void *state,
                        float *out, size_t capacity,
                        size_t *out_frames)
{
    flac_state_t *s = (flac_state_t *)state;
    if (!s->flac) return DECODER_ERR_DECODE;

    drflac_uint64 frames = drflac_read_pcm_frames_f32(s->flac, capacity / s->flac->channels, out);

    if (frames == 0) {
        *out_frames = 0;
        return DECODER_ERR_EOF;
    }

    *out_frames = (size_t)(frames * s->flac->channels);
    return DECODER_OK;
}

static void flac_close(void **state)
{
    if (state && *state) {
        flac_state_t *s = (flac_state_t *)*state;
        if (s->flac) drflac_close(s->flac);
        *state = NULL;
    }
}

static int flac_seek(void *state, uint32_t frame_index)
{
    flac_state_t *s = (flac_state_t *)state;
    return drflac_seek_to_pcm_frame(s->flac, frame_index) ? DECODER_OK : DECODER_ERR_DECODE;
}

const decoder_t decoder_flac = {
    .name   = "flac",
    .open   = flac_open,
    .decode = flac_decode,
    .close  = flac_close,
    .seek   = flac_seek,
};
