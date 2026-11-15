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

/* 单次 SPI 传输超时(ms)。芯片无响应时必须有限返回，不能永久阻塞任务。
 * 562.5kHz 下 3 字节约 43us，3ms 已有足够余量。 */
#define TOUCH_SPI_TIMEOUT_MS 3U

TouchScreenCalibrationInfo_t *pTSCalib = NULL;

/* 触摸 SPI 总线互斥锁：LVGL 输入任务与电量测量状态机共用 */
static osMutexId_t mtxTouchSPI = NULL;

void touch_spi_lock_init(void)
{
    if (mtxTouchSPI == NULL)
        mtxTouchSPI = osMutexNew(NULL);
}

static bool touch_spi_lock(void)
{
    if (mtxTouchSPI == NULL)
        touch_spi_lock_init();
    if (mtxTouchSPI == NULL)
        return false;
    /* 有限超时：即使状态机异常占住总线，触摸读取也不会永久死等 */
    return (osMutexAcquire(mtxTouchSPI, pdMS_TO_TICKS(50)) == osOK);
}

static void touch_spi_unlock(void)
{
    if (mtxTouchSPI != NULL)
        (void)osMutexRelease(mtxTouchSPI);
}

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
 * @retval: true = 读到有效的 12 位数据; false = SPI 超时/错误
 * */
static bool touch_read_data(uint8_t cmd, uint16_t *pValue)
{
    // enable CS
    HAL_GPIO_WritePin(T_CSX_GPIO_Port, T_CSX_Pin, GPIO_PIN_RESET);

    // write command (有限超时：芯片无响应时不能把调用任务永久卡住)
    if (HAL_SPI_Transmit(&hspi, &cmd, 1, TOUCH_SPI_TIMEOUT_MS) != HAL_OK)
    {
        HAL_GPIO_WritePin(T_CSX_GPIO_Port, T_CSX_Pin, GPIO_PIN_SET);
        return false;
    }

    // read 2 data bytes in one transfer (MOSI clocks with 0xFF)
    uint8_t buf[2];
    static const uint8_t txDummy[2] = {0xFF, 0xFF};
    if (HAL_SPI_TransmitReceive(&hspi, (uint8_t *)txDummy, buf, 2, TOUCH_SPI_TIMEOUT_MS) != HAL_OK)
    {
        HAL_GPIO_WritePin(T_CSX_GPIO_Port, T_CSX_Pin, GPIO_PIN_SET);
        return false;
    }

    // The first byte's MSB and last 3 bits of the 12-bit result are invalid
    *pValue = (((uint16_t)(buf[0] & 0x7F) << 8) | buf[1]) >> 3;

    // disable CS
    HAL_GPIO_WritePin(T_CSX_GPIO_Port, T_CSX_Pin, GPIO_PIN_SET);
    return true;
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
//    r = ols_2_var_with_const(real_X, ADC_X, ADC_Y, 5, result->params_X);
    r = ols_1_var_with_const(real_X, ADC_X, 5, result->params_X);
    if(!r)
        return false;
    // solve for Y axis
//    r = ols_2_var_with_const(real_Y, ADC_X, ADC_Y, 5, result->params_Y);
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
    uint16_t Z1, Z2, adcX, adcY;

    if(!touch_spi_lock())
    {
        /* 总线暂时被占用：本次跳过，避免 LVGL 输入任务死等 */
        *pX_orig = 0;
        *pY_orig = 0;
        return 0;
    }

    if(!touch_read_data(HR2046_CMD_READ_Z1, &Z1) ||
       !touch_read_data(HR2046_CMD_READ_Z2, &Z2) ||
       !touch_read_data(HR2046_CMD_READ_X, &adcX) ||
       !touch_read_data(HR2046_CMD_READ_Y, &adcY))
    {
        /* 芯片无响应：本次当作未触摸，不阻塞、不产生乱点 */
        touch_spi_unlock();
        *pX_orig = 0;
        *pY_orig = 0;
        return 0;
    }

    // of the touch pressure
    int Z = (4095 - (int)Z2) + (int)Z1;
    if(Z < 0)
        Z = -Z;
    *pX_orig = (int)adcX;
    *pY_orig = (int)adcY;

    touch_spi_unlock();
    return Z;
}

