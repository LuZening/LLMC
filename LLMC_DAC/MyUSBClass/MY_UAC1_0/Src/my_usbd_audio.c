/**
 ******************************************************************************
 * @file    usbd_audio.c
 * @author  MCD Application Team
 * @brief   This file provides the Audio core functions.
 *
 * @verbatim
 *
 *          ===================================================================
 *                                AUDIO Class  Description
 *          ===================================================================
 *           This driver manages the Audio Class 1.0 following the "USB Device Class Definition for
 *           Audio Devices V1.0 Mar 18, 98".
 *           This driver implements the following aspects of the specification:
 *             - Device descriptor management
 *             - Configuration descriptor management
 *             - Standard AC Interface Descriptor management
 *             - 1 Audio Streaming Interface (with single channel, PCM, Stereo mode)
 *             - 1 Audio Streaming Endpoint
 *             - 1 Audio Terminal Input (1 channel)
 *             - Audio Class-Specific AC Interfaces
 *             - Audio Class-Specific AS Interfaces
 *             - AudioControl Requests: only SET_CUR and GET_CUR requests are supported (for Mute)
 *             - Audio Feature Unit (limited to Mute control)
 *             - Audio Synchronization type: Asynchronous
 *             - Single fixed audio sampling rate (configurable in usbd_conf.h file)
 *          The current audio class version supports the following audio features:
 *             - Pulse Coded Modulation (PCM) format
 *             - sampling rate: 48KHz.
 *             - Bit resolution: 16
 *             - Number of channels: 2
 *             - No volume control
 *             - Mute/Unmute capability
 *             - Asynchronous Endpoints
 *
 * @note     In HS mode and when the DMA is used, all variables and data structures
 *           dealing with the DMA during the transaction process should be 32-bit aligned.
 *
 *
 *  @endverbatim
 *
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

/* BSPDependencies
 - "stm32xxxxx_{eval}{discovery}.c"
 - "stm32xxxxx_{eval}{discovery}_io.c"
 - "stm32xxxxx_{eval}{discovery}_audio.c"
 EndBSPDependencies */

/* Includes ------------------------------------------------------------------*/
#include "usbd_audio.h"

#include "usbd_ctlreq.h"

#include "audio_settings.h"

#include "audio_buffers.h"

#include "cmsis_os2.h"

#include "Config.h"

#include "precise_counter.h"
#include "Audio/audio_settings.h"
#include "LVGL_GUI.h"
/** @addtogroup STM32_USB_DEVICE_LIBRARY
 * @{
 */

/** @defgroup USBD_AUDIO
 * @brief usbd core module
 * @{
 */

/** @defgroup USBD_AUDIO_Private_TypesDefinitions
 * @{
 */
/**
 * @}
 */

/** @defgroup USBD_AUDIO_Private_Defines
 * @{
 */
/**
 * @}
 */

/** @defgroup USBD_AUDIO_Private_Macros
 * @{
 */

#define AUDIO_REQ_SET_MIN                          0x02U
#define AUDIO_REQ_GET_MIN                          0x82U
#define AUDIO_REQ_SET_MAX                          0x03U
#define AUDIO_REQ_GET_MAX                          0x83U
#define AUDIO_REQ_SET_RES                          0x04U
#define AUDIO_REQ_GET_RES                          0x84U

/* Audio Control Requests */
#define AUDIO_CONTROL_REQ                             0x01U
/* Feature Unit, UAC Spec 1.0 p.102 */
#define AUDIO_CONTROL_REQ_FU_MUTE                     0x01U
#define AUDIO_CONTROL_REQ_FU_VOL                      0x02U
/* Volume. See UAC Spec 1.0 p.77 */
#ifndef USBD_AUDIO_VOL_DEFAULT
#define USBD_AUDIO_VOL_DEFAULT                        0x8d00U
#endif

#ifndef USBD_AUDIO_VOL_MAX
#define USBD_AUDIO_VOL_MAX                            0x0000U
#endif

#ifndef USBD_AUDIO_VOL_MIN
#define USBD_AUDIO_VOL_MIN                            0x8100U
#endif

#ifndef USBD_AUDIO_VOL_STEP
#define USBD_AUDIO_VOL_STEP                           0x0100U
#endif /* Total number of steps can't be too many, host will complain. */

/* Audio Streaming Requests */
#define AUDIO_STREAMING_EP_REQ                           0x02U
#define AUDIO_STREAMING_REQ_FREQ_CTRL                 0x01U
#define AUDIO_STREAMING_REQ_PITCH_CTRL                0x02U



#define AUDIO_SAMPLE_FREQ(frq)         (uint8_t)(frq), (uint8_t)((frq >> 8)), (uint8_t)((frq >> 16))

#define AUDIO_PACKET_SZE(frq)          (uint8_t)(((frq * 2U * 3U)/1000U) & 0xFFU), \
                                       (uint8_t)((((frq * 2U * 3U)/1000U) >> 8) & 0xFFU)




uint8_t is_USB_audio_inited = 0; // global
static uint8_t is_USB_playback_inited = 0;

/* Nomial feedback data for different frequencies */

static uint32_t fb_nom;
static uint32_t fb_value;
static uint32_t fb_raw;

/* FNSOF (frame number of SOF) is critical for frequency changing to work */
static uint32_t fnsof = 0;
static uint32_t fnsof_new = 0;
static uint8_t __attribute__((aligned(4))) bufFeedback[4]; // leave some room to avoid overflow
static precise_counter_t lastSampleCount_mic = {0};
static precise_counter_t counter_feedback = {0};
uint32_t global_counter_USB_mic_SOF = 0;
uint32_t global_counter_USB_mic_DataIn = 0;
uint32_t global_counter_USB_mic_loss = 0;
uint32_t global_counter_USB_speaker_loss = 0;
static size_t sizeLastPacket_mic;
/**
 * @}
 */

USBD_AUDIO_HandleTypeDef *haudio = NULL;
profiler_counter_t profUSB_MicIn = {0};
profiler_counter_t profUSB_feedback = {0};
/** @defgroup USBD_AUDIO_Private_FunctionPrototypes
 * @{
 */
static uint8_t USBD_AUDIO_Init(USBD_HandleTypeDef *pdev, uint8_t cfgidx);
static uint8_t USBD_AUDIO_DeInit(USBD_HandleTypeDef *pdev, uint8_t cfgidx);

static uint8_t USBD_AUDIO_Setup(USBD_HandleTypeDef *pdev,
        USBD_SetupReqTypedef *req);

static uint8_t* USBD_AUDIO_GetCfgDesc(uint16_t *length);
static uint8_t* USBD_AUDIO_GetDeviceQualifierDesc(uint16_t *length);
static uint8_t USBD_AUDIO_DataIn(USBD_HandleTypeDef *pdev, uint8_t epnum);
static uint8_t USBD_AUDIO_DataOut(USBD_HandleTypeDef *pdev, uint8_t epnum);
static void AUDIO_OUT_Restart(USBD_HandleTypeDef *pdev);
static void AUDIO_IN_Restart(USBD_HandleTypeDef *pdev);
static uint8_t USBD_AUDIO_EP0_RxReady(USBD_HandleTypeDef *pdev);
static uint8_t USBD_AUDIO_EP0_TxReady(USBD_HandleTypeDef *pdev);
static uint8_t USBD_AUDIO_SOF(USBD_HandleTypeDef *pdev);

static uint8_t USBD_AUDIO_IsoINIncomplete(USBD_HandleTypeDef *pdev,
        uint8_t epnum);
static uint8_t USBD_AUDIO_IsoOutIncomplete(USBD_HandleTypeDef *pdev,
        uint8_t epnum);
static void AUDIO_REQ_GetCurrent(USBD_HandleTypeDef *pdev,
        USBD_SetupReqTypedef *req);
static void AUDIO_REQ_SetCurrent(USBD_HandleTypeDef *pdev,
        USBD_SetupReqTypedef *req);

static void AUDIO_REQ_GetMin(USBD_HandleTypeDef *pdev,
        USBD_SetupReqTypedef *req);
static void AUDIO_REQ_GetMax(USBD_HandleTypeDef *pdev,
        USBD_SetupReqTypedef *req);
static void AUDIO_REQ_GetRes(USBD_HandleTypeDef *pdev,
        USBD_SetupReqTypedef *req);

static size_t USB_generate_fake_mic_in_data(uint8_t *buf, size_t n);
/**
 * @}
 */

/** @defgroup USBD_AUDIO_Private_Variables
 * @{
 */

USBD_ClassTypeDef USBD_AUDIO =
{ USBD_AUDIO_Init, USBD_AUDIO_DeInit, USBD_AUDIO_Setup, USBD_AUDIO_EP0_TxReady,
        USBD_AUDIO_EP0_RxReady, USBD_AUDIO_DataIn, USBD_AUDIO_DataOut,
        USBD_AUDIO_SOF, USBD_AUDIO_IsoINIncomplete, USBD_AUDIO_IsoOutIncomplete,
        USBD_AUDIO_GetCfgDesc, USBD_AUDIO_GetCfgDesc, USBD_AUDIO_GetCfgDesc,
        USBD_AUDIO_GetDeviceQualifierDesc, };

/* Device Descriptors */
/* USB Standard Device Descriptor */
uint8_t __ALIGN_BEGIN USBD_AUDIO_DeviceQualifierDesc[USB_LEN_DEV_QUALIFIER_DESC] __ALIGN_END
        =
        {
        USB_LEN_DEV_QUALIFIER_DESC,
        USB_DESC_TYPE_DEVICE_QUALIFIER, 0x00, 0x02, 0x00, 0x00, 0x00, 0x40,
                0x01, 0x00, };
/* Configuration Descriptors */

