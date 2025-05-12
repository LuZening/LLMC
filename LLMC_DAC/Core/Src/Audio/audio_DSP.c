/*
 * audio_DSP.c
 *
 *  Created on: Apr 6, 2024
 *      Author: cpholzn
 */


#include<string.h>

#include "Audio/audio_DSP.h"




#ifndef MAX
#define MAX(a, b)  (((a) > (b)) ? (a) : (b))
#endif

#if (!defined(LVGL_SIM) & !defined(SIM))
#include "Audio/audio_settings.h"
#endif

#if (!defined(LVGL_SIM) & !defined(SIM))


void audio_decimator_init(audio_decimator_t* p, uint8_t downsample, dsp_value_t *bufState, uint16_t max_count_for_single_process)
{
	float32_t* pCoeffs;
	p->max_count_for_single_process = max_count_for_single_process;
	switch(downsample)
	{
	case 2:
		pCoeffs = audio_decimate_coeffs_96K_to_48K;
		break;
	default:
		pCoeffs = audio_decimate_coeffs_96K_to_48K;
	}
#ifndef USE_FIXED_POINT_DSP
	arm_fir_decimate_init_f32(&p->filter, N_FIR_TAPS, downsample, pCoeffs, bufState, max_count_for_single_process);
#else
	arm_fir_decimate_init_q31(&p->filter, N_FIR_TAPS, downsample, pCoeffs, bufState, max_count_for_single_process);
#endif
}



size_t audio_decimator_filter_apply(audio_decimator_t* arrDecimators, dsp_value_t* pDst,  dsp_value_t* pSrc, uint16_t countAllChs, uint8_t nCh)
{
//	assert(count > arrDecimators->max_count_for_single_process);

	dsp_value_t *end = pSrc + countAllChs;
	uint16_t countEachCh = countAllChs / nCh;
	uint16_t countEachCh_down = countEachCh / arrDecimators->filter.M;
	/* convert by channel */
	for(int i = 0; i < nCh; ++i)
	{
		uint16_t blockSize = arrDecimators[i].max_count_for_single_process;
		uint16_t blockSize_down = blockSize / arrDecimators->filter.M;
		int countRemian = countEachCh;
		dsp_value_t *pChSrcBuf_begin = pSrc + i * countEachCh;
		dsp_value_t *pChDstBuf_begin = pDst + i * countEachCh_down;
		while(countRemian > 0)
		{
			// NOTE: inplace filtering
#ifndef USE_FIXED_POINT_DSP
			arm_fir_decimate_f32(&(arrDecimators[i].filter), pChSrcBuf_begin, pChDstBuf_begin, MIN(blockSize, countRemian));
#else
			arm_fir_decimate_q31(&(arrDecimators[i].filter), pChSrcBuf_begin, pChDstBuf_begin, MIN(blockSize, countRemian));
#endif
			pChSrcBuf_begin += blockSize;
			pChDstBuf_begin += blockSize_down;
			countRemian -= blockSize;
		}
	}
	return countEachCh_down * nCh;
}


void audio_interplolator_init(audio_interpolator_t* p, uint8_t upsample, dsp_value_t *bufState, uint16_t max_count_for_single_process)
{
	float32_t* pCoeffs;
	p->max_count_for_single_process = max_count_for_single_process;
	switch(upsample)
	{
	case 2:
		pCoeffs = audio_interpolate_coeffs_48K_to_96K;
		break;
	default:
		pCoeffs = audio_interpolate_coeffs_48K_to_96K;
	}
#ifndef USE_FIXED_POINT_DSP
	arm_fir_interpolate_init_f32(&p->filter, upsample, N_FIR_TAPS, pCoeffs, bufState, max_count_for_single_process);
#else
	arm_fir_interpolate_init_q31(&p->filter, upsample, N_FIR_TAPS, pCoeffs, bufState, max_count_for_single_process);
#endif
}



