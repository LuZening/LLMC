/*
 * playing.c
 *
 *  Created on: 2024年3月17日
 *      Author: cpholzn
 */



#include <stdint.h>
#include <assert.h>
#include "double_buffer.h"
#include "kfifo_DMA.h"
#include "audio_buffers.h"
#include "audio_settings.h"
#include "Config.h"

#include "audio_DSP.h"
#include "usbd_audio.h"
#include "audio_player.h"
/* prepare data for output fifos, paced by SAI interrupts */
static size_t output_fifo_mixer_absorb(size_t maxlen);
static void process_encoder_data(
		float* bufInOut /* input data and dry output share the same buffer*/,
		float* bufTemp1_OutDSP /* wet output,  share the same buffer with temp1 buffer*/,
		float* bufTemp2,
		size_t cntAllChs);

#define N_INPUT_DEVICES 3U
SAI_HandleTypeDef *hsai_out = &hsai_BlockB2;


// if output to USB record is enabled
uint8_t enable_to_USB_record = 0U;
uint8_t ready_for_output_USB_record = 0U; // ready for output when the output FIFO of USB record is half filled
uint8_t bufUSBRecord[USB_RECORDER_BUF_SIZE]; // 64kb ~ 100ms@96K, 200ms@48K
size_t sizePreheatBytes_USB_record = USB_RECORDER_BUF_SIZE >> 1; // absorption from FIFO will be allowed when FIFO is filled to that much. updated according to different sample rates and bit depths
KFIFO_DMA fifo_to_USB_record;
uint32_t fifoOverflowCounter_USB_record = 0;
uint32_t fifoUnderflowCounter_USB_record = 0;
uint32_t channels_connected_to_USB_record = 0;
audio_decimator_t arr_audio_decimator_96K_to_48K_USB_record[N_CHANNELS_USB];
float32_t __attribute__((section(".DTCMRAM1_heap"))) bufsDecimatorState_USB_record[MAX_CHANNELS][COUNT_FIR_BUF]; // 1KB
float volumef_USB_record = 1.f;

// if output to decoder is enabled
uint8_t enable_to_decoder = 0U;
uint8_t ready_for_output_decoder = 1U; // decoder has no output fifo, so always ready
uint32_t sampleCounter_decoder = 0;
uint32_t channels_connected_to_decoder = 0;
// non-linearized volume 0~1
float volumef_decoder = 0.; // a placeholder var, will be using the DAC's hardware attenuator, no need to do it in software
//uint8_t bufDecoder[AUDIO_OUTPUT_BUF_SIZE] = {0};
//KFIFO_DMA fifo_to_decoder;
double_buffer_t dbuf_to_decoder_i2s;
uint8_t __attribute__((section(".DMA_no_cache"))) bufToDecoderDMA[I2S_DMA_BUF_SIZE];
//float bufDecoder[AUDIO_OUTPUT_BUF_SIZE / sizeof(float)] __attribute__ ((aligned(32))) = {0}; // 32KB


uint16_t outputs_dsp_usage[N_OUTPUTS] = {0};

/* call after SAI init BEGIN */
static bool output_to_usb_inited = false;
static bool output_to_decoder_inited = false;
void output_to_usb_init()
{
	output_to_usb_inited = true;
	memset(bufUSBRecord, 0, sizeof(bufUSBRecord));
	kfifo_DMA_static_init(&fifo_to_USB_record, bufUSBRecord, sizeof(bufUSBRecord), sizeof(bufUSBRecord) >> 1);
	for(int i = 0; i < N_CHANNELS_USB; ++i)
	{
		audio_decimator_init(&arr_audio_decimator_96K_to_48K_USB_record[i], 2, bufsDecimatorState_USB_record[i], BLOCK_SIZE_FIR);
	}
	enable_to_USB_record = 0;
}

void output_to_usb_start()
{
	enable_to_USB_record = 1;
}
void output_to_usb_stop()
{
	output_to_usb_inited = false;
	enable_to_USB_record = 0;
}

