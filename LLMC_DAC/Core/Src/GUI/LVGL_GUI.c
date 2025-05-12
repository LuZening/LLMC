/*
 * LVGL_GUI.c
 *
 *  Created on: 2020年6月11日
 *      Author: Zening
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "LVGL_GUI.h"
#include "LLMC_LVGL_assets.h"
#include <math.h>
#ifndef LVGL_SIM
#include "MyUSB.h"
#include "touch_HR2046.h"
#include "cmsis_os.h"
#include "FreeRTOS.h"
#include "Config.h"

// global flag
bool isGUIReady = false;
bool is_back_light_on = false;

void set_back_light(uint8_t state)
{
	HAL_GPIO_WritePin(BK_EN_GPIO_Port, BK_EN_Pin, state);
	is_back_light_on = state;
}
void set_display_enable(uint8_t state)
{
	HAL_GPIO_WritePin(TFT_DISP_GPIO_Port, TFT_DISP_Pin, state);
}
#endif
lv_mutex_t lvLock = {0};
void lv_lock()
{
	lv_mutex_lock(&lvLock);
}

void lv_unlock()
{
	lv_mutex_unlock(&lvLock);
}



static lv_obj_t* create_volume_button(lv_obj_t* parent);
static void create_battery_SOC_temp_symbol(lv_obj_t* parent);

#define N_BTNS 3


const char* STR_FAREWELL = "このきずな、\nもう二度と切り離せませんわよ";

const lv_color_t COLOR_PLAIN_TEXT = LV_COLOR_MAKE(32, 32, 32);
const lv_color_t COLOR_BG = LV_COLOR_MAKE(232, 232, 232);
const lv_color_t COLOR_ACCENT = LV_COLOR_MAKE(220, 39, 70); // lovelive style redish
const lv_color_t COLOR_SEMI_TRANSP = LV_COLOR_MAKE(255, 255, 255);
const lv_color_t COLOR_BLACK = LV_COLOR_MAKE(0, 0, 0);
const lv_color_t COLOR_SHADOW = LV_COLOR_MAKE(0, 0, 0);
const lv_color_t COLOR_WHITE = LV_COLOR_MAKE(255, 255, 255);
const lv_color_t COLOR_BUTTON = LV_COLOR_MAKE(252, 130, 145); // lovelive AS pink-ish button
/*****************************************************************
 *  Input Group management BEGIN
 ****************************************************************/


// Current objects to control by external inputs
// when modify, must be protected by Mutex


lv_group_t * lvGroups[N_GROUP_STACK_SIZE] =	{NULL};
uint8_t idxGroups = 0; // 指向当前正在操作的group的后一个空位
lv_group_t* get_current_group()
{
	if(idxGroups <= N_GROUP_STACK_SIZE && idxGroups > 0)
		return lvGroups[ idxGroups - 1];
	else
		return NULL;
}

bool register_group(lv_group_t* p)
{
	if(idxGroups < N_GROUP_STACK_SIZE)
	{
		lvGroups[idxGroups++] = p;
		lv_indev_set_group(encoder_indev, p);
		return true;
	}
	else
		return false;
}

bool deregister_group(lv_group_t* p)
{
	bool r = false;
	uint8_t i,j;
	if(p == NULL)
		return false;
	for(i = 0; i < idxGroups; ++i)
	{
		if(lvGroups[i] == p)
		{
			lv_group_del(lvGroups[i]);
			lvGroups[i] = NULL;
			if(idxGroups - i == 1) // the group to be deregistered is the focused group now, then use the previous group
			{
				// look backward to find the first non-NULL group pointer, and promote it as the focused group
				for(j = i; j > 0; --j)
				{
					if(lvGroups[j - 1] != NULL)
					{
						idxGroups = j;
						lv_indev_set_group(encoder_indev, lvGroups[j - 1]);
						break;
					}
				}
				// if not found, set idx to 0, set current focused group to NULL
				if(j == 0)
				{
					idxGroups = 0;
					lv_indev_set_group(encoder_indev, NULL);
				}
			}
			r = true;
			break;
		}
	}
	return r;
}

bool deregister_group_without_del(lv_group_t* p)
{
	bool r = false;
	uint8_t i, j;
	if (p == NULL)
		return false;
	for (i = 0; i < idxGroups; ++i)
	{
		if (lvGroups[i] == p)
		{
			lvGroups[i] = NULL;
			if (idxGroups - i == 1) // the group to be deregistered is the focused group now, then use the previous group
			{
				// look backward to find the first non-NULL group pointer, and promote it as the focused group
				for (j = i; j > 0; --j)
				{
					if (lvGroups[j - 1] != NULL)
					{
						idxGroups = j;
						lv_indev_set_group(encoder_indev, lvGroups[j - 1]);
						break;
					}
				}
				// if not found, set idx to 0, set current focused group to NULL
				if (j == 0)
				{
					idxGroups = 0;
					lv_indev_set_group(encoder_indev, NULL);
				}
			}
			r = true;
			break;
		}
	}
	return r;
}



