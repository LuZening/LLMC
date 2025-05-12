/*
 * LVGL_GUI_signal_path.c
 *
 *  Created on: May 23, 2024
 *      Author: cpholzn
 */

#ifdef __cplusplus
extern "C" {
#endif


#include "LVGL_GUI.h"

#ifndef LVGL_SIM
#include "cmsis_os2.h"
#include "Config.h"
#include "audio_buffers.h"
#include "audio_DSP.h"
#include "audio_player.h"
#include "audio_settings.h"
#include "PCM179x.h"
#include "TLV320ADCx120.h"
#include "async_i2c.h"
#endif
static char sValue[8];
/************************ Volume control bar BEGIN ************************/
bool isVolumeControlCreated = false;
lv_group_t* lvGroupVolumeControl = NULL;
static lv_obj_t* lvCont = NULL;
#define N_CONTROLS 4U
#ifndef LVGL_SIM
/* Digital volumes */
// when change volume, change the target volume first, and the real volume will be updated in another routine function
uint8_t* arr_pvolumes100_target[N_CONTROLS] = { &cfg.nVolume100_decoder, &cfg.nVolume100_encoder[0], &cfg.nVolume100_encoder[1], &cfg.nVolume100_FS};
// stores the the true volume now (but integer, must be non-linearized before apply to the audio processors)
uint8_t arr_volumes100_now[N_CONTROLS] = {0};
// control the float valued volume which actually applied to scale the processed signal
float* arr_pvolumesf_now[N_CONTROLS] = {&volumef_decoder, &volumefs_encoder[0], &volumefs_encoder[1], &volumef_FS};

/* levels */
float* arr_plevels[N_MICROPHONES] = {&arr_level_trackers_encoder[0].peak, &arr_level_trackers_encoder[1].peak};

/* Pregains */
uint8_t* arr_ppregainsDB2_target[N_MICROPHONES] = {&cfg.nPregainsDB2_encoder[0], &cfg.nPregainsDB2_encoder[1]};
uint8_t arr_pregainsDB2_now[N_MICROPHONES] = {0};

#else
#include <math.h>

#ifndef N_MICROPHONES
#define N_MICROPHONES 2
#endif
uint8_t volumes100[N_CONTROLS] = { 0 };
float volume100_to_volumef(uint8_t volume100)
{
	float vf = 0.;
	// db ranges between -60db ~ 0db
	if (volume100 > 0)
	{
		float db = ((float)volume100 - 100.f) * 60.f / 100.f;
		vf = powf(10, db / 20);
	}
	return vf;
}
float volumef[N_CONTROLS] = { 0 };
// when change volume, change the target volume first, and the real volume will be updated in another routine function
uint8_t* arr_pvolumes100_target[N_CONTROLS] = { &volumes100[0],  &volumes100[1], &volumes100[2], &volumes100[3] };
// stores the the true volume now (but integer, must be non-linearized before apply to the audio processors)
uint8_t arr_volumes100_now[N_CONTROLS] = {0};
float* arr_pvolumesf_now[N_CONTROLS] = {&volumef[0], &volumef[1], &volumef[2], &volumef[3]};
float levels[N_MICROPHONES] = {0.1, 0.02};
float* arr_plevels[N_MICROPHONES] = {&levels[0], &levels[1]};
uint8_t pregainDB2_1, pregainDB2_2;
uint8_t* arr_ppregainsDB2_target[N_MICROPHONES] = {&pregainDB2_1, &pregainDB2_2};
uint8_t arr_pregainsDB2_now[N_MICROPHONES] = {0};
#endif
bool arr_volumes_changed[N_CONTROLS] = { false };
bool arr_pregains_changed[N_MICROPHONES] = {false};
lv_obj_t* arr_level_bars[N_MICROPHONES] = {NULL};
const uint8_t arr_volumes100_range_max[N_CONTROLS] = { 100, 100, 100, 100 };
const uint8_t arr_pregainsDB2_range_max[N_MICROPHONES] = { 80, 80 };
static lv_timer_t* timer = NULL;
#define CNT_DISMISS 200U //  unit: 100ms, how long to wait before dismiss
static uint16_t cntNothingChanged = 0; // counts when to dismiss volume control

// call at MCU boot as early as possible
void init_volume_control()
{
	for(uint8_t i = 0; i < N_CONTROLS; ++i)
	{
		arr_volumes100_now[i] = 0;
		arr_volumes_changed[i] = true; // need a sync right after init
	}

}

void sync_volumes(bool nonblock)
{
	uint8_t data[4];
	/* process volumes */
	for(uint8_t i = 0; i < N_CONTROLS; ++i)
	{
		uint8_t nVol100_tg = *arr_pvolumes100_target[i];
		uint8_t nVol100_old = arr_volumes100_now[i];
		if(nVol100_tg != nVol100_old)
		{
#ifndef LVGL_SIM
			if(nonblock) // nonblock method
			{
				if(i == 0)
				{
					// set decoder volume
					uint8_t v = volume100_to_255(volume100_linear_to_log(nVol100_tg));
//					PCM179x_set_attenuation(&pcm179x, v);
					uint8_t len = PCM179x_set_attenuation_async(&pcm179x, v, data);
					transmit_async_i2c(0, data, len);
				}
				else if(i == 1)
				{
					// set encoder mic1(ch2) volume
//					adcx120_set_volume(&adcx120, 2, volume100_linear_to_log(nVol100_tg));
					uint8_t len = adcx120_set_volume_async(&adcx120, 2, volume100_linear_to_log(nVol100_tg), data);
					transmit_async_i2c(1, data, len);

				}
				else if(i == 2)
				{
					// set encoder mic2(ch1) volume
//					adcx120_set_volume(&adcx120, 1, volume100_linear_to_log(nVol100_tg));
					uint8_t len = adcx120_set_volume_async(&adcx120, 1, volume100_linear_to_log(nVol100_tg), data);
					transmit_async_i2c(1, data, len);
				}
				else if(i == 3)
				{
					// set FS volume
				}
			}
			else // blocked method
			{
				if(i == 0)
				{
					// set decoder volume
					PCM179x_set_attenuation(&pcm179x, volume100_to_255(volume100_linear_to_log(nVol100_tg)));
				}
				else if(i == 1)
				{
					// set encoder mic1(ch2) volume
					adcx120_set_volume(&adcx120, 2, volume100_linear_to_log(nVol100_tg));

				}
				else if(i == 2)
				{
					// set encoder mic2(ch1) volume
					adcx120_set_volume(&adcx120, 1, volume100_linear_to_log(nVol100_tg));
				}
				else if(i == 3)
				{
					// set FS volume
				}
			}
#endif
			// update float
			if(arr_pvolumesf_now[i])
				*arr_pvolumesf_now[i] = volume100_to_volumef(nVol100_tg);
			arr_volumes100_now[i] = nVol100_tg;
			arr_volumes_changed[i] = false;
		}
	}
	/* process pregains */
	for(uint8_t i = 0; i < N_MICROPHONES; ++i)
	{
		uint8_t n_target = *arr_ppregainsDB2_target[i];
		uint8_t n_old = arr_pregainsDB2_now[i];
		if(n_target != n_old)
		{
#ifndef LVGL_SIM
			if(nonblock) // nonblock method
			{
				if(i == 0)
				{
					// set decoder gain
//					adcx120_set_gain(&adcx120, 2, n_target);
					uint8_t len = adcx120_set_gain_async(&adcx120, 2, n_target, data);
					transmit_async_i2c(1, data, len);
				}
				else if(i == 1)
				{
					// set encoder mic1(ch2) gain
//					adcx120_set_gain(&adcx120, 1, n_target);
					uint8_t len = adcx120_set_gain_async(&adcx120, 1, n_target, data);
					transmit_async_i2c(1, data, len);
				}
			}
			else
			{
				if(i == 0)
				{
					// set decoder gain
					adcx120_set_gain(&adcx120, 2, n_target);
				}
				else if(i == 1)
				{
					// set encoder mic1(ch2) gain
					adcx120_set_gain(&adcx120, 1, n_target);
				}
			}
#endif
			arr_pregainsDB2_now[i] = n_target;
			arr_pregains_changed[i] = false;
		}
	}
}



static void dismiss_volume_control_screen()
{
	// stop timer first
	if (timer)
	{
		lv_timer_delete(timer);
		timer = NULL;
	}

	// destroy widgets
	if (lvCont)
	{
		lv_obj_del_async(lvCont);
		lvCont = NULL;
		isVolumeControlCreated = false;
		deregister_group(lvGroupVolumeControl);
		lvGroupVolumeControl = NULL;

	}
}


static void volume_change_event_cb(lv_event_t* e)
{

	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t* slider = lv_event_get_target(e);
	// sent to cb function: index
	int idxVolume = (int)lv_event_get_user_data(e);
	// the value label object is carried by the slider
	lv_obj_t* label = lv_obj_get_user_data(slider);
	// update label text
	uint8_t nVolume_new = lv_slider_get_value(slider);
	number2text(sValue, nVolume_new, 0, 0);
	lv_label_set_text(label, sValue);
	// update volume variable
	arr_volumes_changed[idxVolume] = true;
	if (arr_pvolumes100_target[idxVolume])
	{
		*arr_pvolumes100_target[idxVolume] = nVolume_new;
#ifndef LVGL_SIM
		isModified = true;
#endif
	}

}


static void reset_audio_analog_chips_event_cb(lv_event_t* e)
{
	audio_analog_chips_need_reset = true;
}


static void pregains_change_event_cb(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t* slider = lv_event_get_target(e);
	// sent to cb function: index
	int idx = (int)lv_event_get_user_data(e);
	// update volume variable
	uint8_t nNew = lv_slider_get_value(slider);
	uint16_t dbx10 = (uint16_t)nNew * 10 / 2;
	// the value label object is carried by the slider
	lv_obj_t* label = lv_obj_get_user_data(slider);
	// update label text
	uint8_t lenWrite = number2text(sValue, dbx10, 1, 0);
	sValue[lenWrite++] = 'd'; sValue[lenWrite++] = 'B'; sValue[lenWrite] = 0;
	lv_label_set_text(label, sValue);
	// update the modify
	arr_pregains_changed[idx] = true;
	if (arr_ppregainsDB2_target[idx])
	{
		*arr_ppregainsDB2_target[idx] = nNew;
#ifndef LVGL_SIM
		isModified = true;
#endif
	}

}

static void value_update_timer_cb(lv_timer_t* timer)
{
	//  call each 200ms to check if the window can be dismissed
	uint8_t nChanged = 0;
	for (int i = 0; i < N_CONTROLS; ++i)
	{
		if (arr_volumes_changed[i])
		{
			nChanged++;
			break;
		}
	}
	for (int i = 0; i < N_MICROPHONES; ++i)
	{
		if (arr_pregains_changed[i])
		{
			nChanged++;
			break;
		}
	}
	if (nChanged == 0)
	{
		cntNothingChanged++;
		if (cntNothingChanged >= CNT_DISMISS)
			dismiss_volume_control_screen();
	}
	else
	{
		sync_volumes(true); // non-block update
		cntNothingChanged = 0;
	}
	/* update level progressbars */
	const int MIN_DB = (-255 / 3); // -85db
	const int MAX_DB = 0;
	for (int i = 0; i < N_MICROPHONES; ++i)
	{
		if (arr_plevels[i] && arr_level_bars[i])
		{
			lv_obj_t* bar = arr_level_bars[i];
			lv_obj_t* label = (lv_obj_t*)lv_obj_get_user_data(bar);
			float level = *arr_plevels[i]; // 0~1
			int db, progress; // 0 ~ 255
			if (level > 0)
			{
				db = (log10f(level) * 20);
				progress = (db + (MAX_DB - MIN_DB)) * (255 / (MAX_DB - MIN_DB)); // 0db -> 255, -85db -> 0
				if (progress < 0)
					progress = 0;
			}
			else
			{
				db = MIN_DB;
				progress = 0;
			}
			lv_bar_set_value(bar, progress, LV_ANIM_OFF);
			uint8_t lenWrite = number2text(sValue, db, 0, 0);
			sValue[lenWrite++] = 'd'; sValue[lenWrite++] = 'B'; sValue[lenWrite] = 0;
			lv_label_set_text(label, sValue);
		}
	}
}


void display_volume_control_screen()
{


	if (isVolumeControlCreated)
		goto EXIT_VOLUME_CONTROL_SCREEN;

	isVolumeControlCreated = true;

	#define MARGIN_TO_BORDER_X	30
	#define MARGIN_TO_BORDER_Y  10

	lvGroupVolumeControl = lv_group_create();
	register_group(lvGroupVolumeControl);

	/* 主界面背景 BEGIN */

	// backgound
	lvCont = lv_obj_create(lv_layer_sys()); // always on top
	lv_obj_set_size(lvCont, LV_PCT(98), LV_PCT(98));
	lv_obj_set_style_bg_opa(lvCont, LV_OPA_COVER, 0);
	lv_obj_center(lvCont); // put to the most lower-right corner
	// Create the close button
	lv_obj_t *close_btn = lv_btn_create(lvCont);
	lv_group_add_obj(lvGroupVolumeControl, close_btn);
	lv_obj_set_size(close_btn, 50, 30);
	lv_obj_set_pos(close_btn, 10, 10);
	// user_data： send the idx_DSP as a parameter to the event callback, so the prest of the target DSP could be saved in the callback
	lv_obj_add_event_cb(close_btn, dismiss_volume_control_screen, LV_EVENT_CLICKED, NULL);
	lv_obj_t *close_label = lv_label_create(close_btn);
	lv_label_set_text(close_label, "关闭");
	// create reset PCM1792 button
	lv_obj_t *reset_pcm1792_btn = lv_btn_create(lvCont);
	lv_group_add_obj(lvGroupVolumeControl, reset_pcm1792_btn);
	lv_obj_set_size(reset_pcm1792_btn, 80, 30);
	lv_obj_set_pos(reset_pcm1792_btn, 160, 10);
	// user_data： send the idx_DSP as a parameter to the event callback, so the prest of the target DSP could be saved in the callback
	lv_obj_add_event_cb(reset_pcm1792_btn, reset_audio_analog_chips_event_cb, LV_EVENT_CLICKED, NULL);
	lv_obj_t *reset_pcm1792_label = lv_label_create(reset_pcm1792_btn);
	lv_label_set_text(reset_pcm1792_label, "重置DAC");
	// content container - with grid layout
	lv_obj_t* subcont = lv_obj_create(lvCont);
	lv_obj_set_size(subcont, LV_PCT(100), LV_PCT(80));
	lv_obj_add_style(subcont, &lvStyleInvisibleCont, 0);
	lv_obj_align(subcont, LV_ALIGN_BOTTOM_LEFT, 0, 0); // put to the most lower-right corner
	lv_obj_set_layout(subcont, LV_LAYOUT_GRID);
	lv_obj_remove_flag(subcont, LV_OBJ_FLAG_SCROLLABLE);
//	lv_obj_remove_flag(lvCont, LV_OBJ_FLAG_CLICKABLE);
	// grid layout, rows: outputs, index: inputs

	// 3-controls: decoder_out (headphone), encoder_in (microphone) 1, encoder_in (microphone) 1, FS_player
	// icons 20x20 pixels
//	static const char* arrPathImgs[N_CONTROLS] = { LV_SYMBOL_VOLUME_MAX, "麦克风1", "麦克风2", LV_SYMBOL_AUDIO};

	#define HEIGHT_MARGIN 10
	#define SLIDER_HEIGHT 10
	#define SLIDER_WIDTH 100
	/* 6 columns
	 * 3 columns each group x 2 groups
	 * */
	static const int32_t column_dsc[] = { LV_GRID_FR(2), LV_GRID_FR(3), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(3), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };
	/* 6 rows
	 * 听觉音量
	 * 麦克风1电平
	 * 麦克风1音量 麦克风1增益
	 * 麦克风2电平
	 * 麦克风2音量 麦克风2增益
	 * 文件音量 */
	static const int32_t row_dsc[] = { LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };
	static const char* arrCaptions[] = {"耳机音量", "麦克风1电平", "音量", "增益", "麦克风2电平", "音量", "增益", "音乐播放音量"};
	static const char* arrItemTypes[] = { "volume", "level", "volume", "gain", "level", "volume", "gain", "volume" };
	static const uint8_t arrNCols[] = {1,1,2,1,2,1};
	static const uint8_t NROWS = sizeof(row_dsc) / sizeof(int32_t) - 1;
	static const uint8_t NITEMS = sizeof(arrCaptions) / sizeof(char*);
	lv_obj_set_grid_dsc_array(subcont, column_dsc, row_dsc);

	// put widgets of control bars and image
	int i = 0, irow = 0, icol =0, ivol = 0, ilevel = 0, igain = 0;
	for ( irow = 0; (irow < NROWS) && (i < NITEMS); ++irow)
	{
		for(icol = 0; (icol < arrNCols[irow]) && (i < NITEMS); ++icol)
		{
			// no subcont
			const char* sCaption = arrCaptions[i];
			const char* sItemType = arrItemTypes[i];
			// widget 1: caption
			lv_obj_t* caption = lv_label_create(subcont);
			lv_obj_add_style(caption, &lvStyleTextSmall, 0);
			lv_obj_set_grid_cell(caption, LV_GRID_ALIGN_END, icol * 3 + 0, 1, LV_GRID_ALIGN_CENTER, irow, 1);
			lv_label_set_text(caption, sCaption);
			// sync initial value with set volume numder
			if(strcmp(sItemType, "volume") == 0)
			{
				// widget 2: volume slider
				lv_obj_t* slider = lv_slider_create(subcont);
				lv_obj_set_size(slider, SLIDER_WIDTH, SLIDER_HEIGHT); // height > width, verticle automatically
				lv_obj_set_grid_cell(slider, LV_GRID_ALIGN_START, icol * 3+1, 1, LV_GRID_ALIGN_CENTER, irow, 1);
				lv_slider_set_mode(slider, LV_SLIDER_MODE_NORMAL);
				lv_obj_add_flag(slider, LV_OBJ_FLAG_ADV_HITTEST); // set the slider to react on dragging the knob only
				lv_slider_set_range(slider, 0, arr_volumes100_range_max[ivol]);
				// sent to cb function: the index of the control
				lv_obj_add_event_cb(slider, volume_change_event_cb, LV_EVENT_VALUE_CHANGED, (void*)ivol);
				lv_group_add_obj(lvGroupVolumeControl, slider);
				// widget 3: value label
				lv_obj_t* label = lv_label_create(subcont);
				lv_obj_add_style(label, &lvStyleTextSmall, 0);
				lv_obj_set_grid_cell(label, LV_GRID_ALIGN_START, icol * 3+2, 1, LV_GRID_ALIGN_CENTER, irow, 1);
				if(arr_pvolumes100_target[ivol])
				{
					lv_slider_set_value(slider, *(arr_pvolumes100_target[ivol]), LV_ANIM_OFF);
					number2text(sValue, *(arr_pvolumes100_target[ivol]), 0, 0);
					lv_label_set_text(label, sValue);
				}
				// let the slider carry the value label object as user data, so the label's text can be modified on the fly
				lv_obj_set_user_data(slider, (void*)label);
				// prioritize the focus on the headphone volume, so the user can quickly change it upon open
				if (ivol == 0)
				{
					lv_group_focus_obj(slider);
					lv_group_set_editing(lvGroupVolumeControl, true);
				}
				ivol++;
			}
			else if(strcmp(sItemType, "gain") == 0)
			{
				// widget 2: gain slider
				lv_obj_t* slider = lv_slider_create(subcont);
				lv_obj_set_size(slider, SLIDER_WIDTH, SLIDER_HEIGHT); // height > width, verticle automatically
				lv_obj_set_grid_cell(slider, LV_GRID_ALIGN_START, icol * 3+1, 1, LV_GRID_ALIGN_CENTER, irow, 1);
				lv_slider_set_mode(slider, LV_SLIDER_MODE_NORMAL);
				lv_obj_add_flag(slider, LV_OBJ_FLAG_ADV_HITTEST); // set the slider to react on dragging the knob only
				lv_slider_set_range(slider, 0, arr_pregainsDB2_range_max[igain]);
				// sent to cb function: the index of the control
				lv_obj_add_event_cb(slider, pregains_change_event_cb, LV_EVENT_VALUE_CHANGED, (void*)igain);
				lv_group_add_obj(lvGroupVolumeControl, slider);
				// widget 3: value label
				lv_obj_t* label = lv_label_create(subcont);
				lv_obj_add_style(label, &lvStyleTextSmall, 0);
				lv_obj_set_grid_cell(label, LV_GRID_ALIGN_START, icol * 3+2, 1, LV_GRID_ALIGN_CENTER, irow, 1);
				if(arr_ppregainsDB2_target[igain])
				{
					lv_slider_set_value(slider, *(arr_ppregainsDB2_target[igain]), LV_ANIM_OFF);
					uint16_t dbx10 = (uint16_t)(*(arr_ppregainsDB2_target[igain])) * 10 / 2;
					uint8_t lenWrite = number2text(sValue, dbx10, 1, 0);
					sValue[lenWrite++] = 'd'; sValue[lenWrite++] = 'B'; sValue[lenWrite] = 0;
					lv_label_set_text(label, sValue);
				}
				// let the slider carry the value label object as user data, so the label's text can be modified on the fly
				lv_obj_set_user_data(slider, (void*)label);
				igain++;
			}
			else if(strcmp(sItemType, "level") == 0)
			{
				// widget 2: progress bar
				lv_obj_t* bar = lv_bar_create(subcont);
				lv_obj_set_size(bar, SLIDER_WIDTH, SLIDER_HEIGHT);
				lv_bar_set_range(bar, 0, 0xff); // 0 mean -inf db or level=0, 0xff means 0db or level=1
				lv_bar_set_start_value(bar, 0, LV_ANIM_OFF);
				lv_obj_set_grid_cell(bar, LV_GRID_ALIGN_START, icol * 3+1, 1, LV_GRID_ALIGN_CENTER, irow, 1);
				// widget 3: value label
				lv_obj_t* label = lv_label_create(subcont);
				lv_obj_add_style(label, &lvStyleTextSmall, 0);
				lv_obj_set_grid_cell(label, LV_GRID_ALIGN_START, icol * 3+2, 1, LV_GRID_ALIGN_CENTER, irow, 1);
				// no need to set value here, for it will be updated in the timer
				uint8_t lenWrite = number2text(sValue, -100, 0, 0);
				sValue[lenWrite++] = 'd'; sValue[lenWrite++] = 'B'; sValue[lenWrite] = 0;
				lv_label_set_text(label, sValue);
				// let the bar carry the value label object as user data, so the label's text can be modified on the fly
				lv_obj_set_user_data(bar, (void*)label);
				arr_level_bars[ilevel] = bar;
				ilevel++;
			}
			i++;
		}
	}
	// update volume control commands to chip
	cntNothingChanged = 0;
	timer = lv_timer_create(value_update_timer_cb, 100, NULL);
	lv_timer_set_repeat_count(timer, -1);
EXIT_VOLUME_CONTROL_SCREEN:
	return;
}


/************************ Volume control bar END ************************/


bool audio_analog_chips_need_reset = false;
void reset_audio_analog_chips()
{
	pcm179x_hw_reset();
	adcx120_reset(&adcx120);
	osDelay(pdMS_TO_TICKS(10));
	adcx120_awake(&adcx120);
	pcm179x_init(&pcm179x, pcm179x.fclk_Hz);
	osDelay(pdMS_TO_TICKS(50));
  // apply audio connection and prepare transmissions
  adcx120_init_t adcx120_init_cfg = {
		  .pregain_db2 = {cfg.nPregainsDB2_encoder[0], cfg.nPregainsDB2_encoder[1]},
		  .volume100 = {cfg.nVolume100_encoder[0], cfg.nVolume100_encoder[1]},
  };
	adcx120_init(&adcx120, &adcx120_init_cfg);
	// reset all cached current volume states, so sync volumes will sync them all
	memset(arr_volumes100_now, 0, sizeof(arr_volumes100_now));
	memset(arr_pregainsDB2_now, 0, sizeof(arr_pregainsDB2_now));
	for (int i = 0; i < N_CONTROLS; ++i)
	{
		arr_volumes_changed[i] = true;
	}
	for (int i = 0; i < N_MICROPHONES; ++i)
	{
		arr_pregains_changed[i] = true;
	}
    sync_volumes(false); // blocked

}

#ifdef __cplusplus
}
#endif