void output_to_decoder_init()
{
	output_to_decoder_inited = true;
	// init double buffer
	// dbuf will be filled in I2S DMA TxHalfCplt and TxCplt ISRs
	double_buffer_init(&dbuf_to_decoder_i2s, bufToDecoderDMA, sizeof(bufToDecoderDMA));
	// init fifo
	// 0: sample counter
	sampleCounter_decoder = 0;
	// reset to internal system Fs and Bitdepth
	set_audio_sample_rate_and_bit_depth(0, DEFAULT_SAMPLE_RATE_T, 32);
//	memset(bufDecoder, 0, sizeof(bufDecoder));
//	kfifo_DMA_static_init(&fifo_to_decoder, bufDecoder, sizeof(bufDecoder), sizeof(bufDecoder) >> 1);
	// never stop
	// note: unit of Size depends on DMA Data Width, which is WORD in this case
	HAL_StatusTypeDef r = HAL_SAI_Transmit_DMA(hsai_out, bufToDecoderDMA, sizeof(bufToDecoderDMA) / 4);

}
/* call after SAI init END */

/* call when update connections BEGIN */
void output_to_decoder_start()
{
	if(!output_to_decoder_inited)
	{
		output_to_decoder_init();
		// output to decoder always on, for clocking
	}
	if(enable_to_decoder == 0)
	{
		enable_to_decoder = 1;

	}

}

void output_to_decoder_stop()
{
//	HAL_SAI_Abort(hsai_out);
	enable_to_decoder = 0;
}
/* call when update connections END */





// when ready for read, means fifo is half filled, absorber can absorb data from this fifo.
uint8_t ready_for_read_encoder;
uint8_t ready_for_read_USB_playback;
uint8_t ready_for_read_FS;
uint8_t* Ready_for_reads[] = {&ready_for_read_encoder, &ready_for_read_USB_playback, &ready_for_read_FS};

#if defined(TEST_DSP) & defined(DEBUG)
// use FS player's fifo for microphones, so DSP can be tested without manpower()
KFIFO_DMA* pFifos_input[] = {&fifo_from_encoder,  &fifo_from_USB_playback, &fifo_from_FS,};
#else
KFIFO_DMA* pFifos_input[] = {&fifo_from_encoder,  &fifo_from_USB_playback, &fifo_from_FS,};
#endif
uint32_t* pUnderflowCounts[] = {&fifoUnderflowCounter_encoder, &fifoUnderflowCounter_USB_playback, &fifoUnderflowCounter_FS};

