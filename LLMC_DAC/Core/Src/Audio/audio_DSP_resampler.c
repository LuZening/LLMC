/*
 * audio_DSP_resampler.c
 *
 *  Created on: 2025年3月6日
 *      Author: cpholzn
 */


#include <stdint.h>




#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "audio_DSP.h"

// 计算最大公约数（GCD）
static uint32_t gcd(uint32_t a, uint32_t b) {
    while (b != 0) {
        uint32_t temp = a % b;
        a = b;
        b = temp;
    }
    return a;
}

// 计算最小公倍数（LCM）
static uint32_t lcm(uint32_t a, uint32_t b) {
    // 处理零的情况
    if (a == 0 || b == 0) {
        return 0; // 数学上未定义，但通常返回0
    }

    // 计算绝对值的乘积，避免溢出风险
    uint32_t gcd_val = gcd(a, b);
    return (a / gcd_val) * b; // 分步计算避免溢出
}


// 修正零阶贝塞尔函数I0(x)的近似计算
static float bessel_i0(float x) {
    float ax = fabsf(x);
    if (ax < 3.75f) {
        // 小参数多项式近似 (|x| < 3.75)
        float t = x / 3.75f;
        t *= t;
        return 1.0f + t*(3.5156229f + t*(3.0899424f +
               t*(1.2067492f + t*(0.2659732f +
               t*(0.0360768f + t*0.0045813f)))));
    } else {
        // 大参数近似 (|x| >= 3.75)
        float y = 3.75f / ax;
        float t = y*y;
        float ans = 0.39894228f + t*(0.01328592f + t*(0.00225319f +
                    t*(-0.00157565f + t*(0.00916281f + t*(-0.02057706f +
                    t*(0.02635537f + t*(-0.01647633f + t*0.00392377f)))))));
        ans *= expf(ax) / sqrtf(ax);
        return ans;
    }
}

// 修正后的Kaiser窗生成函数
static void kaiser_window(float* window, int n, float beta) {
    float i0_beta = bessel_i0(beta); // 预计算分母项
    float inv_i0_beta = 1.0f / i0_beta;

    for(int i=0; i<n; i++) {
        // 计算归一化位置 [-1,1]
        float x = (2.0f*i)/(n-1) - 1.0f;
        // 计算窗函数值
        float t = sqrtf(1.0f - x*x);
        window[i] = bessel_i0(beta * t) * inv_i0_beta;
    }
}

// 初始化重采样器
void audio_sinc_resampler_init(audio_sinc_resampler_t* rs, float* coeffs, uint32_t src_rate, uint32_t target_rate, uint8_t nch)
{

    // 计算采样率参数
    rs->src_rate = src_rate;
    rs->target_rate = target_rate;
    rs->grand_LCM = lcm(src_rate, target_rate); // 预计算的44.1k和48k的LCM

    rs->input_phase_step = rs->grand_LCM / src_rate;    // 160
    rs->output_phase_step = rs->grand_LCM / target_rate; // 147

    rs->coeffs = coeffs; // look up table


    // init the phase pose
    rs->phase_pos_int = 0;
    rs->phase_pos_frac = 0;
    rs->coeffs_row_idx = 0;

    rs->nch = nch;
/*
    // 分配多相滤波器系数内存
    int phases = rs->input_phase_step;
    // 生成加窗sinc系数
    float beta = 10.0f;
    for(int phase=0; phase<phases; phase++) {
        float frac_delay = (float)phase / phases;

        // 生成时间序列
        float t[N_SINC_TAPS];
        for(int i=0; i<N_SINC_TAPS; i++) {
            float offset = (i - N_SINC_TAPS/2 + 1) - frac_delay;
            t[i] = offset * (float)rs->input_phase_step / rs->output_phase_step;
        }

        // 计算sinc函数
//        float cutoff = 21000.0f * 2.0f / target_rate;
        float cutoff = 1.f;
        float sum = 0.0f;
        for(int i=0; i<N_SINC_TAPS; i++) {
            float x = cutoff * t[i] * PI; // pi x
            rs->coeffs[phase*N_SINC_TAPS + i] = (x == 0) ? 1.0f : sinf(x)/x;
            sum += rs->coeffs[phase*N_SINC_TAPS + i];
        }

        // 应用Kaiser窗并归一化
        float window[N_SINC_TAPS];
        kaiser_window(window, N_SINC_TAPS, beta);
        sum = 0.0f;
        for(int i=0; i<N_SINC_TAPS; i++) {
            rs->coeffs[phase*N_SINC_TAPS + i] *= window[i];
            sum += rs->coeffs[phase*N_SINC_TAPS + i];
        }
//        for(int i=0; i<N_SINC_TAPS; i++) {
//            rs->coeffs[phase*N_SINC_TAPS + i] /= sum;
//        }
        arm_scale_f32(rs->coeffs, 1./sum,rs->coeffs,N_SINC_TAPS); // unify
    }
*/


}

