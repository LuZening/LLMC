/*
 * recording.c
 *
 *  Created on: Mar 17, 2024
 *      Author: cpholzn
 */


#include "main.h"
#include <stdint.h>

#include "audio_buffers.h"
#include "double_buffer.h"
#include "kfifo_DMA.h"

#include "assert.h"
#include "freeRTOS.h"
#include "cmsis_os2.h"
#include "MyUSB.h"
#include "Config_user_define.h"

#include "audio_DSP.h"
#include "audio_settings.h"



#ifndef MIN
#define MIN(a, b)  (((a) < (b)) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a, b)  (((a) > (b)) ? (a) : (b))
#endif
/************* for Encoder BEGIN ********************************************/

SAI_HandleTypeDef *hsai_in = &hsai_BlockA2;
extern TIM_HandleTypeDef htim5;

// a collection of the pointers to input DSPs,NULL means no DSP for this input source
const char *arr_DSP_names[N_INPUTS] = {"麦克风1", "麦克风2", "USB播放", "文件播放"};
audio_DSP_t* const  arr_DSPs[N_INPUTS] = {&DSPs_for_encoder[0], &DSPs_for_encoder[1], NULL, NULL};
const uint8_t arr_N_DSP_channels[N_INPUTS] = {1, 1, MAX_CHANNELS, MAX_CHANNELS};
// encder ch1, encoder ch2, USB playback, FS player
const uint8_t arr_num_channels_to_share_DSP[N_INPUTS] = {1, 1, 2, 2};

// dbuf.fifoOut stores output data after processing for other procedures.
// dbuf.fifoOut has 1 writer here.
static uint8_t __attribute__((section(".DMA_no_cache"))) bufIn_mic[I2S_DMA_BUF_SIZE] __attribute__ ((aligned(32))); // 8KB
double_buffer_t dbuf_from_encoder_i2s;

float bufEncoder[RECORDING_BUF_OUT_SIZE / sizeof(float)] = {0}; // 32KB
KFIFO_DMA fifo_from_encoder;
// counts how many bytes have been received,
uint32_t sampleCounter_encoder = 0;
uint32_t fifoOverflowCounter_encoder = 0;
uint32_t fifoUnderflowCounter_encoder = 0;
float volumefs_encoder[N_MICROPHONES] = {0};// a placeholder var, will be using the DAC's hardware attenuator, no need to do it in software
audio_level_tracker_t arr_level_trackers_encoder[N_MICROPHONES] = {0};
/* DSP Variables BEGIN */
audio_DSP_t DSP_MEMORY_SECTION DSPs_for_encoder[N_MICROPHONES];
//audio_DSP_t DSP_MEMORY_SECTION DSPs_for_USB_playback[N_CHANNELS_USB];
/* DSP Veriables END */

/* call at beginning */
static bool is_encoder_input_inited = false;
static void encocder_input_init()
{
    is_encoder_input_inited = true;
    double_buffer_init(&dbuf_from_encoder_i2s, bufIn_mic, sizeof(bufIn_mic));
    assert(I2S_DMA_BUF_SIZE % 4 == 0);
    kfifo_DMA_static_init(&fifo_from_encoder, (uint8_t*)bufEncoder, sizeof(bufEncoder), sizeof(bufEncoder) >> 1);
    sampleCounter_encoder = 0;
    // reset to internal system Fs and Bitdepth
    set_audio_sample_rate_and_bit_depth(1, DEFAULT_SAMPLE_RATE_T, 32);


    // NOTE: encoder is not always on, at this point differs from decoder, which is always transmitting
    // never stop
    // note: unit of Size depends on DMA Data Width, which is WORD in this case
    HAL_SAI_Receive_DMA(hsai_in, dbuf_from_encoder_i2s.buf, dbuf_from_encoder_i2s.size / 4);
    // start FS frequency counter (96kHz)
    HAL_TIM_Base_Start(&htim5);
}



