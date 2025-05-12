/*
 * LVGL_input.c
 *
 *  Created on: Mar 3, 2024
 *      Author: cpholzn
 */
#ifdef __cplusplus
extern "C" {
#endif


#include <stdlib.h>
#include "LVGL_GUI.h"
#include "touch_HR2046.h"
#include "RotEnc.h"
#include "Config.h"
#include "LVGL_toast.h"


/* External Input Controllers */
lv_indev_t * lvIndev_touchscreen;
lv_indev_t * encoder_indev;



static TouchScreenCalibrationInfo_t tsCalib;

/* Touch Screen */
osMutexId_t mtxTouchScreen = NULL;
// following variables are sync by mutex
bool isTSCalibrating = false;//indicate if the user is running touch screen calibration
static bool TS_touched = false;
static int TS_X, TS_Y, TS_Z; // the latest touch screen X, Y position
static int TS_X_ADC, TS_Y_ADC, TS_Z_ADC; // the latest touch screen X,Y ADC values



//osThreadId_t TouchScreenTaskHandle = NULL;
//const osThreadAttr_t TouchScreenTask_attr = {
//  .name = "TSTask",
//  .stack_size = 2048,
//  .priority = (osPriority_t) osPriorityRealtime,
//};
//void task_touchscreen();

osThreadId_t TouchScreenCalibTaskHandle = NULL; // Touch screen calibration task, shows the calibration screen and takes 5 points
const osThreadAttr_t TouchScreenCalibTask_attr = {
  .name = "TSCalib",
  .stack_size = 2048,
  .priority = (osPriority_t) osPriorityBelowNormal1,
};
void task_touch_screen_calibrate();


/* Rotary Encoder */



void init_LVGL_input()
{

	/* init RotEnc */
	RotEnc_init(&rotenc, 2000U, ENCA_GPIO_Port, ENCA_Pin, ENCB_GPIO_Port, ENCB_Pin, ENCSW_GPIO_Port, ENCSW_Pin);
	/*Register the driver in LVGL and save the created input device object*/
	// init touch screen
	lvIndev_touchscreen = lv_indev_create();
	lv_indev_set_type(lvIndev_touchscreen, LV_INDEV_TYPE_POINTER);
	lv_indev_set_read_cb(lvIndev_touchscreen, lvGetTouchscreenXY);

	// init Rotary Encoder
	encoder_indev = lv_indev_create();
	lv_indev_set_type(encoder_indev, LV_INDEV_TYPE_ENCODER);
	lv_indev_set_read_cb(encoder_indev, lvGetRotEnc);

}


void start_LVGL_input_tasks(bool calibrate_touchscreen)
{
	TouchScreenCalibTaskHandle = osThreadNew(task_touch_screen_calibrate, NULL, &TouchScreenCalibTask_attr);
	if(calibrate_touchscreen)
	{
//		TouchScreenCalibTaskHandle = osThreadNew(task_touch_screen_calibrate, NULL, &TouchScreenCalibTask_attr);
		start_calibrate_touchscreen();
	}
	else // use existing calibration info
	{
		touch_set_calibrate(cfg.TSCalibInfo.real_X, cfg.TSCalibInfo.real_Y,
				cfg.TSCalibInfo.ADC_X, cfg.TSCalibInfo.ADC_Y, &cfg.TSCalibInfo);
	}

}

void start_calibrate_touchscreen()
{
	if(TouchScreenCalibTaskHandle)
		osThreadFlagsSet(TouchScreenCalibTaskHandle, TOUCHSCREEN_CALIB_START);
}

// FreeRTOS task for reading touch screen events:
#define N_SAMPLES  4U
void task_touch_screen_calibrate()
{
    /* This task is going to use floating point operations. Therefore it calls
       portTASK_USES_FLOATING_POINT() once on task entry, before entering the loop
       that implements its functionality. From this point on the task has a floating
       point context.
       This task also conflicts with USB MSC mode, for the two applications will race on one SDIO device
       */
	portTASK_USES_FLOATING_POINT();
	for(;;)
	{
		osThreadFlagsWait(TOUCHSCREEN_CALIB_START, osFlagsWaitAll, osWaitForever);
		lv_lock();
		display_touchscreen_calib_widgets();
		lv_unlock();
		osDelay(pdMS_TO_TICKS(200));

		static bool isPointConfirmed = false;
		uint8_t i;
		/* wait for a signal to start the task */
	//		if(!TSCalibAfterStart) // 如果设置在上电时即启动校准界面，则不用等待标志位
	//			osThreadFlagsWait(TOUCHSCREEN_CALIB_START, osFlagsWaitAll, osWaitForever);
		/* show calibration scene */
		isTSCalibrating = true;
		/* init calibration config */
		// Upper Left
		cfg.TSCalibInfo.real_X[0] = TOUCHSCREEN_CALIB_LEFT_MARGIN * LV_HOR_RES_MAX;
		cfg.TSCalibInfo.real_Y[0] = TOUCHSCREEN_CALIB_UPPER_MARGIN * LV_VER_RES_MAX;
		// Upper Right
		cfg.TSCalibInfo.real_X[1] = (1. - TOUCHSCREEN_CALIB_LEFT_MARGIN) * LV_HOR_RES_MAX;
		cfg.TSCalibInfo.real_Y[1] = TOUCHSCREEN_CALIB_UPPER_MARGIN * LV_VER_RES_MAX;
		// Lower Left
		cfg.TSCalibInfo.real_X[2] = TOUCHSCREEN_CALIB_LEFT_MARGIN * LV_HOR_RES_MAX;
		cfg.TSCalibInfo.real_Y[2] = (1. - TOUCHSCREEN_CALIB_UPPER_MARGIN) * LV_VER_RES_MAX;
		// Lower Right
		cfg.TSCalibInfo.real_X[3] = (1. - TOUCHSCREEN_CALIB_LEFT_MARGIN) * LV_HOR_RES_MAX;
		cfg.TSCalibInfo.real_Y[3] = (1. - TOUCHSCREEN_CALIB_UPPER_MARGIN) * LV_VER_RES_MAX;
		// Center
		cfg.TSCalibInfo.real_X[4] = 0.5 * LV_HOR_RES_MAX;
		cfg.TSCalibInfo.real_Y[4] = 0.5 * LV_VER_RES_MAX;
		/* Wait for touches */
		osThreadFlagsClear(0xffff); // clear all flags before entering calibration process
		while(1)
		{
			for(i = 0; i < 5; ++i)
			{
				// GUI move touch point to desired position
				lv_lock();
				lv_obj_set_pos(lvCircTouchPoint,
						cfg.TSCalibInfo.real_X[i] - LV_HOR_RES_MAX / 2 , cfg.TSCalibInfo.real_Y[i] -  LV_VER_RES_MAX / 2); // REFERENCED TO CENTER
				lv_unlock();
	//			osThreadFlagsClear(0xffff);
	//			osDelay(pdMS_TO_TICKS(200));
				osDelay((pdMS_TO_TICKS(500))); // delay 500ms
				osThreadFlagsClear(0xffff);

				osThreadFlagsWait(TOUCHSCREEN_CALIB_POINT_CAPTURED, osFlagsWaitAll, osWaitForever);

				osMutexAcquire(mtxTouchScreen, osWaitForever);
				cfg.TSCalibInfo.ADC_X[i] = TS_X_ADC;
				cfg.TSCalibInfo.ADC_Y[i] = TS_Y_ADC;
				osMutexRelease(mtxTouchScreen);
			}
			bool isSolved = touch_set_calibrate(cfg.TSCalibInfo.real_X, cfg.TSCalibInfo.real_Y,
					cfg.TSCalibInfo.ADC_X, cfg.TSCalibInfo.ADC_Y, &cfg.TSCalibInfo);
			if(!isSolved)
			{
				// show a warning
				lv_lock();
				show_toast("外れ値が含まれているので、もう一回してみよう～");
				lv_unlock();
			}
			else // done!
			{
				break;
			}
			// if not solved (the OLS matrix is singular or error too large), redo the calibration process
		}
		isTSCalibrating = false;
		/* Touch screen calibration finished */
		osMutexAcquire(mtxConfig, osWaitForever);
		isModified = true;
		osMutexRelease(mtxConfig);
		lv_lock();
		dismiss_touchscreen_calib_widgets();
		display_main_widgets();
		lv_unlock();
	//		}
	}
}

static void insertionSort(int arr[], int n) {
	// ascending
    int i, key, j;
    for (i = 1; i < n; i++) {
        key = arr[i];
        j = i - 1;
        while (j >= 0 && arr[j] > key) {
            arr[j + 1] = arr[j];
            j = j - 1;
        }
        arr[j + 1] = key;
    }
}


void lvGetTouchscreenXY(lv_indev_t* indev, lv_indev_data_t* data)
{
	// touch screen deactivated when backlight turned off
	if(!is_back_light_on) return;

	data->state = LV_INDEV_STATE_RELEASED;
	const int Z_threshold = 300;
	static int X[N_SAMPLES], Y[N_SAMPLES],Z[N_SAMPLES];
	int avg_X = 0, avg_Y = 0, avg_Z = 0;
//	taskENTER_CRITICAL();
	for(int i = 0; i < N_SAMPLES; ++i)
	{
		Z[i] = touch_read_XY_ADC(&X[i], &Y[i]);
	}
//	taskEXIT_CRITICAL();
	// mid-value filtering
	insertionSort(Z, N_SAMPLES);
	if(Z[1] > Z_threshold) // make sure almost all samples are pressed solid
	{
		TS_Z_ADC =(Z[N_SAMPLES >> 1] + Z[(N_SAMPLES >> 1) - 1]) / 2;
		insertionSort(X, N_SAMPLES);
		TS_X_ADC = (X[N_SAMPLES >> 1] + X[(N_SAMPLES >> 1) - 1]) / 2;
		insertionSort(Y, N_SAMPLES);
		TS_Y_ADC = (Y[N_SAMPLES >> 1] + Y[(N_SAMPLES >> 1) - 1]) / 2;
		// TODO: impelemt indev touchscreen cb
		if(TS_Z_ADC > Z_threshold)
		{
			if(isTSCalibrating)
			{
				osThreadFlagsSet(TouchScreenCalibTaskHandle, TOUCHSCREEN_CALIB_POINT_CAPTURED);
			}
			else
			{
				touch_convert_XY_ADC_to_physical(TS_X_ADC, TS_Y_ADC, TS_Z_ADC, &TS_X, &TS_Y, &TS_Z );
				data->point.x = TS_X;
				data->point.y = TS_Y;
				data->state = LV_INDEV_STATE_PRESSED;
				// move cursor
				if(lvTouchCursor)
					lv_obj_set_pos(lvTouchCursor, TS_X - LV_HOR_RES_MAX / 2, TS_Y - LV_VER_RES_MAX / 2);

			}

		}
	}
	data->continue_reading = false;
}

void lvGetRotEnc(lv_indev_t* indev, lv_indev_data_t* data)
{

	static uint32_t cntPrev = 0;
	data->enc_diff = rotenc.cntRot - cntPrev;
	cntPrev = rotenc.cntRot;
//	if(rotenc.SW.state == 0) // state=0 means pressed
	if(HAL_GPIO_ReadPin(ENCSW_GPIO_Port, ENCSW_Pin) == 0)
		data->state = LV_INDEV_STATE_PRESSED;
	else // state=1 means released
		data->state = LV_INDEV_STATE_RELEASED;
	data->continue_reading = false;
}

#ifdef __cplusplus
}
#endif