// Configuration Desc:  for UAC1.0
uint8_t __ALIGN_BEGIN USBD_AUDIO_CfgDesc[USB_UAC1_0_CONFIG_DESC_SIZ] __ALIGN_END
        =
        {
        /* Device descriptor */

        /* Configuration 1 (9)*/
#if 1
                9U, /* bLength */
                USB_DESC_TYPE_CONFIGURATION, /* bDescriptorType */
                LOBYTE(USB_UAC1_0_CONFIG_DESC_SIZ), /* wTotalLength  9+87+52+52 bytes*/
                HIBYTE(USB_UAC1_0_CONFIG_DESC_SIZ),
                0x03U, /* bNumInterfaces +speaker +feedback +mic*/
                0x01U, /* bConfigurationValue */
                0x00U, /* iConfiguration */
#if (USBD_SELF_POWERED == 1U)
                0xC0U, /* bmAttributes: Bus Powered according to user configuration */
#else
                0x80U, /* bmAttributes: Bus Powered according to user configuration */
#endif
                USBD_MAX_POWER, /* bMaxPower = 100 mA */
                /* 09 byte*/
#endif

                /* UAC Standard interface descriptor */
                9U, /* bLength */
                USB_DESC_TYPE_INTERFACE, /* bDescriptorType */
                0x00U, /* bInterfaceNumber */
                0x00U, /* bAlternateSetting */
                0x00U, /* bNumEndpoints (excluding endpoint0)*/
                USB_DEVICE_CLASS_AUDIO, /* bInterfaceClass */
                AUDIO_SUBCLASS_AUDIOCONTROL, /* bInterfaceSubClass */
                0x00U, /* bInterfaceProtocol */
                0x00U, /* iInterface */
                /* 09 byte*/

                // I0 USB Speaker Class-specific AC Interface Descriptor */
                AUDIO_INTERFACE_DESC_SIZE, /* bLength */
                AUDIO_INTERFACE_DESCRIPTOR_TYPE, /* bDescriptorType */
                AUDIO_CONTROL_HEADER, /* bDescriptorSubtype */
                0x00U, /* UAC 1.00 0x0100*//* bcdADC Lo */
                0x01U, /* bcdADC Hi */
                0x4EU, /* wTotalLength Lo *//*10+34+34 = 78 = 0x004E */
                0x00U, /* wTotalLength Hi */
                0x02U, /* bInCollection +mic */
                0x01U, /* baInterfaceNr Speaker */
                0x02U, /* baInterfaceNr MIC　*/
                /*10 byte */


                /* I0 cont T1: USB Speaker Input Terminal Descriptor - from PC */
                AUDIO_INPUT_TERMINAL_DESC_SIZE, /* bLength fixed to 12bytes */
                AUDIO_INTERFACE_DESCRIPTOR_TYPE, /* bDescriptorType */
                AUDIO_CONTROL_INPUT_TERMINAL, /* bDescriptorSubtype */
                0x01U, /* bTerminalID */
                0x01U, /* wTerminalType AUDIO_TERMINAL_USB_STREAMING Lo  0x0101 */
                0x01U, /* wTerminalType AUDIO_TERMINAL_USB_STREAMING Hi  0x0101 */
                0x00U, /* bAssocTerminal */
                0x02U, /* bNrChannels */
                0x03U, /* wChannelConfig 0x0003  L+R */
                0x00U, 0x00U, /* iChannelNames */
                0x00U, /* iTerminal */
                /* 12 byte*/

                /* I0 cont FU1: USB Speaker Audio Control Feature Unit Descriptor - Volume Contol */
                13U, /* bLength */
                AUDIO_INTERFACE_DESCRIPTOR_TYPE, /* bDescriptorType */
                AUDIO_CONTROL_FEATURE_UNIT, /* bDescriptorSubtype */
                AUDIO_SPEAKER_FEATURE_UNIT, /* bUnitID */
                0x01U, /* bSourceID connected to Speaker Input Terminal  */
                0x02U, /* bControlSize 2bytes for each control */

                AUDIO_CONTROL_MUTE | 0x02U, /* bmaControls(0) Mute + Volume Lo */
                0x00U, /* bmaControls(0) Mute + Volume Hi */

                0x00U, /* bmaControls(1) Lo */
                0x00U, /* bmaControls(1) Hi */

                0x00U, /* bmaControls(2) Lo */
                0x00U, /* bmaControls(2) Hi */

                0x00U, /* iTerminal */
                /* 13 byte*/

                // I0 cont T2: USB Speaker Output Terminal Descriptor - to Loudspeakers */
                9U, /* bLength */
                AUDIO_INTERFACE_DESCRIPTOR_TYPE, /* bDescriptorType */
                AUDIO_CONTROL_OUTPUT_TERMINAL, /* bDescriptorSubtype */
                0x03U, /* bTerminalID */
                0x01U, /* wTerminalType  Lo 0x0301 i.e. Loudspeaker*/
                0x03U, /* wTerminalType  Hi 0x0301 i.e. Loudspeaker*/
                0x00U, /* bAssocTerminal */
                AUDIO_SPEAKER_FEATURE_UNIT, /* bSourceID Feature Unit addr = 0x02*/
                0x00U, /* iTerminal */
                /* 09 byte*/


                /* I0 cont T3: USB Mic Input Terminal Descriptor - from Microphone L+R */
                12U, /* bLength */
                AUDIO_INTERFACE_DESCRIPTOR_TYPE, /* bDescriptorType */
                AUDIO_CONTROL_INPUT_TERMINAL, /* bDescriptorSubtype */
                0x04U, /* bTerminalID */
                0x01U, /* wTerminalType Microphone 0x0201 */
                0x02U, 0x00U, /* bAssocTerminal */
                N_CHANNELS_USB, /* bNrChannels L+R */
                0x03U, /* wChannelConfig 0x0003  L+R */
                0x00U, 0x00U, /* iChannelNames */
                0x00U, /* iTerminal */
                /* 12 byte*/

                /* I0 cont FU2 : USB Mic Audio Feature Unit Descriptor */
                13U, /* bLength */
                AUDIO_INTERFACE_DESCRIPTOR_TYPE, /* bDescriptorType */
                AUDIO_CONTROL_FEATURE_UNIT, /* bDescriptorSubtype */
                AUDIO_MIC_FEATURE_UNIT, /* bUnitID */
                0x04U, /* bSourceID Microphone Input Terminal */
                0x02U, /* bControlSize 2byte for each ch */

                AUDIO_CONTROL_MUTE | 0x02, /* ch:master bmaControls Lo*/
                0U, /* ch:master    bmaControls Hi */

                0U, /* ch1 bmaControls Lo */
                0U, /* ch1 bmaControls Hi */

                0U, /*ch2 bmaControls Lo */
                0U, /*ch2 bmaControls Hi */

                0x00U, /* iTerminal */
                /* 13 byte*/


                /* I0 cont T4: USB Mic Output Terminal Descriptor */
                9U, /* bLength */
                AUDIO_INTERFACE_DESCRIPTOR_TYPE, /* bDescriptorType */
                AUDIO_CONTROL_OUTPUT_TERMINAL, /* bDescriptorSubtype */
                0x06U, /* bTerminalID */
                0x01U, /* wTerminalType  0x0101 i.e. Microphone */
                0x01U, 0x00U, /* bAssocTerminal */
                AUDIO_MIC_FEATURE_UNIT, /* bSourceID Feature Unit ID = 0x05*/
                0x00U, /* iTerminal */
                /* 9 byte*/

                /* Interface 1: Speaker 播放 流描述 （61）*/

                /* I1A0: USB Speaker Standard AS Interface Descriptor - Audio Streaming Zero Bandwidth */
                9U, /* bLength */
                USB_DESC_TYPE_INTERFACE, /* bDescriptorType */
                0x01, /* bInterfaceNumber */
                0x00, /* bAlternateSetting */
                0x00, /* bNumEndpoints */
                USB_DEVICE_CLASS_AUDIO, /* bInterfaceClass */
                AUDIO_SUBCLASS_AUDIOSTREAMING, /* bInterfaceSubClass */
                AUDIO_PROTOCOL_UNDEFINED, /* bInterfaceProtocol */
                0x00, /* iInterface */
                /* 09 byte*/

                /* I1A1: USB Speaker Standard Audio Stream Interface Descriptor - Audio Streaming Operational */
                9U, /* bLength */
                USB_DESC_TYPE_INTERFACE, /* bDescriptorType */
                0x01, /* bInterfaceNumber */
                0x01, /* bAlternateSetting */
                0x02, /* bNumEndpoints Streaming EP + Feedback EP*/
                USB_DEVICE_CLASS_AUDIO, /* bInterfaceClass */
                AUDIO_SUBCLASS_AUDIOSTREAMING, /* bInterfaceSubClass */
                AUDIO_PROTOCOL_UNDEFINED, /* bInterfaceProtocol */
                0x00, /* iInterface */
                /* 09 byte*/

                /* I1A1 cont: USB Speaker Audio Streaming Interface Descriptor */
                7U, /* bLength */
                AUDIO_INTERFACE_DESCRIPTOR_TYPE, /* bDescriptorType */
                AUDIO_STREAMING_GENERAL, /* bDescriptorSubtype */
                0x01, /* bTerminalLink Speaker input terminal */
                0x00, /* bDelay */
                0x01, /* wFormatTag Lo AUDIO_FORMAT_PCM  0x0001 */
                0x00, /* wFormatTag Hi AUDIO_FORMAT_PCM  0x0001 */
                /* 07 byte*/

                /* I1A1 cont: USB Speaker Audio Type I Format Interface Descriptor */
                11U, /* bLength */
                AUDIO_INTERFACE_DESCRIPTOR_TYPE, /* bDescriptorType */
                AUDIO_STREAMING_FORMAT_TYPE, /* bDescriptorSubtype */
                AUDIO_FORMAT_TYPE_I, /* bFormatType */
                0x02U, /* bNrChannels */
                0x03U, /* bSubFrameSize :  3 Bytes per frame (24bits) */
                24U, /* bBitResolution (24-bits per sample) */
                0x01U, /* bSamFreqType: 1 frequency supported */
                /* Audio sampling frequency coded on 3 bytes */
                // 3 bytes
                AUDIO_SAMPLE_FREQ(48000U),
                /* 11 byte*/

                /* I1A1 Endpoint  1: Standard Descriptor */
                9U, /* bLength */
                USB_DESC_TYPE_ENDPOINT, /* bDescriptorType */
                AUDIO_SPEAKER_OUT_EP, /* bEndpointAddress Dir = OUT(b7 = 0) Addr = 1(b0=1)  endpoint */
                (USBD_EP_TYPE_ISOC) | (0x01U << 2), /* bmAttributes iso + async mode*/
                // 2 bytes
                (uint8_t)(AUDIO_OUT_PACKET_MAX & 0xffU), /* wMaxPacketSize in Bytes (Freq(Samples)*2(Stereo)*2(HalfWord)) */
                (uint8_t)(AUDIO_OUT_PACKET_MAX >> 8U), /* wMaxPacketSize in Bytes (Freq(Samples)*2(Stereo)*2(HalfWord)) */
                AUDIO_FS_BINTERVAL, /* bInterval */
                AUDIO_FEEDBACK_BINTERVAL, /* bRefresh pow(2, AUDIO_FEEDBACK_BINTERVAL)ms*/
                AUDIO_SPEAKER_SYNC_EP, /* bSynchAddress sync on Endpoint IN 1*/
                /* 09 byte*/

                /* I1A1 Endpoint 1 cont: Audio Streaming Descriptor*/
                7U, /* bLength */
                AUDIO_ENDPOINT_DESCRIPTOR_TYPE, /* bDescriptorType */
                AUDIO_ENDPOINT_GENERAL, /* bDescriptor */
                0x00U, /* bmAttributes */
                0x00U, /* bLockDelayUnits */
                0x00U, /* wLockDelay */
                0x00U,
                /* 07 byte*/

                /* I1A1 Endpoint IN 2: Speaker feedback endpoint Descriptor */
                9U, /* bLength */
                USB_DESC_TYPE_ENDPOINT, /* bDescriptorType */
                AUDIO_SPEAKER_SYNC_EP, // =0x82                      /* bEndpointAddress Dir = IN (b7 = 1) Addr = 1(b[2:0]=1) 0x81 endpoint */
                0x11, /* bmAttributes */
                (uint8_t)(3U), /* wMaxPacketSize in Bytes (Freq(Samples)*2(Stereo)*2(HalfWord)) */
                (uint8_t)(0U), /* wMaxPacketSize in Bytes (Freq(Samples)*2(Stereo)*2(HalfWord)) */
                AUDIO_FS_BINTERVAL, /* bInterval */
                AUDIO_FEEDBACK_BINTERVAL, /* bRefresh feedback at an interval of 2^x millisecs 1<=x<=9, thus the interval ranges from 2ms to 512ms*/
                0x00, /* bSynchAddress */
                /* 09 byte*/

                /* Interface 2: Microphone 录音 流描述 （52）*/
                /* I2A0: USB Mic Standard AS Interface Descriptor - Audio Streaming Zero Bandwidth */
                /*                                             */
                9U, /* bLength */
                USB_DESC_TYPE_INTERFACE, /* bDescriptorType */
                0x02, /* bInterfaceNumber */
                0x00, /* bAlternateSetting */
                0x00, /* bNumEndpoints */
                USB_DEVICE_CLASS_AUDIO, /* bInterfaceClass */
                AUDIO_SUBCLASS_AUDIOSTREAMING, /* bInterfaceSubClass */
                AUDIO_PROTOCOL_UNDEFINED, /* bInterfaceProtocol */
                0x00, /* iInterface */
                /* 09 byte*/

                /* I2A1: USB Mic Standard AS Interface Descriptor */
                /* Interface 2, Alternate Setting 1                                           */
                9U, /* bLength */
                USB_DESC_TYPE_INTERFACE, /* bDescriptorType */
                0x02, /* bInterfaceNumber */
                0x01, /* bAlternateSetting */
                0x01, /* bNumEndpoints */
                USB_DEVICE_CLASS_AUDIO, /* bInterfaceClass */
                AUDIO_SUBCLASS_AUDIOSTREAMING, /* bInterfaceSubClass */
                AUDIO_PROTOCOL_UNDEFINED, /* bInterfaceProtocol */
                0x00, /* iInterface */
                /* 09 byte*/

                /* I2A1 cont: USB Mic Audio Streaming Interface Descriptor */
                7U, /* bLength */
                AUDIO_INTERFACE_DESCRIPTOR_TYPE, /* bDescriptorType */
                AUDIO_STREAMING_GENERAL, /* bDescriptorSubtype */
                0x06U, /* bTerminalLink Microphone Mic output terminal*/
                0x00U, /* bDelay */
                0x01U, /* wFormatTag AUDIO_FORMAT_PCM  0x0001 */
                0x00U, /* wFormatTag AUDIO_FORMAT_PCM  0x0001 */
                /* 07 byte*/

                /* I2A1 cont: USB Mic Audio Type I Format Interface Descriptor */
                11U, /* bLength */
                AUDIO_INTERFACE_DESCRIPTOR_TYPE, /* bDescriptorType */
                AUDIO_STREAMING_FORMAT_TYPE, /* bDescriptorSubtype */
                AUDIO_FORMAT_TYPE_I, /* bFormatType */
                0x02, /* bNrChannels */
                /* set bit depth BEGIN */
                0x03, /* bSubFrameSize :  3 Bytes per frame (24bits), 2 Bytes per frame (16bits) position = POS_CFG_MIC_BIT_DEPTH_BYTE = POS_CFG_MIC_BIT_DEPTH - 1 */
                24, /* bBitResolution (24-bits per sample) position = POS_CFG_MIC_BIT_DEPTH = POS_CFG_MIC_SAMPLE_RATE - 2*/
                /* set bit depth END */
                /* set sample rate BEGIN -  */
                0x01, /* bSamFreqType: 1 frequency supported */
                // 3 bytes
                AUDIO_SAMPLE_FREQ(48000U), /* Audio sampling frequency coded on 3 bytes, position = POS_CFG_MIC_SAMPLE_RATE = USB_UAC1_0_CONFIG_DESC_SIZ - 7 - 9 - 3 */
                // 3 bytes
//                AUDIO_SAMPLE_FREQ(96000U), /* Audio sampling frequency coded on 3 bytes */
                /* set sample rates END */
                /* 11 byte*/

                /* Mic Endpoint IN 1 - Standard Descriptor */
                9U, /* bLength */
                USB_DESC_TYPE_ENDPOINT, /* bDescriptorType */
                AUDIO_MIC_IN_EP, /* =0x81 bEndpointAddress Dir = IN(bit7=1) Addr = 2 endpoint */
                USBD_EP_TYPE_ISOC | (0x01U << 2) , /* bmAttributes iso + async */
                // 2 bytes
                (uint8_t)(AUDIO_IN_PACKET_MAX & 0XffU), /* wMaxPacketSize in Bytes (Freq(Samples+1)*2(Stereo)*3(24bit)), position = POS_CFG_MIC_MAX_PACKAGE_SIZE = USB_UAC1_0_CONFIG_DESC_SIZ - 7 - 5  */
                (uint8_t)(AUDIO_IN_PACKET_MAX >> 8U), /* wMaxPacketSize in Bytes (Freq(Samples+1)*2(Stereo)*3(24bit)) */
                AUDIO_FS_BINTERVAL, /* bInterval */
                0x00U, /* bRefresh */
                0x00U, /* bSynchAddress not used for IN endpoint */
//                AUDIO_FEEDBACK_BINTERVAL, /* bRefresh pow(2, AUDIO_FEEDBACK_BINTERVAL)ms*/
//                AUDIO_SPEAKER_SYNC_EP, /* bSynchAddress sync on Endpoint IN 1*/
                /* 09 byte*/

                /* Endpoint 1 Cont - Audio Streaming Descriptor*/
                7U, /* bLength */
                AUDIO_ENDPOINT_DESCRIPTOR_TYPE, /* bDescriptorType */
                AUDIO_ENDPOINT_GENERAL, /* bDescriptor */
                0x00U, /* bmAttributes */
                0x00U, /* bLockDelayUnits */
                0x00U, /* wLockDelay */
                0x00U,
                /* 07 byte*/
        };

