/**
  ******************************************************************************
  * @file    usbd_audio.h
  * @author  MCD Application Team, modified by LZN
  * @brief   header file for the usbd_audio.c file.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2015 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                      www.st.com/SLA0044
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#pragma once

#ifdef __cplusplus
extern "C" {
#endif



/* Includes ------------------------------------------------------------------*/
#include  "usbd_ioreq.h"
#include "usbd_def.h"
#include <stdint.h>
#include "kfifo_DMA.h"
// Descriptor



/** @addtogroup STM32_USB_DEVICE_LIBRARY
  * @{
  */

/** @defgroup USBD_AUDIO
  * @brief This file is the Header file for usbd_audio.c
  * @{
  */


/** @defgroup USBD_AUDIO_Exported_Defines
  * @{
  */
#ifndef USBD_AUDIO_FREQ
/* AUDIO Class Config */
#define USBD_AUDIO_FREQ                               96000U
#endif /* USBD_AUDIO_FREQ */


#define USBD_MAX_NUM_INTERFACES_UAC1_0                      3U


#define AUDIO_STREAM_NUM_INTERFACES 2 // only 2 stream interfaces, the other one is for synchronize

// NOTE: OVERRIDE usbd_conf.h
#include "audio_settings.h"
#define USBD_AUDIO_FREQ_DEFAULT						48000U
#define USBD_AUDIO_FREQ_SPEAKER_DEFAULT				48000U
#define USB_AUDIO_BIT_DEPTH_DEFAULT					DEFAULT_BIT_DEPTH
#define USB_AUDIO_PREHEAT_MS_RECORD						20U
#define USB_AUDIO_PREHEAT_MS_PLAYBACK					20U
#define USB_MAX_IN_PACKET_SIZE_EXPANSION_DENOMINATOR	    5U  // max_size = normal_size + normal_size / denominator
#define USB_MAX_OUT_PACKET_SIZE_EXPANSION_DENOMINATOR	10U  // max_size = normal_size + normal_size / denominator
#define CALC_AUDIO_PACKET_SIZE_FS(frq, bitdep, ch) ((uint16_t)((frq) * ((bitdep) >> 3U) * (ch) / 1000U))

#ifndef AUDIO_HS_BINTERVAL
#define AUDIO_HS_BINTERVAL                            0x01U
#endif /* AUDIO_HS_BINTERVAL */

#ifndef AUDIO_FS_BINTERVAL
#define AUDIO_FS_BINTERVAL                            0x01U
#endif /* AUDIO_FS_BINTERVAL */

#define AUDIO_SPEAKER_OUT_EP                             0x01U
#define AUDIO_SPEAKER_SYNC_EP                            (0x80U | 0x02U)
/* bRefresh feedback at an interval of 2^x millisecs 1<=x<=9, thus the interval ranges from 2ms to 512ms*/
#define AUDIO_FEEDBACK_BINTERVAL						 6U // 64ms,
#define AUDIO_FEEDBACK_PERIOD_MS 						(1U << AUDIO_FEEDBACK_BINTERVAL)
#define AUDIO_MIC_IN_EP                                  (0x80U | 0x01U)
//#define USB_AUDIO_CONFIG_DESC_SIZ                     0x6DU
#define AUDIO_INTERFACE_DESC_SIZE                     0x0AU // 10 BYTES
#define USB_AUDIO_DESC_SIZ                            0x09U
//#define AUDIO_STANDARD_ENDPOINT_DESC_SIZE             0x09U
//#define AUDIO_STREAMING_ENDPOINT_DESC_SIZE            0x07U

#define AUDIO_DESCRIPTOR_TYPE                         0x21U
#define USB_DEVICE_CLASS_AUDIO                        0x01U
#define AUDIO_SUBCLASS_AUDIOCONTROL                   0x01U
#define AUDIO_SUBCLASS_AUDIOSTREAMING                 0x02U
#define AUDIO_PROTOCOL_UNDEFINED                      0x00U
#define AUDIO_STREAMING_GENERAL                       0x01U
#define AUDIO_STREAMING_FORMAT_TYPE                   0x02U

/* Audio Descriptor Types */
#define AUDIO_INTERFACE_DESCRIPTOR_TYPE               0x24U
#define AUDIO_ENDPOINT_DESCRIPTOR_TYPE                0x25U

