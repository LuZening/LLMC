#ifndef __LVGL_GUI_H
#define __LVGL_GUI_H

#ifdef __cplusplus
extern "C" {
#endif
#define LV_USE_SIMPLE_FILE_EXPLORER_UNICODE

#include <string.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#ifndef LVGL_SIM
#include "main.h"
#include "cmsis_os2.h"
#include "RotEnc.h"
#else
#include "sim.h"
#endif
#include "utf_convert.h"
#include "lvgl.h"
#include "LVGL_toast.h"
// NOTE: to be compatible with LVGL utf8, paths are not TCHAR
#ifndef LVGL_SIM
extern osMutexId_t mtxTouchScreen;
#define FS_FOLDER_PICTURES _T("0:pictures")
#define FS_FOLDER_PLAYLIST _T("0:playlist")
#define FS_FNAME_PLAYLSIT_1 _T("1.pl")
#define FS_PATH_MUSIC_FOLDER _T("0:music")
#else
#define FS_FOLDER_PICTURES _T("E:/Projects/You/LLMC/FS_SD/sys/pictures")
#define FS_FOLDER_PLAYLIST _T("E:/Projects/You/LLMC/FS_SD/music/playlist")
#define FS_FNAME_PLAYLSIT_1 _T("1.pl")
#define FS_PATH_MUSIC_FOLDER _T("E:/Projects/You/LLMC/FS_SD/test")
#endif
#define FS_FNAME_PLAYLSIT_TEMP _T("temp.pl")
	// acuqire this mutex before modifying LVGL GUI contents
#define LV_HOR_RES_MAX 480U
#define LV_VER_RES_MAX 272U
#define DEGREE_SIGN "°"

extern bool is_back_light_on;
extern bool isGUIReady;
#ifndef LVGL_SIM
//extern osMutexId_t mtxGUIWidgetsHandle;
//
extern uint16_t framebuf1[LV_HOR_RES_MAX * LV_VER_RES_MAX];
extern uint16_t framebuf2A[LV_HOR_RES_MAX * LV_VER_RES_MAX];
//extern uint16_t framebuf2B[LV_HOR_RES_MAX * LV_VER_RES_MAX / 2];
extern void LTDC_flush_cb(lv_display_t * display, const lv_area_t * area, uint8_t * px_map);
extern volatile uint8_t g_gpu_state;
extern void DMA2D_XferCpltCallback(DMA2D_HandleTypeDef *hdma2d);
/* Display device and frame buffer */

/* External Input Controllers */
extern lv_indev_t * lvIndev_touchscreen;
extern void lvGetTouchscreenXY(lv_indev_t* indev, lv_indev_data_t* data);
extern void lvGetRotEnc(lv_indev_t* indev, lv_indev_data_t* data);
extern RotEnc_t rotenc;
extern void init_LVGL_input();
extern void start_LVGL_input_tasks(bool calibrate_touchscreen);
extern void start_calibrate_touchscreen();
extern osThreadId_t TouchScreenTaskHandle;
extern osThreadId_t TouchScreenCalibTaskHandle;

#define TOUCH_SCREEN_PENIRQ_SIGNAL 0b10
// call in EXTI IRQ service for PENIRQ pin
extern void send_touch_screen_penirq_signal();

#endif
extern lv_indev_t* encoder_indev;
extern lv_display_t* display;
extern lv_mutex_t lvLock;
extern bool isPlay_lvgl;
// indicate if playlist file is opened
extern bool isOpen_PL; 
extern FIL filePL;
extern FIL filePL_new;
extern FIL f;
extern uint8_t bufFile[512];
void lv_lock();
void lv_unlock();

#define N_GROUP_STACK_SIZE 16U // 最多可以容纳16层嵌套菜单

lv_group_t* get_current_group();
bool register_group(lv_group_t* p);
bool deregister_group(lv_group_t* p);
bool deregister_group_without_del(lv_group_t* p);
void init_volume_control(); // call at MCU boot as early as possible
void sync_volumes(bool nonblock);  // call after config loaded;
void init_LVGL_GUI();

extern const uint16_t BACK_BTN_WIDTH;
extern const uint16_t BACK_BTN_HEIGHT;

extern const lv_color_t COLOR_PLAIN_TEXT;
extern const lv_color_t COLOR_BG;
extern const lv_color_t COLOR_ACCENT; // lovelive style redish
extern const lv_color_t COLOR_SEMI_TRANSP;
extern const lv_color_t COLOR_BLACK;
extern const lv_color_t COLOR_SHADOW;
extern const lv_color_t COLOR_WHITE;
extern const lv_color_t COLOR_BUTTON; // lovelive AS pink-ish button
/*****************************************************************
 *  fonts BEGIN
 ****************************************************************/
// NOTE: disable place data in their own sections in GCC Compiler Optimization settings
LV_FONT_DECLARE(MyCJK_12)
#define LV_FONT_12 MyCJK_12
#define FONT_SMALL MyCJK_12

LV_FONT_DECLARE(MyCJK_16)
#define LV_FONT_16 MyCJK_16
#define FONT_MED MyCJK_16

LV_FONT_DECLARE(MyAwesome_24)
#define LV_FONT_AWESOME_LARGE MyAwesome_24
LV_FONT_DECLARE(MyAwesome_16)
#define LV_FONT_AWESOME MyAwesome_16
LV_FONT_DECLARE(MyAwesome_12)
#define LV_FONT_AWESOME_SMALL MyAwesome_12

/*****************************************************************
 *  fonts END
 ****************************************************************/

 // 小号英文数字
extern lv_style_t lvStyleTextSmall;
// 大号英文数字
extern lv_style_t lvStyleTextLarge;

// 半透明底框
extern lv_style_t lvStyleSemiTranspFrame;
extern lv_style_t lvStyleInvisibleCont;
//按钮样式
extern lv_style_t lvStyleBtn;
extern lv_style_t lvStyleBtn_focused;

extern lv_style_t lvStyleImageBtn;


/*****************************************************************
 *  utils BEGIN
 ****************************************************************/
// return the length of
uint8_t number2text(char* dest,
	int number, // value without decimal point
	uint8_t decimal, // digits of decimals (0: integer, 1: 1.1, 2: 1.10)
	char suffix);

void my_utoa(char* dest, uint32_t num);
void my_i16toa(char* dest, int16_t num);

/*****************************************************************
 *  utils END
 ****************************************************************/

/*****************************************************************
 *  LCD hardware BEGIN
 ****************************************************************/
void set_back_light(uint8_t state);
void set_display_enable(uint8_t state);
/*****************************************************************
 *  LCD hardware END
 ****************************************************************/

/*****************************************************************
 *  display scenes BEGIN
 ****************************************************************/
extern lv_obj_t* lvTouchCursor;
// create main scene
void display_main_widgets();
void display_audio_signal_path_screen();
void display_FS_player_screen();
void display_settings_screen();

/* TS Calib Scene Methods begin */
extern lv_obj_t* lvContCalib; // touch screen calibration background
extern lv_obj_t* lvLblCalibPrompt; // text: Calibrating
extern lv_obj_t* lvCircTouchPoint;
extern lv_group_t* lvGroupTouchScreenCalib;
// create calibration scene
void display_touchscreen_calib_widgets();
void dismiss_touchscreen_calib_widgets();
void GUI_set_TSCalib_touchpoint_pos(uint16_t real_X, uint16_t real_Y);
/* TS Calib Scene Methods end */

lv_obj_t* create_back_button(lv_obj_t* parent, lv_group_t* group, void (cb)(void));


void display_volume_control_screen();
/*****************************************************************
 *  display scenes END
 ****************************************************************/



/*****************************************************************
 *  player BEGIN
 ****************************************************************/
// track_info must be static,
typedef struct
{
	char sTitle[128];
	char sAuthor[32];
	char sGenre[32];
	bool is_in_playlist;
	uint32_t duration_ms;
} track_info_t;

extern track_info_t track_info;
int GUI_notify_change_track(track_info_t* p, size_t duration_ms, const TCHAR* wcsFname, size_t cntFname, const TCHAR* wcsFullPath, size_t cntFullPath);
void GUI_notify_play_pause(uint8_t state);
void GUI_notify_sample_rate_and_bitdepth(uint32_t fsHz, uint8_t bitdepth);
/*****************************************************************
 *  player END
 ****************************************************************/


 /*****************************************************************
  *  DSP BEGIN
  ****************************************************************/
lv_obj_t* create_dsp_ui(int idx_DSP);
void dismiss_dsp_ui(lv_obj_t* cont, int idx_DSP);
/*****************************************************************
 *  DSP END
 ****************************************************************/


/*****************************************************************
 *  utils BEGIN
 ****************************************************************/
lv_obj_t* show_msgbox_warning(const char* content);
lv_obj_t* show_msgbox_farewell();
/*****************************************************************
 *  utils BEGIN
 ****************************************************************/

 /*****************************************************************
  *  external interfaces  BEGIN
  ****************************************************************/
void GUI_set_temperature(int16_t v);
void GUI_set_battery_SOC(uint8_t i);
extern bool audio_analog_chips_need_reset;
void reset_audio_analog_chips();
/*****************************************************************
 *  external interfaces BEGIN
 ****************************************************************/


#ifdef __cplusplus
}
#endif

#endif