/* prepare data for output fifos, paced by SAI interrupts */
static size_t output_fifo_mixer_absorb(size_t maxlen)
{
	/* Inputs */


	/* Outputs */
	// counter for how many inputs are connected to the output
	uint8_t nUsedChannels_decoder = 0;
	uint8_t nUsedChannels_USB_record = 0;

	/* Mixer */
//	float* bufMixerInplace_f32 = NULL;
	static float __attribute__((aligned(4)))  __attribute__((section(".DTCMRAM1_heap"))) bufMixerAbsorbed_f32[I2S_DMA_BUF_SIZE / 2];
	static float __attribute__((aligned(4)))  __attribute__((section(".DTCMRAM1_heap"))) bufMixerAbsorbed_temp_f32[I2S_DMA_BUF_SIZE / 2];
	static float __attribute__((aligned(4)))  __attribute__((section(".DTCMRAM1_heap"))) bufMixerAbsorbed_temp2_f32[I2S_DMA_BUF_SIZE / 2];
	static float __attribute__((aligned(4)))  __attribute__((section(".DTCMRAM1_heap"))) bufMixerToDecoder_f32[I2S_DMA_BUF_SIZE / 2]; // 1.5x lager than actually needed
	static float __attribute__((aligned(4)))  __attribute__((section(".DTCMRAM1_heap"))) bufMixerToUSBRecord_f32[I2S_DMA_BUF_SIZE / 2];
	static float __attribute__((aligned(4)))  __attribute__((section(".DTCMRAM1_heap"))) bufMixerToUSBRecord_temp_f32[I2S_DMA_BUF_SIZE / 2];

//	if(enable_to_decoder && (channels_connected_to_decoder != 0))
//	{
//		memset(bufMixerToDecoder_f32, 0, sizeof(bufMixerToDecoder_f32));
//	}
//	if(enable_to_USB_record && (channels_connected_to_USB_record != 0))
//	{
//		memset(bufMixerToUSBRecord_f32, 0, sizeof(bufMixerToUSBRecord_f32));
//	}
	/* calculate double buffer size to fill */
	size_t sizeToAbsorb_f32, sizeAbsorbed_f32, countSamplesToAbsorb; // bytes to absorb, but the fifo stores float32 data

	// unit: bytes
	sizeToAbsorb_f32 = maxlen;
	// uinit: samples for each channel
	countSamplesToAbsorb =	maxlen / sizeof(float); // float in dbuf

	int i, j;
	/* iterate each input FIFO */
	for(i = 0; i < N_INPUT_DEVICES; ++i)
	{
		uint8_t mask_conn;
		if(i == 0)
			mask_conn = 0b11; // encoder has 2 channels
		else
			mask_conn = 1U << (i + 1);
		KFIFO_DMA *pFifo = (pFifos_input[i]);
		size_t size_input = kfifo_get_filled_size(pFifo);
		// reset ready for read flag by judging if the fifo is half filled after underrun
		uint8_t *pReadyForRead = Ready_for_reads[i];

		/* Update original data FIFO state */
		// check if ready_for_read flag can be restored (e.g. the target input fifo has been half filled)
		if(*pReadyForRead == 0)
		{
			if(pFifo == &fifo_from_USB_playback)
			{
				if(size_input > sizePreheatBytes_USB_playback)
				{
					*pReadyForRead = 1;
				}
			}
			else
			{
				if(size_input >= pFifo->half)
				{
					*pReadyForRead = 1;
				}
			}
		}

		/* Absorb Original Data */
		// read from input fifo only if the fifo is ready for read (half filled after underrun )
		if(*pReadyForRead > 0)
		{
			// USB DataOut is time sensitive, so it stores only padded to int32_t data in FIFO, not yet converted to float, not yet up-sampled or down-sampled
			uint8_t oversample = 1;
			if(pFifo == &fifo_from_USB_playback)
			{
				if(haudio->sample_rate_Hzs[0] < DEFAULT_SAMPLE_RATE_HZ)
				{
					oversample = 2;
					sizeAbsorbed_f32 = kfifo_get(pFifo, (uint8_t*)bufMixerAbsorbed_f32, sizeToAbsorb_f32 / 2);
				}
				else
				{
					sizeAbsorbed_f32 = kfifo_get(pFifo, (uint8_t*)bufMixerAbsorbed_f32, sizeToAbsorb_f32);
				}
				size_t countAbsorbed_f32 = sizeAbsorbed_f32 / sizeof(float);
				if(oversample > 1)
				{
					// convert to float (temp <- in)
					convert_i32_to_float((float*)bufMixerAbsorbed_temp_f32, (int32_t*)bufMixerAbsorbed_f32,countAbsorbed_f32);
					// deinterlace (in <- temp) : ~ 200us for 2048 cnts
					audio_DSP_deinterlace_channels(bufMixerAbsorbed_f32, bufMixerAbsorbed_temp_f32, countAbsorbed_f32, N_CHANNELS_USB);
					// filtering (temp <- in ) : ~2ms for 2048 cnts
					size_t countPadded = audio_interpolator_filter_apply(arr_audio_interpolator_48K_to_96K_USB_playback, bufMixerAbsorbed_temp_f32, bufMixerAbsorbed_f32, countAbsorbed_f32,
							N_CHANNELS_USB);
					// interlace( in <- temp) : ~200us for 4096 cnts
					audio_DSP_interlace_channels(bufMixerAbsorbed_f32, bufMixerAbsorbed_temp_f32, countPadded, N_CHANNELS_USB);
					sizeAbsorbed_f32 = countPadded * sizeof(float);
				}
				else
				{
					// do not need temp buffer if no oversample
					convert_i32_to_float((float*)bufMixerAbsorbed_f32, (int32_t*)bufMixerAbsorbed_f32, countAbsorbed_f32);
				}
			}
			// for other sources, processed already, so no more processes needed here
			else
			{
				sizeAbsorbed_f32 = kfifo_get(pFifo, (uint8_t*)bufMixerAbsorbed_f32, sizeToAbsorb_f32);

			}
			// detect fifo underflow, clear the ready flag and recover it only when the fifo is half filled again
			if(sizeAbsorbed_f32 < sizeToAbsorb_f32)
			{
				// fill silence
				memset(((uint8_t*)bufMixerAbsorbed_f32) + sizeAbsorbed_f32, 0, sizeToAbsorb_f32- sizeAbsorbed_f32 );
				*pReadyForRead = 0;
				if(pFifo == &fifo_from_FS)
				{
					// only count underflow when the FS player is playing normally
					if((!FS_is_EOF) && (FS_player_state == FS_PLAYER_PLAY))
						++(*pUnderflowCounts[i]);
				}
				else
					++(*pUnderflowCounts[i]);
			}
//			/* if DSP used on the input, then produce another copy for post-DSP audio */
//			if(
//			// CASE 1: the input fifo beging process now is the encoder, which takes over 2 independent DSPs
//			((i == 0) && (((cfg.outputs_dsp_usage[OUTPUT_IDX_DECODER] & 0b11) > 0) ||  ((cfg.outputs_dsp_usage[OUTPUT_IDX_USB_RECORDING] & 0b11) > 0)))
//			// CASE 2: the input fifo beging process now is not the encoder, which takes over 1 independent DSP
//			|| ((i > 0) && (((cfg.outputs_dsp_usage[OUTPUT_IDX_DECODER] & (1 << (i + 1))) > 0) ||  ((cfg.outputs_dsp_usage[OUTPUT_IDX_USB_RECORDING] & (1 << (i+1))) > 0)))
//			)
//			{
//
//			}
		}
		else
		{
			// if fifo not ready, zero pad the whole absorpted buffer
			memset((uint8_t*)bufMixerAbsorbed_f32, 0, sizeToAbsorb_f32);
			sizeAbsorbed_f32 = sizeToAbsorb_f32;
		}

//		/* Notify FIFO writing Tasks */ WARNING: SAI interrupts have high priority, higher than the RTOS, so no RTOS functions are allowed here!!!
//		// send FIFO empty task flag to FS player, to notify the task to prepare more data
//		if((pFifo == &fifo_from_FS) && (taskFSLoader))
//		{
//			if(kfifo_get_free_space(pFifo) < pFifo->half)
//			{
//				osThreadFlagsSet(taskFSLoader, FLAG_FIFO_HALF_EMPTY);
//			}
//		}

		/* MIXER BEGIN */
		/* Output 1:　decoder BEGIN*/
		float* bufBeforeMixer_f32 = NULL;
		bool encoder_data_processed = false;
		if(enable_to_decoder && ((channels_connected_to_decoder & mask_conn) != 0))
		{
			// special process for ENCODER
			// REASON: encoder has multiple inputs occupies L-ch and R-ch respectively
			// NOTE: L-ch: MIC1(ch2), R-ch: MIC2(ch1)
			// NOTE: need to deinterlace
			if((pFifo == &fifo_from_encoder) && (*pReadyForRead > 0) )
			{
				process_encoder_data(bufMixerAbsorbed_f32, bufMixerAbsorbed_temp_f32, bufMixerAbsorbed_temp2_f32, countSamplesToAbsorb);
				encoder_data_processed = true;
				// use DSP processed data if any  of the microphone channels satisfies
				if((cfg.outputs_dsp_usage[OUTPUT_IDX_DECODER] & 0b11) > 0)
				{
					bufBeforeMixer_f32 = bufMixerAbsorbed_temp_f32;
				}
				else
				{
					bufBeforeMixer_f32 = bufMixerAbsorbed_f32;
				}
			}
			// OTHER inputs only have 1 subchannel
			else
			{
				bufBeforeMixer_f32 = bufMixerAbsorbed_f32;
			}

			if(nUsedChannels_decoder == 0) // to save CPU cycles to zero buffers
			{
				memcpy(bufMixerToDecoder_f32, bufBeforeMixer_f32, sizeToAbsorb_f32);
			}
			else
			{
				//NOTE: check if arm_add_f32 supports inplace operation
				arm_add_f32(bufMixerToDecoder_f32, bufBeforeMixer_f32,bufMixerToDecoder_f32, countSamplesToAbsorb);
			}
			++nUsedChannels_decoder;

		}
		/* Output 1:　decoder END */

		/* Output 2:　USB recording BEGIN*/
		// USB recording
		if(is_USB_audio_inited && enable_to_USB_record && ((channels_connected_to_USB_record & mask_conn) != 0) && (haudio->alt_settings[1] > 0))
		{
			// NOTE: encoder has multiple inputs occupies L-ch and R-ch respectively
			// NOTE: L-ch: MIC2, R-ch: MIC1
			// NOTE: need to deinterlace
			if((pFifo == &fifo_from_encoder) && (*pReadyForRead > 0) )
			{
				if(!encoder_data_processed)
				{
					process_encoder_data(bufMixerAbsorbed_f32, bufMixerAbsorbed_temp_f32, bufMixerAbsorbed_temp2_f32, countSamplesToAbsorb);
					encoder_data_processed = true;
				}
				// use DSP processed data if any  of the microphone channels satisfies
				if((cfg.outputs_dsp_usage[OUTPUT_IDX_USB_RECORDING] & 0b11) != 0)
				{
					bufBeforeMixer_f32 = bufMixerAbsorbed_temp_f32; // wet sound
				}
				else
				{
					bufBeforeMixer_f32 = bufMixerAbsorbed_f32; // dry sound
				}
			}
			// OTHER inputs only have 1 subchannel
			else
			{
				bufBeforeMixer_f32 = bufMixerAbsorbed_f32;
			}


			if(nUsedChannels_USB_record == 0) // to save CPU cycles to zero buffers
			{
				memcpy(bufMixerToUSBRecord_f32, bufBeforeMixer_f32, sizeToAbsorb_f32);
			}
			else
			{
				//NOTE: check if arm_add_f32 supports inplace operation
				arm_add_f32(bufMixerToUSBRecord_f32, bufBeforeMixer_f32, bufMixerToUSBRecord_f32, countSamplesToAbsorb);
			}
			++nUsedChannels_USB_record;
		}
		/* Output 2:　USB recording END*/
		/* MIXER END */
	}

	/* MIXER unification BEGIN */
	// NOTE: check if a plain average algorithm is sufficient  for mixing audio sample
	/* Output 1:　decoder BEGIN*/
	if(enable_to_decoder &&  nUsedChannels_decoder)
	{
		// convert to i32
		convert_float_to_i32((int32_t*)dbuf_to_decoder_i2s.pIn, bufMixerToDecoder_f32, countSamplesToAbsorb);
	}
	else // zero pad
	{
		memset(dbuf_to_decoder_i2s.pIn, 0, dbuf_to_decoder_i2s.size_half);
	}
	/* Output 1:　decoder END*/

	/* Output 2:　USB recording BEGIN*/
	if(is_USB_audio_inited && enable_to_USB_record && nUsedChannels_USB_record && (haudio->alt_settings[1] > 0))
	{
		// MIXER MODE 2: don't average the sample (risk of overflow, but keeps the loudness of each channel)
		// convert to i24
		// reuse the same memory space, for i24 takes 1/4 less room than f32
		uint8_t downsample = 1;
		size_t sizePadded;
		if(haudio->sample_rate_Hzs[1] <= 48000)
		{
			downsample = 2;
		}
		// apply decimation filter to avoid aliasing
		float32_t* bufBefore = bufMixerToUSBRecord_f32;
		float32_t* bufAfter = bufMixerToUSBRecord_temp_f32;
		if(downsample > 1)
		{
			// deinterlace (temp <- in) : ~ 200us for 2048 cnts
			audio_DSP_deinterlace_channels(bufAfter, bufBefore, countSamplesToAbsorb, N_CHANNELS_USB);
			// filtering (in <- temp ) : ~2ms for 2048 cnts
			countSamplesToAbsorb = audio_decimator_filter_apply(arr_audio_decimator_96K_to_48K_USB_record, bufBefore, bufAfter, countSamplesToAbsorb, N_CHANNELS_USB);
			// interlace( temp(same as in) <- in) : ~200us for 4096 cnts
			audio_DSP_interlace_channels(bufAfter, bufBefore, countSamplesToAbsorb, N_CHANNELS_USB);
			// final results stored in bufAfter(temp)
		}
		else
		{
			bufAfter = bufBefore; // do not need temp buffer if no downsample
		}
		// volume control
		if(volumef_USB_record < .99f)
			arm_scale_f32(bufAfter, volumef_USB_record, bufAfter, countSamplesToAbsorb);
		// convert to mono
		if(cfg.USB_record_force_mono)
		{
			// add R-ch to L-ch, and eliminate R-ch
			for(int c = 0; c < countSamplesToAbsorb; c+=MAX_CHANNELS)
			{
				bufAfter[c] += bufAfter[c+1];
				bufAfter[c] /= MAX_CHANNELS;
				bufAfter[c+1] = 0;
			}
		}
		// conert to integer
		if(haudio->bit_depths[1] == 16U)
		{
			sizePadded = convert_float_to_i16((int16_t*)bufBefore, bufAfter, countSamplesToAbsorb, 1, MAX_CHANNELS);
		}
		else if(haudio->bit_depths[1] == 24U)
		{
			sizePadded = convert_float_to_i24((uint8_t*)bufBefore, bufAfter, countSamplesToAbsorb, 1, MAX_CHANNELS);
		}
		else
		{
			assert("never come here");
			sizePadded = countSamplesToAbsorb << 2;
		}
		// push into fifo
		size_t nSpaceRemain = kfifo_get_free_space(&fifo_to_USB_record);
		if(nSpaceRemain >= sizePadded)
		{
			// NOTE: write well-done integer data to USB fifo,
			// so no need to do float point computations in USB IS
			kfifo_put(&fifo_to_USB_record,
					(uint8_t*)bufBefore,
					sizePadded);
		}
		else
		{
			++fifoOverflowCounter_USB_record;
		}

	}
	/* Output 2:　USB recording END*/
	/* MIXER unification END */
	return countSamplesToAbsorb;
}


