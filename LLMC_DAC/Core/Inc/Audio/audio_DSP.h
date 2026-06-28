/*
 * audio_DSP.h
 *
 *  Created on: Apr 6, 2024
 *      Author: cpholzn
 */
#pragma once
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#if (!defined(LVGL_SIM) & !defined(SIM))
#include "stm32h7b0xx.h"
#include "arm_math.h"
#include "audio_settings.h"
#else
typedef float float32_t;
#endif
#ifndef MIN
#define MIN(a, b)  (((a) < (b)) ? (a) : (b))
#endif
#define AUDIO_DSP_SAMPLE_RATE (DEFAULT_SAMPLE_RATE_HZ)
#define AUDIO_DSP_SAMPLE_RATE_IN_MS (DEFAULT_SAMPLE_RATE_HZ / 1000U)

#define N_EQ_SEGMENTS 8U
#define N_FIR_TAPS 128U
#define BLOCK_SIZE_FIR 128U
#define COUNT_FIR_BUF (N_FIR_TAPS + BLOCK_SIZE_FIR - 1)
#define HISTORY_COUNT (N_SINC_TAPS - 1)
#define RESAMPLER_BLOCK_SIZE (48U * 10U)
#define N_SINC_TAPS_HALF 31U
#define N_SINC_TAPS (N_SINC_TAPS_HALF * 2)
#define LCM_441_480 (147U*160U)
#define N_441_480_INPUT_FRACSTEPS 160U // 44.1K sample rate in
#define N_441_480_OUTPUT_FRACSTEPS 147U // 48K sample rate out
#define N_441_480_PHASE_MODES 	N_441_480_INPUT_FRACSTEPS // has 160 different phase modes
#define PATH_DSP_PROFILE_FOLDER L"0:DSP"
#define FILENAME_DSP_PROFILE L"dsp_profile_" // append a number to this filename

extern float audio_decimate_coeffs_96K_to_48K[N_FIR_TAPS];
extern float *audio_interpolate_coeffs_48K_to_96K;
extern float audio_sinc_coeffs_44K_to_48K[160 * 62];
//#define USE_FIXED_POINT_DSP

#ifndef USE_FIXED_POINT_DSP
    typedef float dsp_value_t;
#else
    typedef int32_t dsp_value_t;
#endif

#if (!defined(LVGL_SIM) & !defined(SIM))
typedef struct
{
    uint16_t max_count_for_single_process;
    // filter instance
    arm_fir_decimate_instance_f32 filter;

} audio_decimator_t;

typedef struct
{

    uint16_t max_count_for_single_process;
    // filter instance
    arm_fir_interpolate_instance_f32 filter;

} audio_interpolator_t;


typedef struct {
    uint8_t nch; // number of channels
    uint32_t src_rate;
    uint32_t target_rate;
    uint32_t grand_LCM;
    uint16_t input_phase_step;
    uint16_t output_phase_step;
    int phase_pos_int; // the integer part of the index marked to the input samples
    uint16_t phase_pos_frac; // the fraction part
    uint16_t coeffs_row_idx; // the incremental index of the phase fraction, so the SINC table can be visted in a continous order, which gives better cache efficieny
    float* coeffs;          // 多相滤波器系数 [input_phase_step][SINC_TAPS]
    float histories[2][HISTORY_COUNT + N_SINC_TAPS]; // stores histroy + new input data with the same length as sinc tap
} audio_sinc_resampler_t;

void audio_sinc_resampler_init(audio_sinc_resampler_t* rs, float* coeffs, uint32_t src_rate, uint32_t target_rate, uint8_t nch);
void audio_sinc_resampler_process(audio_sinc_resampler_t* rs,
                      float* output,
                      const float* input, size_t in_len_each_ch,
                      /* outs */
                      size_t* out_generated_each_ch, size_t* in_consumed_each_ch);