// collection of all endpoints
static uint8_t AUDIO_ENDPOINTS[] =
{ AUDIO_SPEAKER_OUT_EP, AUDIO_SPEAKER_SYNC_EP, AUDIO_MIC_IN_EP };
/**
 * @}
 */

/* Convert USB volume value to % */
uint8_t VOL_PERCENT(int16_t vol)
{
    // note: volume from USB is logarithm and always negatie
    // e.g.
    // max volume v=0 -> 100
    // mid volume v=-2916(50%) -> 91
    // 0 volume v=-32512 -> 0
    uint8_t vol100 =  (uint8_t)((vol - (int16_t)USBD_AUDIO_VOL_MIN) / (((int16_t)USBD_AUDIO_VOL_MAX - (int16_t)USBD_AUDIO_VOL_MIN) / 100));
  return vol100;
}

/** @defgroup USBD_AUDIO_Private_Functions
 * @{
 */

/**
 * @brief  USBD_AUDIO_Init
 *         Initialize the AUDIO interface
 * @param  pdev: device instance
 * @param  cfgidx: Configuration index
 * @retval status
 */

static uint8_t USBD_AUDIO_Init(USBD_HandleTypeDef *pdev, uint8_t cfgidx)
{
    UNUSED(cfgidx);
    /* Allocate Audio structure */
    haudio = USBD_malloc (sizeof(USBD_AUDIO_HandleTypeDef));

    if (haudio == NULL)
    {
        pdev->pClassData = NULL;
        return (uint8_t) USBD_EMEM;
    }

    pdev->pClassData = (void*) haudio;

    memset(&profUSB_SOF, 0, sizeof(profiler_counter_t));

    if (pdev->dev_speed == USBD_SPEED_HIGH)
    {
        pdev->ep_out[AUDIO_SPEAKER_OUT_EP & EP_ADDR_MSK].bInterval =
                AUDIO_HS_BINTERVAL;
        pdev->ep_in[AUDIO_MIC_IN_EP & EP_ADDR_MSK].bInterval =
                AUDIO_HS_BINTERVAL;
        pdev->ep_in[AUDIO_SPEAKER_SYNC_EP & EP_ADDR_MSK].bInterval =
                AUDIO_HS_BINTERVAL;

    }
    else /* LOW and FULL-speed endpoints */
    {
        pdev->ep_out[AUDIO_SPEAKER_OUT_EP & EP_ADDR_MSK].bInterval =
                AUDIO_FS_BINTERVAL;
        pdev->ep_in[AUDIO_MIC_IN_EP & EP_ADDR_MSK].bInterval =
                AUDIO_FS_BINTERVAL;
        pdev->ep_in[AUDIO_SPEAKER_SYNC_EP & EP_ADDR_MSK].bInterval =
                AUDIO_FS_BINTERVAL;
    }

    /* Open Endpoints */
    uint8_t ep_addr;
    ep_addr = AUDIO_SPEAKER_OUT_EP; //EPOUT 1
    (void) USBD_LL_OpenEP(pdev, ep_addr, USBD_EP_TYPE_ISOC, AUDIO_OUT_PACKET_MAX);
    pdev->ep_out[ep_addr & EP_ADDR_MSK].is_used = 1U;
    pdev->ep_out[ep_addr & EP_ADDR_MSK].maxpacket = AUDIO_OUT_PACKET_MAX;

    ep_addr = AUDIO_SPEAKER_SYNC_EP; // EPIN 2
    (void) USBD_LL_OpenEP(pdev, ep_addr, USBD_EP_TYPE_ISOC, AUDIO_SYNC_PACKET);
    pdev->ep_in[ep_addr & EP_ADDR_MSK].is_used = 1U;
    pdev->ep_in[ep_addr & EP_ADDR_MSK].maxpacket = AUDIO_SYNC_PACKET;
      /* Flush feedback endpoint */
      USBD_LL_FlushEP(pdev, ep_addr);

    ep_addr = AUDIO_MIC_IN_EP; // EPIN 1
    (void) USBD_LL_OpenEP(pdev, ep_addr, USBD_EP_TYPE_ISOC, AUDIO_IN_PACKET_MAX);
    pdev->ep_in[ep_addr & EP_ADDR_MSK].is_used = 1U;
    pdev->ep_in[ep_addr & EP_ADDR_MSK].maxpacket = AUDIO_IN_PACKET_MAX;
      /* Flush microphone endpoint */
      USBD_LL_FlushEP(pdev, ep_addr);


    for (uint8_t i = 0; i < AUDIO_STREAM_NUM_INTERFACES; ++i)
    {
        haudio->alt_settings[i] = 0U;
        haudio->offsets[i] = AUDIO_OFFSET_UNKNOWN;
//        haudio->wr_ptrs[i] = 0U;
//        haudio->rd_ptrs[i] = 0U;
        haudio->rd_enables[i] = 0U;
        haudio->wr_enables[i] = 0U;
        haudio->vols[i] = USBD_AUDIO_VOL_DEFAULT;
        if(i == 0) // speaker
        {
            haudio->sample_rate_Hzs[i] = USBD_AUDIO_FREQ_SPEAKER_DEFAULT;
            haudio->bit_depths[i] = USB_AUDIO_BIT_DEPTH_DEFAULT;
            haudio->packet_sizes[i] = CALC_AUDIO_PACKET_SIZE_FS(USBD_AUDIO_FREQ_SPEAKER_DEFAULT, USB_AUDIO_BIT_DEPTH_DEFAULT, MAX_CHANNELS);
        }
        else if(i == 1) // microphone
        {
            haudio->sample_rate_Hzs[i] = cfg.USB_record_audio_sample_rate_Hz;
            haudio->bit_depths[i] = cfg.USB_record_audio_bit_depth;
            haudio->packet_sizes[i] = CALC_AUDIO_PACKET_SIZE_FS(haudio->sample_rate_Hzs[i], haudio->bit_depths[i], MAX_CHANNELS);
        }
        haudio->EPIN_NSOF[i] = 0;
    }
    haudio->EPOUT_NSOF[0] = 0;
    /* Initialize feedback variables */
    get_UAC1_0_feedback_bytes(haudio->sample_rate_Hzs[0] << 4,(uint32_t*)bufFeedback);
    /**
     * Set tx_flag 1 to block feedback transmission in SOF handler since
     * device is not ready.
     */
    haudio->EPIN_tx_flags[0] = 255U; // microphone EPIN, 255 = NEED FIRST TRANSMIT TO INITIATE
    memset(&lastSampleCount_mic, 0, sizeof(precise_counter_t));
    haudio->EPIN_tx_flags[1] = 255U; // sync EPIN
    memset(&counter_feedback, 0, sizeof(precise_counter_t));
    /* Initialize the Audio output Hardware layer */
    // Interface 1 speaker
  if (((USBD_AUDIO_ItfTypeDef *)pdev->pUserData)->Init(0, USBD_AUDIO_FREQ_DEFAULT,
                                                       AUDIO_DEFAULT_VOLUME,
                                                       USB_AUDIO_BIT_DEPTH_DEFAULT) != 0U)
  {
    return (uint8_t)USBD_FAIL;
  }
    // Interface 2 microphone
    if (((USBD_AUDIO_ItfTypeDef *)pdev->pUserData)->Init(1, USBD_AUDIO_FREQ_DEFAULT,
                                                         AUDIO_DEFAULT_VOLUME,
                                                           USB_AUDIO_BIT_DEPTH_DEFAULT) != 0U)
    {
      return (uint8_t)USBD_FAIL;
    }

    is_USB_playback_inited = 0;
    is_USB_audio_inited = 1;

    // EP IN 1: mic in


    return (uint8_t) USBD_OK;
}

