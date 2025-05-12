/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2022 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "cmsis_os.h"
/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */
// thread flags
#define FLAG_TASK_START 1U
#define FLAG_SHUTDOWN_BUTTON_LONG_PRESSED (1U << 1)
#define FLAG_SHUTDOWN_NOW (1U << 2)
#define FLAG_MIC_1_PLUGGED (1U << 3)
#define FLAG_MIC_2_PLUGGED (1U << 4)
#define FLAG_OUT_1_PLUGGED (1U << 5)
#define FLAG_OUT_2_PLUGGED (1U << 6)
#define FLAG_MIC_1_UNPLUGGED (1U << 7)
#define FLAG_MIC_2_UNPLUGGED (1U << 8)
#define FLAG_OUT_1_UNPLUGGED (1U << 9)
#define FLAG_OUT_2_UNPLUGGED (1U << 10)
#define FLAG_SHUTDOWN_BUTTON_SHORT_CLICKED (1U << 11)
extern osThreadId_t defaultTaskHandle;
extern osThreadId_t taskLVGLTimerHandle;
extern osThreadId_t taskHumanInputHandle;


/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */
void hold_power_enable();
void power_off();
void charging_disable();
void charging_enable();
void NP_power_enable(uint8_t state);
void encoder_preamp_power_enable(uint8_t ch, uint8_t state);
/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define NP_EN_Pin GPIO_PIN_3
#define NP_EN_GPIO_Port GPIOE
#define BMS_KEY_Pin GPIO_PIN_13
#define BMS_KEY_GPIO_Port GPIOC
#define T_MOSI_Pin GPIO_PIN_7
#define T_MOSI_GPIO_Port GPIOA
#define ENCA_Pin GPIO_PIN_7
#define ENCA_GPIO_Port GPIOE
#define ENCB_Pin GPIO_PIN_8
#define ENCB_GPIO_Port GPIOE
#define ENCSW_Pin GPIO_PIN_9
#define ENCSW_GPIO_Port GPIOE
#define TFT_DISP_Pin GPIO_PIN_15
#define TFT_DISP_GPIO_Port GPIOE
#define DECODER_RSTX_Pin GPIO_PIN_11
#define DECODER_RSTX_GPIO_Port GPIOB
#define BK_EN_Pin GPIO_PIN_13
#define BK_EN_GPIO_Port GPIOB

#define DETECT_SDIO_Pin GPIO_PIN_15
#define DETECT_SDIO_GPIO_Port GPIOA
#define PREAMP_PD1_Pin GPIO_PIN_0
#define PREAMP_PD1_GPIO_Port GPIOD
#define PREAMP_PD2_Pin GPIO_PIN_1
#define PREAMP_PD2_GPIO_Port GPIOD
#define T_CSX_Pin GPIO_PIN_3
#define T_CSX_GPIO_Port GPIOD
#define T_INTX_Pin GPIO_PIN_4
#define T_INTX_GPIO_Port GPIOD
#define T_INTX_EXTI_IRQn EXTI4_IRQn
#define T_BUSY_Pin GPIO_PIN_5
#define T_BUSY_GPIO_Port GPIOD
#define T_SCK_Pin GPIO_PIN_3
#define T_SCK_GPIO_Port GPIOB
#define T_MISO_Pin GPIO_PIN_4
#define T_MISO_GPIO_Port GPIOB
#define SPI1_CS_Pin GPIO_PIN_5
#define SPI1_CS_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */
// outputs
#define NOCHRGX_Pin GPIO_PIN_13 // // reset to HIGH to disallow charging
#define NOCHRGX_GPIO_Port GPIOC
#define LED_MIC1_Pin GPIO_PIN_15 // // reset to HIGH
#define LED_MIC1_GPIO_Port GPIOD
#define LED_MIC2_Pin GPIO_PIN_14 // reset to HIGH
#define LED_MIC2_GPIO_Port GPIOD
#define PWR_EN_Pin GPIO_PIN_1 // reset to LOW
#define PWR_EN_GPIO_Port GPIOE
// inputs
#define MIC_DETECT1_Pin GPIO_PIN_8 // // reset to HIGH
#define MIC_DETECT1_GPIO_Port GPIOA
#define MIC_DETECT2_Pin GPIO_PIN_12 // // reset to HIGH
#define MIC_DETECT2_GPIO_Port GPIOB
#define OUT1_DETECT_Pin GPIO_PIN_9 // pull up
#define OUT1_DETECT_GPIO_Port GPIOD
#define OUT2_DETECT_Pin GPIO_PIN_8 // pull up
#define OUT2_DETECT_GPIO_Port GPIOD
#define SHUTDOWNX_Pin GPIO_PIN_4 // pull up
#define SHUTDOWNX_GPIO_Port GPIOE
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
