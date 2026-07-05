/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : usbd_storage_if.c
  * @version        : v1.0_Cube
  * @brief          : Memory management layer.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "usbd_storage_if.h"

//#include "main.h"
#include "main.h"
extern SD_HandleTypeDef hsd1;
#include "sd_diskio.h"
#include "FreeRTOS.h"
/* USER CODE END INCLUDE */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/

/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/

/* USER CODE END PV */

/** @addtogroup STM32_USB_OTG_DEVICE_LIBRARY
  * @brief Usb device.
  * @{
  */

/** @defgroup USBD_STORAGE
  * @brief Usb mass storage device module
  * @{
  */

/** @defgroup USBD_STORAGE_Private_TypesDefinitions
  * @brief Private types.
  * @{
  */

/* USER CODE BEGIN PRIVATE_TYPES */

/* USER CODE END PRIVATE_TYPES */

/**
  * @}
  */

/** @defgroup USBD_STORAGE_Private_Defines
  * @brief Private defines.
  * @{
  */

#define STORAGE_LUN_NBR                  1
#define STORAGE_BLK_NBR                  0x10000
#define STORAGE_BLK_SIZ                  0x200
#define SYSTICK_COUNT_TIMEOUT             (256000U * 300U)

/* USER CODE BEGIN PRIVATE_DEFINES */
/* TIM6 timeout for SD card busy-wait (APB1 timer clock = 128MHz) */
#define SD_TIMEOUT_PRESCALER  (128000U - 1U)  /* 128MHz / 128000 = 1kHz (1ms tick) */
#define SD_TIMEOUT_PERIOD     (2000U - 1U)     /* ARR = 2000 ticks = 2000ms */
/* USER CODE END PRIVATE_DEFINES */

/**
  * @}
  */

/** @defgroup USBD_STORAGE_Private_Macros
  * @brief Private macros.
  * @{
  */

/* USER CODE BEGIN PRIVATE_MACRO */

/* USER CODE END PRIVATE_MACRO */

/**
  * @}
  */

/** @defgroup USBD_STORAGE_Private_Variables
  * @brief Private variables.
  * @{
  */

/* USER CODE BEGIN INQUIRY_DATA_HS */
/** USB Mass storage Standard Inquiry Data. */
const int8_t STORAGE_Inquirydata_HS[] = {/* 36 */

  /* LUN 0 */
  0x00,
  0x80,
  0x02,
  0x02,
  (STANDARD_INQUIRY_DATA_LEN - 5),
  0x00,
  0x00,
  0x00,
  'L', 'Z', 'N', ' ', ' ', ' ', ' ', ' ', /* Manufacturer : 8 bytes */
  'P', 'r', 'o', 'd', 'u', 'c', 't', ' ', /* Product      : 16 Bytes */
  ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
  '0', '.', '0' ,'1'                      /* Version      : 4 Bytes */
};
/* USER CODE END INQUIRY_DATA_HS */

/* USER CODE BEGIN PRIVATE_VARIABLES */

/* USER CODE END PRIVATE_VARIABLES */

/**
  * @}
  */

/** @defgroup USBD_STORAGE_Exported_Variables
  * @brief Public variables.
  * @{
  */

extern USBD_HandleTypeDef hUsbDeviceHS;

/* USER CODE BEGIN EXPORTED_VARIABLES */

/* USER CODE END EXPORTED_VARIABLES */

/**
  * @}
  */

/** @defgroup USBD_STORAGE_Private_FunctionPrototypes
  * @brief Private functions declaration.
  * @{
  */

static int8_t STORAGE_Init_HS(uint8_t lun);
static int8_t STORAGE_GetCapacity_HS(uint8_t lun, uint32_t *block_num, uint16_t *block_size);
static int8_t STORAGE_IsReady_HS(uint8_t lun);
static int8_t STORAGE_IsWriteProtected_HS(uint8_t lun);
static int8_t STORAGE_Read_HS(uint8_t lun, uint8_t *buf, uint32_t blk_addr, uint16_t blk_len);
static int8_t STORAGE_Write_HS(uint8_t lun, uint8_t *buf, uint32_t blk_addr, uint16_t blk_len);
static int8_t STORAGE_GetMaxLun_HS(void);