/**
 * @brief  USBD_AUDIO_Init
 *         DeInitialize the AUDIO layer
 * @param  pdev: device instance
 * @param  cfgidx: Configuration index
 * @retval status
 */
static uint8_t USBD_AUDIO_DeInit(USBD_HandleTypeDef *pdev, uint8_t cfgidx)
{
    UNUSED(cfgidx);

    /* Close Endpoints */
    uint8_t ep_addr;
    // EP1 speaker out
    ep_addr = AUDIO_SPEAKER_OUT_EP;
    (void) USBD_LL_CloseEP(pdev, ep_addr);
    pdev->ep_out[ep_addr & EP_ADDR_MSK].is_used = 0U;
    pdev->ep_out[ep_addr & EP_ADDR_MSK].bInterval = 0U;
    // EP2 in speaker sync
    ep_addr = AUDIO_SPEAKER_SYNC_EP;
      /* Flush feedback endpoint */
      USBD_LL_FlushEP(pdev, ep_addr);
    (void) USBD_LL_CloseEP(pdev, ep_addr);
    pdev->ep_in[ep_addr & EP_ADDR_MSK].is_used = 0U;
    pdev->ep_in[ep_addr & EP_ADDR_MSK].bInterval = 0U;
    // EP3 MIC in

    ep_addr = AUDIO_MIC_IN_EP;
      /* Flush microphone endpoint */
      USBD_LL_FlushEP(pdev, ep_addr);
    (void) USBD_LL_CloseEP(pdev, ep_addr);
    pdev->ep_in[ep_addr & EP_ADDR_MSK].is_used = 0U;
    pdev->ep_in[ep_addr & EP_ADDR_MSK].bInterval = 0U;

    /* DeInit  physical Interface components */
    if (pdev->pClassData != NULL)
    {
//    for(uint8_t i = 0; i < AUDIO_STREAM_NUM_INTERFACES; ++i)
//      ((USBD_AUDIO_ItfTypeDef *)pdev->pUserData)[i]->DeInit(0U);
        (void) USBD_free(pdev->pClassData);
        pdev->pClassData = NULL;
    }

    /* Clear feedback transmission flag */
    haudio->EPIN_tx_flags[1] = 0U; // sync EPIN
    return (uint8_t) USBD_OK;
}

/**
 * @brief  USBD_AUDIO_Setup
 *         Handle the AUDIO specific requests
 * @param  pdev: instance
 * @param  req: usb requests
 * @retval status
 * https://www.usbzh.com/article/detail-417.html
 * https://www.usbzh.com/article/detail-172.html
 */
static uint8_t USBD_AUDIO_Setup(USBD_HandleTypeDef *pdev,
        USBD_SetupReqTypedef *req)
{
    USBD_AUDIO_HandleTypeDef *haudio;
    uint16_t len;
    uint8_t *pbuf;
    uint16_t status_info = 0U;
    uint8_t itfnum = req->wIndex & 0x0f;
    uint8_t epnum = itfnum; // req->wIndex LOWBYTE carries either Interface num or Endpoint num
    uint8_t vlo = req->wValue & 0x0f;
    uint8_t vhi = (req->wValue & 0xf0) >> 4;
    USBD_StatusTypeDef ret = USBD_OK;

    haudio = (USBD_AUDIO_HandleTypeDef*) pdev->pClassData;

    if (haudio == NULL)
    {
        return (uint8_t) USBD_FAIL;
    }

    switch (req->bmRequest & USB_REQ_TYPE_MASK)
    // look at bit 6,5
    {
    case USB_REQ_TYPE_CLASS: // bmRequest = x01x xxxx
        switch (req->bRequest)
        {
        case AUDIO_REQ_GET_CUR:
            AUDIO_REQ_GetCurrent(pdev, req);
            break;

        case AUDIO_REQ_SET_CUR:
            AUDIO_REQ_SetCurrent(pdev, req);
            break;
            // TODO: process GET MIN request
        case AUDIO_REQ_GET_MIN:
            AUDIO_REQ_GetMin(pdev, req);
            break;
        case AUDIO_REQ_GET_MAX:
            AUDIO_REQ_GetMax(pdev, req);
            break;
        case AUDIO_REQ_GET_RES:
            AUDIO_REQ_GetRes(pdev, req);
            break;
        default:
            USBD_CtlError(pdev, req);
            ret = USBD_FAIL;
            break;
        }
        break;

    case USB_REQ_TYPE_STANDARD: // bmRequest = x00x xxxx
        switch (req->bRequest)
        {
        case USB_REQ_GET_STATUS:
            if (pdev->dev_state == USBD_STATE_CONFIGURED)
            {
                (void) USBD_CtlSendData(pdev, (uint8_t*) &status_info, 2U);
            }
            else
            {
                USBD_CtlError(pdev, req);
                ret = USBD_FAIL;
            }
            break;

        case USB_REQ_GET_DESCRIPTOR: //
            if ((req->wValue >> 8) == AUDIO_DESCRIPTOR_TYPE)
            {
                pbuf = USBD_AUDIO_CfgDesc + 18;
                len = MIN(AUDIO_INTERFACE_DESC_SIZE, req->wLength); // only send 10 bytes

                (void) USBD_CtlSendData(pdev, pbuf, len);
            }
            break;

        case USB_REQ_GET_INTERFACE:
            if (pdev->dev_state == USBD_STATE_CONFIGURED
                    && itfnum <= AUDIO_STREAM_NUM_INTERFACES)
            {
                (void) USBD_CtlSendData(pdev,
                        (uint8_t*) &(haudio->alt_settings[itfnum - 1]), 1U);
            }
            else
            {
                USBD_CtlError(pdev, req);
                ret = USBD_FAIL;
            }
            break;

        case USB_REQ_SET_INTERFACE:
            if (pdev->dev_state == USBD_STATE_CONFIGURED)
            {
                if (itfnum <= AUDIO_STREAM_NUM_INTERFACES)
                {
                    if(vlo != haudio->alt_settings[itfnum-1])
                    {
                        haudio->alt_settings[itfnum - 1] = vlo;
                        /* set bit depth */
                        /* interface 1: speaker*/
                        if(itfnum == 1)
                        {
                            if (vlo == 0U) {
//                              AUDIO_OUT_StopAndReset(pdev);
                            } else if (vlo == 1U) {
                              haudio->bit_depths[0] = 24U;
                              AUDIO_OUT_Restart(pdev);
                            } else if (vlo == 2U) {
                              haudio->bit_depths[0] = 24U;
                              AUDIO_OUT_Restart(pdev);
                            }
                            USBD_LL_FlushEP(pdev, AUDIO_SPEAKER_OUT_EP);
                        }
                        /* interface 2: microphone*/
                        else if(itfnum == 2)
                        {
                            if (vlo == 0U) {
//                              AUDIO_OUT_StopAndReset(pdev);
                            } else if (vlo == 1U) {
                              haudio->bit_depths[1] = cfg.USB_record_audio_bit_depth;
                              AUDIO_IN_Restart(pdev);
                            } else if (vlo == 2U) {
                              haudio->bit_depths[1] = cfg.USB_record_audio_bit_depth;
                              AUDIO_IN_Restart(pdev);
                            }
                            USBD_LL_FlushEP(pdev, AUDIO_MIC_IN_EP);
                        }

                    }
                }
                else
                {
                    /* Call the error management function (command will be NAKed */
                    USBD_CtlError(pdev, req);
                    ret = USBD_FAIL;
                }
            }
            else
            {
                USBD_CtlError(pdev, req);
                ret = USBD_FAIL;
            }
            break;

        case USB_REQ_CLEAR_FEATURE:
            break;

        default:
            USBD_CtlError(pdev, req);
            ret = USBD_FAIL;
            break;
        }
        break;
    default:
        USBD_CtlError(pdev, req);
        ret = USBD_FAIL;
        break;
    }

    return (uint8_t) ret;
}

/**
 * @brief  USBD_AUDIO_GetCfgDesc
 *         return configuration descriptor
 * @param  speed : current device speed
 * @param  length : pointer data length
 * @retval pointer to descriptor buffer
 */
