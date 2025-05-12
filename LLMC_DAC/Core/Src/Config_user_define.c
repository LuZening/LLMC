/*
 * Config_user_define.c
 *
 *  Created on: Sep 5, 2023
 *      Author: cpholzn
 */

#include <string.h>
#include "Config/Config.h"
#include "Config_user_define.h"
#include "audio_settings.h"
#include "ff.h"



const char* VALID_STRING = "LLMCDEDICATEDFORYOU";

// use '.' to indicate the positon in an array
// for '.' is well supported in HTML URL encoding
// either '()' or '[]' will be converted in HTML URL
// WARNING: names can't be longer than 8bytes
config_var_map_t configNameMapper[CONFIG_NUM_OF_VARS] = {
//    {"TSCalibInfo", &(cfg.TSCalibInfo), CONFIG_VAR_OBJ},
//    {"sel.2", &(cfg.nRadioToAntNums[1]), CONFIG_VAR_U8},
//	{"label.1", &(cfg.sAntNames[0]), CONFIG_VAR_BYTESTRING_LONG},
//	{"label.2", &(cfg.sAntNames[1]), CONFIG_VAR_BYTESTRING_LONG},
//	{"label.3", &(cfg.sAntNames[2]), CONFIG_VAR_BYTESTRING_LONG},
//	{"label.4", &(cfg.sAntNames[3]), CONFIG_VAR_BYTESTRING_LONG},
//	{"label.5", &(cfg.sAntNames[4]), CONFIG_VAR_BYTESTRING_LONG},
//	{"label.6", &(cfg.sAntNames[5]), CONFIG_VAR_BYTESTRING_LONG},
		0

};

void init_config(Config *p)
{
    int i;
//    if(mtxConfig == NULL) mtxConfig = osMutexNew(NULL);
    // set valid string
    strncpy(p->sValid, VALID_STRING,sizeof(p->sValid)-1);
    // set config inital values
    memset(&(p->TSCalibInfo), 0, sizeof(TouchScreenCalibrationInfo_t));

    p->audio_sample_rate = DEFAULT_SAMPLE_RATE_T;
    p->audio_sample_rate_Hz = DEFAULT_SAMPLE_RATE_HZ;
    p->audio_bit_depth = 24U;


    p->nVolume100_decoder = 60;
    p->nVolume100_encoder[0] = 100; // by default the microphone has no attenuation
    p->nVolume100_encoder[1] = 100; // by default the microphone has no attenuation
    p->nVolume100_FS = 60;
    p->nPregainsDB2_encoder[0] = 12; // +6db by default
    p->nPregainsDB2_encoder[1] = 12; // +6db by default
    memset(p->audio_connections , 0 , sizeof(p->audio_connections));
    memset(p->outputs_dsp_usage, 0, sizeof(p->outputs_dsp_usage));
    memset(p->dsp_preset_ids, 0, sizeof(p->dsp_preset_ids));
    p->usbCfgDescSelector = USB_CFG_DESC_MASS_STORAGE;
    p->USB_record_audio_sample_rate_Hz = DEFAULT_SAMPLE_RATE_HZ;
    p->USB_record_audio_bit_depth = 24U;

    p->USB_volume_control = 0; // do not allow volume control through USB
    p->USB_record_force_mono = 0;
    p->USB_self_powered = 1;
    isModified = false;

}

bool config_check_valid(Config* p)
{

    // VALID STRING
	uint8_t check = strncmp(p->sValid, VALID_STRING, sizeof(p->sValid));
	if(check != 0)
		goto CONFIG_CHECK_FAILED;

	// sample rate
	int i;
	for(i = 0; i < N_VALID_AUDIO_SAMPLE_RATES; ++i)
	{
		if(cfg.USB_record_audio_sample_rate_Hz == arr_valid_audio_sample_rates_Hz[i])
			break;
	}
	if(i >= N_VALID_AUDIO_SAMPLE_RATES)
		goto CONFIG_CHECK_FAILED;

	// bit depth
	for(i = 0; i < N_VALID_AUDIO_BIT_DEPTHS; ++i)
	{
		if(cfg.USB_record_audio_bit_depth == arr_valid_audio_bit_depths[i])
			break;
	}
	if(i >= N_VALID_AUDIO_BIT_DEPTHS)
		goto CONFIG_CHECK_FAILED;

	/* DSP setting */
	// EQ


	return true;
CONFIG_CHECK_FAILED:
    return false;

}

