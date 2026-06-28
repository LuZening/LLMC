/*
 * decoder_mp3.c
 *
 * MP3 decoder using dr_mp3 (single-header, pure C, no assembly).
 * Uses shared static pool for all allocations (heap-free).
 */
#include "decoder.h"
#include "decoder_pool.h"
#include "main.h"
#include <string.h>

#define DRMP3_MALLOC(sz)    decoder_pool_malloc(sz)
#define DRMP3_REALLOC(p,sz) decoder_pool_realloc(p,sz)
#define DRMP3_FREE(p)       decoder_pool_free(p)

#define DRMP3_DATA_CHUNK_SIZE   32768
#define DR_MP3_FLOAT_OUTPUT
#define DR_MP3_IMPLEMENTATION
#define DR_MP3_NO_STDIO
#include "dr_mp3.h"

typedef struct {
    FIL   *fp;
    drmp3  mp3;
    int    channels;
} mp3_state_t;

static size_t on_read(void *pUserData, void *pDst, size_t bytes)
{
    // WARNING:dr_wav decoder uses variables assigned at RTOS task stack as pDst to accmodate file contents.
    // WARNING: so pDst may not within the AXI SRAM section, which is the only place SDMMC can access
    static uint8_t  __attribute__((section(".DMA_no_cache"))) __attribute__((aligned(32))) bufFileOnRead[DRMP3_DATA_CHUNK_SIZE];
    // pDst must point to AXI SRAM, or the SDMMC will not have access to.
    FIL *fp = (FIL *)pUserData;
    UINT br;
    if (f_read(fp, bufFileOnRead, bytes, &br) != FR_OK) return 0;
    /* Invalidate D-cache after SDMMC DMA read into cacheable pool */
//    SCB_InvalidateDCache_by_Addr((uint32_t *)pDst, (int32_t)br);
    memcpy(pDst, bufFileOnRead, br);
    return br;
}

static drmp3_bool32 on_seek(void *pUserData, int offset, drmp3_seek_origin origin)
{
    FIL *fp = (FIL *)pUserData;
    FSIZE_t pos;
    if (origin == DRMP3_SEEK_SET)      pos = offset;
    else if (origin == DRMP3_SEEK_CUR) pos = f_tell(fp) + offset;
    else return DRMP3_FALSE;
    return (f_lseek(fp, pos) == FR_OK) ? DRMP3_TRUE : DRMP3_FALSE;
}

static drmp3_bool32 on_tell(void *pUserData, drmp3_uint64 *pPos)
{
    FIL *fp = (FIL *)pUserData;
    *pPos = (drmp3_uint64)f_tell(fp);
    return DRMP3_TRUE;
}

static int mp3_open(FIL *fp, void **state, decoder_info_t *info)
{
    decoder_pool_reset();

    mp3_state_t *s = (mp3_state_t *)decoder_pool_malloc(sizeof(mp3_state_t));
    if (!s) return DECODER_ERR_OPEN;

    f_rewind(fp);
    s->fp = fp;

    if (!drmp3_init(&s->mp3, on_read, on_seek, on_tell, NULL, fp, NULL)) {
        return DECODER_ERR_PARSE;
    }

    s->channels = s->mp3.channels;
    info->sample_rate    = s->mp3.sampleRate;
    info->bits_per_sample = 16;
    info->num_channels   = (uint8_t)s->mp3.channels;
    info->total_samples  = (uint32_t)drmp3_get_pcm_frame_count(&s->mp3);
    info->duration_ms    = (uint32_t)((drmp3_get_pcm_frame_count(&s->mp3) / s->mp3.sampleRate) * 1000);

    /* System only supports stereo, 44.1k / 48k / 96k and 16/24-bit */
    if (info->num_channels != 2 ||
        (info->sample_rate != 44100 && info->sample_rate != 48000 && info->sample_rate != 96000)) {
        drmp3_uninit(&s->mp3);
        return DECODER_ERR_PARSE;
    }

    *state = s;
    return DECODER_OK;
}

static int mp3_decode(void *state,
                       float *out, size_t capacity,
                       size_t *out_frames)
{
    mp3_state_t *s = (mp3_state_t *)state;

    drmp3_uint64 frames = drmp3_read_pcm_frames_f32(&s->mp3, capacity / s->channels, out);

    if (frames == 0) {
        *out_frames = 0;
        return DECODER_ERR_EOF;
    }

    *out_frames = (size_t)(frames * s->channels);
    return DECODER_OK;
}

static void mp3_close(void **state)
{
    if (state && *state) {
        mp3_state_t *s = (mp3_state_t *)*state;
        drmp3_uninit(&s->mp3);
        *state = NULL;
    }
}

static int mp3_seek(void *state, uint32_t frame_index)
{
    mp3_state_t *s = (mp3_state_t *)state;
    return drmp3_seek_to_pcm_frame(&s->mp3, frame_index) ? DECODER_OK : DECODER_ERR_DECODE;
}

const decoder_t decoder_mp3 = {
    .name   = "mp3",
    .open   = mp3_open,
    .decode = mp3_decode,
    .close  = mp3_close,
    .seek   = mp3_seek,
};