int touch_convert_XY_ADC_to_physical(int ADC_X,int ADC_Y,int ADC_Z,
        int* pX, int* pY, int* pZ)
{
    if(pTSCalib != NULL && pTSCalib->isCalibrated)
    {
        // ADC_X[4]: X_Center
//        *pX = pTSCalib->params_X[0] + pTSCalib->params_X[1] * ADC_X + pTSCalib->params_X[2] * ADC_Y;
        *pX = pTSCalib->params_X[0] + pTSCalib->params_X[1] * ADC_X;
        // ADC_Y[4]: Y_Center
//        *pY = pTSCalib->params_Y[0] + pTSCalib->params_Y[1] * ADC_X + pTSCalib->params_Y[2] * ADC_Y;
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

/* 状态机超时保护：调用方异常未推进时强制释放总线 */
#define TOUCH_MEAS_TIMEOUT_MS 100U

/* 温度换算系数：数据手册公式为 T(°C)=2.573*ΔV(mV)-273。
 * 按 25°C 环境温度做单点标定，等效系数为 2.741；
 * 如需更准可在第二个温度点再标定。 */
#define HR2046_TEMP_COEF_X1000 2741

typedef enum
{
    TOUCH_MEAS_IDLE = 0,
    TOUCH_MEAS_BUSY
} touch_meas_state_t;

static touch_meas_state_t meas_state = TOUCH_MEAS_IDLE;
static uint32_t meas_start_tick = 0U;
static uint8_t meas_with_temperature = 0U;
static int meas_vbat_mv = 0;
static int meas_temperature_C = INT16_MIN;

bool touch_meas_start(uint8_t with_temperature)
{
    /* 上一次测量若没有被推进完，超时后强制结束，避免锁死触摸总线 */
    if ((meas_state != TOUCH_MEAS_IDLE) &&
        ((HAL_GetTick() - meas_start_tick) > TOUCH_MEAS_TIMEOUT_MS))
    {
        meas_state = TOUCH_MEAS_IDLE;
        touch_spi_unlock();
    }

    if (meas_state != TOUCH_MEAS_IDLE)
        return false;

    if (mtxTouchSPI == NULL)
        touch_spi_lock_init();
    if ((mtxTouchSPI == NULL) || (osMutexAcquire(mtxTouchSPI, 0U) != osOK))
        return false;

    /* VREF 为板上外部常开的 3.28V，无需先打开/等待基准；
     * 这里只占住总线，实际转换在 touch_meas_step() 中完成 */
    meas_start_tick = HAL_GetTick();
    meas_with_temperature = with_temperature;
    meas_state = TOUCH_MEAS_BUSY;
    return true;
}

int touch_meas_step(void)
{
    if (meas_state == TOUCH_MEAS_IDLE)
        return 1;

    uint16_t vADC;
    if(!touch_read_data(HR2046_CMD_READ_VBAT, &vADC))
    {
        /* 芯片无响应：结束本次测量，保留上一次有效值 */
        meas_state = TOUCH_MEAS_IDLE;
        touch_spi_unlock();
        return 1;
    }
    /* VBAT 内部 1/4 分压，基准为外部 VREF(3250mV)，12 位量化 */
    meas_vbat_mv = ((int)vADC * HR2046_VREF_MV * 4) / 4095;

    if (meas_with_temperature)
    {
        /* VREF 常开，TEMP0/TEMP1 可以直接转换 */
        uint16_t vADC_TEMP0, vADC_TEMP1;
        if(!touch_read_data(HR2046_CMD_READ_TEMP, &vADC_TEMP0) ||
           !touch_read_data(HR2046_CMD_READ_TEMP_91, &vADC_TEMP1))
        {
            meas_temperature_C = INT16_MIN; /* 读数无效 */
        }
        else
        {
            int delta_code = (int)vADC_TEMP1 - (int)vADC_TEMP0;
            if (delta_code > 0)
            {
                /* ΔV(mV) = code * VREF / 4095，先算到 0.01mV 避免整数精度损失 */
                int delta_mv_x100 = (delta_code * HR2046_VREF_MV * 100) / 4095;
                /* T(°C) = coef * ΔV(mV) - 273，coef 已按实测温度做单点标定 */
                int temp_C_x1000 = (delta_mv_x100 * HR2046_TEMP_COEF_X1000) / 100 - 273000;
                meas_temperature_C = temp_C_x1000 / 1000;
            }
            else
            {
                meas_temperature_C = INT16_MIN; /* 读数无效 */
            }
        }
    }

    meas_state = TOUCH_MEAS_IDLE;
    touch_spi_unlock();
    return 1;
}

int touch_meas_get_vbat_mv(void)
{
    return meas_vbat_mv;
}

int touch_meas_get_temperature(void)
{
    return meas_temperature_C;
}


/* 单节锂离子电池典型 OCV-SOC 曲线（静置/轻载）。可按实际电芯与负载情况微调 */
static const struct
{
    uint16_t mv;
    uint8_t percent;
} battery_soc_curve[] =
{
    {4200, 100}, {4150, 95}, {4100, 90}, {4050, 85}, {4000, 80},
    {3950, 75},  {3900, 70}, {3850, 63}, {3800, 56}, {3750, 48},
    {3700, 38},  {3650, 29}, {3600, 21}, {3550, 14}, {3500, 9},
    {3450, 6},   {3400, 4},  {3300, 2},  {3200, 0},
};

int battery_mv_to_soc_percent(int mv)
{
    int n = (int)(sizeof(battery_soc_curve) / sizeof(battery_soc_curve[0]));
    if (mv >= battery_soc_curve[0].mv)
        return battery_soc_curve[0].percent;
    if (mv <= battery_soc_curve[n - 1].mv)
        return battery_soc_curve[n - 1].percent;
    for (int i = 0; i < n - 1; ++i)
    {
        int vHi = battery_soc_curve[i].mv;
        int vLo = battery_soc_curve[i + 1].mv;
        if ((mv <= vHi) && (mv >= vLo))
        {
            int socHi = battery_soc_curve[i].percent;
            int socLo = battery_soc_curve[i + 1].percent;
            /* 分段线性插值 */
            return socLo + (mv - vLo) * (socHi - socLo) / (vHi - vLo);
        }
    }
    return 0;
}