static void encoder_global_cb(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t* obj = lv_event_get_target(e);

	if (code == LV_EVENT_LONG_PRESSED)
	{
		display_volume_control_screen();
	}

}

/*****************************************************************
 *  Input Group management END
 ****************************************************************/

void init_LVGL_GUI()
{
	/* init Mutex */
	lv_mutex_init(&lvLock);
#ifndef LVGL_SIM
	vQueueAddToRegistry(lvLock.xMutex, "lvLock");
	// TODO: how to init DMA2D


	// init LVGL library
	lv_init();
	lv_tick_set_cb(HAL_GetTick);

	// init screen
	display = lv_display_create(LV_HOR_RES_MAX, LV_VER_RES_MAX); /*Create the display*/
	lv_display_set_flush_cb(display, LTDC_flush_cb);        /*Set a flush callback to draw to the display*/
	/*Declare a buffer */
	lv_display_set_buffers(display, framebuf2A, NULL, sizeof(framebuf2A), LV_DISPLAY_RENDER_MODE_DIRECT);   /*Initialize the display buffer*/
	init_LVGL_input();

#endif
}




/*****************************************************************
 *  FileSystem BEGIN
 ****************************************************************/

/*****************************************************************
 *  FileSystem END
 ****************************************************************/


/*****************************************************************
 *  styles BEGIN
 ****************************************************************/

// 小号英文数字
lv_style_t lvStyleTextSmall;
// 大号英文数字
lv_style_t lvStyleTextLarge;


// 半透明底框
lv_style_t lvStyleSemiTranspFrame;
// 不可见窗口
lv_style_t lvStyleInvisibleCont;

//按钮样式
lv_style_t lvStyleBtn;
lv_style_t lvStyleBtn_focused;

// 弹窗样式
lv_style_t lvStyleMsgBox;

//图片按钮样式
lv_style_t lvStyleImageBtn;

// 开关样式 
lv_style_t lvStyleSwitch;

// progress bar style
lv_style_t lvStyleProgBarBg;
lv_style_t lvStyleProgBarInd;

static lv_theme_t lvThemeNew;