typedef struct
{
    float attack_time_ms;
    float release_time_ms;
    float threshold_db;
    float makeup_gain_db;
    float ratio;
    float knee_width_db;
} audio_compress_cfg_t;


extern const audio_compress_cfg_t audio_compress_cfg_default;
typedef struct
{
    /* external */
    audio_compress_cfg_t cfg;
    /* internal */
    float attack_coeff;
    float release_coeff;
    float threshold_lin; // linear
    float makeup_gain_lin; // linear
    float knee_width_lin;
    float slope; // from ratio
    /* states */
    float history_rms;
    float history_gain;
} audio_compress_t;


void audio_compress_init(audio_compress_t *p, audio_compress_cfg_t *pcfg);
void audio_compress(audio_compress_t *p, float *output, float *input, size_t count);
void audio_compress_modify_params(audio_compress_t *p, audio_compress_cfg_t *pnewcfg);

// central frequency and q-value
extern const float32_t EQ_center_freqs[N_EQ_SEGMENTS];
extern const float32_t EQ_q;

typedef struct
{
    float gains_db[N_EQ_SEGMENTS];
} audio_EQ_cfg_t;

extern const audio_EQ_cfg_t audio_EQ_cfg_default;

typedef struct
{
    audio_EQ_cfg_t cfg;
    // biquad IIR instances
    arm_biquad_casd_df1_inst_f32 biquad_chain;
    float buf_biquad_state[N_EQ_SEGMENTS * 4];
    // each biquad unit requires 5 coefficients
    float coeffs[N_EQ_SEGMENTS * 5];
} audio_EQ_t;

void audio_EQ_init(audio_EQ_t *p, audio_EQ_cfg_t *pcfg);
void audio_EQ_modify_gain(audio_EQ_t *p, uint8_t idx_band, float gain);
void audio_EQ(audio_EQ_t *p, float *output, const float* input, size_t cnt );


#define REVERB_PREDELAY_LENGTH (4U * 1024U) // NUMBER OF FLOATS 16KB
#define REVERB_PREDELAY_MOD(x) ((x) & (REVERB_PREDELAY_LENGTH - 1U)) // calculate mod
#define REVERB_PREDELAY_MS_MAX (REVERB_PREDELAY_LENGTH / (AUDIO_DSP_SAMPLE_RATE_IN_MS)) // 40ms for 16KB, 85ms for 32KB

typedef struct
{
    /* comb filter */
    float predelay_ms;
    float decay_ms;
    /* allpass filter */
    float feedback; // -0.9 < x < 0.9
} audio_reverb_cfg_t;

extern const audio_reverb_cfg_t audio_reverb_cfg_default;

typedef struct
{
    audio_reverb_cfg_t cfg;

    /* internal */
    float decay_factor;
    uint32_t num_predelay_samples;
    /* states */
    float buf_comb[REVERB_PREDELAY_LENGTH]; // 50ms ~19kb each ch @96K FLOAT
    float buf_allpass[REVERB_PREDELAY_LENGTH]; // same
    uint32_t idx_comb;
    uint32_t idx_allpass;
} audio_reverb_t;

void audio_reverb_init(audio_reverb_t *p, audio_reverb_cfg_t *pcfg);
void audio_reverb_modify_params(audio_reverb_t *p, audio_reverb_cfg_t* pnewcfg);
void audio_reverb(audio_reverb_t *p, float32_t *output, const float32_t *input, uint32_t cnt);



#define DSP_FUNCTION_BIT_COMPRESS 1U
#define DSP_FUNCTION_BIT_EQ (1U << 1U)
#define DSP_FUNCTION_BIT_REVERB (1U << 2U)

typedef struct
{
    audio_compress_cfg_t cfg_compress;
    audio_EQ_cfg_t cfg_EQ;
    audio_reverb_cfg_t cfg_reverb;
    uint32_t function_bits;
} audio_DSP_cfg_t;