static void process_encoder_data(
		float* bufInOut /* input data and dry output share the same buffer*/,
		float* bufTemp1_OutDSP /* wet output,  share the same buffer with temp1 buffer*/,
		float* bufTemp2,
		size_t cntAllChs)
{
	uint8_t channel_LR_enable_bits = cfg.audio_connections[0] & 0b11U;
	// 4 bytes for each sample each channel
	// channel_LR_enable_bits: 0b01 = L-only, copy to R; 0b10 = R-only, copy to L,
//				deinterlace_decoder_channels_f32((float*)bufMixerAbsorbed_f32, (float*)bufMixerAbsorbed_f32, countSamplesToAbsorb, 2, channel_LR_enable_bits);
	if(channel_LR_enable_bits != 0)
	{
		audio_DSP_deinterlace_channels((float*)bufTemp1_OutDSP, (float*)bufInOut, cntAllChs, 2);
		size_t countSamplesToAbsorb_single_mic = cntAllChs / 2;
		// DSP on MIC 1 - L-ch
		uint8_t maskCh = (1U << 0);
		if((channel_LR_enable_bits & maskCh) != 0)
		{
			// if DSP enabled on any output destination, then process the
			if(
				(arr_DSPs[0])
				&&
				(((cfg.outputs_dsp_usage[OUTPUT_IDX_DECODER] & maskCh) != 0) || ((cfg.outputs_dsp_usage[OUTPUT_IDX_USB_RECORDING] & maskCh) != 0))
			)
			{
				audio_DSP_apply(arr_DSPs[0],
						bufTemp2, bufTemp1_OutDSP,
						countSamplesToAbsorb_single_mic);
			}
			// if DSP disabled, direct copy the channel original data
			else
			{
				memcpy(bufTemp2, bufTemp1_OutDSP, countSamplesToAbsorb_single_mic * sizeof(float));
			}
			// expand mono to stereo if the other channel is not activated
			if((channel_LR_enable_bits & (0b10)) == 0)
			{
				memcpy(bufTemp1_OutDSP + countSamplesToAbsorb_single_mic, bufTemp1_OutDSP, countSamplesToAbsorb_single_mic * sizeof(float));
				memcpy(bufTemp2 + countSamplesToAbsorb_single_mic, bufTemp2, countSamplesToAbsorb_single_mic * sizeof(float));
			}
		}


		// DSP on MIC 2 - R-ch
		maskCh = (1U << 1);
		if((channel_LR_enable_bits & maskCh) != 0)
		{
			if(
				(arr_DSPs[1])
				&&
				(((cfg.outputs_dsp_usage[OUTPUT_IDX_DECODER] & maskCh) != 0) || ((cfg.outputs_dsp_usage[OUTPUT_IDX_USB_RECORDING] & maskCh) != 0))
			)
			{
				audio_DSP_apply(arr_DSPs[1],
						bufTemp2 + countSamplesToAbsorb_single_mic,
						bufTemp1_OutDSP + countSamplesToAbsorb_single_mic,
						countSamplesToAbsorb_single_mic);
			}
			else
			{
				memcpy(bufTemp2 + countSamplesToAbsorb_single_mic,
						bufTemp1_OutDSP + countSamplesToAbsorb_single_mic,
					countSamplesToAbsorb_single_mic * sizeof(float));
			}
			// expand mono to stereo if the other channel is not activated
			if((channel_LR_enable_bits & 0b01) == 0)
			{
				memcpy(bufTemp1_OutDSP, bufTemp1_OutDSP + countSamplesToAbsorb_single_mic, countSamplesToAbsorb_single_mic * sizeof(float));
				memcpy(bufTemp2, bufTemp2 + countSamplesToAbsorb_single_mic, countSamplesToAbsorb_single_mic * sizeof(float));
			}
		}
		audio_DSP_interlace_channels((float*)bufInOut, bufTemp1_OutDSP, cntAllChs, 2); // deinterlace dry sound
		audio_DSP_interlace_channels((float*)bufTemp1_OutDSP, bufTemp2, cntAllChs, 2); // deinterlace wet sound
	}
	else
	{
		memset(bufInOut, 0 , cntAllChs* sizeof(float));
		memset(bufTemp1_OutDSP, 0 , cntAllChs* sizeof(float));
	}
}