static void new_theme_apply_cb(lv_theme_t * th, lv_obj_t * obj)
{
    LV_UNUSED(th);

    if(lv_obj_check_type(obj, &lv_button_class))
    {
        // normal state
    	lv_obj_add_style(obj, &lvStyleBtn, 0);
    	// hover or pressed state
        lv_obj_add_style(obj, &lvStyleBtn_focused, LV_STATE_FOCUSED | LV_STATE_PRESSED);

    }
	else if (lv_obj_check_type(obj, &lv_table_class))
	{
		// for tight rows
		lv_obj_set_style_pad_all(obj, 1, LV_PART_ITEMS);
		lv_obj_set_style_margin_all(obj, 1, LV_PART_ITEMS);
	}
	else if(lv_obj_check_type(obj, &lv_label_class))
	{
		lv_obj_set_style_text_font(obj, &FONT_SMALL, 0);
	}
    // for container
    else if(lv_obj_check_type(obj, &lv_obj_class))
    {
    	lv_obj_add_style(obj, &lvStyleSemiTranspFrame, 0);
        lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
		lv_obj_remove_flag(obj, LV_OBJ_FLAG_EVENT_BUBBLE); // not allowing any event pass through the layer to the next
//        lv_obj_remove_flag(obj, LV_OBJ_FLAG_CLICKABLE); // any click will be past through the layer
    }
    else if(lv_obj_check_type(obj, &lv_bar_class))
    {
    	lv_obj_add_style(obj, &lvStyleProgBarBg, LV_PART_MAIN);
    	lv_obj_add_style(obj, &lvStyleProgBarInd, LV_PART_INDICATOR);
    	lv_obj_remove_flag(obj, LV_OBJ_FLAG_CLICKABLE);
    }
    // when focused by encoder input
    lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_border_color(obj, COLOR_ACCENT, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
}


void init_styles()
{

	static bool isStyleInitialized = false;
	if(isStyleInitialized)
		return;

	//@Color: white
	//@Font: 12pt
	lv_style_init(&lvStyleTextSmall);
	lv_style_set_text_color(&lvStyleTextSmall, COLOR_PLAIN_TEXT);
	lv_style_set_text_font(&lvStyleTextSmall, &FONT_SMALL);
	lv_style_set_pad_left(&lvStyleTextSmall, 2);

	//@Color: white
	//@Font: 16pt
	lv_style_init(&lvStyleTextLarge);
	lv_style_set_text_color(&lvStyleTextLarge,  COLOR_PLAIN_TEXT);
	lv_style_set_text_font(&lvStyleTextLarge,  &FONT_MED);
	lv_style_set_pad_left(&lvStyleTextLarge,  4);


	lv_style_init(&lvStyleBtn);
	lv_style_set_radius(&lvStyleBtn,  5);
	lv_style_set_bg_opa(&lvStyleBtn,  LV_OPA_COVER);
	lv_style_set_bg_color(&lvStyleBtn,  COLOR_BUTTON);
	lv_style_set_text_color(&lvStyleBtn,  COLOR_PLAIN_TEXT);
	lv_style_set_border_width(&lvStyleBtn, 0);
	lv_style_set_border_opa(&lvStyleBtn,  LV_OPA_COVER);
//	lv_style_set_shadow_color(&lvStyleBtn, COLOR_SHADOW);
//	lv_style_set_shadow_opa(&lvStyleBtn, LV_OPA_20);
//	lv_style_set_shadow_width(&lvStyleBtn, 2);
//	lv_style_set_shadow_spread(&lvStyleBtn, 2);

	lv_style_init(&lvStyleBtn_focused);
	lv_style_set_bg_color(&lvStyleBtn_focused,  COLOR_ACCENT);
	lv_style_set_border_width(&lvStyleBtn_focused, 3);
	lv_color_t cBorder = lv_color_lighten(COLOR_BUTTON, LV_OPA_20);
	lv_style_set_border_color(&lvStyleBtn_focused, cBorder);
	lv_style_set_border_opa(&lvStyleBtn_focused,  LV_OPA_COVER);


	lv_style_init(&lvStyleMsgBox);
	lv_style_set_radius(&lvStyleMsgBox, 4);
	lv_style_set_bg_color(&lvStyleMsgBox, COLOR_BG);
	lv_style_set_bg_opa(&lvStyleMsgBox, LV_OPA_COVER);
	lv_style_set_border_width(&lvStyleMsgBox,4);
	lv_style_set_border_opa(&lvStyleMsgBox, LV_OPA_COVER);
	lv_style_set_border_color(&lvStyleMsgBox, COLOR_ACCENT);
//	lv_style_set_shadow_color(&lvStyleMsgBox, COLOR_SHADOW);
//	lv_style_set_shadow_opa(&lvStyleMsgBox, LV_OPA_50);
//	lv_style_set_shadow_spread(&lvStyleMsgBox, 4);
//	lv_style_set_shadow_width(&lvStyleMsgBox, 4);
	lv_style_set_text_color(&lvStyleMsgBox, COLOR_PLAIN_TEXT);
	lv_style_set_text_font(&lvStyleMsgBox,  &FONT_MED);


	lv_style_init(&lvStyleSemiTranspFrame);
	lv_style_set_radius(&lvStyleSemiTranspFrame, 4);
	lv_style_set_bg_opa(&lvStyleSemiTranspFrame, LV_OPA_50);
	lv_style_set_bg_color(&lvStyleSemiTranspFrame, COLOR_SEMI_TRANSP);
	lv_style_set_border_width(&lvStyleSemiTranspFrame, 1);
	lv_style_set_border_opa(&lvStyleSemiTranspFrame, LV_OPA_80);
//	lv_style_set_shadow_color(&lvStyleSemiTranspFrame, COLOR_SHADOW);
//	lv_style_set_shadow_opa(&lvStyleSemiTranspFrame, LV_OPA_20);
//	lv_style_set_shadow_width(&lvStyleSemiTranspFrame, 4);
//	lv_style_set_shadow_spread(&lvStyleSemiTranspFrame, 4);
	lv_style_set_pad_all(&lvStyleSemiTranspFrame, 0);

	lv_style_init(&lvStyleInvisibleCont);
	lv_style_set_radius(&lvStyleInvisibleCont, 0);
	lv_style_set_bg_opa(&lvStyleInvisibleCont, LV_OPA_0);
	lv_style_set_border_width(&lvStyleInvisibleCont, 0);
	lv_style_set_pad_all(&lvStyleInvisibleCont, 0);

	/* switch */
	lv_style_init(&lvStyleSwitch);
	lv_style_set_radius(&lvStyleSwitch, 0);
	lv_style_set_bg_opa(&lvStyleSwitch, LV_OPA_COVER);
	lv_style_set_border_width(&lvStyleSwitch, 1);
	lv_style_set_border_opa(&lvStyleSwitch, LV_OPA_COVER);
//	lv_style_set_shadow_color(&lvStyleSwitch, COLOR_SHADOW);
//	lv_style_set_shadow_opa(&lvStyleSwitch, LV_OPA_20);

	/* progress bar */
	lv_style_init(&lvStyleProgBarBg);
	lv_style_set_bg_color(&lvStyleProgBarBg, COLOR_WHITE);
	lv_style_set_bg_opa(&lvStyleProgBarBg, LV_OPA_COVER);
	lv_style_set_radius(&lvStyleProgBarBg, 2);
	lv_style_set_pad_top(&lvStyleProgBarBg, 0);
	lv_style_set_pad_left(&lvStyleProgBarBg, 0);
	lv_style_set_pad_right(&lvStyleProgBarBg, 0);
	lv_style_set_pad_bottom(&lvStyleProgBarBg, 0);

	lv_style_init(&lvStyleProgBarInd);
	lv_style_set_radius(&lvStyleProgBarInd, 0);
	lv_style_set_bg_opa(&lvStyleProgBarInd, LV_OPA_COVER);
	lv_style_set_bg_color(&lvStyleProgBarInd, lv_color_hex(0xB7FFA0)); // light green
	lv_style_set_bg_grad_color(&lvStyleProgBarInd, lv_color_hex(0xFFA4A4)); // red
	lv_style_set_bg_main_stop(&lvStyleProgBarInd, (int)(0.9f * 256)); // 0 ~ 255
	lv_style_set_bg_grad_stop(&lvStyleProgBarInd, (int)(0.95f * 256)); // 0 ~ 255
	lv_style_set_bg_grad_dir(&lvStyleProgBarInd, LV_GRAD_DIR_HOR);

	/* set the default theme */
	if(1)
	{
		uint8_t darkmode = false;
		
		lv_theme_default_init(display, COLOR_BUTTON, COLOR_BG, darkmode,  /*Use the DPI, size, etc from this display*/
												&FONT_SMALL); /*Small, normal, large fonts*/
		lv_theme_t* plvThemeDefault = lv_display_get_theme(NULL);
		// extending default theme
		lvThemeNew = *plvThemeDefault;
		lv_theme_set_parent(&lvThemeNew, plvThemeDefault);
		lv_theme_set_apply_cb(&lvThemeNew, new_theme_apply_cb);
	    lv_display_set_theme(display, &lvThemeNew);
	}

	isStyleInitialized = true;
}


/*****************************************************************
 *  styles END
 ****************************************************************/


const uint16_t BACK_BTN_WIDTH = LV_HOR_RES_MAX / 8;
const uint16_t BACK_BTN_HEIGHT = LV_VER_RES_MAX / 8;
const char* STR_BACK = "返回";


/************************ Main screen BEGIN ************************/
bool isMainWidgetsCreated = false;
lv_group_t * lvGroupMain = NULL;

static const char* arrBtnCaptions[N_BTNS] = { "调音台", "作品集", "设置" };

// "录音"
static void btn_record_cb()
{
	display_audio_signal_path_screen();
}
// "播放"
static void btn_player_cb()
{
	display_FS_player_screen();
}
// "设置"
static void btn_settings_cb()
{
	display_settings_screen();
}
// transfer functions' array
void (*arrBtnOnClicks[N_BTNS])() = { btn_record_cb , btn_player_cb , btn_settings_cb };
// handle transfer between different scenes
static void transfer_btn_cb(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t* obj = lv_event_get_target(e);

	const char* sBtnCaption = lv_event_get_user_data(e);
	
	for (int i = 0; i < N_BTNS; ++i)
	{
		// found matched caption
		if (strcmp(sBtnCaption, arrBtnCaptions[i]) == 0)
		{
			// call the i-th function
			void (*fn)(void) = arrBtnOnClicks[i];
			if(fn) fn();
			break;
		}
	}
}

lv_obj_t* lvTouchCursor = NULL;
void display_main_widgets()
{


	if(isMainWidgetsCreated)
		goto EXIT_INIT_MAIN_WIDGETS;

	init_styles();

	lv_obj_t* scr = lv_scr_act();
#define MARGIN_TO_BORDER_X  30
#define MARGIN_TO_BORDER_Y 10
	isMainWidgetsCreated = true;

	//// 创建外部输入group，并聚焦
	lvGroupMain = lv_group_create();
	register_group(lvGroupMain);

	/* 光标图标 */
	lvTouchCursor = lv_img_create(lv_layer_sys());
	lv_img_set_src(lvTouchCursor, &character_icon_small_transp);
	lv_obj_center(lvTouchCursor);
	/* 主界面背景 BEGIN */
	//// 背景底色
	lv_color_t c = COLOR_BG;
	lv_obj_set_style_bg_color(scr, c, 0);

	//// 背景图片

	// NOTE: use UTF8 path string, for it will be converted to UTF-16 in lv_fs interface
	const void *pathPicBg = &main_background_1;
	lv_obj_t* lvImgBg = lv_image_create(scr);
	lv_obj_set_size(lvImgBg, LV_HOR_RES_MAX, LV_VER_RES_MAX);
	lv_obj_align(lvImgBg, LV_ALIGN_TOP_LEFT,0,0);
	lv_image_set_src(lvImgBg, pathPicBg);
	/* 主界面背景 END */


	/* 主界面图片按钮组 BEGIN */

	const uint16_t CONT_WIDTH_MARGIN = LV_HOR_RES_MAX / 20; // 480/20 = 24
	const uint16_t CONT_HEIGHT_MARGIN = LV_VER_RES_MAX / 8; // 272/8 = 34
	const uint16_t CONT_WIDTH = ((LV_HOR_RES_MAX - (N_BTNS + 1) * CONT_WIDTH_MARGIN) / N_BTNS); // ~128
	const uint16_t CONT_HEIGHT = ((LV_VER_RES_MAX - 2 * CONT_HEIGHT_MARGIN)); // ~

	const uint16_t CONT_IMG_WIDTH = 100;
	const uint16_t CONT_IMG_HEIGHT = 100;

	const uint16_t CONT_BTN_WIDTH = 100;
	const uint16_t CONT_BTN_HEIGHT = 32;
	//// 主界面按键1 - 录音 2 - 播放 3 - 设置
	uint8_t iBtn;
	uint16_t x_begin, y_begin;

	static const void* arrPathBtnImgs[N_BTNS] = {&btn_img_1, &btn_img_2, &btn_img_3};
	
	
	for(iBtn = 0; iBtn < N_BTNS; ++iBtn)	
	{
		x_begin = iBtn * (CONT_WIDTH_MARGIN + CONT_WIDTH) + CONT_WIDTH_MARGIN;
		y_begin = CONT_HEIGHT_MARGIN;
		// 半透明背景
		lv_obj_t* lvContForBtn1 = lv_obj_create(lvImgBg);
		lv_obj_set_size(lvContForBtn1, CONT_WIDTH, CONT_HEIGHT);
		lv_obj_add_style(lvContForBtn1, &lvStyleSemiTranspFrame, 0);
		lv_obj_align(lvContForBtn1, LV_ALIGN_TOP_LEFT, x_begin, y_begin);

		// 图片
		lv_obj_t* lvImgForBtn1 = lv_image_create(lvContForBtn1);
		lv_obj_set_size(lvImgForBtn1, CONT_IMG_WIDTH, CONT_IMG_HEIGHT);
		lv_image_set_src(lvImgForBtn1, arrPathBtnImgs[iBtn]);
		lv_obj_align(lvImgForBtn1, LV_ALIGN_CENTER, 0, -CONT_HEIGHT / 4);
#if 0
		// 描述文字
		const char* arrBtnHintLabels[N_BTNS] = {"启动音频采集并选择混音器路径", "浏览并播放存储的音频文件", "个性化配置你的设备"};
		lv_obj_t * lvLblForBtn1 = lv_label_create(lvContForBtn1);
		lv_obj_set_width(lvLblForBtn1, CONT_WIDTH);  /*Set smaller width to make the lines wrap*/
		lv_obj_add_style(lvLblForBtn1, &lvStyleTextSmall, 0);
		lv_obj_set_style_pad_all(lvLblForBtn1, 5, 0);
		lv_label_set_long_mode(lvLblForBtn1, LV_LABEL_LONG_WRAP);     /*Break the long lines*/
		lv_obj_set_style_text_align(lvLblForBtn1, LV_TEXT_ALIGN_CENTER, 0);
		lv_label_set_text(lvLblForBtn1, arrBtnHintLabels[iBtn]);
		lv_obj_align(lvLblForBtn1, LV_ALIGN_CENTER, 0, CONT_HEIGHT / 4);
#endif	

		// 按钮
		lv_obj_t *lvBtn1 = lv_button_create(lvContForBtn1);
		lv_obj_set_size(lvBtn1, CONT_BTN_WIDTH, CONT_BTN_HEIGHT);
		lv_obj_add_style(lvBtn1, &lvStyleBtn, 0);
		lv_obj_add_style(lvBtn1, &lvStyleBtn_focused, LV_STATE_FOCUSED);
		lv_obj_align(lvBtn1, LV_ALIGN_BOTTOM_MID, 0, -CONT_HEIGHT / 8);
		lv_group_add_obj(lvGroupMain, lvBtn1);
		// 按钮跳转,，将按钮caption当作user data传输给事件响应函数，作为识别跳转目标的方式
		lv_obj_add_event_cb(lvBtn1, transfer_btn_cb, LV_EVENT_CLICKED, (void*)(arrBtnCaptions[iBtn]));
#ifndef LVGL_SIM
		if((iBtn == 1) && (cfg.usbCfgDescSelector == USB_CFG_DESC_MASS_STORAGE))
		{
			lv_obj_set_state(lvBtn1, LV_STATE_DISABLED, 1);
		}
#endif

		// 按钮caption
		lv_obj_t *lvLblBtn1 = lv_label_create(lvBtn1);
		lv_label_set_text(lvLblBtn1, arrBtnCaptions[iBtn]);
		lv_obj_add_style(lvLblBtn1, &lvStyleTextLarge, 0);
		lv_obj_center(lvLblBtn1);


	}


	create_volume_button(lv_layer_sys()); // always on top
	create_battery_SOC_temp_symbol(lv_layer_sys()); // always on top
	// make a global event callback on Encoder input device
	lv_indev_add_event_cb(encoder_indev, encoder_global_cb, LV_EVENT_LONG_PRESSED, NULL);

EXIT_INIT_MAIN_WIDGETS:
	return;
}






/************************ Main screen END ************************/


/************************ Generic widgets BEGIN ***********************/
static void back_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);

    void (*func_dismiss)(void) = lv_event_get_user_data(e);

    if(func_dismiss)
    {
    	func_dismiss();
    }
}