/* Audio Control Interface Descriptor Subtypes */
#define N_CHANNELS_USB 2U
#define AUDIO_CONTROL_HEADER                          0x01U
#define AUDIO_CONTROL_INPUT_TERMINAL                  0x02U
#define AUDIO_CONTROL_OUTPUT_TERMINAL                 0x03U
#define AUDIO_CONTROL_FEATURE_UNIT                    0x06U

#define AUDIO_INPUT_TERMINAL_DESC_SIZE                0x0CU
#define AUDIO_OUTPUT_TERMINAL_DESC_SIZE               0x09U
#define AUDIO_STREAMING_INTERFACE_DESC_SIZE           0x07U

#define AUDIO_CONTROL_MUTE                            0x0001U

#define AUDIO_FORMAT_TYPE_I                           0x01U
#define AUDIO_FORMAT_TYPE_III                         0x03U

#define AUDIO_ENDPOINT_GENERAL                        0x01U

#define AUDIO_REQ_GET_CUR                             0x81U
#define AUDIO_REQ_SET_CUR                             0x01U

#define AUDIO_SPEAKER_FEATURE_UNIT						0x02U
#define AUDIO_OUT_STREAMING_CTRL                      AUDIO_SPEAKER_FEATURE_UNIT
#define AUDIO_MIC_FEATURE_UNIT						0x05U

#define AUDIO_OUT_TC                                  0x01U
#define AUDIO_IN_TC                                   0x02U




#define AUDIO_OUT_PACKET_MAX                           CALC_AUDIO_PACKET_SIZE_FS((USBD_AUDIO_FREQ_SPEAKER_DEFAULT + USBD_AUDIO_FREQ_SPEAKER_DEFAULT / USB_MAX_OUT_PACKET_SIZE_EXPANSION_DENOMINATOR) , MAX_BIT_DEPTH, MAX_CHANNELS )
#define AUDIO_IN_PACKET_MAX                           CALC_AUDIO_PACKET_SIZE_FS((USBD_AUDIO_FREQ_DEFAULT + USBD_AUDIO_FREQ_DEFAULT / USB_MAX_IN_PACKET_SIZE_EXPANSION_DENOMINATOR) , MAX_BIT_DEPTH, MAX_CHANNELS )
#define AUDIO_SYNC_PACKET                              3U
#define AUDIO_DEFAULT_VOLUME                          20U

/* Number of sub-packets in the audio transfer buffer. You can modify this value but always make sure
  that it is an even number and higher than 3 */
#define AUDIO_OUT_PACKET_NUM                          4U
#define AUDIO_IN_PACKET_NUM                          4U
/* Total size of the USB audio transfer buffer */
#define AUDIO_TOTAL_BUF_SIZE                          ((AUDIO_OUT_PACKET_MAX * AUDIO_OUT_PACKET_NUM))
#define AUDIO_TOTAL_BUF_IN_SIZE                       ((AUDIO_IN_PACKET_MAX * AUDIO_IN_PACKET_NUM))

extern uint8_t is_USB_audio_inited;
extern uint32_t global_counter_USB_mic_SOF;
extern uint32_t global_counter_USB_mic_DataIn;
extern uint32_t global_counter_USB_mic_loss;
extern uint32_t global_counter_USB_speaker_loss;
/* Audio Commands enumeration */
typedef enum
{
  AUDIO_CMD_START = 1,
  AUDIO_CMD_PLAY,
  AUDIO_CMD_STOP,
} AUDIO_CMD_TypeDef;


typedef enum
{
  AUDIO_OFFSET_NONE = 0,
  AUDIO_OFFSET_HALF,
  AUDIO_OFFSET_FULL,
  AUDIO_OFFSET_UNKNOWN,
} AUDIO_OffsetTypeDef;
/**
  * @}
  */


/** @defgroup USBD_CORE_Exported_TypesDefinitions
  * @{
  */
typedef struct
{
  uint8_t __attribute__((aligned(4))) data[USB_MAX_EP0_SIZE]; // changed order for alignment
  uint8_t cmd;
  uint8_t len;
  uint8_t unit,epnum;
  uint8_t req_type, cs, cn;

} USBD_AUDIO_ControlTypeDef;


