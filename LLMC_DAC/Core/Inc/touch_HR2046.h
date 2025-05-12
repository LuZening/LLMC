/*
 * touch_HR2046.h
 *
 *  Created on: Jun 28, 2020
 *      Author: Zening
 */

#ifndef TOUCH_HR2046_H_
#define TOUCH_HR2046_H_

#include "stm32h7xx_hal.h"

//#define TOUCHSCREEN_XY_REVERSED

extern SPI_HandleTypeDef hspi1;

#if defined(TOUCHSCREEN_XY_REVERSED)
#define HR2046_CMD_READ_Y 0xD0
#define HR2046_CMD_READ_X 0x90
#else
#define HR2046_CMD_READ_X 0xD0
#define HR2046_CMD_READ_Y 0x90
#endif
//#define HR2046_CMD_READ_Z1 0xb1	/* Read Z1 */
//#define HR2046_CMD_READ_Z2 0xc1    /* read Z2 */
#define HR2046_CMD_READ_Z1 0xb0	/* Read Z1 */
#define HR2046_CMD_READ_Z2 0xc0    /* read Z2 */
#define HR2046_CMD_READ_TEMP 0x84
#define HR2046_CMD_READ_TEMP_91 0xF4
#define HR2046_CMD_READ_VBAT 0xA4

#define TOUCHSCREEN_CALIB_START 0b01
#define TOUCHSCREEN_CALIB_POINT_CAPTURED 0b10

#define TOUCHSCREEN_CALIB_LEFT_MARGIN 0.1
#define TOUCHSCREEN_CALIB_UPPER_MARGIN 0.1

#define HR2046_VREF_MV 2500U

#include <stdbool.h>

typedef struct
{
	bool isCalibrated;
	// UL, UR, LL, LR, Center
	int real_X[5];
	int real_Y[5]; // real pixel positions, originate at the upper-left corner
	// UL, UR, LL, LR, Center
	int ADC_X[5];
	int ADC_Y[5]; // 5 point calibration info
	// calibration parameters y = p0 + p1x1 + p2x2 where x1 is ADC_X, x2 is ADC_Y
	float params_X[3], params_Y[3];
} TouchScreenCalibrationInfo_t;

// allocate pTSCalib somewhere in main program
// e.g. load from EEPROM
extern TouchScreenCalibrationInfo_t *pTSCalib;
void init_touch_screen_calib_info(TouchScreenCalibrationInfo_t *p);



// set and analyze raw calibration info
// @param real_X, real_Y: the ideal position of 5 points, (UL, UR, LL, LR, C)
// @param ADC_X, ADC_Y: the ADC readings of 5 points when calibratings
// @retval result: a place to store calibration info
bool touch_set_calibrate(int real_X[5], int real_Y[5],
		int ADC_X[5], int ADC_Y[5],
		TouchScreenCalibrationInfo_t *result
		);

// read x,y as original ADC values, and return pressure value
int touch_read_XY_ADC(int *pX_orig, int *pY_orig);

// read x,y mapped to physical world, and return whether screen is pressed (1 = pressed, 0 = released)
int touch_read_XY_physical(int *pX, int *pY);

int touch_convert_XY_ADC_to_physical(int ADC_X,int ADC_Y,int ADC_Z,
		int* pX, int* pY, int* pZ);

int touch_read_VBAT_mv();

int touch_read_temperature();

#endif /* TOUCH_HR2046_H_ */