lv_obj_t* create_back_button(lv_obj_t* parent, lv_group_t* group, void (cb)(void))
{
	lv_obj_t* btn = lv_button_create(parent);
	lv_obj_set_size(btn, BACK_BTN_WIDTH, BACK_BTN_HEIGHT);
	lv_obj_align(btn, LV_ALIGN_TOP_LEFT, -5, -5);
	lv_obj_add_event_cb(btn, back_event_cb, LV_EVENT_CLICKED, cb);

	lv_obj_t* lbl = lv_label_create(btn);
	lv_label_set_text(lbl, STR_BACK);
	lv_obj_center(lbl);

	lv_group_add_obj(group, btn);
	return btn;
}

static void volume_button_event_cb(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t* obj = lv_event_get_target(e);

	void (*func_dismiss)(void) = lv_event_get_user_data(e);

	if (func_dismiss)
	{
		func_dismiss();
	}
}

static lv_obj_t* create_volume_button(lv_obj_t* parent)
{
	lv_obj_t* btn = lv_button_create(parent);
	lv_obj_set_size(btn, BACK_BTN_WIDTH, BACK_BTN_HEIGHT);
	lv_obj_align(btn, LV_ALIGN_BOTTOM_RIGHT, -5, -5);
	lv_obj_add_event_cb(btn, volume_button_event_cb, LV_EVENT_CLICKED , display_volume_control_screen);
	lv_obj_move_foreground(btn);

	lv_obj_t* lbl = lv_label_create(btn);
	lv_label_set_text(lbl, LV_SYMBOL_VOLUME_MAX);
	lv_obj_set_style_text_font(lbl, &LV_FONT_AWESOME, 0);
	lv_obj_center(lbl);

	return btn;
}


