/*
 * audio_settings.h
 *
 *  Created on: 2024年4月16日
 *      Author: cpholzn
 */

#pragma once
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "PCM179x.h"
#include "TLV320ADCx120.h"
#include "profiler.h"


//extern unsigned int audio_sample_rate_SAI; //unit Hz, e.g. 96000U
typedef enum
{
    AUDIO_SAMPLE_RATE_44K1 = 0,
    AUDIO_SAMPLE_RATE_48K,
    AUDIO_SAMPLE_RATE_96K,
} audio_sample_rate_t;

#define N_VALID_AUDIO_SAMPLE_RATES 3U
extern const uint32_t arr_valid_audio_sample_rates_Hz[N_VALID_AUDIO_SAMPLE_RATES];
#define N_VALID_AUDIO_BIT_DEPTHS 2U
extern const uint8_t arr_valid_audio_bit_depths[N_VALID_AUDIO_BIT_DEPTHS];


#define MAX_BIT_DEPTH 24U
#define MAX_CHANNELS 2U
#define IDX_CHANNEL_MIC1 2 // MIC1 connected to CH2(but placed to L in ADCX120 init)
#define IDX_CHANNEL_MIC2 1 // MIC2 connected to CH1(but placed to R in ADCX120 init)
/* Internal system Fs and Bitdepth */
#define DEFAULT_SAMPLE_RATE_HZ 96000U
#define DEFAULT_SAMPLE_RATE_T  AUDIO_SAMPLE_RATE_96K
#define DEFAULT_BIT_DEPTH 24U

#define MAX_SINGLE_SAMPLE_BYTES (DEFAULT_BIT_DEPTH / 8U * MAX_CHANNELS) // 6 bytes per sample

float volume100_to_volumef(uint8_t volume100);
uint8_t volume100_linear_to_log(uint8_t volume100);
uint8_t volume100_to_255(uint8_t volume100);
void set_mic_LEDs(uint8_t i, uint8_t on);
// convert audio_sample_rate_t to numeric value
uint32_t audio_sample_rate_to_Hz(audio_sample_rate_t t);
audio_sample_rate_t audio_sample_rate_from_Hz(uint32_t Hz);
// Init Order 0:
void SAI_decoder_init(uint32_t audio_sample_rate_Hz_SAI);
void SAI_encoder_init(uint32_t audio_sample_rate_Hz_SAI);
// Init Order 1:
void set_audio_sample_rate_and_bit_depth(uint8_t idx, audio_sample_rate_t rate_new, uint8_t bitdepth_new);
// prepare SAI transmissions
void audio_connections_apply(uint16_t *audio_connections);
// activate dry/wet sound distribution to outputs
// output_dsp_cnnection[N_OUTPUTS]
void audio_outpus_dsp_connection_apply(const uint16_t *output_dsp_cnnection);

/* profilers */
extern profiler_counter_t profSAITx;
extern profiler_counter_t profUSB_SOF;
extern profiler_counter_t profUSB_feedback;
extern profiler_counter_t profUSB_MicIn;