size_t audio_interpolator_filter_apply(audio_interpolator_t* arrInterpolators, dsp_value_t* __restrict pDst,  dsp_value_t* __restrict pSrc, uint16_t countAllChs, uint8_t nCh)
{
//	assert(count > arrDecimators->max_count_for_single_process);
	float32_t *end = pSrc + countAllChs;
	uint16_t countEachCh = countAllChs / nCh;
	uint16_t countEachCh_up = countEachCh * arrInterpolators->filter.L;

	/* convert by channel */
	for(int i = 0; i < nCh; ++i)
	{
		uint16_t blockSize = arrInterpolators[i].max_count_for_single_process;
		uint16_t blockSize_up = blockSize *arrInterpolators->filter.L;
		int countRemian = countEachCh;
		dsp_value_t *pChSrcBuf_begin = pSrc + i * countEachCh;
		dsp_value_t *pChDstBuf_begin = pDst + i * countEachCh_up;
		while(countRemian > 0)
		{
			// NOTE: inplace filtering
#ifndef USE_FIXED_POINT_DSP
			arm_fir_interpolate_f32(&(arrInterpolators[i].filter), pChSrcBuf_begin, pChDstBuf_begin, MIN(blockSize, countRemian));
#else
			arm_fir_interpolate_q31(&(arrInterpolators[i].filter), pChSrcBuf_begin, pChDstBuf_begin, MIN(blockSize, countRemian));
#endif
			pChSrcBuf_begin += blockSize;
			pChDstBuf_begin += blockSize_up;
			countRemian -= blockSize;
		}
	}
	return countEachCh_up * nCh;
}







void audio_DSP_deinterlace_channels( dsp_value_t* __restrict pDst, dsp_value_t* __restrict pSrc, size_t countAllChs, uint8_t nCh)
{
	int i;

	size_t countEachCh = countAllChs / nCh;
	dsp_value_t *end = pSrc + countAllChs;
	dsp_value_t *out = pDst;
	for(i = 0; i < nCh; ++i)
	{
		dsp_value_t *cur = pSrc + i;
		while(cur < end)
		{
			*out = *cur;
			cur+=nCh;
			out++;
		}
	}
}


void audio_DSP_interlace_channels( dsp_value_t* __restrict pDst, dsp_value_t* __restrict pSrc, size_t countAllChs, uint8_t nCh)
{
	int i;

	size_t countEachCh = countAllChs / nCh;
	dsp_value_t* end = pDst + countAllChs;
	dsp_value_t *cur = pSrc;
	for(i = 0; i < nCh; ++i)
	{
		dsp_value_t *out = pDst + i;
		while(out < end)
		{
			*out = *cur;
			out+=nCh;
			cur++;
		}
	}
}


void audio_DSP_init(audio_DSP_t *p,
		audio_DSP_cfg_t *pcfg)
{
	if(pcfg)
	{
		p->compress.cfg = pcfg->cfg_compress;
		audio_compress_init(&p->compress, &p->compress.cfg);
		p->eq.cfg = pcfg->cfg_EQ;
		audio_EQ_init(&p->eq, &p->eq.cfg);
		p->reverb.cfg = pcfg->cfg_reverb;
		audio_reverb_init(&p->reverb, &p->reverb.cfg);
		p->function_bits = pcfg->function_bits;
	}
}

void audio_DSP_extrct_cfg(audio_DSP_t *p, /*output*/audio_DSP_cfg_t *pcfg)
{
	pcfg->cfg_compress = p->compress.cfg;
	pcfg->cfg_EQ = p->eq.cfg;
	pcfg->cfg_reverb = p->reverb.cfg;
	pcfg->function_bits = p->function_bits;
}