/* call when update connections BEGIN */
uint8_t encoder_input_started = 0;
void encocder_input_start()
{
    if(!is_encoder_input_inited)
    {
        encocder_input_init();
    }
    if(!encoder_input_started)
    {
        encoder_input_started = 1;
    }
}

void encocder_input_stop()
{
    encoder_input_started = 0;
//	HAL_SAI_Abort(hsai_in);
}
/* call when update connections END */

void deinterlace_decoder_channels_f32(float* dst, const float* src, size_t count, uint8_t nChannels, uint8_t channel_LR_enable_bits)
{
    // NOTE: alignment
    if((channel_LR_enable_bits & 0b11U) < 0b11U)// when channel_LR_enable_bits is full, no need to deinterlace
    {

        if((channel_LR_enable_bits & 0b01) != 0) // L only, copy L data to R
        {
            for(int i = 0; i < count; i += nChannels)
            {
                *(dst + i + 1) = *(src + i);
                *(dst + i) = *(dst + i + 1);
            }
        }
        else if((channel_LR_enable_bits & 0b10) != 0)  // R only, copy R data to L
        {
            for(int i = 0; i < count; i+= nChannels)
            {
                *(dst + i) = *(src + i + 1);
                *(dst + i + 1) = *(dst + i);
            }
        }
    }
    else if(dst != src)
    {
        memcpy(dst, src, count * sizeof(float));
    }

}



// returns free space in dbuf, when freespace < len, means an overwrite has occured, the data is written into the fifo, but overrided existing data.
static size_t push_to_encoder_fifo(size_t len)
{
    static float buf_f32[I2S_DMA_BUF_SIZE / sizeof(float) / 2];
    uint8_t* pContent = dbuf_from_encoder_i2s.pOut;
    // convert to f32
    size_t cnt = len / sizeof(int32_t);
    convert_i32_to_float((float*)buf_f32, (int32_t*)pContent, cnt);
    // track audio level
    for(int i = 0; i < cnt; i+=N_MICROPHONES)
    {
        for(int j = 0; j < N_MICROPHONES; ++j)
            audio_level_track(&arr_level_trackers_encoder[j], buf_f32[i + j]);
    }
    // NOTE: fifo can be over written, returned value is the unoverwritten length
    size_t nFreeWrite = __kfifo_put(&fifo_from_encoder, (uint8_t*)buf_f32, len);
    // detect overflow
    if(nFreeWrite < len)
        ++fifoOverflowCounter_encoder;
    return nFreeWrite;

}

// takeover SAI2A DMA interrupts
void encoder_HAL_SAI_RxHalfCpltCallback(SAI_HandleTypeDef *hsai)
{
    int r = double_buffer_in_transfer_HalfCplt_cb(&dbuf_from_encoder_i2s);
    if(encoder_input_started)
    {
        size_t nFreeWritten = push_to_encoder_fifo(dbuf_from_encoder_i2s.size_half);
    }
}

void encoder_HAL_SAI_RxCpltCallback(SAI_HandleTypeDef *hsai)
{
    const int CNT_INC = I2S_DMA_BUF_SIZE / (4 * MAX_CHANNELS);
    __disable_irq();
    sampleCounter_encoder += CNT_INC;
    __enable_irq();
    int r = double_buffer_in_transfer_Cplt_cb(&dbuf_from_encoder_i2s);
    if(encoder_input_started)
    {
        size_t nFreeWritten = push_to_encoder_fifo(dbuf_from_encoder_i2s.size_half);
    }

}