lv_obj_t* lblBatterySOC_sym = NULL;
lv_obj_t* lblTempC = NULL;
static void create_battery_SOC_temp_symbol(lv_obj_t* parent)
{

	if(lblBatterySOC_sym != NULL)
		return;

	/* container */
	lv_obj_t* cont = lv_obj_create(parent);
	
	lv_obj_align(cont, LV_ALIGN_TOP_RIGHT, 0,0);
	lv_obj_remove_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_add_style(cont, &lvStyleInvisibleCont, 0);
	lv_obj_set_layout(cont, LV_LAYOUT_FLEX);
	lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW);
	lv_obj_set_size(cont, 64,32);


	lblBatterySOC_sym = lv_label_create(cont);
	lv_obj_set_style_text_font(lblBatterySOC_sym, &LV_FONT_AWESOME, 0);
	lv_obj_set_style_text_color(lblBatterySOC_sym, COLOR_BUTTON, 0);
	lv_label_set_text(lblBatterySOC_sym, LV_SYMBOL_BATTERY_EMPTY);


	lblTempC = lv_label_create(cont);
	lv_obj_set_style_text_font(lblTempC, &FONT_SMALL, 0);
	lv_obj_set_style_text_color(lblTempC, COLOR_BUTTON, 0);
	lv_obj_set_style_pad_all(lblTempC, 0, 0);
	lv_label_set_text(lblTempC, "25" DEGREE_SIGN);


}
/************************ Generic widgets END ***********************/
void GUI_set_temperature(int16_t v)
{
	char buf[8];
	if(lblTempC)
	{
		if((v > -255) && (v < 255))
		{

			my_i16toa(buf, v);
			strcat(buf, DEGREE_SIGN);
			buf[sizeof(buf) - 1] = 0;
		}
		lv_label_set_text(lblTempC, buf);
	}
}