void audio_DSP_set_function_bit(audio_DSP_t* p, uint32_t funcbit, uint8_t enable)
{
	if (!enable) // to turn off
	{
		__disable_irq();
		p->function_bits = (p->function_bits & (~funcbit));
		// clear the state of each effector
		if(funcbit & DSP_FUNCTION_BIT_EQ)
		{
			audio_EQ_init(&p->eq, &p->eq.cfg);
		}
		if(funcbit & DSP_FUNCTION_BIT_COMPRESS)
		{
			audio_compress_init(&p->compress, &p->compress.cfg);
		}
		if(funcbit & DSP_FUNCTION_BIT_REVERB)
		{
			audio_reverb_init(&p->reverb, &p->reverb.cfg);
		}
		__enable_irq();
	}
	else
	{
		// to turn on
		p->function_bits = (p->function_bits | funcbit);
	}
}

void audio_DSP_apply(audio_DSP_t* p, dsp_value_t* output, dsp_value_t* input, size_t cnt)
{

	dsp_value_t* pSrc = input;
	dsp_value_t* pDst = output;
	dsp_value_t* pDSPFrom = pSrc;
	if((p->function_bits & DSP_FUNCTION_BIT_COMPRESS) != 0)
	{
		audio_compress(&p->compress, pDst, pDSPFrom, cnt);
		pDSPFrom = pDst;
		// swap in out buffer
	}
	if((p->function_bits & DSP_FUNCTION_BIT_EQ) != 0 )
	{
		audio_EQ(&p->eq, pDst, pDSPFrom, cnt);
		pDSPFrom = pDst;
	}
	if((p->function_bits & DSP_FUNCTION_BIT_REVERB) != 0)
	{
		// still under construction, reverb function will be bypassed first
//		audio_reverb(&p->reverb, pDst, pSrc, cnt);
		// bypass the effector
		if(pDSPFrom != pDst)
		{
			memcpy(pDst, pDSPFrom, cnt*sizeof(dsp_value_t));
		}
	}
	if(pDSPFrom != output) // make sure the output buffer has
	{
		memcpy(output, pDSPFrom, cnt * sizeof(dsp_value_t));
	}

}


void audio_level_track(audio_level_tracker_t *p, float level)
{
	float new_value = (1-AUDIO_LEVEL_ALPHA) * p->value + AUDIO_LEVEL_ALPHA * fabsf(level);
	if(new_value > p->peak)
	{
		p->peak = new_value;
	}
	else
	{
		p->peak = (1-AUDIO_LEVEL_PEAK_ALPHA) * p->peak + AUDIO_LEVEL_PEAK_ALPHA * new_value;
	}
	p->value = new_value;
}


#endif





size_t pad_i16_to_i32(int32_t* __restrict pDst, const int16_t* __restrict pSrc, size_t bytes, uint8_t oversample, uint8_t nch)
{
	// left justified 0xffff -> 0xffff0000
	const int16_t *pEnd = pSrc + (bytes >> 1);
	const uint8_t bytespersample = 2;
	if(oversample <= 1)
	{
		while(pSrc < pEnd)
		{
			// little endian
			int32_t v = (int32_t)( ((uint32_t)(pSrc[0]) << 16U) | ((uint32_t)(pSrc[1]) << 24U) );
			*(pDst++) = v;
			// pointer to the data of the specified channel
			pSrc += bytespersample;
		}
		return (bytes << 1);
	}
	else
	{
		while(pSrc < pEnd)
		{
			// oversampling
			for(uint8_t j = 0; j < oversample; ++j)
			{
				// multi channels
				for(uint8_t c = 0; c < nch; ++c)
				{
					// pointer to the data of the specified channel
					const int16_t *pSrc_c = pSrc + c;
					// little endian
					int32_t v = (int32_t)( ((int32_t)(*pSrc_c) << 16U));
					*(pDst++) = v;
				}
			}
			pSrc += nch;
		}
		return (bytes * oversample * 2);
	}

}

