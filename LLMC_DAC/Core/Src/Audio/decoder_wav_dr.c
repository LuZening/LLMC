/*
 * decoder_wav_dr.c
 *
 * WAV decoder using dr_wav (single-header, callback-based streaming).
 */
#include "decoder.h"
#include "decoder_pool.h"
#include "main.h"
#include <string.h>

#define DRWAV_MALLOC(sz)     decoder_pool_malloc(sz)
#define DRWAV_REALLOC(p,sz)  decoder_pool_realloc(p,sz)
#define DRWAV_FREE(p)        decoder_pool_free(p)

#define DR_WAV_IMPLEMENTATION
#define DR_WAV_NO_STDIO
#include "dr_wav.h"

typedef struct {
    drwav wav;
} wav_dr_state_t;

static size_t on_read(void *pUserData, void *pDst, size_t bytes)
{
    // WARNING:dr_wav decoder uses variables assigned at RTOS task stack as pDst to accmodate file contents.
    // WARNING: so pDst may not within the AXI SRAM section, which is the only place SDMMC can access
    static uint8_t  __attribute__((section(".DMA_no_cache"))) __attribute__((aligned(32))) bufFileOnRead[4096];
    // pDst must point to AXI SRAM, or the SDMMC will not have access to.
    FIL *fp = (FIL *)pUserData;
    UINT br;
    if (f_read(fp, bufFileOnRead, bytes, &br) != FR_OK) return 0;
    /* Invalidate D-cache after SDMMC DMA read into cacheable pool */
//    SCB_InvalidateDCache_by_Addr((uint32_t *)pDst, (int32_t)br);
    memcpy(pDst, bufFileOnRead, br);
    return br;
}

static drwav_bool32 on_seek(void *pUserData, int offset, drwav_seek_origin origin)
{
    FIL *fp = (FIL *)pUserData;
    FSIZE_t pos;
    if (origin == DRWAV_SEEK_SET)      pos = offset;
    else if (origin == DRWAV_SEEK_CUR) pos = f_tell(fp) + offset;
    else return DRWAV_FALSE;
    return (f_lseek(fp, pos) == FR_OK) ? DRWAV_TRUE : DRWAV_FALSE;
}

static drwav_bool32 on_tell(void *pUserData, drwav_uint64 *pPos)
{
    FIL *fp = (FIL *)pUserData;
    *pPos = (drwav_uint64)f_tell(fp);
    return DRWAV_TRUE;
}

static int wav_dr_open(FIL *fp, void **state, decoder_info_t *info)
{
    decoder_pool_reset();
    wav_dr_state_t *s = (wav_dr_state_t *)decoder_pool_malloc(sizeof(wav_dr_state_t));
    if (!s) return DECODER_ERR_OPEN;

    f_rewind(fp);

    if (!drwav_init(&s->wav, on_read, on_seek, on_tell, fp, NULL)) {
        return DECODER_ERR_PARSE;
    }

    info->sample_rate    = s->wav.sampleRate;
    info->bits_per_sample = 16; /* dr_wav outputs f32, source depth doesn't matter downstream */
    info->num_channels   = (uint8_t)s->wav.channels;
    info->total_samples  = (uint32_t)s->wav.totalPCMFrameCount;
    info->duration_ms    = (uint32_t)((s->wav.totalPCMFrameCount / s->wav.sampleRate) * 1000);

    /* System only supports stereo, 44.1k / 48k / 96k and 16/24-bit */
    if (info->num_channels != 2 ||
        (info->sample_rate != 44100 && info->sample_rate != 48000 && info->sample_rate != 96000) ||
        (info->bits_per_sample != 16 && info->bits_per_sample != 24)) {
        drwav_uninit(&s->wav);
        return DECODER_ERR_PARSE;
    }

    *state = s;
    return DECODER_OK;
}

static int wav_dr_decode(void *state,
                          float *out, size_t capacity,
                          size_t *out_frames)
{
    wav_dr_state_t *s = (wav_dr_state_t *)state;

    drwav_uint64 frames = drwav_read_pcm_frames_f32(&s->wav, capacity / s->wav.channels, out);

    if (frames == 0) {
        *out_frames = 0;
        return DECODER_ERR_EOF;
    }

    *out_frames = (size_t)(frames * s->wav.channels);
    return DECODER_OK;
}

static void wav_dr_close(void **state)
{
    if (state && *state) {
        wav_dr_state_t *s = (wav_dr_state_t *)*state;
        drwav_uninit(&s->wav);
        *state = NULL;
    }
}

static int wav_dr_seek(void *state, uint32_t frame_index)
{
    wav_dr_state_t *s = (wav_dr_state_t *)state;
    return drwav_seek_to_pcm_frame(&s->wav, frame_index) ? DECODER_OK : DECODER_ERR_DECODE;
}

const decoder_t decoder_wav_dr = {
    .name   = "wav_dr",
    .open   = wav_dr_open,
    .decode = wav_dr_decode,
    .close  = wav_dr_close,
    .seek   = wav_dr_seek,
};