/* USER CODE BEGIN PRIVATE_FUNCTIONS_DECLARATION */
static void SD_Timeout_Init(void);
static void SD_Timeout_Start(void);
static uint8_t SD_Timeout_HasElapsed(void);
static void SD_Timeout_Stop(void);
/* USER CODE END PRIVATE_FUNCTIONS_DECLARATION */

/**
  * @}
  */

USBD_StorageTypeDef USBD_Storage_Interface_fops_HS =
{
  STORAGE_Init_HS,
  STORAGE_GetCapacity_HS,
  STORAGE_IsReady_HS,
  STORAGE_IsWriteProtected_HS,
  STORAGE_Read_HS,
  STORAGE_Write_HS,
  STORAGE_GetMaxLun_HS,
  (int8_t *)STORAGE_Inquirydata_HS
};

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  .
  * @param  lun: .
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  */
int8_t STORAGE_Init_HS(uint8_t lun)
{
  /* USER CODE BEGIN 9 */
    SD_Timeout_Init();
  return (USBD_OK);
  /* USER CODE END 9 */
}

/**
  * @brief  .
  * @param  lun: .
  * @param  block_num: .
  * @param  block_size: .
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  */
int8_t STORAGE_GetCapacity_HS(uint8_t lun, uint32_t *block_num, uint16_t *block_size)
{
  /* USER CODE BEGIN 10 */

  *block_num  = hsd1.SdCard.BlockNbr;
  *block_size = hsd1.SdCard.BlockSize;
  return (USBD_OK);
  /* USER CODE END 10 */
}

/**
  * @brief  .
  * @param  lun: .
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  */
int8_t STORAGE_IsReady_HS(uint8_t lun)
{
  /* USER CODE BEGIN 11 */
//  int stat = SD_Driver.disk_status(lun);
//    int8_t ret =USBD_FAIL;
//    if (HAL_SD_GetCardState(&hsd1) == HAL_SD_CARD_TRANSFER)
//                    ret = USBD_OK;
//  return stat;
  return USBD_OK;
  /* USER CODE END 11 */
}

/**
  * @brief  .
  * @param  lun: .
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  */
int8_t STORAGE_IsWriteProtected_HS(uint8_t lun)
{
  /* USER CODE BEGIN 12 */
  return (USBD_OK);
  /* USER CODE END 12 */
}

/**
  * @brief  .
  * @param  lun: .
  * @param  buf: .
  * @param  blk_addr: .
  * @param  blk_len: .
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  */
/* Wait for SD card hardware to exit busy state (TIM6-based timeout). */
static int8_t SD_WaitReady(void)
{
    SD_Timeout_Start();
    while (HAL_SD_GetState(&hsd1) == HAL_SD_STATE_BUSY ||
        HAL_SD_GetCardState(&hsd1) != HAL_SD_CARD_TRANSFER) {
        if (SD_Timeout_HasElapsed()) return USBD_FAIL;
    }
    SD_Timeout_Stop();
    return USBD_OK;
}

