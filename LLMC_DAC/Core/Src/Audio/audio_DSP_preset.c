/*
 * audio_DSP_preset.c
 *
 *  Created on: 2025年1月21日
 *      Author: cpholzn
 */
#include "audio_DSP.h"
#include "ff.h"
#include "LVGL_GUI.h"

int save_DSP_preset(audio_DSP_t* pDSP, int idxPreset)
{
	int r = 0;
	if (idxPreset == 0)
	{
		r = -1;
		goto SAVE_DSP_PRESET_FAILED;
	}

	audio_DSP_cfg_t cfgDSP;
	audio_DSP_extrct_cfg(pDSP, &cfgDSP);

	// check folder existance
	FRESULT fr;
	DIR dir;
	// make dir if not exist
	fr = f_opendir(&dir, PATH_DSP_PROFILE_FOLDER);
	if(fr == FR_OK)
	{
		f_closedir(&dir);
	}
	else
	{
		fr = f_mkdir(PATH_DSP_PROFILE_FOLDER);
		if(fr != FR_OK)
		{
			r = -2;
			goto SAVE_DSP_PRESET_FAILED;
		}

	}

	// build path
	TCHAR wcsProfilePath[64];
	TCHAR *wcsID = strncpy_TCHAR(wcsProfilePath,
			PATH_DSP_PROFILE_FOLDER _T("/") FILENAME_DSP_PROFILE, 62);
	// add number to path
	*wcsID = _T('0') + idxPreset;
	// terminate the string
	*(wcsID + 1) = 0;

	fr = f_open(&f, wcsProfilePath, FA_WRITE | FA_CREATE_ALWAYS);
	if(fr != 0)
	{
		r = -2;
		goto SAVE_DSP_PRESET_FAILED;
	}

	size_t bytesWritten;
	fr = f_write(&f, &cfgDSP, sizeof(audio_DSP_cfg_t), &bytesWritten);
	if((fr != 0) || (bytesWritten < sizeof(audio_DSP_cfg_t)))
	{
		r = -3;
		goto SAVE_DSP_PRESET_FAILED;
	}

	f_close(&f);

SAVE_DSP_PRESET_FAILED:
	return r;

}

int load_and_apply_DSP_preset(audio_DSP_t* pDSP, int idxPreset)
{
	int r = 0;
	if (idxPreset == 0)
	{
		r = -1;
		goto LOAD_DSP_PRESET_FAILED;
	}

	/* make path */
	// build path
	TCHAR wcsProfilePath[64];
	TCHAR *wcsID = strncpy_TCHAR(wcsProfilePath,
			PATH_DSP_PROFILE_FOLDER _T("/") FILENAME_DSP_PROFILE, 62);
	// add number to path
	*wcsID = _T('0') + idxPreset;
	// terminate the string
	*(wcsID + 1) = 0;
	/* open file */
	// try to open the file
	FRESULT fr = f_open(&f, wcsProfilePath, FA_READ | FA_OPEN_EXISTING);
	if (fr != FR_OK)
	{
		r = -2;
		goto LOAD_DSP_PRESET_FAILED;
	}

	size_t lenRead;
	fr = f_read(&f, bufFile, sizeof(audio_DSP_cfg_t), &lenRead);
	if ((fr != FR_OK) || (lenRead < sizeof(audio_DSP_cfg_t)))
	{
		r = -3;
		goto LOAD_DSP_PRESET_FAILED;
	}
	f_close(&f);

	audio_DSP_cfg_t cfgDSP;
	memcpy(&cfgDSP, bufFile, sizeof(audio_DSP_cfg_t));

	__disable_irq();
	audio_DSP_init(pDSP, &cfgDSP);
	__enable_irq();

LOAD_DSP_PRESET_FAILED:
	return r;
}