static uint8_t* USBD_AUDIO_GetCfgDesc(uint16_t *length)
{
    *length = (uint16_t) sizeof(USBD_AUDIO_CfgDesc);
    // replace sample rates and bit depth according to config
    // sample rate: 3bytes
    if((cfg.USB_record_audio_sample_rate_Hz >= arr_valid_audio_sample_rates_Hz[0]) && (cfg.USB_record_audio_sample_rate_Hz <= arr_valid_audio_sample_rates_Hz[N_VALID_AUDIO_SAMPLE_RATES - 1]))
    {
        // all success
    }
    else
    {
        cfg.USB_record_audio_sample_rate_Hz =USBD_AUDIO_FREQ_DEFAULT;
        cfg.USB_record_audio_bit_depth = USB_AUDIO_BIT_DEPTH_DEFAULT;
        isModified = true;
    }
    uint8_t bytesFS[3] = {AUDIO_SAMPLE_FREQ(cfg.USB_record_audio_sample_rate_Hz)};
    for(int i = 0; i < 3; ++i)
        USBD_AUDIO_CfgDesc[POS_CFG_MIC_SAMPLE_RATE + i] = bytesFS[i];
    if((cfg.USB_record_audio_bit_depth == 16) || (cfg.USB_record_audio_bit_depth == 24) )
    {
        // frame size: 1byte
        USBD_AUDIO_CfgDesc[POS_CFG_MIC_BIT_DEPTH_BYTES] = cfg.USB_record_audio_bit_depth / 8;
        // bit depth: 1byte
        USBD_AUDIO_CfgDesc[POS_CFG_MIC_BIT_DEPTH] = cfg.USB_record_audio_bit_depth;
        // max package size: x (1 + 1/USB_MAX_IN_PACKET_SIZE_EXPANSION_DENOMINATOR)
        uint16_t sizeMaxPack = CALC_AUDIO_PACKET_SIZE_FS((cfg.USB_record_audio_sample_rate_Hz + cfg.USB_record_audio_sample_rate_Hz / USB_MAX_IN_PACKET_SIZE_EXPANSION_DENOMINATOR),  cfg.USB_record_audio_bit_depth, MAX_CHANNELS );
        USBD_AUDIO_CfgDesc[POS_CFG_MIC_MAX_PACKAGE_SIZE] = sizeMaxPack & 0xff;
        USBD_AUDIO_CfgDesc[POS_CFG_MIC_MAX_PACKAGE_SIZE + 1] = sizeMaxPack >> 8U;
    }

    /* self powered */
    if(cfg.USB_self_powered)
    {
        USBD_AUDIO_CfgDesc[POS_CFG_MAX_CURRENT] = 50U; // 100mA
        USBD_AUDIO_CfgDesc[POS_CFG_POWER_SOURCE] = 0xC0U; // SELF powered
    }
    else
    {
        USBD_AUDIO_CfgDesc[POS_CFG_MAX_CURRENT] = 200U; // 400mA
        USBD_AUDIO_CfgDesc[POS_CFG_POWER_SOURCE] = 0x80U; // BUS powered
    }
    return USBD_AUDIO_CfgDesc;
}

/**
 * @brief  absorb_from_fifo_mic_in
 *         absord all data from mic in fifo, for USB IN status
 *         NOTE: to save USB time, data has already been upsampled/downsampled and processed
 * @param  buf: place to store absorbed data
 * @param  n: max bytes to absord, size of buf
 * @retval data length absorbed
 */

size_t USB_absorb_from_fifo_mic_in(USBD_HandleTypeDef *pdev,  uint8_t *buf, size_t n)
{
    USBD_AUDIO_HandleTypeDef *haudio;
    const size_t STANDARD_COUNT_DIFF = 96; // standard count of samples per each USB frame
    const size_t MAX_COUNT = STANDARD_COUNT_DIFF + STANDARD_COUNT_DIFF / USB_MAX_IN_PACKET_SIZE_EXPANSION_DENOMINATOR; // USB frame may tolerate a slightly longer packet than standard case
    size_t standard_count_diff = STANDARD_COUNT_DIFF;
    size_t max_count = MAX_COUNT;
    haudio = (USBD_AUDIO_HandleTypeDef*) pdev->pClassData;
    size_t sizePerSampleCount = (haudio->bit_depths[1] >> 3) * MAX_CHANNELS;
    // internal Fs is 96000, down sample if needed
    uint8_t rshift_bits = 0;
    if(haudio->sample_rate_Hzs[1] <= 48000U)
    {
        rshift_bits = 1;
        max_count >>= 1;
        standard_count_diff >>= 1;
    }
#if 1 // according to FS TIM
    // get latest samples(2 channels as 1 sample)
    size_t nowSampleCount = get_encoder_I2S_sample_count_TIM(NULL);
    // update previous mark and calculate the difference
    size_t countDiff;
    if(lastSampleCount_mic.state == PRECISE_COUNTER_COUNTING)
        countDiff = precise_counter_update(&lastSampleCount_mic, nowSampleCount, rshift_bits);
    else // counter is waiting for its initial counting
    {
        precise_counter_update(&lastSampleCount_mic, nowSampleCount, rshift_bits);
        lastSampleCount_mic.state = PRECISE_COUNTER_COUNTING;
        countDiff = standard_count_diff;
    }
    // convert sample count to bytes
    size_t countDiff_limited;
    size_t countToDrop = 0;
    if (countDiff > max_count)
    {
        countDiff_limited = max_count;
        countToDrop += countDiff - max_count;
        global_counter_USB_mic_loss += countToDrop * sizePerSampleCount;
    }
    else
        countDiff_limited = countDiff;
#else // always submit standard length
    size_t countDiff_limited = STANDARD_COUNT_DIFF;
#endif
	/* Drop overflowed samples if necessary */
    int32_t sizeToGet = (int32_t)(countDiff_limited * sizePerSampleCount);
    uint32_t sizeGot = 0;
    size_t sizeAvail = kfifo_get_filled_size(&fifo_to_USB_record);
    if(countToDrop > 0)
    {
        fifo_to_USB_record.out += MIN(sizeAvail, countToDrop * sizePerSampleCount);
        sizeAvail = kfifo_get_filled_size(&fifo_to_USB_record);
    }
    uint32_t sizeFree = fifo_to_USB_record.size - sizeAvail;
	/* Jump start when data is warmed up */
    if(ready_for_output_USB_record == 0)
    {
        if(sizeAvail >= (sizePreheatBytes_USB_record))
            ready_for_output_USB_record = 1;
    }
	/* transfer data with adjusted length from the FIFO to the USB transmit buffer */
	static int32_t accum_adjust = 0;
	/* Reset accum_adjust on restart (counter re-initialised) */
	if(lastSampleCount_mic.state == PRECISE_COUNTER_INIT)
	    accum_adjust = 0;
    if(ready_for_output_USB_record)
    {
        /* Cumulative fractional adjustment: smoothly distribute +/-1 sample
         * across multiple SOFs instead of abrupt jumps. */
        int32_t deviation = ((int32_t)countDiff_limited - (int32_t)standard_count_diff) * (int32_t)sizePerSampleCount;
        accum_adjust += deviation;
        if (accum_adjust >= (int32_t)sizePerSampleCount) {
            sizeToGet += (int32_t)sizePerSampleCount;
            accum_adjust -= (int32_t)sizePerSampleCount;
        } else if (accum_adjust <= -(int32_t)sizePerSampleCount) {
            sizeToGet -= (int32_t)sizePerSampleCount;
            accum_adjust += (int32_t)sizePerSampleCount;
        }
        if (sizeToGet < (int32_t)sizePerSampleCount)
            sizeToGet = (int32_t)sizePerSampleCount;
        if (sizeToGet > (int32_t)(max_count * sizePerSampleCount))
            sizeToGet = (int32_t)(max_count * sizePerSampleCount);
        /* Clamp to caller-provided buffer size (packet_sizes[1]) */
        if (sizeToGet > (int32_t)n)
            sizeToGet = (int32_t)n;

        // FIFO is exhaust
        if (sizeAvail < (size_t)sizeToGet)
        {
            sizeGot = sizeAvail;
        }
        else
            sizeGot = (uint32_t)sizeToGet;
        //NOTE: to save USB time, data has already been upsampled/downsampled and processed
        sizeGot = kfifo_get(&fifo_to_USB_record, buf, sizeGot);
    }
    // pad zero if fifo underflow
    if (sizeGot < (size_t)sizeToGet)
    {
        memset(buf + sizeGot, 0, (size_t)sizeToGet - sizeGot);
        ++fifoUnderflowCounter_USB_record;
        ready_for_output_USB_record = 0;
    }

    return sizeToGet;
}


// testing
static size_t USB_generate_fake_mic_in_data(uint8_t *buf, size_t n)
{
    // keep track of sent sample count on decoder, to sync with decoder clock
    static size_t lastSampleCount = 0;
    // the increase of bytes
    size_t sizeSampleCountDiff = n;
    if (sizeSampleCountDiff > n)
        sizeSampleCountDiff = n;
    // saw wave
    const uint32_t fHz = 1000; // 1kHz
    const uint32_t samplesPerMilli = 96 * 3 * 2;
    const uint32_t T = 1000 * samplesPerMilli / fHz;
    uint32_t t0 = lastSampleCount;
    uint32_t len = sizeSampleCountDiff;

    uint32_t delta = INT32_MAX / (T + ((lastSampleCount / samplesPerMilli / 1000) % 2) * T);
    int v = INT32_MIN;
    for(int i = 0; i < sizeSampleCountDiff ; ++i)
    {
        buf[i] = v;
        v += delta;
    }


    lastSampleCount += sizeSampleCountDiff;
    if(lastSampleCount > 2000*samplesPerMilli)
        lastSampleCount = 0;
    return sizeSampleCountDiff;
}




/**
 * @brief  USBD_AUDIO_DataIn
 *         handle data IN Stage
 * @param  pdev: device instance
 * @param  epnum: endpoint index
 * @retval status
 */
static uint8_t USBD_AUDIO_DataIn(USBD_HandleTypeDef *pdev, uint8_t epnum)
{
    /*
     * NOTE: DO NOT prepare data in the DataIn callback function
     * this callback indicates the end of transmission of the TX buffer
     * fill TX buffer prior to this callback and fill the next packet when callback
     *
     */
    USBD_AUDIO_HandleTypeDef *haudio;
    haudio = (USBD_AUDIO_HandleTypeDef*) pdev->pClassData;
    /* Endpoint buffer transmitted: microphone in */
    if (epnum == (AUDIO_MIC_IN_EP & (0x0F)))
    {

        if(lastSampleCount_mic.state == PRECISE_COUNTER_INIT) // precise counter need an init
        {
            precise_counter_update(&lastSampleCount_mic, get_encoder_I2S_sample_count_TIM(NULL), 0);
            lastSampleCount_mic.state = PRECISE_COUNTER_COUNTING;
            // send silence data
            uint8_t *bufEPin = haudio->bufDI;
            sizeLastPacket_mic = haudio->packet_sizes[1];
            memset(bufEPin, 0, sizeLastPacket_mic);
            USBD_LL_FlushEP(pdev, AUDIO_MIC_IN_EP);
            USBD_LL_Transmit(pdev, AUDIO_MIC_IN_EP, bufEPin, sizeLastPacket_mic);
            haudio->EPIN_tx_flags[0] = 1U; // mic EPIN
            haudio->EPIN_NSOF[0] = fnsof_new;
            global_counter_USB_mic_DataIn = 1;
        }
        else
        {
            uint8_t *bufEPin = haudio->bufDI;
            // absorb data from mixer fifo
//            USBD_LL_FlushEP(pdev, AUDIO_MIC_IN_EP);
            sizeLastPacket_mic = USB_absorb_from_fifo_mic_in(pdev, bufEPin, haudio->packet_sizes[1]);
            (void) USBD_LL_Transmit(pdev, AUDIO_MIC_IN_EP, bufEPin, sizeLastPacket_mic);
            haudio->EPIN_tx_flags[0] = 1U; // mic EPIN
            haudio->EPIN_NSOF[0] = fnsof_new; // record the latest SOF number here, so in ISOInIncomplete callback we can tell if the transfer of microphone data is transferred
            global_counter_USB_mic_DataIn++;
        }
#ifdef PROFILER
        profiler_mark(&profUSB_MicIn);
#endif
    }

    /* Endpoint buffer transmitted : Feedback */
    else if(epnum == (AUDIO_SPEAKER_SYNC_EP & (0x0F)))
    {
        haudio->EPIN_tx_flags[1] = 0U; // clear the transmitting flag of Feedback, so in SOF irq feedback buffer will be updated
        // if transmit FB in DataIn, do not send it again in SOF and ISOIncomplete
//        if(haudio->EPIN_tx_flags[1] == 0U)
//        {
//            USBD_LL_Transmit(pdev, AUDIO_SPEAKER_SYNC_EP, (uint8_t*)bufFeedback, 3U);
//            haudio->EPIN_tx_flags[1] = 1U; // Sync EPIN
//            haudio->EPIN_NSOF[1] = fnsof_new;
//        }
#ifdef PROFILER
        profiler_mark(&profUSB_feedback);
#endif
    }
    return (uint8_t) USBD_OK;
}