int8_t STORAGE_Read_HS(uint8_t lun, uint8_t *buf, uint32_t blk_addr, uint16_t blk_len)
{
  /* USER CODE BEGIN 13 */
    if (blk_len > MSC_MEDIA_PACKET / BLOCKSIZE) return USBD_FAIL;
    static uint8_t __attribute__((section(".DMA_no_cache"))) buf4DMA[MSC_MEDIA_PACKET];
    /* Wait for previous write DMA to finish (TIM6-based 100ms timeout).
     * HAL_GetTick() cannot be used because USB ISR priority blocks SysTick. */
    SD_Timeout_Start();
    while (g_SDCard_transmitting) {
        if (SD_Timeout_HasElapsed()) return USBD_FAIL;
    }
    SD_Timeout_Stop();
    if (SD_WaitReady() != USBD_OK) return USBD_FAIL;
    int r = HAL_SD_ReadBlocks_DMA(&hsd1, buf4DMA, blk_addr, blk_len);
    if(r == HAL_OK)
    {
        if (SD_WaitReady() != USBD_OK) return USBD_FAIL;
        memcpy(buf, buf4DMA, blk_len * BLOCKSIZE);
        return USBD_OK;
    }
    return USBD_FAIL;
  /* USER CODE END 13 */
}

/**
  * @brief  .
  * @param  lun: .
  * @param  buf: .
  * @param  blk_addr: .
  * @param  blk_len: .
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  */
int8_t STORAGE_Write_HS(uint8_t lun, uint8_t *buf, uint32_t blk_addr, uint16_t blk_len)
{
  /* USER CODE BEGIN 14 */
    static uint8_t __attribute__((section(".DMA_no_cache"))) buf4DMA[MSC_MEDIA_PACKET];
    if (blk_len > MSC_MEDIA_PACKET / BLOCKSIZE) return USBD_FAIL;

    /* Wait for previous write DMA to finish (TIM6-based 100ms timeout).
     * HAL_GetTick() cannot be used because USB ISR priority blocks SysTick. */
    SD_Timeout_Start();
    while (g_SDCard_transmitting) {
        if (SD_Timeout_HasElapsed()) return USBD_FAIL;
    }
    SD_Timeout_Stop();
    if (SD_WaitReady() != USBD_OK) return USBD_FAIL;

    memcpy(buf4DMA, buf, blk_len * BLOCKSIZE);
    int r = HAL_SD_WriteBlocks_DMA(&hsd1, buf4DMA, blk_addr, blk_len);
    if(r == HAL_OK)
    {
        g_SDCard_transmitting = true;
        return USBD_OK;
    }
    return USBD_FAIL;
  /* USER CODE END 14 */
}

/**
  * @brief  .
  * @param  None
  * @retval .
  */
int8_t STORAGE_GetMaxLun_HS(void)
{
  /* USER CODE BEGIN 15 */
  return (STORAGE_LUN_NBR - 1);
  /* USER CODE END 15 */
}

/* USER CODE BEGIN PRIVATE_FUNCTIONS_IMPLEMENTATION */

/* ---- TIM6-based 100ms timeout for SD card busy-wait ----
 * TIM6 is a basic timer on APB1 with NVIC priority 1 (higher than USB=3).
 * APB1 timer clock = 128MHz. Prescaler = 128000 → 1ms tick.
 * Auto-reload = 100 → UIF flag set after 100ms.
 * We poll the UIF flag; no interrupt handler needed.
 */

static void SD_Timeout_Init(void)
{
    __HAL_RCC_TIM6_CLK_ENABLE();
    HAL_NVIC_SetPriority(TIM6_DAC_IRQn, 1, 0);
    TIM6->PSC = SD_TIMEOUT_PRESCALER;
    TIM6->ARR = SD_TIMEOUT_PERIOD;
}

static void SD_Timeout_Start(void)
{
    TIM6->CNT = 0;
    TIM6->SR &= (~TIM_SR_UIF);
    TIM6->CR1 |= TIM_CR1_CEN;
}

static uint8_t SD_Timeout_HasElapsed(void)
{
//    if (TIM6->SR & TIM_SR_UIF)
//        return  1U;
//    else
//        return 0U;
    /* always return 0 to do dead polling until g_transmitting_flag cleared*/
    return 0;
}

static void SD_Timeout_Stop(void)
{
    TIM6->CR1 &= ~TIM_CR1_CEN;
    TIM6->SR &= (~TIM_SR_UIF);
}

/* USER CODE END PRIVATE_FUNCTIONS_IMPLEMENTATION */

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