uint32_t get_encoder_I2S_sample_count(uint32_t* pPrevcnt)
{
    // NOTE: make sure dbuf_to_decoder_i2s.size is fine grinded, e.g. < standardcount / 2,
    // NOTE: as of USB FS FS=48k, each USB poll requires 48 samples, standardcount = 48
    // NOTE: dbuf_to_decoder_i2s.size < 24/48 * 2ch * 3bytes-per-ch (144Bytes/288Bytes)
    const uint32_t standardcount = DEFAULT_SAMPLE_RATE_HZ / 1000;
    const uint32_t sampleunit = (I2S_DMA_BUF_SIZE / (4 * MAX_CHANNELS));
    const int lowerbound = standardcount - (sampleunit / 2);
    const int upperbound = standardcount + (sampleunit / 2);
    /* Acquire DMA count */
    __disable_irq();
//	HAL_SAI_DMAPause(hsai_out);
    uint32_t dmacount = (hsai_in->XferCount - __HAL_DMA_GET_COUNTER(hsai_in->hdmarx)) / (4 * MAX_CHANNELS);
//	HAL_SAI_DMAResume(hsai_out);
    // NOTE: SAI must have higher interrupt priority than USB, so USB routines will always get the latest DMA count updates.
    // OTHERWISE: when DMA circularly rewinds, DMA_COUNTER rewinds but the dbuf has not been updated yet
    uint32_t finecount = sampleCounter_encoder + dmacount;
    __enable_irq();
    //	__HAL_UNLOCK(hsai_out);
    if(pPrevcnt != NULL)
    {
        int cntdiff = (int)(finecount - *pPrevcnt);
        if(cntdiff < lowerbound)
        {
            finecount += sampleunit;
        }
        else if(cntdiff > upperbound)
        {
            finecount -= sampleunit;
        }
        uint32_t finecount_limited = MAX(MIN(finecount, upperbound + *pPrevcnt), lowerbound +*pPrevcnt);
        *pPrevcnt = finecount;
        return finecount_limited; // samples
    }
    else // initialize prev cont
    {
        return finecount;
    }
}

uint32_t get_encoder_I2S_sample_count_TIM(uint32_t* pPrevcnt)
{
    // NOTE: make sure dbuf_to_decoder_i2s.size is fine grinded, e.g. < standardcount / 2,
    // NOTE: as of USB FS FS=48k, each USB poll requires 48 samples, standardcount = 48
    // NOTE: dbuf_to_decoder_i2s.size < 24/48 * 2ch * 3bytes-per-ch (144Bytes/288Bytes)
//	const uint32_t standardcount = DEFAULT_SAMPLE_RATE_HZ / 1000;
//	const uint32_t sampleunit = (I2S_DMA_BUF_SIZE / (4 * MAX_CHANNELS));
    /* Acquire DMA count */
    size_t finecount = htim5.Instance->CNT;
    //	__HAL_UNLOCK(hsai_out);
    if(pPrevcnt != NULL)
    {
        int cntdiff = (int)(finecount - *pPrevcnt);
        *pPrevcnt = finecount;
        return finecount; // samples
    }
    else // initialize prev cont
    {
        return finecount;
    }
}



/************* for Encoder END ********************************************/



/************* for USB Player BEGIN ********************************************/

// input buffer for USB played sound,, will be stuffed by USB OUT interrupts
float bufFromUSBPlayback[USB_PLAYBACK_BUF_SIZE / sizeof(float)];
KFIFO_DMA fifo_from_USB_playback;
uint32_t fifoOverflowCounter_USB_playback = 0;
uint32_t fifoUnderflowCounter_USB_playback = 0;
size_t sizePreheatBytes_USB_playback = USB_PLAYBACK_BUF_SIZE / 2; // will be modified when USB init to a fixed millisecond

// counts how many bytes have been received,
uint32_t nCounter_from_USB_playback;
audio_interpolator_t arr_audio_interpolator_48K_to_96K_USB_playback[MAX_CHANNELS];
float32_t  bufsInterpolatorState_USB_playback[MAX_CHANNELS][COUNT_FIR_BUF]; // 1KB
float volumef_USB_playback = 0.f;
void init_USB_player()
{
    kfifo_DMA_static_init(&fifo_from_USB_playback, (uint8_t*)bufFromUSBPlayback, sizeof(bufFromUSBPlayback), 0);

    for(int i = 0; i < MAX_CHANNELS; ++i)
    {
        audio_interplolator_init(&(arr_audio_interpolator_48K_to_96K_USB_playback[i]), 2, bufsInterpolatorState_USB_playback[i], 96);
    }

}


/************* for USB Player END ********************************************/
