/*
 * Config_params.h
 *
 *  Created on: Sep 5, 2023
 *      Author: cpholzn
 */

#ifndef CONFIG_PARAMS_H_
#define CONFIG_PARAMS_H_

#include <stdint.h>

#ifndef FREERTOS
#define FREERTOS 1
#define USE_MUTEX_ON_CFG 1
#endif
#include "audio_settings.h"
#include "audio_buffers.h"
#include "touch_HR2046.h"
#include "MyUSB.h"
// by default use EEPROM 24C64 with 8KBytes capacity
//#define USE_24C_EEPROM
//#define USE_STM32_FLASH_EEPROM
#define USE_STM32_FATFS
#define FS_FOLDER_SYS L"0:/sys"
#define FS_PATH_CONFIG_FILE  L"0:/sys/config"
/* do not use float parameters */
// type of variables: uint8_t, int, char
#define CONFIG_VAR_NAME_LEN 32
#define CONFIG_BYTESTRING_LEN 32
#define CONFIG_BYTESTRING_LONG_LEN 256

#define CONFIG_NUM_OF_VARS 3

typedef struct
{
    // check if storage is valid
    char sValid[CONFIG_BYTESTRING_LEN];
    // TSCalib
    TouchScreenCalibrationInfo_t TSCalibInfo;
    /* Audio params */
    audio_sample_rate_t audio_sample_rate; // internal Fs, use enum type
    uint32_t audio_sample_rate_Hz; // internal Fs, in Hz unit
    uint8_t audio_bit_depth; // internal bitdepth,
    /* audio_connections */
    // outputs
    // audio_connections[0] = Decoder
    // audio_connections[1] = USB
    // inputs
    // bit0 "麦克风1", LSB
    // bit1 "麦克风2",
    // bit2 "USB播放",
    // bit3 "文件播放"
    uint16_t audio_connections[N_OUTPUTS];
    uint16_t outputs_dsp_usage[N_OUTPUTS]; // dry/wet distributions
    uint8_t dsp_preset_ids[N_INPUTS]; // each input can have a DSP. each DSP can have a preset profile numbering from 1 to 255
    uint8_t nVolume100_decoder;
    uint8_t nVolume100_encoder[2];
    uint8_t nVolume100_FS;
    uint8_t nPregainsDB2_encoder[2];
    USBCfgDescSelector_t usbCfgDescSelector;
    uint32_t USB_record_audio_sample_rate_Hz; // USB Fs, in Hz unit
    uint8_t USB_record_audio_bit_depth; // internal bitdepth,
    uint8_t USB_volume_control; // 1: allow volume control over USB
    uint8_t USB_record_force_mono; // 1: mono, 0: unchanged
    uint8_t USB_self_powered;
} Config; // size of config is around 192Bytes



extern Config cfg;


#endif /* CONFIG_PARAMS_H_ */