/**
 * @brief  Restart playing with new parameters
 * @param  pdev: instance
 */
static void AUDIO_OUT_Restart(USBD_HandleTypeDef *pdev)
{
    USBD_AUDIO_HandleTypeDef *haudio;
    haudio = (USBD_AUDIO_HandleTypeDef*) pdev->pClassData;

    /* update packet sizes */
    int i = 0;
    haudio->packet_sizes[i] = CALC_AUDIO_PACKET_SIZE_FS(haudio->sample_rate_Hzs[i], haudio->bit_depths[i], MAX_CHANNELS);
//    kfifo_invalidate(&fifo_from_USB_playback); // exhause all un-outed data on the FIFO
    haudio->EPIN_tx_flags[1] = 255U; // Feedback EP, NEED FIRST TRANSMIT TO INITIATE
    memset(&counter_feedback, 0, sizeof(precise_counter_t));
    memset(&profUSB_feedback, 0, sizeof(profiler_counter_t));
    is_USB_playback_inited = 1U;
    sizePreheatBytes_USB_playback = haudio->packet_sizes[i] * USB_AUDIO_PREHEAT_MS_PLAYBACK;

    /* Prepare Out endpoint to receive 1st packet */
    // EP OUT 1: speaker out
    (void) USBD_LL_PrepareReceive(pdev, AUDIO_SPEAKER_OUT_EP,
            haudio->bufDO,
            haudio->packet_sizes[0]);
    haudio->EPOUT_NSOF[0] = fnsof_new;

    // clear the packet loss counter
    global_counter_USB_speaker_loss = 0;

    // When USB plugged in, VBUS from USB will cause fluctuation on power supply
    // which may put the DAC chips and ADC chips in gliches
    // if you hear while noise from DAC after the USB cord plugged in, audio chips need reset after USB plugged in
//    audio_analog_chips_need_reset = true;

}


static void AUDIO_IN_Restart(USBD_HandleTypeDef *pdev)
{
    USBD_AUDIO_HandleTypeDef *haudio;
    haudio = (USBD_AUDIO_HandleTypeDef*) pdev->pClassData;

    /* update packet sizes */
    int i = 1;
    haudio->packet_sizes[i] = CALC_AUDIO_PACKET_SIZE_FS(haudio->sample_rate_Hzs[i], haudio->bit_depths[i], MAX_CHANNELS);
//    kfifo_invalidate(&fifo_to_USB_record); // exhause all un-outed data on the FIFO
    haudio->EPIN_tx_flags[0] = 255U;// Sync EP, NEED FIRST TRANSMIT TO INITIATE
    haudio->wr_enables[1] = 1U;

    ready_for_output_USB_record = 0;
    sizePreheatBytes_USB_record = USB_AUDIO_PREHEAT_MS_RECORD * haudio->packet_sizes[i];
    // set initial value to measure I2S speed
//                lastSampleCount_mic = get_encoder_I2S_sample_count(NULL);
    memset(&lastSampleCount_mic, 0, sizeof(precise_counter_t));
    memset(&profUSB_MicIn, 0, sizeof(profiler_counter_t));

    sizeLastPacket_mic = haudio->packet_sizes[i];
    memset(haudio->bufDI, 0, sizeof(haudio->bufDI));

    global_counter_USB_mic_loss = 0;
}

/**
 * @brief  USBD_AUDIO_EP0_RxReady
 *         handle EP0 Rx Ready event
 * @param  pdev: device instance
 * @retval status
 */
static uint8_t USBD_AUDIO_EP0_RxReady(USBD_HandleTypeDef *pdev)
{
    USBD_AUDIO_HandleTypeDef *haudio;
    haudio = (USBD_AUDIO_HandleTypeDef*) pdev->pClassData;

    if (haudio == NULL)
    {
        return (uint8_t) USBD_FAIL;
    }
    /* cmd 1: SET CUR */
    if (haudio->control.cmd == AUDIO_REQ_SET_CUR)
    {
        /* In this driver, to simplify code, only SET_CUR request is managed */
        // enriched by user
        int16_t v;
        v = *(int16_t*) &haudio->control.data[0];

        /* type 1: UNIT-WISE 0x01*/
        if (haudio->control.req_type == AUDIO_CONTROL_REQ)
        {

            switch (haudio->control.cs)
            {
            case AUDIO_CONTROL_REQ_FU_MUTE:
                if (haudio->control.unit == AUDIO_SPEAKER_FEATURE_UNIT)
                {
                    ((USBD_AUDIO_ItfTypeDef*) pdev->pUserData)->MuteCtl(0,
                            haudio->control.data[0]);
                    haudio->control.cmd = 0U;
                    haudio->control.len = 0U;
                }
                else if (haudio->control.unit == AUDIO_MIC_FEATURE_UNIT)
                {
                    ((USBD_AUDIO_ItfTypeDef*) pdev->pUserData)->MuteCtl(1,
                            haudio->control.data[1]);
                    haudio->control.cmd = 0U;
                    haudio->control.len = 0U;
                }
                break;
            case AUDIO_CONTROL_REQ_FU_VOL:
                if(haudio->control.unit == AUDIO_SPEAKER_FEATURE_UNIT) // set DATA OUT volume
                {
                    haudio->vols[0] = v;
                    ((USBD_AUDIO_ItfTypeDef*) pdev->pUserData)->VolumeCtl(0, VOL_PERCENT(v));
                }
                else if(haudio->control.unit == AUDIO_MIC_FEATURE_UNIT) // set DATA IN volume
                {
                    haudio->vols[1] = v;
                    ((USBD_AUDIO_ItfTypeDef*) pdev->pUserData)->VolumeCtl(1, VOL_PERCENT(v));
                }
                break;
            }
        }
        /* type 2: ENDPOINT-WISE 0x02*/
        else if (haudio->control.req_type == AUDIO_STREAMING_EP_REQ)
        {
            uint8_t epnum = haudio->control.epnum;
            if (haudio->control.cs == AUDIO_STREAMING_REQ_FREQ_CTRL)
            {
                uint32_t new_freq = *((uint32_t*)&haudio->control.data) & 0x00ffffff;
                if(epnum == AUDIO_SPEAKER_OUT_EP)
                {
                    if (haudio->sample_rate_Hzs[0] != new_freq)
                    {
                        haudio->sample_rate_Hzs[0] = new_freq;

                        AUDIO_OUT_Restart(pdev);
                    }
                }
                else if(epnum == AUDIO_MIC_IN_EP)
                {
                    if (haudio->sample_rate_Hzs[1] != new_freq)
                    {
                        haudio->sample_rate_Hzs[1] = new_freq;
                        AUDIO_IN_Restart(pdev);
                    }
                }
            }
        }

    }

    return (uint8_t) USBD_OK;
}
/**
 * @brief  USBD_AUDIO_EP0_TxReady
 *         handle EP0 TRx Ready event
 * @param  pdev: device instance
 * @retval status
 */
static uint8_t USBD_AUDIO_EP0_TxReady(USBD_HandleTypeDef *pdev)
{
    UNUSED(pdev);

    /* Only OUT control data are processed */
    return (uint8_t) USBD_OK;
}
/**
 * @brief  USBD_AUDIO_SOF
 *         handle SOF event
 * @param  pdev: device instance
 * @retval status
 */

