#pragma once

#include <stddef.h>
#include <stdint.h>

#include <stdbool.h>
#include "main.h"
#include "double_buffer.h"
#include "kfifo_DMA.h"
#include "cmsis_os2.h"
#include "audio_DSP.h"

#include "audio_settings.h"


// Order of inputs:
// Mic1 (CH2): 0
// Mic2 (CH1): 1
// USB Playback : 2
// Filesystem : 3
#define N_INPUTS 4U
#define N_MICROPHONES 2U
#define N_INPUT_DEVICES 3U

#define MUXMIXER_INPUT_ID_ENCODER_CH_1 0U
#define MUXMIXER_SELECT_ENCODER_CH_1 0b1U // R-ch
#define MUXMIXER_INPUT_ID_ENCODER_CH_2 1U
#define MUXMIXER_SELECT_ENCODER_CH_2 0b10U // L-ch
#define MUXMIXER_INPUT_ID_USB_PLAYBACK 2U
#define MUXMIXER_SELECT_USB_PLAYBACK 0b100U
#define MUXMIXER_INPUT_ID_FS 3U
#define MUXMIXER_SELECT_FS 0b1000U

#define DSP_MEMORY_SECTION __attribute__((section(".RAM_CD_heap")))
#define DSP_MEMORY_SECTION_2 __attribute__((section(".ITCM_heap")))
extern audio_DSP_t DSPs_for_encoder[]; // for realworld applications
//extern audio_DSP_t DSPs_for_USB_playback[]; // for testing
// a collection of the pointers to input DSPs,NULL means no DSP for this input source
extern audio_DSP_t* const  arr_DSPs[N_INPUTS];
extern const char *arr_DSP_names[N_INPUTS];
extern const uint8_t arr_N_DSP_channels[N_INPUTS];
// encder ch1, encoder ch2, USB playback, FS player
extern const uint8_t arr_num_channels_to_share_DSP[N_INPUTS];

// encder ch1, encoder ch2, USB playback, FS player
extern const uint8_t arr_num_channels_to_share_DSP[N_INPUTS];

extern uint32_t* pUnderflowCounts[];

// Order of outputs:
// Decoder: 0
// USB recording : 1
#define N_OUTPUTS 2U
#define N_HEADPHONES 2U
#define OUTPUT_IDX_DECODER 0U
#define MUXMIXER_OUTPUT_DECODER (1U << OUTPUT_IDX_DECODER)
#define OUTPUT_IDX_USB_RECORDING 1U
#define MUXMIXER_OUTPUT_USB_RECORDING (1U << OUTPUT_IDX_USB_RECORDING)

#define N_FIFOS 4

/* AUDIO inputs BEGIN */
#define INPUT_IDX_ENCODER 0U
extern SAI_HandleTypeDef hsai_BlockA2;
/* Input: Encoder */
//#define RECORDING_BUF_MS 8U
#define RECORDING_BUF_OUT_SIZE (16U * 1024U) // 32KBytes around 40ms at 96K 2ch 32bit
// from encoder (microphone) (DMA)
extern double_buffer_t dbuf_from_encoder_i2s;
// (f32) input buffer for encoder,, will be stuffed by I2S DMA Half/Full Cplt interrupts
extern float bufEncoder[];
extern KFIFO_DMA fifo_from_encoder;
extern uint32_t fifoOverflowCounter_encoder;
extern uint32_t fifoUnderflowCounter_encoder;
// counts how many bytes have been received,
extern uint32_t sampleCounter_encoder;
extern uint8_t ready_for_read_encoder;
extern float volumefs_encoder[N_MICROPHONES];// a placeholder var, will be using the DAC's hardware attenuator, no need to do it in software
extern audio_level_tracker_t arr_level_trackers_encoder[N_MICROPHONES];
// indicate whether the output destination requires the dry sound (0) or wet sound (1) from encoder
// note: encoder has multi-channels, thus each entry refers to a bit. i.e. ch2(dry),ch1(wet) = 0b01, ch2(wet),ch1(wet) = 0b11
extern uint16_t outputs_dsp_usage[]; // 0: decoder, 1:



void deinterlace_decoder_channels_f32(float* dst, const float* src, size_t count, uint8_t nChannels, uint8_t channel_LR_enable_bits);


/* Input: FS player */
#define INPUT_IDX_FS_PLAYER 2U
// (f32) input buffer for wave files on SD card(no DMA), will be stuffed by file handling tasks
extern KFIFO_DMA fifo_from_FS;
extern uint8_t ready_for_read_FS;
extern uint32_t fifoOverflowCounter_FS;
extern uint32_t fifoUnderflowCounter_FS;
extern float volumef_FS;


/* Input: USB playback */
#define INPUT_IDX_USB_PLAYBACK 1U
//#define USB_LOADER_SIGNAL_STOP (1U << FS_PLAYER_STOP)
// OS signaling when USB Out data received
#define USB_LOADER_SIGNAL_RECEIVED 0x01U
//#define USB_LOADER_SIGNAL_PAUSE (1U << FS_PLAYER_PAUSE)

