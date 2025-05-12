/*
 * LVGL_GUI_settings.c
 *
 *  Created on: May 19, 2024
 *      Author: cpholzn
 */
#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include "LVGL_GUI.h"
#include <string.h>
#include <stdint.h>
#include "LLMC_LVGL_assets.h"

#ifndef LVGL_SIM
#include "main.h"
#include"Config.h"
#include "cmsis_os2.h"
#include "myUSB.h"
#include "audio_buffers.h"
#include "profiler.h"
#include "usb_device.h"
#endif


/************************ Settings Screen BEGIN ************************/
bool isSettingsScreenCreated = false;
lv_group_t * lvGroupSettingsScreen = NULL;
lv_obj_t* main_cont = NULL;
static void dismiss_settings_screen(lv_event_t* e)
{

	deregister_group(lvGroupSettingsScreen);
	lvGroupSettingsScreen = NULL;

	lv_obj_del_async(main_cont);
	isSettingsScreenCreated = false;

}



/* menu settings objs */
lv_obj_t* lvRoller_USB_config_select=NULL;
lv_obj_t* lvDropdown_USB_record_data_format_select=NULL;
lv_obj_t* lvCheck_USB_volume_control = NULL;
lv_obj_t* lvCheck_USB_record_force_mono = NULL;
lv_obj_t* lvCheck_USB_self_powered = NULL;
lv_obj_t* lvBtn_calibrate_touchscreen=NULL;
lv_obj_t* lvLabel_power_off =NULL;
lv_obj_t* lvBtn_reset_config = NULL;
static void menu_settings_event_cb(lv_event_t * e)
{
	if(!isSettingsScreenCreated) return;
    lv_obj_t * obj = lv_event_get_target(e);
//    lv_obj_t * menu = lv_event_get_user_data(e);

	// setting1: select which USB configuration to show when connect to PC or Phone
    if(obj == lvRoller_USB_config_select)
    {
    	uint32_t i = lv_roller_get_selected(obj);
#ifndef LVGL_SIM
    	switch(i)
    	{
    	case 0: //USB音频
    		USB_switch_to_configuration(USB_CFG_DESC_UAC1_0);
    		cfg.usbCfgDescSelector = USB_CFG_DESC_UAC1_0;
    		isModified = true;
    		break;
    	case 1: //USB存储
    		USB_switch_to_configuration(USB_CFG_DESC_MASS_STORAGE);
    		cfg.usbCfgDescSelector = USB_CFG_DESC_MASS_STORAGE;
    		isModified = true;
    		break;

    	}
    	MX_USB_DEVICE_Init();
#endif
    }
	else if(obj == lvDropdown_USB_record_data_format_select)
	{
		static char sText[32];
    	lv_dropdown_get_selected_str(obj, sText, sizeof(sText));
#ifndef LVGL_SIM
    	uint8_t bitdepth_new;
    	// bit depth first
    	if(strnstr(sText, "24", sizeof(sText)))
    		bitdepth_new = 24;
    	else
    		bitdepth_new = 16;
    	if(bitdepth_new != cfg.USB_record_audio_bit_depth)
    	{
    		cfg.USB_record_audio_bit_depth = bitdepth_new;
			isModified = true;
    	}
    	// sample rate later
    	uint32_t FS_new;
		if(strnstr(sText, "192k", sizeof(sText)))
			FS_new = 192000U;
		else if(strnstr(sText, "96k", sizeof(sText)))
			FS_new = 96000U;
		else if(strnstr(sText, "48k", sizeof(sText)))
			FS_new = 48000U;
		else
			FS_new = 44100U;
		if(FS_new != cfg.USB_record_audio_sample_rate_Hz)
		{
			cfg.USB_record_audio_sample_rate_Hz = FS_new;
			isModified = true;
		}
#endif
	}
	else if(obj == lvCheck_USB_volume_control)
	{
#ifndef LVGL_SIM
		cfg.USB_volume_control = ((lv_obj_get_state(obj) & LV_STATE_CHECKED) != 0)?(1):(0);
		isModified = true;
#endif
	}
	else if(obj == lvCheck_USB_record_force_mono)
	{
#ifndef LVGL_SIM
		cfg.USB_record_force_mono = ((lv_obj_get_state(obj) & LV_STATE_CHECKED) != 0)?(1):(0);
		isModified = true;
#endif
	}
	else if(obj == lvCheck_USB_self_powered)
	{
#ifndef LVGL_SIM
		cfg.USB_self_powered = ((lv_obj_get_state(obj) & LV_STATE_CHECKED) != 0)?(1):(0);
		isModified = true;
#endif
	}
    else if(obj == lvBtn_calibrate_touchscreen)
    {
#ifndef LVGL_SIM
    	start_calibrate_touchscreen();
#endif
    }
    else if (obj == lvLabel_power_off)
    {
        show_msgbox_farewell();
#ifndef LVGL_SIM
        osThreadFlagsSet(taskHumanInputHandle, FLAG_SHUTDOWN_NOW);
#endif
    }
    else if(obj == lvBtn_reset_config)
    {
#ifndef LVGL_SIM
        __disable_irq();
        init_config(&cfg);
        isModified = true;
        __enable_irq();
#endif
    }

}