static uint8_t USBD_AUDIO_SOF(USBD_HandleTypeDef *pdev)
{
    USBD_AUDIO_HandleTypeDef *haudio;
    haudio = (USBD_AUDIO_HandleTypeDef*) pdev->pClassData;
    USB_OTG_GlobalTypeDef* USBx = USB_OTG_HS;
    uint32_t USBx_BASE = (uint32_t)USBx;
    fnsof_new = (USBx_DEVICE->DSTS & USB_OTG_DSTS_FNSOF) >> 8;
    /**
     * 1. Must be static so that the values are kept when the function is
     *    again called.
     * 2. Must be volatile so that it will not be optimized out by the compiler.
     */
    // static volatile uint32_t fb_value = AUDIO_FB_DEFAULT;
    // static volatile uint32_t audio_buf_writable_size_last = AUDIO_TOTAL_BUF_SIZE / 2U;
    // static volatile int32_t fb_raw = AUDIO_FB_DEFAULT;
    static volatile uint32_t sof_count = 0;


    if(sof_count != 0 )
    {

        // TODO: check if PrepareReceive is needed in DataIn status
        //  (void)USBD_LL_PrepareReceive(pdev, AUDIO_SPEAKER_OUT_EP, haudio->buffers_out[0],
        //                               AUDIO_OUT_PACKET);
        // clear all transmitting flags, for SOF ensures a termination to the previous frame's transmission
        /* EP IN 1: microphone first packet here, after that in the USBD_AUDIO_DataIn IRQ */
        if(haudio->EPIN_tx_flags[0] == 255) // NEED A FIRST PACKAGE TO FIRE UP THE DATAIN interrupt
        {
            if(haudio->alt_settings[1] > 0)
            {
                // send silence data
                uint8_t *bufEPin = haudio->bufDI;
                memset(bufEPin, 0,  haudio->packet_sizes[1]);
                USBD_LL_FlushEP(pdev, AUDIO_MIC_IN_EP);
                USBD_LL_Transmit(pdev, AUDIO_MIC_IN_EP, bufEPin, haudio->packet_sizes[1]);
                haudio->EPIN_tx_flags[0] = 1U; // mic EPIN
                haudio->EPIN_NSOF[0] = fnsof_new;
                global_counter_USB_mic_SOF = 1;
            }
        }
        else if(haudio->alt_settings[1] > 0)
        {
            global_counter_USB_mic_SOF++;
        }

        /* EP IN 2: feedback */
        if((haudio->rd_enables[0] > 0) || (haudio->wr_enables[1] > 0) // either speaker activated or microphone activated, calculate Feedback value
//                && (haudio->EPIN_tx_flags[1] != 1)
                ) // EPIN_tx_flags[1] = 0 or 255 means need to fill up the feedback buffer
        {
            size_t nowSampleCount_x1000 = get_encoder_I2S_sample_count_TIM(NULL) * 1000;
            if(counter_feedback.state != PRECISE_COUNTER_COUNTING) // counter need init
            {
                precise_counter_update(&counter_feedback, nowSampleCount_x1000, 0);
                counter_feedback.state = PRECISE_COUNTER_COUNTING;
                counter_feedback.interval_cnt = 0;
                // fill feedback fifo with an artifact
                get_UAC1_0_feedback_bytes(haudio->sample_rate_Hzs[0] << 4, (uint32_t*)bufFeedback);
                USBD_LL_FlushEP(pdev, AUDIO_SPEAKER_SYNC_EP);
                USBD_LL_Transmit(pdev, AUDIO_SPEAKER_SYNC_EP, (uint8_t*)bufFeedback, 3U);
                haudio->EPIN_tx_flags[1] = 1;
                haudio->EPIN_NSOF[1] = fnsof_new;
            }
            else // counter counts
            {
                counter_feedback.interval_cnt++;
                if(counter_feedback.interval_cnt >= AUDIO_FEEDBACK_PERIOD_MS)
                {
                    uint8_t rshift_bits = AUDIO_FEEDBACK_BINTERVAL;
                    if(haudio->sample_rate_Hzs[0] <= 48000U)
                        rshift_bits+=1;
                    size_t feedback_diff = precise_counter_update(&counter_feedback, nowSampleCount_x1000, 0); // to keep precision, do not rshift here
                    size_t feedbackHz_x16 = (feedback_diff) >> (rshift_bits - 4/*x16 = lshift 4*/ ); // 1000 = 2*2*2*5*5*5
                    counter_feedback.cum_err = feedback_diff - (feedbackHz_x16 << (rshift_bits - 4/*x16 = lshift 4*/));
                    /* fine tune the feedback Hz according to fifo free space */
                    size_t sizeFree = kfifo_get_free_space(&fifo_from_USB_playback);
                    size_t sizeFilled = kfifo_get_filled_size(&fifo_from_USB_playback);
                    if(sizeFilled < sizePreheatBytes_USB_playback * 2)
                        feedbackHz_x16 += ((sizePreheatBytes_USB_playback * 2) - sizeFilled) * 2;
                    if(sizeFree < sizePreheatBytes_USB_playback * 2)
                        feedbackHz_x16 -= ((sizePreheatBytes_USB_playback * 2) - sizeFree) * 2;
                    /* convert Hz to bytes */
                    get_UAC1_0_feedback_bytes(feedbackHz_x16, (uint32_t*)bufFeedback);
                    // if transmit FB in SOF, do not send it again in DataIn, but in ISOIncomplete
                    if(haudio->EPIN_tx_flags[1] == 0U)
                    {
                        USBD_LL_Transmit(pdev, AUDIO_SPEAKER_SYNC_EP, (uint8_t*)bufFeedback, 3U);
                        haudio->EPIN_tx_flags[1] = 1U; // Sync EPIN
                        haudio->EPIN_NSOF[1] = fnsof_new;
                    }
                    counter_feedback.interval_cnt -= AUDIO_FEEDBACK_PERIOD_MS;
                }
            }
        }
    }
    sof_count++;
    if(sof_count == 0)
        sof_count = 1;
#ifdef PROFILER
    profiler_mark(&profUSB_SOF);
#endif
    return USBD_OK;
}

/** NOT USED, buffers are filled in other threads
 * @brief  Synchronizes pointers for data sending or reception

 * @param  pdev: device instance
 * @retval status
 */
void USBD_AUDIO_Sync(USBD_HandleTypeDef *pdev, AUDIO_OffsetTypeDef offset)
{

}


/**
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 * USBD_AUDIO_IsoINIncomplete & USBD_AUDIO_IsoOutIncomplete are not
 * enabled by default.
 *
 * Go to Middlewares/ST/STM32_USB_Device_Library/Core/Src/usbd_core.c
 * Fill in USBD_LL_IsoINIncomplete and USBD_LL_IsoOUTIncomplete with
 * actual handler functions.
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 */

/**
 * @brief  USBD_AUDIO_IsoINIncomplete
 *         handle data ISO IN Incomplete event
 * @param  pdev: device instance
 * @param  epnum: endpoint index
 * @retval status
 */
static uint8_t USBD_AUDIO_IsoINIncomplete(USBD_HandleTypeDef *pdev,
        uint8_t epnum)
{
    /*!!! NOTE !!!
     * IsoInIncomplete Event means the packet buffered by USBD_LL_Transmit is not retrieved by the HOST
     * before the end of current Frame
     * It does not mean USB link is broken
     * a possible cause is the unmatched EVEN/ODD filter on FNSOF of the transmitting packet
     * if so, the packet will be retrieved by the HOST in the next frame
     * or, the Endpoint is set to be retreived by the HOST at an interval, which is common for the Feedback Endpoint
     * */

    UNUSED(pdev);
    UNUSED(epnum);

      USB_OTG_GlobalTypeDef* USBx = USB_OTG_HS;
      uint32_t USBx_BASE = (uint32_t)USBx;
      /* EP IN: flush mic */
      // NOTE: seems like ignoring the MIC iso incomplete interrupt gives the best results
      // *DO NOT FLUSH mic in EP here, or the audio will distort
      // *DO NOT retransmit here,
      // *DO NOT absorb from mic FIFO here
      // tests show that let it go will give the best results

#if 0
      if((haudio->EPIN_NSOF[0] & 0x1) != (fnsof_new & 0x1)) // if EVEN/ODD filter differs, means the packet was ought to be sent in this frame, something wrong must happen
      {
        haudio->EPIN_tx_flags[0] = 1U; // incomplete
        haudio->EPIN_NSOF[0] = fnsof_new;
        if(haudio->alt_settings[1] > 0)
        {
            uint8_t *bufEPin = haudio->bufDI;
            USBD_LL_FlushEP(pdev, AUDIO_MIC_IN_EP);
            // submit a silent block
            sizeLastPacket_mic = haudio->packet_sizes[1];
            memset(bufEPin, 0, sizeLastPacket_mic);
            USBD_LL_Transmit(pdev, AUDIO_MIC_IN_EP, bufEPin, sizeLastPacket_mic);
        }
      }
#endif
      /* EP IN: flush feedback */

#if 1
      if(haudio->EPIN_tx_flags[1] == 1U) // if EVEN/ODD filter differs, means the packet was ought to be sent in this frame, something wrong must have happened
      {
          USBD_LL_FlushEP(pdev, AUDIO_SPEAKER_SYNC_EP);
          haudio->EPIN_NSOF[1] = fnsof_new; // NOTE: Feedback is either retreived in EVEN frames or ODD frames, not alternatively.
          // send existing feedback bytes
          USBD_LL_Transmit(pdev, AUDIO_SPEAKER_SYNC_EP, bufFeedback, 3);
#ifdef PROFILER
//          profiler_mark(&profUSB_feedback);
#endif
      }
#endif
    return (uint8_t) USBD_OK;
}
/**
 * @brief  USBD_AUDIO_IsoOutIncomplete
 *         handle data ISO OUT Incomplete event
 * @param  pdev: device instance
 * @param  epnum: endpoint index
 * @retval status
 */
static uint8_t USBD_AUDIO_IsoOutIncomplete(USBD_HandleTypeDef *pdev,
        uint8_t epnum)
{
    UNUSED(pdev);
    UNUSED(epnum);

#if 0
    USBD_AUDIO_HandleTypeDef *haudio;
    haudio = (USBD_AUDIO_HandleTypeDef*) pdev->pClassData;
    if (all_ready == 1)
    {
        if((haudio->EPOUT_NSOF[0] & 0x1) != (fnsof_new & 0x1))
        {
            /* Get received data packet length */
            KFIFO_DMA* pfifo = &(haudio->FIFOs[0]);
            size_t PacketSize = USBD_LL_GetRxDataSize(pdev, AUDIO_SPEAKER_OUT_EP);
            if(PacketSize > 0)
            {
                size_t bytesFreeBeforeWrite = kfifo_put(pfifo, haudio->bufDO, PacketSize);
            }
            /* Prepare Out endpoint to receive next audio packet */
            (void) USBD_LL_PrepareReceive(pdev, AUDIO_SPEAKER_OUT_EP,
                    haudio->bufDO, AUDIO_OUT_PACKET_MAX);
            haudio->EPOUT_NSOF[0] = fnsof_new;
        }

    }
#endif
    return (uint8_t) USBD_OK;
}



static size_t USB_push_speaker_out_data_to_fifo(USBD_HandleTypeDef *pdev, uint8_t* buf, size_t sizeToPush)
{
    static int32_t bufCastToI32[AUDIO_TOTAL_BUF_SIZE / 2] = {0}; // 2x bigger than USB buffer
    USBD_AUDIO_HandleTypeDef *haudio;
    haudio = (USBD_AUDIO_HandleTypeDef*) pdev->pClassData;
//    uint8_t oversample = 1;
//    if(haudio->sample_rate_Hzs[0] <= 48000)
//        oversample = 2;
    size_t sizePadded;
    // pad to int32 format
    switch(haudio->bit_depths[0])
    {
    case 16U:
        sizePadded = pad_i16_to_i32(((int32_t*)bufCastToI32), (int16_t*)buf, sizeToPush, 1, MAX_CHANNELS);
        break;
    case 24U:
        sizePadded = pad_i24_to_i32(((int32_t*)bufCastToI32), buf, sizeToPush, 1, MAX_CHANNELS);
        break;
    default:
        memcpy(((int32_t*)bufCastToI32), buf, sizeToPush);
        sizePadded = sizeToPush; // warning: never come here
    }
    // NOTE: convert i32 to float and filter in audio output interrupts, to save USB interrupt cycles
    size_t nFreeWritten = kfifo_put(&fifo_from_USB_playback,
                                    (uint8_t*)bufCastToI32,
                                    sizePadded);

    // detect overflow
    if(nFreeWritten < sizePadded)
        fifoOverflowCounter_USB_playback += 1;

    return nFreeWritten;

}


/**
 * @brief  USBD_AUDIO_DataOut
 *         handle data OUT Stage
 * @param  pdev: device instance
 * @param  epnum: endpoint index
 * @retval status
 */