// takeover SAI2A DMA interrupts, to fill up I2S out DMA buffer when half exhuasted
void decoder_HAL_SAI_TxHalfCpltCallback(SAI_HandleTypeDef *hsai)
{
	int r;
	// switch ptrs
	r = double_buffer_out_transfer_HalfCplt_cb(&dbuf_to_decoder_i2s);
	UNUSED(r);
//	sampleCounter_decoder += dbuf_to_decoder_i2s.size_half;

	// absord audio data from each input channel to stuff the decoder's output dbuf AND the USB recorder's output
	size_t nAbsorbed = output_fifo_mixer_absorb(dbuf_to_decoder_i2s.size_half);

}


// takeover SAI2A DMA interrupts, to fill up I2S out DMA buffer when half exhuasted
void decoder_HAL_SAI_TxCpltCallback(SAI_HandleTypeDef *hsai)
{
	const int CNT_INC = I2S_DMA_BUF_SIZE / 4 / MAX_CHANNELS;
	// switch ptrs
	int r = double_buffer_out_transfer_Cplt_cb(&dbuf_to_decoder_i2s);
	UNUSED(r);
	// maintain the counter
	sampleCounter_decoder += CNT_INC;
	// absord audio data from each input channel to stuff the decoder's output dbuf AND the USB recorder's output
	size_t nAbsorbed = output_fifo_mixer_absorb(dbuf_to_decoder_i2s.size_half);
}