void back_event_cb(lv_event_t * e)
{

    lv_obj_t * obj = lv_event_get_target(e);
    lv_obj_t * menu = lv_event_get_user_data(e);

    if(lv_menu_back_button_is_root(menu, obj)) {
    	dismiss_settings_screen(e);
    }

}


/**/

#ifndef LVGL_SIM
static const KFIFO_DMA *FIFOs[N_FIFOS] = {&fifo_from_encoder, &fifo_from_FS, &fifo_from_USB_playback, &fifo_to_USB_record};
#else
#define N_FIFOS 4U
#endif

const char *names_FIFO[] = {"fifo_from_encoder", "fifo_from_FS", "fifo_from_USB_playback", "fifo_to_USB_record"};

#define N_PROFILERS 4U
#ifndef LVGL_SIM
static const profiler_counter_t *Profilers[N_PROFILERS] = {&profSAITx, &profUSB_SOF, &profUSB_feedback, &profUSB_MicIn};
#endif
const char *names_profiler[] = {"SAITx", "USB SOF", "USB fb in", "USB mic in"};
const char *headers_profiler[] = {"now", "mean", "max", };
lv_obj_t *bars_FIFO[N_FIFOS] = {NULL};
lv_obj_t *lbls_Profiler[N_PROFILERS][3] = {NULL};

#define N_COUNTERS (1U + 5U)
static const char* arr_counter_names[N_COUNTERS] = {"uf_FS", "mic_SOF", "mic_DataIn", "diff", "mic_loss", "spk_loss"};
#ifdef LVGL_SIM
uint32_t fifoUnderflowCounter_FS=0;
uint32_t global_counter_USB_mic_SOF = 0, global_counter_USB_mic_DataIn = 0, global_counter_USB_mic_loss = 0, global_counter_USB_speaker_loss = 0;
#endif
const uint32_t* arr_pcounters[N_COUNTERS] = {&fifoUnderflowCounter_FS, &global_counter_USB_mic_SOF, &global_counter_USB_mic_DataIn, NULL, &global_counter_USB_mic_loss, &global_counter_USB_speaker_loss};
lv_obj_t *lbls_counter[N_COUNTERS] = {NULL};

static lv_timer_t* timer_FIFO_refresh = NULL;

static void buffer_dashboard_timer_cb(lv_timer_t* timer)
{
	/*1: FIFO monitors */
	for(int i = 0; i < N_FIFOS; ++i)
	{
		if(bars_FIFO[i])
		{
#ifndef LVGL_SIM
			lv_bar_set_value(bars_FIFO[i], kfifo_get_filled_size(FIFOs[i]), LV_ANIM_OFF);
#endif
		}
	}
	static char sValue[32];
	/* 2: USB mic counters*/
    for(int i = 0; i <N_COUNTERS; ++i)
    {
    	if(i != 2)
    	{
    		my_utoa(sValue,(*arr_pcounters[i]));
    	}
    	else
    	{
    		int diff = ((*arr_pcounters[0]) - (*arr_pcounters[1]));
    		number2text(sValue, diff, 0, 0);
    	}
    	lv_label_set_text(lbls_counter[i], sValue);
    }
	/*3 : profiler counts */
	lv_obj_t* lbl;
#ifndef LVGL_SIM
	for(int i=0; i < N_PROFILERS; ++i )
	{
		// now
		lbl = lbls_Profiler[i][0];
		if(lbl)
		{
			my_utoa(sValue, Profilers[i]->arrCycDiffs[Profilers[i]->iwr]);
			lv_label_set_text(lbl, sValue);
		}
		// mean
		lbl = lbls_Profiler[i][1];
		if(lbl)
		{
			my_utoa(sValue, Profilers[i]->cycDiff_mean);
			lv_label_set_text(lbl, sValue);
		}
		// max
		lbl = lbls_Profiler[i][2];
		if(lbl)
		{
			my_utoa(sValue, Profilers[i]->cycDiff_max);
			lv_label_set_text(lbl, sValue);
		}
	}
#endif
}

