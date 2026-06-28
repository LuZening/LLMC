/*
 * touch_HR2046.c
 *
 *  Created on: Jun 28, 2020
 *      Author: Zening
 */

#include "touch_HR2046.h"
#include "main.h"
#include "string.h"

#ifndef ABS
#define ABS(x) (((x) > 0) ? (x) : (-(x)))
#endif

#define hspi hspi1

TouchScreenCalibrationInfo_t *pTSCalib = NULL;

static bool ols_2_var_with_const(int* Y, int *X1, int *X2, uint8_t n, float *params);
static void reverse_ols_2_var_with_const(int* X1, int* X2, float* params,/* out */ int* Y, size_t n);

static bool ols_1_var_with_const(int* Y, int *X1, uint8_t n, float *params);
static void reverse_ols_1_var_with_const(int* X1, float* params,/* out */ int* Y, size_t n);


void init_touch_screen_calib_info(TouchScreenCalibrationInfo_t *p)
{
    p->isCalibrated = false;
}


static bool ols_2_var_with_const(int* Y, int *X1, int *X2, uint8_t n, float *params)
{
    float sum_x1 = 0, sum_x2 = 0, sum_y = 0;
    float sum_x1_sq = 0, sum_x2_sq = 0, sum_x1_x2 = 0;
    float sum_x1_y = 0, sum_x2_y = 0;

    for (int i = 0; i < n; i++) {
        sum_x1 += X1[i];
        sum_x2 += X2[i];
        sum_y += Y[i];
        sum_x1_sq += X1[i] * X1[i];
        sum_x2_sq += X2[i] * X2[i];
        sum_x1_x2 += X1[i] * X2[i];
        sum_x1_y += X1[i] * Y[i];
        sum_x2_y += X2[i] * Y[i];
    }

    float det = sum_x1_sq * sum_x2_sq - sum_x1_x2 * sum_x1_x2;
    if ((det > -1e-4) && (det < 1e-4)) {
        // singular
        return false;
    }

    float beta1 = (float)(sum_x2_sq * sum_x1_y - sum_x1_x2 * sum_x2_y) / (float)det;
    float beta2 = (float)(sum_x1_sq * sum_x2_y - sum_x1_x2 * sum_x1_y) / (float)det;
    float beta0 = (float)(sum_y - beta1 * sum_x1 - beta2 * sum_x2) / (float)n;

    params[0] = beta0;
    params[1] = beta1;
    params[2] = beta2;

    // check error
    int Yhat[5];
    int m = (n < 5)?(n):5;
    reverse_ols_2_var_with_const(X1, X2, params, Yhat, m);
    int sum_error = 0;
    int sum_Y = 0;
    for(int i = 0; i < m; ++i)
    {
    	sum_error += ABS(Yhat[i] - Y[i]);
    	sum_Y += ABS(Y[i]);
    }
    int Y_to_error = sum_Y / sum_error;
#ifdef DEBUG
    if(Y_to_error < 10) // error should < 10%
    {
    	return false;
    }
#else
    if(Y_to_error < 20) // error should < 5%
    {
    	return false;
    }
#endif
    return true;
}

static void reverse_ols_2_var_with_const(int* X1, int* X2, float* params,/* out */ int* Y, size_t n)
{
    for (int i = 0; i < n; ++i)
    {
        float y = params[0] + X1[i] * params[1] + X2[i] * params[2];
        Y[i] = (int)y;
    }
}



static bool ols_1_var_with_const(int* Y, int *X1, uint8_t n, float *params)
{
    float x_mean = 0.0f;
    float y_mean = 0.0f;
    float numerator = 0.0f;
    float denominator = 0.0f;

    // 计算x和y的均值
    for (uint8_t i = 0; i < n; i++) {
        x_mean += X1[i];
        y_mean += Y[i];
    }
    x_mean /= n;
    y_mean /= n;

    // 计算斜率
    for (uint8_t i = 0; i < n; i++) {
        numerator += (X1[i] - x_mean) * (Y[i] - y_mean);
        denominator += (X1[i] - x_mean) * (X1[i] - x_mean);
    }
    float beta1 = numerator / denominator;
    float beta0 = y_mean - beta1 * x_mean;

    params[0] = beta0;
    params[1] = beta1;

    // check error
    int Yhat[5];
    int m = (n < 5)?(n):5;
    reverse_ols_1_var_with_const(X1, params, Yhat, m);
    int sum_error = 0;
    int sum_Y = 0;
    for(int i = 0; i < m; ++i)
    {
    	sum_error += ABS(Yhat[i] - Y[i]);
    	sum_Y += ABS(Y[i]);
    }
    int Y_to_error = sum_Y / sum_error;
#ifdef DEBUG
    if(Y_to_error < 10) // error should < 10%
    {
    	return false;
    }
#else
    if(Y_to_error < 20) // error should < 5%
    {
    	return false;
    }
#endif
    return true;
}

static void reverse_ols_1_var_with_const(int* X1, float* params,/* out */ int* Y, size_t n)
{
    for (int i = 0; i < n; ++i)
    {
        float y = params[0] + X1[i] * params[1];
        Y[i] = (int)y;
    }
}

/*
 * @retval: read data
 * */