// 处理音频块
void audio_sinc_resampler_process(audio_sinc_resampler_t* rs,
                      float* output,
                      const float* input, size_t in_cnt_each_ch,
                      /* outs */
                      size_t* out_generated, size_t* in_consumed)
{
//    int phases = rs->input_phase_step;
    size_t total_avail = HISTORY_COUNT + in_cnt_each_ch;

    // each channel
    for(int c = 0; c < rs->nch; c++)
    {
        // build history buffer
        float *history = rs->histories[c];
        if((rs->phase_pos_int  == 0) && (rs->phase_pos_frac == 0)) // very fisrt input, fill history with 0
        {
            memset(history, 0, HISTORY_COUNT * sizeof(float));
            // move the phase pose - make sure the very 1st input sample point aligned with the middle of the SINC scope
            rs->phase_pos_int = HISTORY_COUNT;
            rs->phase_pos_frac = 0;
        }
        // the history buffer is made of HISTORY_COUNT of history data and N_SINC_TAPS of new data
        memcpy(history + HISTORY_COUNT, input + c * in_cnt_each_ch, MIN(N_SINC_TAPS, in_cnt_each_ch) * sizeof(float)); // copy deinterlaced input by channel
    }


    int gen = 0;
    int input_pos = 0, phase, left, right = 0;
    uint32_t phase_pos_0 = rs->phase_pos_int * rs->input_phase_step + rs->phase_pos_frac; // the inital full phase position
    uint32_t expected_out_len = ((uint32_t)rs->input_phase_step * (total_avail - N_SINC_TAPS_HALF) - 1 - phase_pos_0) / rs->output_phase_step + 1;

    while(gen < expected_out_len)
    {
    	input_pos = rs->phase_pos_int;
        left = input_pos - N_SINC_TAPS_HALF + 1; // left most

        // 检查边界
        if((left < 0 ) || ((left + N_SINC_TAPS) > total_avail) )
        	break;
        right = left + N_SINC_TAPS;
//        const float* coeff = rs->coeffs + rs->phase_pos_frac * N_SINC_TAPS;
        float* coeff = rs->coeffs + rs->coeffs_row_idx * N_SINC_TAPS;
//        for(int i=0; i<N_SINC_TAPS; i++) {
//            sum += buffer[base + i] * coeff[i];
//        }
        for(int c= 0; c<rs->nch;++c)
        {
        	// find the input view
        	const float* in_view;
            if(left < HISTORY_COUNT) // all data needed is in the history buffer, use data stored in the history buffer
                in_view = rs->histories[c] + left;
            else // all data needed is in the input buffer
                in_view = input + c * in_cnt_each_ch + left - HISTORY_COUNT;

            // 卷积计算
            float sum;
        	arm_dot_prod_f32(in_view, coeff, N_SINC_TAPS, &sum);
            output[c * expected_out_len + gen] = sum; // output length may be smaller than out_max of each channel, so this operation will leave some bubble between channels
        }
        gen++;

        rs->phase_pos_frac += rs->output_phase_step;
        if(rs->phase_pos_frac >= rs->input_phase_step)
        {
        	rs->phase_pos_frac -= rs->input_phase_step;
        	rs->phase_pos_int++;
        }
        rs->coeffs_row_idx++;
        if(rs->coeffs_row_idx >= rs->input_phase_step)
        	rs->coeffs_row_idx = 0;
    }

    // update the history buffer
    int input_used = right - HISTORY_COUNT; // excluding history, only counting the usage of the newly input data. input_used should be equal to in_len
    if(input_used > 0) {
        int rollback = input_used;
        rs->phase_pos_int -= rollback;
        for(int c = 0; c < rs->nch; ++c)
        {
            float* history = rs->histories[c];
            // replace history data with new input data
            memcpy(history, input + c * in_cnt_each_ch + input_used - HISTORY_COUNT,  HISTORY_COUNT * sizeof(float));
        }
    }

    *in_consumed = input_used;
    *out_generated = gen;
}