typedef struct
{
  uint32_t alt_settings[AUDIO_STREAM_NUM_INTERFACES];
  // a general buffer for dataout
  uint8_t __attribute__((aligned(4))) bufDO[1024];

  // 0: USB OUT, 1: USB IN, bufsize should be slighty greater than 1 single packet
  uint8_t __attribute__((aligned(4))) bufDI[AUDIO_TOTAL_BUF_IN_SIZE];
  KFIFO_DMA FIFOs[1];

  AUDIO_OffsetTypeDef offsets[AUDIO_STREAM_NUM_INTERFACES];
  uint8_t rd_enables[AUDIO_STREAM_NUM_INTERFACES];
  uint8_t wr_enables[AUDIO_STREAM_NUM_INTERFACES];
//  uint32_t rd_ptrs[AUDIO_STREAM_NUM_INTERFACES];
//  uint32_t wr_ptrs[AUDIO_STREAM_NUM_INTERFACES];
  USBD_AUDIO_ControlTypeDef control; // Control are only on EP0
  int16_t vols[AUDIO_STREAM_NUM_INTERFACES]; // USER added: keep track of volumes
  uint32_t sample_rate_Hzs[AUDIO_STREAM_NUM_INTERFACES];// USER added: keep track of volumes
  uint8_t bit_depths[AUDIO_STREAM_NUM_INTERFACES]; // USER added: keep track of volumes
  uint16_t packet_sizes[AUDIO_STREAM_NUM_INTERFACES]; // USER added: keep track of single packet size of each stream
  volatile uint8_t EPIN_tx_flags[2]; // indicating transmitting, clear in DataIn  callback
  uint32_t EPIN_NSOF[2]; // number of frame
  uint32_t EPOUT_NSOF[1];
} USBD_AUDIO_HandleTypeDef;


extern USBD_AUDIO_HandleTypeDef *haudio;

typedef struct
{
  int8_t (*Init)(uint8_t idx, uint32_t  AudioFreq, uint32_t Volume, uint32_t options);
  int8_t (*DeInit)(uint8_t idx, uint32_t options);
  int8_t (*AudioCmd)(uint8_t idx, uint8_t *pbuf, uint32_t size, uint8_t cmd);
  int8_t (*VolumeCtl)(uint8_t idx, uint8_t vol);
  int8_t (*MuteCtl)(uint8_t idx, uint8_t cmd);
  int8_t (*PeriodicTC)(uint8_t idx, uint8_t *pbuf, uint32_t size, uint8_t cmd);
  int8_t (*GetState)(void);
} USBD_AUDIO_ItfTypeDef;


/**
  * @}
  */
#define USB_UAC1_0_CONFIG_DESC_SIZ (9U+87U+61U+52U)
#define POS_CFG_MIC_SAMPLE_RATE (USB_UAC1_0_CONFIG_DESC_SIZ - 7U - 9U - 3U)
#define POS_CFG_MIC_BIT_DEPTH (POS_CFG_MIC_SAMPLE_RATE - 2U)
#define POS_CFG_MIC_BIT_DEPTH_BYTES (POS_CFG_MIC_BIT_DEPTH - 1U)
#define POS_CFG_MIC_MAX_PACKAGE_SIZE (USB_UAC1_0_CONFIG_DESC_SIZ - 7U - 5U)
#define POS_CFG_POWER_SOURCE 7U
#define POS_CFG_MAX_CURRENT 8U
extern uint8_t USBD_AUDIO_DeviceQualifierDesc[USB_LEN_DEV_QUALIFIER_DESC];
extern uint8_t USBD_AUDIO_CfgDesc[USB_UAC1_0_CONFIG_DESC_SIZ];

/** @defgroup USBD_CORE_Exported_Macros
  * @{
  */

/**
  * @}
  */

/** @defgroup USBD_CORE_Exported_Variables
  * @{
  */

extern USBD_ClassTypeDef USBD_AUDIO;
#define USBD_AUDIO_CLASS &USBD_AUDIO
/**
  * @}
  */

/** @defgroup USB_CORE_Exported_Functions
  * @{
  */
uint8_t USBD_AUDIO_RegisterInterface(USBD_HandleTypeDef *pdev,
                                     USBD_AUDIO_ItfTypeDef *fops);

void USBD_AUDIO_Sync(USBD_HandleTypeDef *pdev, AUDIO_OffsetTypeDef offset);
/**
  * @}
  */




/* Public Variables User Defined BEGIN */

uint32_t get_audio_feedback_Hz_from_fifo_free(int32_t fsHz, int32_t sizeFree, int32_t sizeTotal);

uint8_t get_UAC1_0_feedback_bytes(uint32_t measuredFS, uint32_t* buf);


/* Public Variables User Defined END */


#ifdef __cplusplus
}
#endif

/* __USB_AUDIO_H */
/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