void GUI_set_battery_SOC(uint8_t i)
{
	if(lblBatterySOC_sym)
	{
		switch(i)
		{
		case 0:
			lv_label_set_text(lblBatterySOC_sym, LV_SYMBOL_BATTERY_EMPTY);
			break;
		case 1:
			lv_label_set_text(lblBatterySOC_sym, LV_SYMBOL_BATTERY_1);
			break;
		case 2:
			lv_label_set_text(lblBatterySOC_sym, LV_SYMBOL_BATTERY_2);
			break;
		case 3:
			lv_label_set_text(lblBatterySOC_sym, LV_SYMBOL_BATTERY_3);
			break;
		case 4:
			lv_label_set_text(lblBatterySOC_sym, LV_SYMBOL_BATTERY_FULL);
			break;
		default:
			lv_label_set_text(lblBatterySOC_sym, LV_SYMBOL_BATTERY_EMPTY);
		}
	}
}

/*********************************************************
 * Touch screen calibration BEGIN
 * ******************************************************/
lv_obj_t* lvContCalib = NULL; // touch screen calibration background
lv_obj_t* lvLblCalibPrompt = NULL; // text: Calibrating
lv_obj_t* lvCircTouchPoint = NULL;
lv_group_t* lvGroupTouchScreenCalib = NULL;
// Button Event: dismiss touchscreen
static void btn_event_cb_dismiss_touchscreen_calib(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t* obj = lv_event_get_target(e);
	if(code == LV_EVENT_CLICKED)
	{

		dismiss_touchscreen_calib_widgets();

	}
}
// create calibration scene
//TODO: update LVGLv7 to LVGLv9
void display_touchscreen_calib_widgets()
{

	init_styles();
	lv_obj_t* scr = lv_layer_sys();
	// Solid color fullscreen Background
	if(lvContCalib == NULL)
	{
		lvContCalib = lv_obj_create(scr);
		lv_obj_set_size(lvContCalib, LV_HOR_RES_MAX, LV_VER_RES_MAX);
		lv_obj_align(lvContCalib, LV_ALIGN_BOTTOM_LEFT, 0, 0);
		lv_obj_set_style_bg_color(lvContCalib, COLOR_WHITE, 0);
		// group
		lvGroupTouchScreenCalib = lv_group_create();
		register_group(lvGroupTouchScreenCalib);
	}
	// Prompt Text and exit button(when no touch screen avail)
	if(lvLblCalibPrompt == NULL)
	{
		// Prompt text
		lvLblCalibPrompt = lv_label_create(lvContCalib);
		lv_obj_set_style_text_align(lvLblCalibPrompt, LV_TEXT_ALIGN_CENTER, 0);
		lv_label_set_text(lvLblCalibPrompt, "校准触屏");
		lv_obj_set_style_text_color(lvLblCalibPrompt, COLOR_BLACK, 0);
		lv_obj_align(lvLblCalibPrompt, LV_ALIGN_CENTER, 0, 50);
		// Exit button
		lv_obj_t* lvBtnExit = lv_button_create(lvContCalib);
		lv_obj_align(lvBtnExit, LV_ALIGN_BOTTOM_MID, 0, -10);
		lv_obj_add_event_cb(lvBtnExit, btn_event_cb_dismiss_touchscreen_calib, LV_EVENT_CLICKED, NULL);
		lv_group_add_obj(lvGroupTouchScreenCalib, lvBtnExit);
		lv_obj_t* lvLblBtnExit = lv_label_create(lvBtnExit);
		lv_label_set_text(lvLblBtnExit, "取消");
	}
	// Touch Point
	if(lvCircTouchPoint == NULL)
	{
		lvCircTouchPoint = lv_image_create(lvContCalib);
		lv_image_set_src(lvCircTouchPoint, &character_icon_small_transp);
		lv_obj_set_style_bg_opa(lvCircTouchPoint, LV_OPA_0, 0);
//		lv_obj_set_style_bg_color(lvCircTouchPoint, COLOR_ACCENT, 0);
		lv_obj_set_align(lvCircTouchPoint, LV_ALIGN_CENTER);
		lv_obj_set_pos(lvCircTouchPoint, LV_HOR_RES_MAX * (0.1 - 0.5), LV_VER_RES_MAX * (0.1 - 0.5) ); // zeroed at screen center
	}

}