// (f32) input buffer for USB played sound,, will be stuffed by USB OUT interrupts
#define USB_PLAYBACK_BUF_SIZE  32U * 1024U // 64KBytes
extern float bufFromUSBPlayback[];
extern KFIFO_DMA fifo_from_USB_playback;
extern size_t sizePreheatBytes_USB_playback;
extern uint32_t fifoOverflowCounter_USB_playback;
extern uint32_t fifoUnderflowCounter_USB_playback;
// for feedback
// counts how many bytes have been received,
extern uint32_t nCounter_from_USB_playback;
extern uint8_t ready_for_read_USB_playback;
extern float volumef_USB_playback;


extern audio_interpolator_t arr_audio_interpolator_48K_to_96K_USB_playback[MAX_CHANNELS];
extern float32_t __attribute__((section(".DTCMRAM1_heap"))) bufsInterpolatorState_USB_playback[MAX_CHANNELS][COUNT_FIR_BUF]; // 1KB

void init_USB_player();
void USB_player_task_start();


#define MASK_MIC1	(1U << (IDX_CHANNEL_MIC1 - 1)) // mic1 connected to CH2(R)
#define MASK_MIC2	(1U << (IDX_CHANNEL_MIC2 - 1)) // mic2 wired to CH1(L)
//extern uint8_t ChannelLREnableBits;
uint32_t get_encoder_I2S_sample_count(uint32_t* pPrevcnt);
uint32_t get_encoder_I2S_sample_count_TIM(uint32_t* pPrevcnt);
extern uint8_t encoder_input_started;
void encocder_input_start();
void encocder_input_stop();
// takeover SAI2A DMA interrupts
void encoder_HAL_SAI_RxHalfCpltCallback(SAI_HandleTypeDef *hsai);
void encoder_HAL_SAI_RxCpltCallback(SAI_HandleTypeDef *hsai);
/* AUDIO inputs END */



/* AUDIO outputs BEGIN */
extern SAI_HandleTypeDef hsai_BlockB2;
// output buffer for decoder, will be absorbed by I2S DMA Half/Full Cplt interrupts
#define AUDIO_OUTPUT_BUF_SIZE (16U * 1024U) //
// counts how many samples has been received;
extern uint32_t sampleCounter_decoder;
//extern uint8_t bufDecoder[];
//extern KFIFO_DMA fifo_to_decoder;

/* I2S_DMA_BUF_SIZE used as anchor for the length of other */
// 96k 2CH 32bit 0.5ms or 48k 2Ch 1ms = 288Bytes
#define I2S_TRANSFER_INVERVAL_MS 1U
#define I2S_DMA_BUF_SIZE   (I2S_TRANSFER_INVERVAL_MS * 96 * 4 * 2) // 768, 96k int32 2ch per millis
extern uint8_t enable_to_decoder;
extern uint8_t ready_for_output_decoder;
// sending recorded data to i2s (DMA)
extern double_buffer_t dbuf_to_decoder_i2s;
extern uint32_t channels_connected_to_decoder;
//extern float bufDecoder[];
extern float volumef_decoder; // a placeholder var, will be using the DAC's hardware attenuator, no need to do it in software

//#define USB_PLAYBACK_FIFO_MS 64U
// output buffer for USB recording, will be absorbed by USB IN interrupts
extern uint8_t enable_to_USB_record;
// when output fifo half filled, set ready flag for users to absorb
extern uint8_t ready_for_output_USB_record;
extern size_t sizePreheatBytes_USB_record;
#define USB_RECORDER_BUF_SIZE  64U * 1024U // 64KBytes
extern uint8_t bufUSBRecord[];
extern KFIFO_DMA fifo_to_USB_record;
extern uint32_t fifoOverflowCounter_USB_record;
extern uint32_t fifoUnderflowCounter_USB_record;
extern uint32_t channels_connected_to_USB_record;
extern audio_decimator_t arr_audio_decimator_96K_to_48K_USB_record[MAX_CHANNELS];
extern float32_t __attribute__((section(".DTCMRAM1_heap"))) bufsDecimatorState_USB_record[MAX_CHANNELS][COUNT_FIR_BUF]; // 1KB
extern float volumef_USB_record; // scale the USB record output, set by USB host
void output_to_usb_init();
void output_to_usb_start();
void output_to_usb_stop();

void output_to_decoder_init();
void output_to_decoder_start();
void output_to_decoder_stop();
// takeover SAI2A DMA interrupts, to fill up I2S out DMA buffer when half exhuasted
void decoder_HAL_SAI_TxHalfCpltCallback(SAI_HandleTypeDef *hsai);

// takeover SAI2A DMA interrupts, to fill up I2S out DMA buffer when half exhuasted
void decoder_HAL_SAI_TxCpltCallback(SAI_HandleTypeDef *hsai);

//uint32_t get_decoder_I2S_sample_count(uint32_t prevcnt);



/* AUDIO outputs END */