static void dismiss_sub_page_buffer_dashboard_event_cb(lv_event_t* e)
{
	lv_timer_delete(timer_FIFO_refresh);
	timer_FIFO_refresh = NULL;
	memset(bars_FIFO, 0, sizeof(bars_FIFO));
}

// buffer monitoring dash board
static void create_sub_page_buffer_dashboard(lv_obj_t* sub_cont)
{
	static char sValue[32];
    lv_obj_set_layout(sub_cont, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(sub_cont, LV_FLEX_FLOW_COLUMN);

    lv_obj_t* label;

    /* 1: FIFO free space monitoring */
    for(int i = 0; i < N_FIFOS; ++i)
    {
    	label = lv_label_create(sub_cont);
    	lv_label_set_text(label, names_FIFO[i]);

    	bars_FIFO[i] = lv_bar_create(sub_cont);
    	lv_obj_t* bar = bars_FIFO[i];
    	lv_obj_set_size(bar, LV_PCT(80), 12);
#ifndef LVGL_SIM
    	lv_bar_set_range(bar, 0, FIFOs[i]->size+1);
#endif

    }
    /* 2: USB mic counter and underflow counter */
    lv_obj_t* caption;
    for(int i = 0; i <N_COUNTERS; ++i)
    {
    	caption = lv_label_create(sub_cont);
    	lv_label_set_text(caption, arr_counter_names[i]);
    	label = lv_label_create(sub_cont);
    	lbls_counter[i] = label;
    	if(i != 2)
    	{
    		my_utoa(sValue,*(arr_pcounters[i]));
    	}
    	else
    	{
    		int diff = ((*arr_pcounters[0]) - (*arr_pcounters[1]));
    		number2text(sValue, diff, 0, 0);
    	}
    	lv_label_set_text(label, sValue);
    }

    /* 3: interrupt cyc diff monitoring */
    lv_obj_t* contTb = lv_obj_create(sub_cont);
	lv_obj_remove_flag(contTb, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_remove_flag(contTb, LV_OBJ_FLAG_CLICKABLE);
	lv_obj_set_size(contTb, LV_PCT(90), LV_SIZE_CONTENT);
	lv_obj_add_style(contTb, &lvStyleSemiTranspFrame, 0);
	lv_obj_set_style_bg_opa(contTb, LV_OPA_COVER, 0);
	lv_obj_align(contTb, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_layout(contTb, LV_LAYOUT_GRID);
	/*4 columns: index min max now*/
	static int32_t column_dsc[] = {LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST};
	/*N+1 rows: header [profilers] */
	static int32_t row_dsc[] = { LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST};
	lv_obj_set_grid_dsc_array(contTb, column_dsc, row_dsc);
	// TODO: put items in grid
    for(int i = 0; i < (1 + 3) * (1 + N_PROFILERS); ++i)
    {
    	// columns are inputs, indices are outputs
        uint8_t col= i % (3+1);
        uint8_t row = i / (3+1);
		uint8_t iInput = col - 1;
        uint8_t iOutput = row - 1;
        
        // skip upper-left corner, leave it to be empty
        if(i == 0)
        	continue;

        // making column header: mean min max
        if(row == 0)
        {
        	lv_obj_t *lblHeader = lv_label_create(contTb);
        	/*Stretch the cell horizontally and vertically too
			 *Set span to 1 to make the cell 1 column/row sized*/
			lv_obj_set_grid_cell(lblHeader, LV_GRID_ALIGN_CENTER, col, 1,
					LV_GRID_ALIGN_CENTER, row, 1);
        	lv_label_set_text_static(lblHeader, headers_profiler[iInput]);
        	lv_obj_center(lblHeader);
        }
        // making indices for outputs, buttons that trigger settings menu
        else if(col == 0)
        {
        	lv_obj_t *lblIdx = lv_label_create(contTb);
        	/*Stretch the cell horizontally and vertically too
			 *Set span to 1 to make the cell 1 column/row sized*/
			lv_obj_set_grid_cell(lblIdx, LV_GRID_ALIGN_CENTER, col, 1,
					LV_GRID_ALIGN_CENTER, row, 1);
        	lv_label_set_text_static(lblIdx, names_profiler[iOutput]);
        	lv_obj_center(lblIdx);
        }
        // col > 0 and row > 0, making switches
        else
        {

        	lv_obj_t *lblValue = lv_label_create(contTb);
        	lbls_Profiler[iOutput][iInput] = lblValue;
        	/*Stretch the cell horizontally and vertically too
			 *Set span to 1 to make the cell 1 column/row sized*/
			lv_obj_set_grid_cell(lblValue, LV_GRID_ALIGN_CENTER, col, 1,
					LV_GRID_ALIGN_CENTER, row, 1);
#ifndef LVGL_SIM
            const profiler_counter_t* pProf = Profilers[iOutput];


			if(iInput == 0) // now
			{
				my_utoa(sValue, pProf->arrCycDiffs[pProf->iwr]);
			}
			else if(iInput == 1) // mean
			{
				my_utoa(sValue, pProf->cycDiff_mean);
			}
			else // max
			{
				my_utoa(sValue, pProf->cycDiff_max);
			}
			lv_label_set_text(lblValue, sValue);
#endif
        }

    }

    /* start value refreshing timer*/
    if(timer_FIFO_refresh == NULL)
    	timer_FIFO_refresh = lv_timer_create(buffer_dashboard_timer_cb, 500, NULL);

    lv_obj_add_event_cb(sub_cont, dismiss_sub_page_buffer_dashboard_event_cb, LV_EVENT_DELETE, NULL);
}


void display_settings_screen()
{

	//// 创建外部输入group，并聚焦
	lvGroupSettingsScreen = lv_group_create();
	register_group(lvGroupSettingsScreen);
    lv_group_set_default(lvGroupSettingsScreen);

	lv_obj_t* scr = lv_screen_active();
	main_cont = lv_obj_create(scr);
    lv_obj_remove_flag(main_cont, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(main_cont, LV_OBJ_FLAG_SCROLL_ELASTIC);
    lv_obj_remove_style_all(main_cont);                            /*Make it transparent*/
    lv_obj_set_style_bg_opa(main_cont, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(main_cont, COLOR_WHITE, 0);
    lv_obj_set_size(main_cont, lv_pct(90), lv_pct(90));
    lv_obj_center(main_cont);
    // image
    lv_obj_t* img = lv_image_create(main_cont);
    lv_image_set_src(img, &back_img_settings_1);
    lv_obj_align(img, LV_ALIGN_BOTTOM_RIGHT, 0, 0);


	lv_obj_t* menu = lv_menu_create(main_cont);
    lv_obj_set_style_bg_opa(menu, LV_OPA_0, 0);
	lv_menu_set_mode_root_back_button(menu, LV_MENU_ROOT_BACK_BUTTON_ENABLED); // back button at menu root to dismiss the screen
    lv_obj_set_size(menu, lv_pct(100), lv_pct(100));
    lv_obj_center(menu);
    // 返回按钮单独设置
    lv_obj_t* btnMenuBack = lv_menu_get_main_header_back_button(menu);
    lv_obj_set_size(btnMenuBack, BACK_BTN_WIDTH, BACK_BTN_HEIGHT);
    lv_obj_add_event_cb(menu, back_event_cb, LV_EVENT_CLICKED, menu);

    lv_obj_t* sub_page, *sub_cont, *roller, *label;

    /*Create a main page*/
    lv_obj_t * main_page = lv_menu_page_create(menu, NULL);

    /* Contents BEGIN */
    /* content1 : USB device selection BEGIN */
    // create subpage
    sub_page = lv_menu_page_create(menu, NULL);
    // item 1: USB device type select
    sub_cont = lv_menu_cont_create(sub_page);
    roller = lv_roller_create(sub_cont);
    lv_obj_set_width(roller, LV_PCT(50));
    lvRoller_USB_config_select = roller;
    lv_roller_set_options(roller,
                              "USB声音\n"
                              "USB存储",
                              LV_ROLLER_MODE_NORMAL);
    lv_roller_set_visible_row_count(roller, 2);
    lv_obj_add_event_cb(roller, menu_settings_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
#ifndef LVGL_SIM
    // sync with cfg
    if(cfg.usbCfgDescSelector == USB_CFG_DESC_UAC1_0)
    {
    	lv_roller_set_selected(roller, 0, LV_ANIM_OFF);
    }
    else if(cfg.usbCfgDescSelector == USB_CFG_DESC_MASS_STORAGE)
    {
    	lv_roller_set_selected(roller, 1, LV_ANIM_OFF);
    }
#endif
    // item 2: USB record data format select
    sub_cont = lv_menu_cont_create(sub_page);
    lv_obj_t* dropdown = lv_dropdown_create(sub_cont);
    lvDropdown_USB_record_data_format_select = dropdown;
    lv_dropdown_set_options(dropdown, "96kHz@24(高质量)\n48kHz@24(高质量)\n48kHz@16(兼容性)");
    lv_obj_add_event_cb(dropdown, menu_settings_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
#ifndef LVGL_SIM
    // sync with cfg
    if(cfg.USB_record_audio_bit_depth == 24)
    {
    	if(cfg.USB_record_audio_sample_rate_Hz == 96000)
    		lv_dropdown_set_selected(dropdown, 0);
    	else
    		lv_dropdown_set_selected(dropdown, 1);
    }
    else if(cfg.USB_record_audio_bit_depth == 16)
    {
    	if(cfg.USB_record_audio_sample_rate_Hz == 48000)
    		lv_dropdown_set_selected(dropdown, 2);
    }
#endif
    // item 3: USB volume control
    sub_cont = lv_menu_cont_create(sub_page);
    lv_obj_t* checkbox = lv_checkbox_create(sub_cont);
    lvCheck_USB_volume_control = checkbox;
    lv_checkbox_set_text(checkbox, "允许通过USB控制耳机音量");
    lv_obj_add_event_cb(checkbox, menu_settings_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
#ifndef LVGL_SIM
    // sync with cfg
    if(cfg.USB_volume_control )
    {
    	lv_obj_set_state(checkbox, LV_STATE_CHECKED, 1);
    }
    else
    {
    	lv_obj_remove_state(checkbox, LV_STATE_CHECKED);
    }
#endif
    // item 4: USB force mono
    sub_cont = lv_menu_cont_create(sub_page);
    checkbox = lv_checkbox_create(sub_cont);
    lvCheck_USB_record_force_mono = checkbox;
    lv_checkbox_set_text(checkbox, "USB录音强制转换为单声道");
    lv_obj_add_event_cb(checkbox, menu_settings_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
#ifndef LVGL_SIM
    // sync with cfg
    if(cfg.USB_record_force_mono)
    {
    	lv_obj_set_state(checkbox, LV_STATE_CHECKED, 1);
    }
    else
    {
    	lv_obj_remove_state(checkbox, LV_STATE_CHECKED);
    }
#endif
    // item 5: USB self powered
    sub_cont = lv_menu_cont_create(sub_page);
    checkbox = lv_checkbox_create(sub_cont);
    lvCheck_USB_self_powered = checkbox;
    lv_checkbox_set_text(checkbox, "不通过USB接口取电（自供电）");
    lv_obj_add_event_cb(checkbox, menu_settings_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
#ifndef LVGL_SIM
    // sync with cfg
    if(cfg.USB_self_powered)
    {
    	lv_obj_set_state(checkbox, LV_STATE_CHECKED, 1);
    }
    else
    {
    	lv_obj_remove_state(checkbox, LV_STATE_CHECKED);
    }
#endif
    // create main page entry
    sub_cont = lv_menu_cont_create(main_page);
    label = lv_label_create(sub_cont);
    lv_label_set_text(label, "USB设置");
    lv_group_add_obj(lvGroupSettingsScreen, sub_cont);
    // link main page entry and subpage
    lv_menu_set_load_page_event(menu, sub_cont, sub_page);
    /* content1 : USB device selection END */



    /* content2 : Buffer Dashboard BEGIN */
    // create subpage
    sub_page = lv_menu_page_create(menu, NULL);
    sub_cont = lv_menu_cont_create(sub_page);
    create_sub_page_buffer_dashboard(sub_cont);
    // create main page entry
    sub_cont = lv_menu_cont_create(main_page);
    label = lv_label_create(sub_cont);
    lv_label_set_text(label, "缓冲区监控");
    lv_group_add_obj(lvGroupSettingsScreen, sub_cont);
    // link main page entry and subpage
    lv_menu_set_load_page_event(menu, sub_cont, sub_page);
    /* content2 : Buffer Dashboard END */



    /* content3 :  Touchscreen BEGIN */
    // create subpage
    sub_page = lv_menu_page_create(menu, NULL);
    // item 1: brightness
    sub_cont = lv_menu_cont_create(sub_page);
    lv_obj_t* slider = lv_slider_create(sub_cont);
    lv_obj_set_size(slider, LV_PCT(50), 20);
    lv_slider_set_range(slider, 0, 100);
    lv_group_add_obj(lvGroupSettingsScreen, sub_cont);
    // item 2: calibrate touchscreen
    sub_cont = lv_menu_cont_create(sub_page);
    lvBtn_calibrate_touchscreen = lv_btn_create(sub_cont);
    label = lv_label_create(lvBtn_calibrate_touchscreen);
    lv_label_set_text(label, "校准触屏");
    lv_obj_center(label);
//    lv_obj_add_flag(label, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(lvBtn_calibrate_touchscreen, menu_settings_event_cb, LV_EVENT_CLICKED, NULL);
    lv_group_add_obj(lvGroupSettingsScreen, sub_cont);
    // create main page entry
    sub_cont = lv_menu_cont_create(main_page);
    label = lv_label_create(sub_cont);
    lv_label_set_text(label, "屏幕设置");
    lv_group_add_obj(lvGroupSettingsScreen, sub_cont);
    // link main page entry and subpage
    lv_menu_set_load_page_event(menu, sub_cont, sub_page);
    /* content3 : Touchscreen END */


    /* content4 :  power mgmt */
    // create subpage
    sub_page = lv_menu_page_create(menu, NULL);
    // item 1: power off button
    sub_cont = lv_menu_cont_create(sub_page);
    label = lv_label_create(sub_cont);
    lvLabel_power_off = label;
    lv_label_set_text(label, "关闭电源");
    lv_obj_add_flag(label, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(label, menu_settings_event_cb, LV_EVENT_CLICKED, NULL);
    lv_group_add_obj(lvGroupSettingsScreen, sub_cont);

    // item 2: reset config button
    sub_cont = lv_menu_cont_create(sub_page);
    lvBtn_reset_config = lv_button_create(sub_cont);
    label = lv_label_create(lvBtn_reset_config);
    lv_label_set_text(label, "重置为出厂设置");
    lv_obj_center(label);
    lv_obj_add_event_cb(lvBtn_reset_config, menu_settings_event_cb, LV_EVENT_CLICKED, NULL);
    lv_group_add_obj(lvGroupSettingsScreen, sub_cont);
    // create main page entry
    sub_cont = lv_menu_cont_create(main_page);
    label = lv_label_create(sub_cont);
    lv_label_set_text(label, "系统设置");
    lv_group_add_obj(lvGroupSettingsScreen, sub_cont);
    // link main page entry and subpage
    lv_menu_set_load_page_event(menu, sub_cont, sub_page);
    /* content3 : Touchscreen END */

    /* Contents END */
    lv_menu_set_page(menu, main_page);

    isSettingsScreenCreated = true;

}



/************************ Settings Screen END ************************/
#ifdef __cplusplus
}
#endif