void dismiss_touchscreen_calib_widgets()
{

	deregister_group(lvGroupTouchScreenCalib);
	lvGroupTouchScreenCalib = NULL;
	lv_obj_del_async(lvContCalib);
	lvContCalib = NULL;
	lvLblCalibPrompt = NULL;
	lvCircTouchPoint = NULL;

}


/* 出错警告msgbox */
#ifndef LVGL_SIM
osSemaphoreId_t sphWarnMsgBoxDismissed = NULL;
#endif
lv_group_t* lvGroupMsgboxWarning = NULL;
lv_obj_t* lvMsgBoxWarning = NULL;
lv_obj_t* lvMsgBoxImg = NULL;
static void msgbox_event_handler(lv_event_t* e)
{

	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t* obj = lv_event_get_target(e);
	if(code == LV_EVENT_CLICKED)
	{
		lv_obj_del(lvMsgBoxWarning);
		lvMsgBoxWarning = NULL;
		if (lvMsgBoxImg)
		{
			lv_obj_del(lvMsgBoxImg);
			lvMsgBoxImg = NULL;
		}
		deregister_group(lvGroupMsgboxWarning);
		lvGroupMsgboxWarning = NULL;

	}
}

static const uint16_t MSGBOX_WIDTH = LV_HOR_RES_MAX / 2;
static const uint16_t MSGBOX_HEIGHT = LV_VER_RES_MAX / 2;
#ifndef LVGL_SIM
static const osSemaphoreAttr_t WarnMsgBoxDismissedSemaphore_attr = {
	  .name = "WMsgBoxDismiss",
	  .cb_mem = NULL,
	  .cb_size = 0,
};
#endif