typedef struct
{
    audio_compress_t compress;
    audio_EQ_t eq;
    audio_reverb_t reverb;
    uint32_t function_bits;
} audio_DSP_t;

void audio_DSP_init(audio_DSP_t *p,
        audio_DSP_cfg_t *pcfg);
void audio_DSP_extrct_cfg(audio_DSP_t *p, /*output*/audio_DSP_cfg_t *pcfg);
void audio_DSP_set_function_bit(audio_DSP_t* p, uint32_t funcbit, uint8_t enable);
void audio_DSP_apply(audio_DSP_t* p, dsp_value_t* output, dsp_value_t* input, size_t cnt);
int save_DSP_preset(audio_DSP_t* pDSP, int idxPreset);
int load_and_apply_DSP_preset(audio_DSP_t* pDSP, int idxPreset);

#define AUDIO_LEVEL_ALPHA 0.01f
#define AUDIO_LEVEL_PEAK_ALPHA 0.00005f
typedef struct
{
    float peak;
    float value;
} audio_level_tracker_t;
void audio_level_track(audio_level_tracker_t *p, float level);

void audio_DSP_deinterlace_channels( dsp_value_t * __restrict pDst, dsp_value_t* __restrict pSrc, size_t countAllChs, uint8_t nCh);
void audio_DSP_interlace_channels( dsp_value_t* __restrict pDst, dsp_value_t* __restrict pSrc, size_t countAllChs, uint8_t nCh);
void audio_decimator_init(audio_decimator_t* p, uint8_t downsample, dsp_value_t *bufState, uint16_t count_for_single_process);
size_t audio_decimator_filter_apply(audio_decimator_t* arrDecimators, dsp_value_t* pDst,  dsp_value_t* pSrc, uint16_t count, uint8_t nCh);
void audio_interplolator_init(audio_interpolator_t* p, uint8_t upsample, dsp_value_t *bufState, uint16_t max_count_for_single_process);
size_t audio_interpolator_filter_apply(audio_interpolator_t* arrInterpolators, dsp_value_t* __restrict pDst,  dsp_value_t* __restrict pSrc, uint16_t countAllChs, uint8_t nCh);
#endif
// simple repeat to upsample
size_t audio_interpolator_simple(uint8_t L, dsp_value_t* __restrict pDst,  dsp_value_t* __restrict pSrc, uint16_t countAllChs, uint8_t nCh);



size_t pad_i16_to_i32(int32_t* __restrict pDst, const int16_t* __restrict pSrc, size_t bytes, uint8_t oversample, uint8_t nch);
size_t pad_i24_to_i32(int32_t* __restrict pDst, const uint8_t* __restrict pSrc, size_t bytes, uint8_t oversample, uint8_t nch);

size_t oversample_and_left_shift_i32(int32_t* __restrict pDst,  int32_t* __restrict pSrc,
        size_t countAllCh, uint8_t oversample, uint8_t nch, uint8_t lshift_bits);
void convert_i24_to_float(float32_t* __restrict pDst,  uint8_t* __restrict pSrc, size_t len);
// INT_MAX -> 1.f, INT_MIN -> -1.f  pSrc inplace conversion OK
void convert_i16_to_float(float32_t* __restrict pDst,  int16_t* __restrict pSrc, size_t cntSampes);
void convert_i32_to_float(float32_t* pDst,   int32_t* pSrc, size_t cntSampes);

// note pSrc will be overwritten
size_t convert_float_to_i16(int16_t* pDst, float32_t* pSrc, size_t lenSrc, uint8_t downsample,uint8_t nch);
// note pSrc will be overwritten
size_t convert_float_to_i24(uint8_t* pDst, float32_t* pSrc, size_t lenSrc, uint8_t downsample, uint8_t nch);
void convert_float_to_i32(int32_t* pDst,  float32_t* pSrc, size_t lenSrc);


void audio_DSP_test();