static uint16_t touch_read_data(uint8_t cmd)
{
    // enable CS
    HAL_GPIO_WritePin(T_CSX_GPIO_Port, T_CSX_Pin, GPIO_PIN_RESET);

    // write command
    uint8_t res = HAL_SPI_Transmit(&hspi, &cmd, 1, HAL_MAX_DELAY);
    UNUSED(res);

    // read 2 data bytes in one transfer (MOSI clocks with 0xFF)
    uint8_t buf[2];
    static const uint8_t txDummy[2] = {0xFF, 0xFF};
    res = HAL_SPI_TransmitReceive(&hspi, (uint8_t *)txDummy, buf, 2, HAL_MAX_DELAY);
    UNUSED(res);
    // The first byte's MSB and last 3 bits of the 12-bit result are invalid
    uint16_t r = (((uint16_t)(buf[0] & 0x7F) << 8) | buf[1]) >> 3;

    // disable CS
    HAL_GPIO_WritePin(T_CSX_GPIO_Port, T_CSX_Pin, GPIO_PIN_SET);
    return r;
}




bool touch_set_calibrate(int real_X[5], int real_Y[5],
        int ADC_X[5], int ADC_Y[5],
        TouchScreenCalibrationInfo_t *result
        )
{
    /* 2 variable OLS
     * x_phy = px0 + px1 * ADC_x + px2 * ADC_y
     * y_phy = py0 + py1 * ADC_x + py2 * ADC_y
     */
    bool r;
    // solve for X axis
//	r = ols_2_var_with_const(real_X, ADC_X, ADC_Y, 5, result->params_X);
    r = ols_1_var_with_const(real_X, ADC_X, 5, result->params_X);
    if(!r)
        return false;
    // solve for Y axis
//	r = ols_2_var_with_const(real_Y, ADC_X, ADC_Y, 5, result->params_Y);
    r = ols_1_var_with_const(real_Y, ADC_Y, 5, result->params_Y);
    if(!r)
        return false;

    if(ADC_X != result->ADC_X)
        memcpy(result->ADC_X, ADC_X, 5*sizeof(uint16_t));
    if(ADC_Y != result->ADC_Y)
        memcpy(result->ADC_Y, ADC_Y, 5*sizeof(uint16_t));
    if(real_X != result->real_X)
        memcpy(result->real_X, real_X, 5*sizeof(uint16_t));
    if(real_Y != result->real_Y)
        memcpy(result->real_Y, real_Y, 5*sizeof(uint16_t));
    result->isCalibrated = true;
    if(pTSCalib == NULL)
        pTSCalib = result;
    return true;
}

int touch_read_XY_ADC(int *pX_orig, int *pY_orig)
{
    // read pressure
    int Z1,Z2,Z;

    Z1 = touch_read_data(HR2046_CMD_READ_Z1);
    Z2 = touch_read_data(HR2046_CMD_READ_Z2);
    // of the touch pressure
    Z = (4095 - Z2) + Z1;
    if(Z < 0)
        Z = -Z;
    // read X
    *pX_orig = touch_read_data(HR2046_CMD_READ_X);
    // read Y
    *pY_orig = touch_read_data(HR2046_CMD_READ_Y);
    return Z;
}

int touch_convert_XY_ADC_to_physical(int ADC_X,int ADC_Y,int ADC_Z,
        int* pX, int* pY, int* pZ)
{
    if(pTSCalib != NULL && pTSCalib->isCalibrated)
    {
        // ADC_X[4]: X_Center
//		*pX = pTSCalib->params_X[0] + pTSCalib->params_X[1] * ADC_X + pTSCalib->params_X[2] * ADC_Y;
        *pX = pTSCalib->params_X[0] + pTSCalib->params_X[1] * ADC_X;
        // ADC_Y[4]: Y_Center
//		*pY = pTSCalib->params_Y[0] + pTSCalib->params_Y[1] * ADC_X + pTSCalib->params_Y[2] * ADC_Y;
        *pY = pTSCalib->params_Y[0] + pTSCalib->params_Y[1] * ADC_Y;
        *pZ = (ADC_Z > 400)?(1):(0);
    }
    else // uncalibrated
    {
        *pX = UINT16_MAX;
        *pY = UINT16_MAX;
        *pZ = 0;
    }
    return *pZ;

}


int touch_read_XY_physical(int *pX, int* pY)
{
    int Z;
    int ADC_X, ADC_Y, ADC_Z;
    ADC_Z = touch_read_XY_ADC(&ADC_X, &ADC_Y);
    return touch_convert_XY_ADC_to_physical(ADC_X, ADC_Y, ADC_Z, pX, pY, &Z);
}

int touch_read_VBAT_mv()
{
    int vADC = touch_read_data(HR2046_CMD_READ_VBAT);
    int mv = (vADC * HR2046_VREF_MV * 4) / (0xFFFFU >> (16 - 12));
    return mv;
}


int touch_read_temperature()
{
    int vADC = touch_read_data(HR2046_CMD_READ_TEMP);
    int vADC_91 = touch_read_data(HR2046_CMD_READ_TEMP_91);
    int delta_mv;
    if(vADC_91 > vADC)
        delta_mv = ((vADC_91 - vADC) * HR2046_VREF_MV) / (0xFFFFU >> (16 - 12));
    else
        delta_mv = 0;
    int temp_C_0_001 = delta_mv * 2573 - 273000;
    return temp_C_0_001 / 1000;
}

