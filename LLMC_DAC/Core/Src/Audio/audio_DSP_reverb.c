/*
 * audio_DSP_reverb.c
 *
 *  Created on: 2025年1月18日
 *      Author: cpholzn
 */

#include "arm_math.h"
#include "audio_settings.h"
#include "audio_DSP.h"

const audio_reverb_cfg_t audio_reverb_cfg_default = {
        .predelay_ms = 50,
        .decay_ms = 100,
        .feedback = 0.5,
};

void audio_reverb_init(audio_reverb_t *p, audio_reverb_cfg_t *pcfg)
{
    if(pcfg)
        p->cfg = *pcfg;
    if(p->cfg.predelay_ms > REVERB_PREDELAY_MS_MAX)
        p->cfg.predelay_ms = REVERB_PREDELAY_MS_MAX;
    if(p->cfg.predelay_ms < 1)
        p->cfg.predelay_ms = 1;
    p->num_predelay_samples = p->cfg.predelay_ms * AUDIO_DSP_SAMPLE_RATE_IN_MS;
    p->decay_factor = powf(0.001f, 1.f / (p->cfg.decay_ms * AUDIO_DSP_SAMPLE_RATE_IN_MS));

    memset(p->buf_comb, 0, sizeof(p->buf_comb));
    p->idx_comb = 0;
    memset(p->buf_allpass, 0, sizeof(p->buf_allpass));
    p->idx_allpass = 0;
}

void audio_reverb_modify_params(audio_reverb_t *p, audio_reverb_cfg_t* pnewcfg)
{
    __disable_irq();
    if(&(p->cfg) != pnewcfg)
    {
        p->cfg = *pnewcfg;
    }
    if(p->cfg.predelay_ms > REVERB_PREDELAY_MS_MAX)
            p->cfg.predelay_ms = REVERB_PREDELAY_MS_MAX;
    if(p->cfg.predelay_ms < 1)
        p->cfg.predelay_ms = 1;
    p->num_predelay_samples = p->cfg.predelay_ms * AUDIO_DSP_SAMPLE_RATE_IN_MS;
    p->decay_factor = powf(0.001f, 1.f / (p->cfg.decay_ms * AUDIO_DSP_SAMPLE_RATE_IN_MS));
    __enable_irq();
}

// process
void audio_reverb(audio_reverb_t *p, float32_t *output, const float32_t *input, uint32_t cnt)
{
    for (int i = 0; i < cnt; i++)
    {
    	/* comb filter */
        float x = input[i];
        float y =  p->buf_comb[p->idx_comb];
        p->buf_comb[p->idx_comb] = x + p->decay_factor * y;
        p->idx_comb = (p->idx_comb+ 1) % p->num_predelay_samples;

        x = y;
        /* all pass filter */
        float tmp = p->buf_allpass[p->idx_allpass];
        y = -x * p->cfg.feedback + tmp;
        p->buf_allpass[p->idx_allpass] = x + tmp *p->cfg.feedback;
        p->idx_allpass = (p->idx_allpass + 1) % p->num_predelay_samples; // TODO: may use different delay length

        output[i] = y;
    }
}