size_t pad_i24_to_i32(int32_t* __restrict  pDst, const uint8_t* __restrict pSrc, size_t bytes, uint8_t oversample, uint8_t nch)
{
	// left justified 0xffffff -> 0xffffff00
	const uint8_t *pEnd = pSrc + bytes;
	const uint8_t bytespersample = 3;
	if(oversample <= 1)
	{
		while(pSrc < pEnd)
		{

			// little endian
			int32_t v = (int32_t)(((uint32_t)(pSrc[0]) << 8U) | ((uint32_t)(pSrc[1]) << 16U) | ((uint32_t)(pSrc[2]) << 24U));
			*(pDst++) = v;
			pSrc += bytespersample;
		}
		return (bytes / 3 * 4);
	}
	else
	{
		while(pSrc < pEnd)
		{
			// oversampling
			for(uint8_t j = 0; j < oversample; ++j)
			{
				// multi channels
				for(uint8_t c = 0; c < nch; ++c)
				{
					// pointer to the data of the specified channel
					const uint8_t *pSrc_c = pSrc + c * bytespersample;
					// little endian
					int32_t v = (int32_t)( ((uint32_t)(pSrc_c[0]) << 8U) | ((uint32_t)(pSrc_c[1]) << 16U) | ((uint32_t)(pSrc_c[2]) << 24U)  );
					*(pDst++) = v;
				}
			}
			pSrc += nch * bytespersample;
		}
		return (bytes / 3 * oversample * 4);
	}
}


// oversampe i32 formated datastream, and left-shift by bits
size_t oversample_and_left_shift_i32(int32_t* __restrict pDst,  int32_t* __restrict pSrc,
		size_t countAllCh, uint8_t oversample, uint8_t nch, uint8_t lshift_bits)
{
	int32_t* pTo = pDst;
	const int32_t *pFrm = pSrc;
	const int32_t* pFrmEnd = pSrc + countAllCh;
	size_t lenBytes = (countAllCh * oversample) << 2;
	if(oversample <= 1)
	{
		if(lshift_bits == 0)
		{
			memcpy(pDst, pSrc, lenBytes);
		}
		else
		{
			while(pFrm < pFrmEnd)
			{
				*(pTo++) = (*(pFrm++)) << lshift_bits;
			}
		}
	}
	else
	{
		if(lshift_bits == 0)
		{
			while(pFrm < pFrmEnd)
			{
				for(int o = 0; o < oversample; ++o)
				{
					for(int c = 0; c < nch; ++c)
					{
						*(pTo++) = (*(pFrm + c));
					}
				}
				pFrm += nch;
			}
		}
		else
		{
			while(pFrm <pFrmEnd)
			{
				for(int o = 0; o < oversample; ++o)
				{
					for(int c = 0; c < nch; ++c)
					{
						*(pTo++) = (*(pFrm + c)) << lshift_bits;
					}
				}
				pFrm += nch;
			}
		}
	}
	return lenBytes;
}


void convert_i24_to_float(float32_t* __restrict pDst, uint8_t* __restrict pSrc, size_t len)
{
	const uint8_t bytespersample = 3;
	const float32_t MAX_I24 = (float32_t)0x7fffff + 1;
	const uint8_t* pEnd = pSrc + len;
	int32_t vi;
	float32_t vf;
	int32_t sign;
	while(pSrc < pEnd)
	{
		// little endian
		sign = -(pSrc[bytespersample-1] >> 7);
		vi = ((uint32_t)pSrc[0]) | ((uint32_t)pSrc[1] << 8) | (((uint32_t)pSrc[2]) << 16) | (sign << 24);
		// normalize to +-1
		vf = (float32_t)vi / MAX_I24;

		*(pDst++) = vf;
		pSrc += bytespersample;
	}
}
void convert_i16_to_float(float32_t* __restrict pDst, int16_t* __restrict pSrc, size_t cntSampes)
{
#if defined(ARM_MATH_CM7)
	arm_q15_to_float(pSrc, pDst, cntSampes);
#else
	for (int i = 0; i < cntSampes; ++i)
	{
		float v = (float)((int16_t) * (pSrc++));
		v /= INT16_MAX;
		*(float*)(pDst++) = v;
	}
#endif
}

