/*
 * audio_DSP_compressor.c
 *
 *  Created on: 2025年1月18日
 *      Author: cpholzn
 */
#include "audio_DSP.h"
#include <stdint.h>
#include <arm_math.h>

const audio_compress_cfg_t audio_compress_cfg_default = {
        .attack_time_ms = 5,
        .release_time_ms = 5,
        .threshold_db = -15,
        .makeup_gain_db = 6,
        .ratio = 3,
        .knee_width_db = 3,
};
// 转换 dB 到线性值
static float db_to_linear(float db)
{
    return powf(10.0f, db / 20.0f);
}

// 转换线性值到 dB
static float linear_to_db(float linear)
{
    return 20.0f * log10f(linear);
}

void audio_compress_init(audio_compress_t *p, audio_compress_cfg_t *pcfg)
{
    if(pcfg)
        p->cfg = *pcfg;

    p->attack_coeff = expf(-1.0f / (p->cfg.attack_time_ms * AUDIO_DSP_SAMPLE_RATE_IN_MS));
    p->release_coeff = expf(-1.0f / (p->cfg.release_time_ms * AUDIO_DSP_SAMPLE_RATE_IN_MS));
    p->makeup_gain_lin = db_to_linear(p->cfg.makeup_gain_db);
    p->threshold_lin = db_to_linear(p->cfg.threshold_db);
    p->slope = 1.f - 1.f / p->cfg.ratio;
    p->knee_width_lin = db_to_linear(p->cfg.knee_width_db);
    p->history_rms = 0.f;
    p->history_gain = 1.f;
}

void audio_compress_modify_params(audio_compress_t *p, audio_compress_cfg_t *pnewcfg)
{
    if(&(p->cfg) != pnewcfg)
    {
        p->cfg = *pnewcfg;
    }
    __disable_irq();
    p->attack_coeff = expf(-1.0f / (p->cfg.attack_time_ms * AUDIO_DSP_SAMPLE_RATE_IN_MS));
    p->release_coeff = expf(-1.0f / (p->cfg.release_time_ms * AUDIO_DSP_SAMPLE_RATE_IN_MS));
    p->makeup_gain_lin = db_to_linear(p->cfg.makeup_gain_db);
    p->threshold_lin = db_to_linear(p->cfg.threshold_db);
    p->slope = 1.f - 1.f / p->cfg.ratio;
    p->knee_width_lin = db_to_linear(p->cfg.knee_width_db);
    __enable_irq();
}

// 计算信号的 RMS 幅度包络
static inline float  calculate_rms(float *input, size_t count)
{
    float squared_sum;
    arm_power_f32(input, count, &squared_sum);
    return sqrtf(squared_sum / count);
}

// 压缩函数
void audio_compress(audio_compress_t *p, float *output, float *input,  size_t count)
{
    float rms;
    rms = calculate_rms(input, count);
    float rms_db = linear_to_db(rms);
    p->history_rms = rms;
    float over_db = rms_db - p->cfg.threshold_db;
    float gain;
    if(over_db < -p->cfg.knee_width_db / 2 ) // no need for attenuation
    {
        gain = 1.f;
    }
    else if(over_db < p->cfg.knee_width_db / 2) // soft knee area before full attenuation
    {
        gain = 1.f - ((over_db + p->cfg.knee_width_db / 2) / p->cfg.knee_width_db) * p->slope;
    }
    else // full attenuation
    {
        gain = 1.f - p->slope;
    }
    float alpha = ((gain < p->history_gain) ? (p->attack_coeff) : (p->release_coeff));

    for(int i = 0; i < count; ++i)
    {
        p->history_gain = alpha * p->history_gain + (1.f - alpha) * gain;
        float compensated_gain = p->history_gain * p->makeup_gain_lin;
        *(output + i) = (*(input + i)) * compensated_gain;
//		arm_scale_f32(input, compensated_gain, output, count);
    }

}




