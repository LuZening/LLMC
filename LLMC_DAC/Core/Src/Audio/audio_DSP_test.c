#include "main.h"
#include "Audio/audio_DSP.h"
#include "Audio/audio_buffers.h"
#include "Audio/audio_player.h"
#include "profiler.h"

#ifdef DEBUG
#ifndef MIN
#define MIN(a, b)  (((a) < (b)) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a, b)  (((a) > (b)) ? (a) : (b))
#endif

void audio_DSP_test()
{
	const uint8_t M = 2;
	const uint16_t blockSize = 96;
	static arm_fir_decimate_instance_f32 filter_d;
	static arm_fir_interpolate_instance_f32 filter_i;

	// init
	arm_fir_decimate_init_f32(&filter_d, N_FIR_TAPS, M, audio_decimate_coeffs_96K_to_48K, bufsDecimatorState_USB_record[0], blockSize);
	arm_fir_interpolate_init_f32(&filter_i, M, N_FIR_TAPS, audio_decimate_coeffs_96K_to_48K, bufsInterpolatorState_USB_playback[0], blockSize);


	// prepare buffer content
	const int CNT = 1000;
	int32_t* buf_i32 = (int32_t*)memFileDecoder1;
	float *buf1_f32 = (float*)memFileDecoder1;
	float *buf2_f32 = (float*)memFileDecoder2;

	const int N_LOOP = 100;
	/* test int to float transformer */
	// prepare buffer
	memset(memFileDecoder1, 0, sizeof(memFileDecoder1));
	memset(memFileDecoder2, 0, sizeof(memFileDecoder2));
	for(int i = 0; i < CNT; ++i)
	{
		buf_i32[i] = i;
	}
	// start test
	static profiler_counter_t prof1;
	init_profiler_counter(&prof1);
	for(int i = 0; i < N_LOOP; ++i)
	{
		profiler_mark(&prof1); // ~7000 cycs per LOOP, CNT=1000
		arm_q31_to_float(buf_i32, buf2_f32, CNT);
	}

	/* test memcpy */
	init_profiler_counter(&prof1);
	for(int i = 0; i < N_LOOP; ++i)
	{
		profiler_mark(&prof1); // 10W cycs per LOOP, 32KB
		memcpy(buf1_f32, buf2_f32, sizeof(memFileDecoder1)); // 32KB
	}

	/* test decimator */
	memset(memFileDecoder1, 0, sizeof(memFileDecoder1));
	memset(memFileDecoder2, 0, sizeof(memFileDecoder2));
	for(int i = 0; i < CNT; ++i)
	{
		buf1_f32[i] = (float)i;
	}
	static profiler_counter_t prof2;
	init_profiler_counter(&prof2);
	for(int i = 0; i < N_LOOP; ++i)
	{
		profiler_mark(&prof2); // 17W cycs per LOOP, CNT = 1000, TAPS=128,
		int cntRemain = CNT;
		float* pChSrcBuf_begin = buf1_f32;
		float* pChDstBuf_begin = buf2_f32;
		while(cntRemain > 0)
		{
			arm_fir_decimate_f32(&filter_d, pChSrcBuf_begin, pChDstBuf_begin, MIN(blockSize, cntRemain));
			cntRemain -= blockSize;
			pChSrcBuf_begin += blockSize;
			pChDstBuf_begin += blockSize / filter_d.M;
		}
	}


	/* test interpolator */
	// prepare buffer
	memset(memFileDecoder1, 0, sizeof(memFileDecoder1));
	memset(memFileDecoder2, 0, sizeof(memFileDecoder2));
	for(int i = 0; i < CNT; ++i)
	{
		buf1_f32[i] = (float)i;
	}
	// start test
	static profiler_counter_t prof3;
	init_profiler_counter(&prof3);
	for(int i = 0; i < N_LOOP; ++i)
	{
		profiler_mark(&prof3); // 30W cycs, CNT = 1000
		int cntRemain = CNT;
		float* pChSrcBuf_begin = buf1_f32;
		float* pChDstBuf_begin = buf2_f32;
		while(cntRemain > 0)
		{
			arm_fir_interpolate_f32(&filter_i, pChSrcBuf_begin, pChDstBuf_begin, MIN(blockSize, cntRemain));
			cntRemain -= blockSize;
			pChSrcBuf_begin += blockSize;
			pChDstBuf_begin += blockSize * filter_i.L;
		}
	}

	memset(memFileDecoder1, 0, sizeof(memFileDecoder1));
	memset(memFileDecoder2, 0, sizeof(memFileDecoder2));
}
#else
void audio_DSP_test()
{

}
#endif