static lv_obj_t* show_msgbox(const char* title, const char* content, const void* imgsrc)
{
	// use a semaphore to notify the Alert task that the popup window has been dismissed
#ifndef LVGL_SIM
	if(sphWarnMsgBoxDismissed == NULL)
	{
		sphWarnMsgBoxDismissed = osSemaphoreNew(1, 0, &WarnMsgBoxDismissedSemaphore_attr);
	}
#endif
	if (lvMsgBoxWarning != NULL)
		goto CREATE_LV_MSGBOX_WARNING_EXIT;
	lv_obj_t* scr = lv_scr_act();
	lvGroupMsgboxWarning = lv_group_create();
	register_group(lvGroupMsgboxWarning);
	lvMsgBoxWarning = lv_msgbox_create(scr);
	lv_obj_set_size(lvMsgBoxWarning, MSGBOX_WIDTH, MSGBOX_HEIGHT);
	lv_obj_add_style(lvMsgBoxWarning, &lvStyleMsgBox, 0);
	lv_obj_add_flag(lvMsgBoxWarning, LV_OBJ_FLAG_CLICKABLE);
	if (title)
		lv_msgbox_add_title(lvMsgBoxWarning, title);
	if (content)
		lv_msgbox_add_text(lvMsgBoxWarning, content);
	//lv_msgbox_add_close_button(lvMsgBoxWarning);
	lv_obj_center(lvMsgBoxWarning);
	if (imgsrc)
	{
		lvMsgBoxImg = lv_image_create(scr);
		lv_image_set_src(lvMsgBoxImg, imgsrc);
		lv_image_set_align(lvMsgBoxImg, LV_IMAGE_ALIGN_CENTER);
		int w = ((lv_image_t*)lvMsgBoxImg)->w;
		lv_obj_align_to(lvMsgBoxImg, lvMsgBoxWarning, LV_ALIGN_OUT_RIGHT_MID, -w /3, 0);
	}
	lv_obj_add_event_cb(lvMsgBoxWarning, msgbox_event_handler, LV_EVENT_CLICKED, NULL);
	lv_group_add_obj(lvGroupMsgboxWarning, lvMsgBoxWarning);
	lv_group_set_editing(lvGroupMsgboxWarning, true);
CREATE_LV_MSGBOX_WARNING_EXIT:
	return lvMsgBoxWarning;
}


lv_obj_t* show_msgbox_warning(const char* content)
{
	return show_msgbox("プップーですわ", content, &warning_img);
}

lv_obj_t* show_msgbox_farewell()
{
	return show_msgbox("またお会いしましょう", STR_FAREWELL, &farewell_img);
}
/* 出错警告msgbox End */
/*********************************************************
 * Touch screen calibration END
 * ******************************************************/


uint8_t number2text(char* dest,
		int number, // value without decimal point
		uint8_t decimal, // digits of decimals (0: integer, 1: 1.1, 2: 1.10)
		char suffix)
{
	uint8_t digits[9];
	uint8_t n = 0, i = 0;
	if (number < 0)
	{
		number = -number;
		dest[i++] = '-';
	}

	do
	{
		digits[n++] = number % 10 + '0';
		number /= 10;
		if(n == decimal)
		    digits[n++] = '.';
	}while(number);
	// append 0 before dot
	while(n <= decimal)
	{
		if(n < decimal)
			digits[n++] = '0';
		else
			digits[n++] = '.';
	}
	// append 0 after trailing dot
	if(n > 0 && digits[n-1] == '.')
		digits[n++] = '0';
	while(n)
	{
		dest[i++] = digits[--n];
	}
	dest[i] = suffix;
	if(suffix != 0) dest[++i] = 0;
	return i;
}

void my_i16toa(char* dest, int16_t num)
{
	char* start = dest;
    if(num)
    {
        bool sign=num<0;
        num=sign?-num:num;
        while(num)
        {
            uint8_t n=num%10;
            *dest++ = '0'+n;
            num /= 10;
        }
        if(sign)
            *dest++='-';
        *dest=0;
//        reverse(res);
        dest--;
        while(start<dest)
        {
        	char temp = *start;
        	*start = *dest;
        	*dest = temp;
        	start++;
        	dest--;
        }
    }
    else
    {
        *dest++='0';
        *dest='\0';
    }
}

void my_utoa(char* dest, uint32_t num)
{
	char* start = dest;
    if(num)
    {
//        sign=num<0;
//        num=sign?-num:num;
        while(num)
        {
            uint8_t n=num%10;
            *dest++ = '0'+n;
            num /= 10;
        }
//        if(sign)
//            *dest++='-';
        *dest=0;
//        reverse(res);
        dest--;
        while(start<dest)
        {
        	char temp = *start;
        	*start = *dest;
        	*dest = temp;
        	start++;
        	dest--;
        }
    }
    else
    {
        *dest++='0';
        *dest='\0';
    }
}




#ifdef __cplusplus
} /*extern "C"*/
#endif


