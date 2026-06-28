#pragma once

#include <stdint.h>
#include <stddef.h>
#include "ff.h"

#define DECODER_NAME_MAX 16

typedef enum {
    DECODER_OK = 0,
    DECODER_ERR_OPEN = -1,
    DECODER_ERR_PARSE = -2,
    DECODER_ERR_DECODE = -3,
    DECODER_ERR_EOF = -4,
    DECODER_ERR_UNSUPPORTED = -5,
} decoder_err_t;

typedef struct {
    uint32_t sample_rate;
    uint8_t  bits_per_sample;
    uint8_t  num_channels;
    uint32_t total_samples;   // 0 if unknown
    uint32_t duration_ms;     // 0 if unknown
} decoder_info_t;

typedef struct decoder_t {
    const char name[DECODER_NAME_MAX];

    // Open: parse stream info from fp (decoder reads via callbacks).
    // fp: file handle positioned at start.
    // state: output - allocated decoder state.
    // info: output - stream info.
    // Returns DECODER_OK on success.
    int (*open)(FIL *fp, void **state, decoder_info_t *info);

    // Decode: decode next PCM frames into interleaved float output.
    // out: float buffer, capacity: max float samples available.
    // out_frames: actual float samples produced (channels × frames).
    int (*decode)(void *state,
                  float *out, size_t capacity,
                  size_t *out_frames);

    // Close and free state.
    void (*close)(void **state);

    // Seek to a PCM frame (per-channel frame index).
    // Returns DECODER_OK on success.
    int (*seek)(void *state, uint32_t frame_index);
} decoder_t;

// Built-in backends
extern const decoder_t decoder_wav_dr;  // dr_wav streaming
extern const decoder_t decoder_flac;    // dr_flac streaming
extern const decoder_t decoder_mp3;     // dr_mp3 streaming
