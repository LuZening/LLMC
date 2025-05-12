/*
 * audio_DSP_EQ.c
 *
 *  Created on: 2025年1月18日
 *      Author: cpholzn
 */


#include "arm_math.h"
#include "audio_settings.h"
#include "audio_DSP.h"

// central frequency and q-value
const float32_t EQ_center_freqs[N_EQ_SEGMENTS] = {30.f, 180.f, 330.f, 600.f, 1000.f, 4000.0f, 12000.0f, 16000.0f};
const float32_t EQ_q = 1.0f / sqrtf(2.0f);
const audio_EQ_cfg_t audio_EQ_cfg_default = {
		.gains_db = {0,},
};

static void calculate_biquad_coeffs(float32_t *coeffs, float32_t center_freq, float32_t Q, float32_t gain_db, float32_t sample_rate, int idx_band)
{
	// 参数保护
    center_freq = fmaxf(fminf(center_freq, 0.49f * sample_rate), 20.0f);
    Q = fmaxf(Q, 0.1f);

    float32_t omega = 2.0f * PI * center_freq / sample_rate;
    float32_t sin_omega = sinf(omega);
    float32_t cos_omega = cosf(omega);
    float32_t A = powf(10.0f, gain_db / 40.0f);
    if(idx_band == 0) // low shelf EQ
    {
		float32_t alpha = sin_omega * sqrtf(A) / 2.f;
		// 峰值均衡器
		coeffs[idx_band * 5 + 0] = A * ( (A+1) - (A-1)*cos_omega + 2.f*sqrtf(A)*alpha );
		coeffs[idx_band * 5 + 1] = 2.f*A * ( (A-1) - (A+1)*cos_omega );
		coeffs[idx_band * 5 + 2] = A * ( (A+1) - (A-1)*cos_omega - 2.f*sqrtf(A)*alpha );
		coeffs[idx_band * 5 + 3] = 2.f * ( (A-1) + (A+1)*cos_omega );// -a1
		coeffs[idx_band * 5 + 4] = -((A+1) + (A-1)*cos_omega - 2*sqrtf(A)*alpha); // -a2

		float32_t gain = 1.0f / ((A+1) + (A-1)*cos_omega + 2*sqrtf(A)*alpha);
		coeffs[idx_band * 5 + 0] *= gain;
		coeffs[idx_band * 5 + 1] *= gain;
		coeffs[idx_band * 5 + 2] *= gain;
		coeffs[idx_band * 5 + 3] *= gain;
		coeffs[idx_band * 5 + 4] *= gain;
    }
    else if(idx_band == N_EQ_SEGMENTS-1) // high shelf EQ
    {
		float32_t alpha = sin_omega / (2.f * sqrtf(A));
		// 峰值均衡器
		coeffs[idx_band * 5 + 0] = A * ( (A+1) + (A-1)*cos_omega + 2.f*sqrtf(A)*alpha );
		coeffs[idx_band * 5 + 1] = -2.f*A * ( (A-1) + (A+1)*cos_omega );
		coeffs[idx_band * 5 + 2] = A * ( (A+1) + (A-1)*cos_omega - 2.f*sqrtf(A)*alpha );
		coeffs[idx_band * 5 + 3] = -2.f * ( (A-1) - (A+1)*cos_omega );// -a1
		coeffs[idx_band * 5 + 4] = -(A+1) + (A-1)*cos_omega + 2.f*sqrtf(A)*alpha; // -a2

		float32_t gain = 1.0f / ((A+1) - (A-1)*cos_omega + 2.f*sqrtf(A)*alpha);
		coeffs[idx_band * 5 + 0] *= gain;
		coeffs[idx_band * 5 + 1] *= gain;
		coeffs[idx_band * 5 + 2] *= gain;
		coeffs[idx_band * 5 + 3] *= gain;
		coeffs[idx_band * 5 + 4] *= gain;
    }
    else // peak EQ
    {
		float32_t alpha = sin_omega / (2.0f * Q);
		// 峰值均衡器
		coeffs[idx_band * 5 + 0] = 1.0f + alpha * A;
		coeffs[idx_band * 5 + 1] = -2.0f * cos_omega;
		coeffs[idx_band * 5 + 2] = 1.0f - alpha * A;
		coeffs[idx_band * 5 + 3] = 2.0f * cos_omega;// -a1
		coeffs[idx_band * 5 + 4] = -(1.0f - alpha / A); // -a2

		float32_t gain = 1.0f / (1.0f + alpha / A);
		coeffs[idx_band * 5 + 0] *= gain;
		coeffs[idx_band * 5 + 1] *= gain;
		coeffs[idx_band * 5 + 2] *= gain;
		coeffs[idx_band * 5 + 3] *= gain;
		coeffs[idx_band * 5 + 4] *= gain;
    }
}


void audio_EQ_modify_gain(audio_EQ_t *p, uint8_t idx_band, float gain)
{
	p->cfg.gains_db[idx_band] = gain;
	// update coeff values in the filter
	// turn off interupt when updaing the coefficient
	// to avoid data race with data absorbing interrupts
	__disable_irq();
	calculate_biquad_coeffs(p->coeffs, EQ_center_freqs[idx_band], EQ_q, p->cfg.gains_db[idx_band], AUDIO_DSP_SAMPLE_RATE, idx_band);
	arm_biquad_cascade_df1_init_f32(&(p->biquad_chain), N_EQ_SEGMENTS, p->coeffs, p->buf_biquad_state);
	__enable_irq();
}


void audio_EQ_init(audio_EQ_t *p, audio_EQ_cfg_t *pcfg)
{
	if(pcfg)
		p->cfg = *pcfg;
	else
		memset(&p->cfg, 0 ,sizeof(p->cfg));
	for(int i = 0; i < N_EQ_SEGMENTS; ++i)
	{
		calculate_biquad_coeffs(p->coeffs, EQ_center_freqs[i], EQ_q, p->cfg.gains_db[i], AUDIO_DSP_SAMPLE_RATE, i);
	}
	arm_biquad_cascade_df1_init_f32(&(p->biquad_chain), N_EQ_SEGMENTS, p->coeffs, p->buf_biquad_state);
}


void audio_EQ(audio_EQ_t *p, float *output, const float* input, size_t cnt )
{
    // take a copy of input for modification
    arm_biquad_cascade_df1_f32(&(p->biquad_chain), input, output, cnt);
}