static uint8_t USBD_AUDIO_DataOut(USBD_HandleTypeDef *pdev, uint8_t epnum)
{
    size_t PacketSize;
    USBD_AUDIO_HandleTypeDef *haudio;
    haudio = (USBD_AUDIO_HandleTypeDef*) pdev->pClassData;
    KFIFO_DMA* pfifo = &(haudio->FIFOs[0]);

    if (haudio == NULL)
    {
        return (uint8_t) USBD_FAIL;
    }

    if (is_USB_playback_inited == 1 && epnum == AUDIO_SPEAKER_OUT_EP)
    {
        /* Get received data packet length */
        PacketSize = USBD_LL_GetRxDataSize(pdev, epnum);
        /* Ignore strangely large packets */
        if (PacketSize > AUDIO_OUT_PACKET_MAX) {
            global_counter_USB_speaker_loss += PacketSize - AUDIO_OUT_PACKET_MAX;
            PacketSize = AUDIO_OUT_PACKET_MAX;
        }

        /* directly write to fifo*/
        // pad to 32 bits, writ to fifo
        size_t bytesFreeBeforeWrite = USB_push_speaker_out_data_to_fifo(pdev, haudio->bufDO, PacketSize);
        // allow app to read when buffer is half filled
        haudio->rd_enables[0] = 1U;

        /* Prepare Out endpoint to receive next audio packet */
        (void) USBD_LL_PrepareReceive(pdev, AUDIO_SPEAKER_OUT_EP,
                haudio->bufDO, AUDIO_OUT_PACKET_MAX);
        haudio->EPOUT_NSOF[0] = fnsof_new;
    }

    return (uint8_t) USBD_OK;
}

/**
 * @brief  AUDIO_Req_GetCurrent
 *         Handles the GET_CUR Audio control request.
 * @param  pdev: instance
 * @param  req: setup class request
 * @retval status
 */
static void AUDIO_REQ_GetCurrent(USBD_HandleTypeDef *pdev,
        USBD_SetupReqTypedef *req)
{
    USBD_AUDIO_HandleTypeDef *haudio;
    haudio = (USBD_AUDIO_HandleTypeDef*) pdev->pClassData;

    if (haudio == NULL)
    {
        return;
    }

    (void) USBD_memset(haudio->control.data, 0, 64U);

    /* Send the current mute state */
    (void) USBD_CtlSendData(pdev, haudio->control.data, req->wLength);

}

/**
 * @brief  AUDIO_Req_SetCurrent
 *         Handles the SET_CUR Audio control request.
 * @param  pdev: instance
 * @param  req: setup class request
 * @retval status
 */
static void AUDIO_REQ_SetCurrent(USBD_HandleTypeDef *pdev,
        USBD_SetupReqTypedef *req)
{
    USBD_AUDIO_HandleTypeDef *haudio;
    haudio = (USBD_AUDIO_HandleTypeDef*) pdev->pClassData;

    if (haudio == NULL)
    {
        return;
    }

    if (req->wLength != 0U)
    {
        /* Prepare the reception of the buffer over EP0 */
        (void) USBD_CtlPrepareRx(pdev, haudio->control.data, req->wLength);

        haudio->control.cmd = AUDIO_REQ_SET_CUR; /* Set the request value */
        haudio->control.len = (uint8_t) req->wLength; /* Set the request data length */
        haudio->control.unit = HIBYTE(req->wIndex); /* Set the request target unit */
        haudio->control.epnum = LOBYTE(req->wIndex); /* Set the request target unit */
        /* USER CODE: richer information added to */
        /* Set the request type, (00000 = 接受者为设备（UAC无此值）
        00001 = 接收者为接口（UAC规范中可以理解为AC，AS中拓扑结构中的端子，单元等）
        00010 = 接受者为端点（UAC规范中为音频流接口的端点）
        00011 = 其他接受者). See UAC Spec 1.0 - 5.2.1 Request Layout
        */
        haudio->control.req_type = req->bmRequest & 0x1f;
        haudio->control.cs = HIBYTE(req->wValue); /* Set the request control selector (high byte) */
        haudio->control.cn = LOBYTE(req->wValue); /* Set the request control number (low byte) */
    }
}

/**
 * @brief  AUDIO_Req_GetMin
 *         Handles the GET_MIN Audio control request.
 * @param  pdev: instance
 * @param  req: setup class request
 * @retval status
 */
static void AUDIO_REQ_GetMin(USBD_HandleTypeDef *pdev,
        USBD_SetupReqTypedef *req)
{
    USBD_AUDIO_HandleTypeDef *haudio;
    haudio = (USBD_AUDIO_HandleTypeDef*) pdev->pClassData;

    if (haudio == NULL)
    {
        return;
    }

    (void) USBD_memset(haudio->control.data, 0, 64U);
    int16_t v;
    if ((req->bmRequest & 0x1f) == AUDIO_CONTROL_REQ)
    {
        switch (HIBYTE(req->wValue))
        {
        case AUDIO_CONTROL_REQ_FU_VOL:
            v = USBD_AUDIO_VOL_MIN;
            *((int16_t*) haudio->control.data) = v;
            break;
        }
    }

    /* Send the current mute state */
    (void) USBD_CtlSendData(pdev, haudio->control.data, req->wLength);

}

/**
 * @brief  AUDIO_Req_SetMin
 *         Handles the SET_MIN Audio control request.
 * @param  pdev: instance
 * @param  req: setup class request
 * @retval status
 */
static void AUDIO_REQ_SetMin(USBD_HandleTypeDef *pdev,
        USBD_SetupReqTypedef *req)
{
    USBD_AUDIO_HandleTypeDef *haudio;
    haudio = (USBD_AUDIO_HandleTypeDef*) pdev->pClassData;

    if (haudio == NULL)
    {
        return;
    }

    if (req->wLength != 0U)
    {
        /* Prepare the reception of the buffer over EP0 */
        (void) USBD_CtlPrepareRx(pdev, haudio->control.data, req->wLength);

        haudio->control.cmd = AUDIO_REQ_SET_MIN; /* Set the request value */
        haudio->control.len = (uint8_t) req->wLength; /* Set the request data length */
        haudio->control.unit = HIBYTE(req->wIndex); /* Set the request target unit */
    }
}

/**
 * @brief  AUDIO_Req_GetMax
 *         Handles the GET_MAX Audio control request.
 * @param  pdev: instance
 * @param  req: setup class request
 * @retval status
 */
static void AUDIO_REQ_GetMax(USBD_HandleTypeDef *pdev,
        USBD_SetupReqTypedef *req)
{
    USBD_AUDIO_HandleTypeDef *haudio;
    haudio = (USBD_AUDIO_HandleTypeDef*) pdev->pClassData;

    if (haudio == NULL)
    {
        return;
    }

    (void) USBD_memset(haudio->control.data, 0, 64U);
    int16_t v;
    if ((req->bmRequest & 0x1f) == AUDIO_CONTROL_REQ)
    {
        switch (HIBYTE(req->wValue))
        {
        case AUDIO_CONTROL_REQ_FU_VOL:
            v = USBD_AUDIO_VOL_MAX;
            *((int16_t*) haudio->control.data) = v;
            break;
        }
    }

    /* Send the current mute state */
    (void) USBD_CtlSendData(pdev, haudio->control.data, req->wLength);

}

/**
 * @brief  AUDIO_Req_SetMax
 *         Handles the SET_Max Audio control request.
 * @param  pdev: instance
 * @param  req: setup class request
 * @retval status
 */
static void AUDIO_REQ_SetMax(USBD_HandleTypeDef *pdev,
        USBD_SetupReqTypedef *req)
{
    USBD_AUDIO_HandleTypeDef *haudio;
    haudio = (USBD_AUDIO_HandleTypeDef*) pdev->pClassData;

    if (haudio == NULL)
    {
        return;
    }

    if (req->wLength != 0U)
    {
        /* Prepare the reception of the buffer over EP0 */
        (void) USBD_CtlPrepareRx(pdev, haudio->control.data, req->wLength);

        haudio->control.cmd = AUDIO_REQ_SET_MAX; /* Set the request value */
        haudio->control.len = (uint8_t) req->wLength; /* Set the request data length */
        haudio->control.unit = HIBYTE(req->wIndex); /* Set the request target unit */
    }
}

/**
 * @brief  AUDIO_Req_GetRes
 *         Handles the GET_RES Audio control request.
 * @param  pdev: instance
 * @param  req: setup class request
 * @retval status
 */
static void AUDIO_REQ_GetRes(USBD_HandleTypeDef *pdev,
        USBD_SetupReqTypedef *req)
{
    USBD_AUDIO_HandleTypeDef *haudio;
    haudio = (USBD_AUDIO_HandleTypeDef*) pdev->pClassData;

    if (haudio == NULL)
    {
        return;
    }

    (void) USBD_memset(haudio->control.data, 0, 64U);
    int16_t v;
    if ((req->bmRequest & 0x1f) == AUDIO_CONTROL_REQ)
    {
        switch (HIBYTE(req->wValue))
        {
        case AUDIO_CONTROL_REQ_FU_VOL:
            v = USBD_AUDIO_VOL_STEP;
            *((int16_t*) haudio->control.data) = v;
            break;
        }
    }

    (void) USBD_CtlSendData(pdev, haudio->control.data, req->wLength);
}

/**
 * @brief  DeviceQualifierDescriptor
 *         return Device Qualifier descriptor
 * @param  length : pointer data length
 * @retval pointer to descriptor buffer
 */
static uint8_t* USBD_AUDIO_GetDeviceQualifierDesc(uint16_t *length)
{
    *length = (uint16_t) sizeof(USBD_AUDIO_DeviceQualifierDesc);

    return USBD_AUDIO_DeviceQualifierDesc;
}

/**
 * @brief  USBD_AUDIO_RegisterInterface
 * @param  fops: Audio interface callback
 * @retval status
 */
uint8_t USBD_AUDIO_RegisterInterface(USBD_HandleTypeDef *pdev,
        USBD_AUDIO_ItfTypeDef *fops)
{
    if (fops == NULL)
    {
        return (uint8_t) USBD_FAIL;
    }

    pdev->pUserData = fops;

    return (uint8_t) USBD_OK;
}
/**
 * @}
 */
uint32_t get_audio_feedback_Hz_from_fifo_free(int32_t fsHz, int32_t sizeFree,
        int32_t sizeTotal)
{

    int32_t fsMaxDelta = fsHz >> 4; // 1/32, +-~3%
    int32_t fsLow = fsHz - fsMaxDelta;
    int32_t fsHigh = fsHz + fsMaxDelta;

    int sizeHalf = sizeTotal >> 1;
//    int sizeQ = sizeTotal >> 2; // 8192

//    if((sizeFree < sizeHalf / 2 ) || (sizeFree > (sizeHalf + sizeHalf / 2)))
//    {
        int deltaHz = ((sizeFree - sizeHalf) * fsMaxDelta / ((int)(USB_PLAYBACK_BUF_SIZE >> 2)));
        fsHz = fsHz + deltaHz;
//    }


    if (fsHz < fsLow)
        fsHz = fsLow;
    else if (fsHz > fsHigh)
        fsHz = fsHigh;

    // higher feedback value, faster the host would transfer
    return fsHz;

}


uint8_t get_UAC1_0_feedback_bytes(uint32_t measuredFS_x16, uint32_t *buf)
{
    // write 10.14 format feedback data into buf
    uint8_t numbytes = 3;
    uint32_t kHz = (measuredFS_x16 >> 4) / 1000;
    uint32_t Hz_x16 = measuredFS_x16 - ((kHz*1000) << 4);
    uint32_t feedback = (kHz << 14) | (Hz_x16 & ((1U << 14) - 1));
    *buf = feedback;
    return numbytes;

}



/**
 * @}
 */

/**
 * @}
 */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