void convert_i32_to_float(float32_t* pDst,  int32_t* pSrc, size_t cntSampes)
{
	// pSrc inplace conversion OK
	// INT_MAX -> 1.f, INT_MIN -> -1.f
#if defined(ARM_MATH_CM7)
	arm_q31_to_float(pSrc, pDst, cntSampes);
#else
	for (int i = 0; i < cntSampes; ++i)
	{
		float v = (float)((int32_t) * (pSrc++));
		v /= INT32_MAX;
		*(float*)(pDst++) = v;
	}
#endif
}




size_t convert_float_to_i16(int16_t* pDst, float32_t* pSrc, size_t cntSrc, uint8_t downsample, uint8_t nch)
{
	const uint8_t bytespersample = 2;
	float32_t *pEnd = pSrc + cntSrc;
	// convert to int32 first

	if(downsample <= 1)
	{
#if defined(ARM_MATH_CM7)
		arm_float_to_q15(pSrc, pDst, cntSrc);
#else
		for (int i = 0; i < cntSrc; ++i)
		{
			float v = *(pSrc++);
			v *= INT16_MAX;
			*(pDst++) = (int16_t)v;
		}
#endif
		return cntSrc << 1;
	}
	else
	{
		convert_float_to_i32((int32_t*)pSrc, pSrc, cntSrc);
		while(pSrc < pEnd)
		{
			for(uint8_t c = 0; c < nch; ++c)
			{
				// pointer to the data of the specified channel
				const int32_t *pSrc_c = (int32_t*)pSrc + c;
				int32_t vi32 = *((const int32_t*)pSrc_c);
				int16_t vi16 = (vi32 >> 16U);
				*(pDst++) = vi16;
			}
			pSrc += (downsample * nch);
		}
		return cntSrc << 1 / downsample;
	}
}

size_t convert_float_to_i24(uint8_t* pDst, float32_t* pSrc, size_t cntSrc, uint8_t downsample, uint8_t nch)
{
	const uint8_t bytespersample = 3;
	float32_t *pEnd = pSrc + cntSrc;
	// LPF to anti aliasing
	// convert to int32 first
	convert_float_to_i32((int32_t*)pSrc, pSrc, cntSrc);
	if(downsample <= 1)
	{
		while(pSrc < pEnd)
		{
			// pointer to the data of the specified channel
			uint32_t vi = *((const uint32_t*)(pSrc++));
			*(pDst++) = (vi >> 8U) & 0xff;
			*(pDst++) = (vi >> 16U) & 0xff;
			*(pDst++) = (vi >> 24U) & 0xff;
		}
		return cntSrc * 3;
	}
	else
	{
		while(pSrc < pEnd)
		{
			for(uint8_t c = 0; c < nch; ++c)
			{
				// pointer to the data of the specified channel
				const int32_t *pSrc_c = (int32_t*)pSrc + c;
				uint32_t vi = *((const uint32_t*)pSrc_c);
				*(pDst++) = (vi >> 8U) & 0xff;
				*(pDst++) = (vi >> 16U) & 0xff;
				*(pDst++) = (vi >> 24U) & 0xff;
			}
			pSrc += (downsample * nch);
		}
		return cntSrc * 3 / downsample;
	}
}


void convert_float_to_i32(int32_t* pDst,  float32_t* pSrc, size_t lenSrc  )
{
	// disable interrupts
#if defined(ARM_MATH_CM7)
	arm_float_to_q31(pSrc, pDst, lenSrc);
#else
	for (int i = 0; i < lenSrc; ++i)
	{
		float v =  *(pSrc++);
		v *= INT32_MAX;
		*(pDst++) = (int32_t)v;
	}
#endif
}




