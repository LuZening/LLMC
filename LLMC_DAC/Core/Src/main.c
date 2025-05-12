/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
/* Includes ------------------------------------------------------------------*/
#include "main.h"


/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include "cmsis_os.h"
#include "fatfs.h"
#include "usb_device.h"
#include "MyUSB.h"
#include "Config.h"
#include "audio_buffers.h"
#include "audio_settings.h"
#include "audio_player.h"
#include "lvgl.h"
#include "sd_diskio.h"
#include "LVGL_GUI.h"
#include "profiler.h"
#include "LVGL_toast.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
extern uint8_t _siDTCMRAM1_heap, _siDTCMRAM2_heap, _siITCMRAM_heap;
extern uint8_t _sDTCMRAM1_heap, _sDTCMRAM2_heap, _sITCMRAM_heap;
extern uint8_t _eDTCMRAM1_heap, _eDTCMRAM2_heap, _eITCMRAM_heap;
// define FreeRTOS heap at CCM address, declared in heap_4.c
uint8_t __attribute__((section(".DTCMRAM1_heap"))) ucHeap[configTOTAL_HEAP_SIZE];

RotEnc_t rotenc; // SW: 0
ButtonDebouncer_t btnShutdown; // 1
ButtonDebouncer_t btnsOutDetect[N_OUTPUTS]; // 2
ButtonDebouncer_t btnsMicDetect[N_MICROPHONES]; // 3
#define N_EXTI_BTNS (1+1+N_OUTPUTS+N_MICROPHONES)
uint32_t cntTimer_EXTI = 0;
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc2;

DMA2D_HandleTypeDef hdma2d;

I2C_HandleTypeDef hi2c1;
DMA_HandleTypeDef hdma_i2c1_tx;
DMA_HandleTypeDef hdma_i2c1_rx;

IWDG_HandleTypeDef hiwdg1;


LTDC_HandleTypeDef hltdc;

OSPI_HandleTypeDef hospi1;

SAI_HandleTypeDef hsai_BlockA2;
SAI_HandleTypeDef hsai_BlockB2;
DMA_HandleTypeDef hdma_sai2_a;
DMA_HandleTypeDef hdma_sai2_b;

SD_HandleTypeDef hsd1;

SPI_HandleTypeDef hspi1;

TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim5;

UART_HandleTypeDef huart1;

MDMA_HandleTypeDef hmdma_mdma_channel40_sw_0;
/* Definitions for defaultTask */
void StartDefaultTask(void *argument);
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 2048,
  .priority = (osPriority_t) osPriorityBelowNormal,
};
/* USER CODE BEGIN PV */


// LVGL Thread Handles
void taskLVGLTimer(void * argument);
const osThreadAttr_t taskLVGLTimer_attr= {
  .name = "lvgl_timer",
  .stack_size = 10*1024U,
  .priority = (osPriority_t) osPriorityNormal,
};
osThreadId_t taskLVGLTimerHandle;

// Human Input Thread Handles
void StartHumanInOutTask(void * argument);
const osThreadAttr_t taskHumanInput_attr= {
  .name = "human_in",
  .stack_size = 2048,
  .priority = (osPriority_t) osPriorityAboveNormal,
};
osThreadId_t taskHumanInputHandle;
uint8_t flagHumanInputTaskReady = 0;


/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void PeriphCommonClock_Config(void);
static void MPU_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_SAI2_Init(void);
static void MX_SPI1_Init(void);
static void MX_SDMMC1_SD_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_IWDG1_Init(void);
static void MX_I2C1_Init(void);
static void MX_LTDC_Init(void);
static void MX_OCTOSPI1_Init(void);
static void MX_TIM2_Init(void); // button debouncing
static void MX_TIM5_Init(void); // SAI clock counter
static void MX_ADC2_Init(void);
static void MX_MDMA_Init(void);
static void MX_DMA2D_Init(void);
void StartDefaultTask(void *argument);

/* USER CODE BEGIN PFP */
GPIO_TypeDef* const  micports[N_MICROPHONES] = {MIC_DETECT1_GPIO_Port, MIC_DETECT2_GPIO_Port};
const uint16_t micpins[N_MICROPHONES] = {MIC_DETECT1_Pin, MIC_DETECT2_Pin};
GPIO_TypeDef* const headphoneports[N_HEADPHONES] = {OUT1_DETECT_GPIO_Port, OUT2_DETECT_GPIO_Port};
const uint16_t headphonepins[N_HEADPHONES] = {OUT1_DETECT_Pin, OUT2_DETECT_Pin};
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void hold_power_enable()
{
	 HAL_GPIO_WritePin(PWR_EN_GPIO_Port, PWR_EN_Pin, 1); // hold the power en pin
}
void power_off()
{
	 HAL_GPIO_WritePin(PWR_EN_GPIO_Port, PWR_EN_Pin, 0); // hold the power en pin
}
void charging_disable()
{
	 HAL_GPIO_WritePin(NOCHRGX_GPIO_Port, NOCHRGX_Pin, 0); // stop charging
}
void charging_enable()
{
	HAL_GPIO_WritePin(NOCHRGX_GPIO_Port, NOCHRGX_Pin, 1); // allow charging
}

void NP_power_enable(uint8_t state)
{
	HAL_GPIO_WritePin(NP_EN_GPIO_Port, NP_EN_Pin, state);
}

void encoder_preamp_power_enable(uint8_t ch, uint8_t state)
{
//	if(ch == 1)
//	{
//		HAL_GPIO_WritePin(PREAMP_PD1_GPIO_Port, PREAMP_PD1_Pin, 1-state);
//	}
//	else if(ch == 2)
//	{
//		HAL_GPIO_WritePin(PREAMP_PD2_GPIO_Port, PREAMP_PD2_Pin, 1-state);
//	}
}

// locally implemented version of save config
static void save_config_local();

// redirect printf to UART
int __io_putchar(int ch)
{
#ifdef DEBUG
	HAL_UART_Transmit(&huart1, (uint8_t*)&ch, 1, 0xffffU);
	return ch;
#else
	return ch;
#endif
}


void SAI_encoder_init(uint32_t audio_sample_rate_Hz_SAI)
{
	  hsai_BlockA2.Instance = SAI2_Block_A;
	  hsai_BlockA2.Init.AudioMode = SAI_MODEMASTER_RX;
	  hsai_BlockA2.Init.Synchro = SAI_ASYNCHRONOUS;
	  hsai_BlockA2.Init.OutputDrive = SAI_OUTPUTDRIVE_DISABLE;
	  hsai_BlockA2.Init.NoDivider = SAI_MASTERDIVIDER_ENABLE;
	  hsai_BlockA2.Init.FIFOThreshold = SAI_FIFOTHRESHOLD_HF;
	  hsai_BlockA2.Init.AudioFrequency = audio_sample_rate_Hz_SAI;
	  hsai_BlockA2.Init.SynchroExt = SAI_SYNCEXT_DISABLE;
	  hsai_BlockA2.Init.MonoStereoMode = SAI_STEREOMODE;
	  hsai_BlockA2.Init.CompandingMode = SAI_NOCOMPANDING;
	  uint8_t to_continue = encoder_input_started;
	  if(to_continue)
	  {
		  encocder_input_stop();
	  }
	  if (HAL_SAI_InitProtocol(&hsai_BlockA2, SAI_I2S_STANDARD, SAI_PROTOCOL_DATASIZE_32BIT, 2) != HAL_OK)
	  {
	    Error_Handler();
	  }
	  if(to_continue)
	  {
		  encocder_input_start();
	  }
}


void SAI_decoder_init(uint32_t audio_sample_rate_Hz_SAI)
{
	  hsai_BlockB2.Instance = SAI2_Block_B;
	  hsai_BlockB2.Init.AudioMode = SAI_MODEMASTER_TX;
	  hsai_BlockB2.Init.Synchro = SAI_ASYNCHRONOUS;
	  hsai_BlockB2.Init.OutputDrive = SAI_OUTPUTDRIVE_ENABLE;
	  hsai_BlockB2.Init.NoDivider = SAI_MASTERDIVIDER_ENABLE;
	  hsai_BlockB2.Init.FIFOThreshold = SAI_FIFOTHRESHOLD_HF;
	  hsai_BlockB2.Init.AudioFrequency = audio_sample_rate_Hz_SAI;
	  hsai_BlockB2.Init.SynchroExt = SAI_SYNCEXT_DISABLE;
	  hsai_BlockB2.Init.MonoStereoMode = SAI_STEREOMODE;
	  hsai_BlockB2.Init.CompandingMode = SAI_NOCOMPANDING;
	  hsai_BlockB2.Init.TriState = SAI_OUTPUT_NOTRELEASED;
	  uint8_t to_continue = enable_to_decoder;
	  if(to_continue)
	  {
		  output_to_decoder_stop();
	  }
	  if (HAL_SAI_InitProtocol(&hsai_BlockB2, SAI_I2S_STANDARD, SAI_PROTOCOL_DATASIZE_32BIT, 2) != HAL_OK)
	  {
	    Error_Handler();
	  }
	  if(to_continue)
	  {
		  output_to_decoder_start();
	  }

}

/*****  I2S DMA ISRs BEGIN  *****/
profiler_counter_t profSAITx = {0};
void HAL_SAI_TxHalfCpltCallback(SAI_HandleTypeDef *hsai)
{
	static uint32_t cnt = 0;
	cnt++;
	// output to decoder
	if(hsai == &hsai_BlockB2)
	{
		decoder_HAL_SAI_TxHalfCpltCallback(hsai);
	}
	else if(hsai == &hsai_BlockA2) // input from encoder
	{
		// pass
	}
}

void HAL_SAI_TxCpltCallback(SAI_HandleTypeDef *hsai)
{

//	static uint32_t cycles1, cycles2;
//	cycles1 = DWT->CYCCNT;
	static uint32_t cnt = 0;
	cnt++;
	if(hsai == &hsai_BlockB2) // output to decoder
	{
		decoder_HAL_SAI_TxCpltCallback(hsai);
	}
	else if(hsai == &hsai_BlockA2) // input from encoder
	{
		// pass
	}
//	cycles2 = DWT->CYCCNT;
//	uint32_t cycles_used = cycles2 - cycles1; // 9000 (1 input), 16000 (2 inputs)
	return;
}

profiler_counter_t profUSB_SOF = {0};
void HAL_SAI_RxHalfCpltCallback(SAI_HandleTypeDef *hsai)
{
	static uint32_t cycles1, cycles2;
	cycles1 = DWT->CYCCNT;
	if(hsai == &hsai_BlockA2) // input from encoder
	{

		encoder_HAL_SAI_RxHalfCpltCallback(hsai);
	}
	else if(hsai == &hsai_BlockB2) // output to decoder
	{
		// pass
	}
	cycles2 = DWT->CYCCNT;
	uint32_t cycles_used = cycles2 - cycles1; // 6719
	return;
}


void HAL_SAI_RxCpltCallback(SAI_HandleTypeDef *hsai)
{
	if(hsai == &hsai_BlockA2) // input from encoder
	{
		encoder_HAL_SAI_RxCpltCallback(hsai);
	}
	else if(hsai == &hsai_BlockB2) // output to decoder
	{
		// pass
	}
}

void HAL_SAI_ErrorCallback(SAI_HandleTypeDef *hsai)
{
	// TODO: implement I2S error callback, I2S pipeline may stall when error occures
}
/*****  I2S DMA ISRs END  *****/

#define ENTER_BUTTON_CRITICAL __HAL_TIM_DISABLE_IT(&htim2, TIM_IT_UPDATE)
#define EXIT_BUTTON_CRITICAL __HAL_TIM_ENABLE_IT(&htim2, TIM_IT_UPDATE)
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	if(flagHumanInputTaskReady == 0)
		return;
	int btn = -1;
	uint8_t state;

	  /*LIST OF EXTI INTERRUPTS
	   * T_INTX:  4 FALLING
	   * SHUTDOWNX: 4 FALLING
	   * ENCA: 7	FALLING/RISING
	   * ENCSW: 9	FALLING
	   * OUT1 DETECT: 9 RISING
	   * OUT2_DETECT: 8 RISING
	   * MIC_DETECT1: 8 RISING
	   * MIC_DETECT2: 12 RISING
	   */
	ENTER_BUTTON_CRITICAL;
	if(GPIO_Pin == SHUTDOWNX_Pin)
	{
		state = HAL_GPIO_ReadPin(SHUTDOWNX_GPIO_Port, SHUTDOWNX_Pin);
		if(state == 0)
		{
			btndeb_interrupt(&btnShutdown, state);
			btn = 1;
		}

	}
	/* TODO: ENCSW_Pin does not trigger the EXTI9-5 interrupt */
	else if(GPIO_Pin == ENCSW_Pin)
	{

		state = HAL_GPIO_ReadPin(ENCSW_GPIO_Port, ENCSW_Pin);
		if(state == 0)
		{
			btndeb_interrupt(&rotenc.SW, state);
			btn = 3;
		}

		state = HAL_GPIO_ReadPin(OUT1_DETECT_GPIO_Port, OUT1_DETECT_Pin);
		if(state == 1)
		{
			btndeb_interrupt(&btnsOutDetect[0], state);
			btn = 4;
		}
	}
	else if(GPIO_Pin == OUT2_DETECT_Pin)
	{
		state = HAL_GPIO_ReadPin(OUT2_DETECT_GPIO_Port, OUT2_DETECT_Pin);
		if(state == 1)
		{
			btndeb_interrupt(&btnsOutDetect[1], state);
			btn = 5;
		}
		state = HAL_GPIO_ReadPin(MIC_DETECT1_GPIO_Port, MIC_DETECT1_Pin);
		if(state == 1)
		{
			btndeb_interrupt(&btnsMicDetect[0], state);
			btn = 6;
		}
	}
	else if(GPIO_Pin == MIC_DETECT2_Pin)
	{
		state = HAL_GPIO_ReadPin(MIC_DETECT2_GPIO_Port, MIC_DETECT2_Pin);
		if(state == 1)
		{
			btndeb_interrupt(&btnsMicDetect[1], state);
			btn = 7;
		}
	}
	else if(GPIO_Pin == ENCA_Pin)
	{
		uint8_t fall_rise = HAL_GPIO_ReadPin(ENCA_GPIO_Port, ENCA_Pin);
		RotEnc_interrupt(&rotenc, fall_rise);
	}


	// start TIM2 timebase only if it has not started
	if(btn >= 0)
	{

		cntTimer_EXTI = 0;
		if((htim2.Instance->CR1 & TIM_CR1_CEN) == 0)
		{
		  HAL_TIM_Base_Start_IT(&htim2);
		}
	}
	EXIT_BUTTON_CRITICAL;
	/* process Rotary */

}

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM8 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */
	//note: HAL Ticks each 500us WHY?
  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM8) {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */
  /* TIM2 for button interrupts
   * NOTE: TIM2's priority must be higher than ALL EXTIs
   * */
  if(htim->Instance == TIM2)
  {
	  const int us = 10000;
	  /* handle encoder */
	  int i = 0;
	  uint32_t bitmapInt = 0;// indicate if a button is at interrupted state
	  uint32_t bitmapAct = 0; // indicate if a button is at active state, to keep timer from shutting down when long press button engaged
	  if(rotenc.SW.interrupted || rotenc.SW.active)
	  {
		  bool flip = btndeb_tick(&rotenc.SW, us);
		  uint8_t active = rotenc.SW.active;
		  if(flip && (active == 0)) // trigger when released
		  {
			  rotenc.SW.interrupted = 0;
		  }
		  bitmapInt |= ((uint32_t)rotenc.SW.interrupted << (i));
		  bitmapAct |= ((uint32_t)(active) << (i));
		  i++;
	  }


	  if(btnShutdown.interrupted || btnShutdown.active)
	  {
		  bool flip = btndeb_tick(&btnShutdown, us);
		  // trigger when pressed
		  if(btnShutdown.active)
		  {
			  btnShutdown.interrupted = 0;
			  if(btnShutdown.cntLongPress >= 2000000)
			  {
				  osThreadFlagsSet(taskHumanInputHandle, FLAG_SHUTDOWN_BUTTON_LONG_PRESSED);
				  btnShutdown.active = 0; // to avoid continous triggering
			  }
		  }
		  else if(btnShutdown.interrupted == 0) // released before long press, the button was in active state, but now not
		  {
			  osThreadFlagsSet(taskHumanInputHandle, FLAG_SHUTDOWN_BUTTON_SHORT_CLICKED);
			  btnShutdown.active = 0; // to avoid continous triggering
		  }
		  bitmapInt |= ((uint32_t)btnShutdown.interrupted << (i));
		  bitmapAct |= ((uint32_t)(btnShutdown.active) << (i));
		  i++;
	  }

	  // TODO: two sided trigger
	  for(uint8_t j = 0; j < N_OUTPUTS; ++j)
	  {

		  if(btnsOutDetect[j].interrupted || btnsOutDetect[j].active)
		  {
			  bool flip = btndeb_tick(&btnsOutDetect[j], us);
			  uint8_t active = (btnsOutDetect[j].state == btnsOutDetect[j].lvlAct);
			  // trigger when pressed
			  if(flip)
			  {
				  btnsOutDetect[j].interrupted = 0;
				  if(active) // plugged
				  {
					  btnsOutDetect[j].active = 0;
				  }
				  else // unplugged
				  {

				  }
			  }
			  bitmapInt |= ((uint32_t)btnsOutDetect[j].interrupted << (i++));
		  }
	  }

	  for(uint8_t j = 0; j < N_MICROPHONES; ++j)
	  {

		  if(btnsMicDetect[j].interrupted || btnsMicDetect[j].active)
		  {
			  bool flip = btndeb_tick(&btnsMicDetect[j], us);
			  uint8_t active = (btnsMicDetect[j].state == btnsMicDetect[j].lvlAct);
			  // trigger when pressed
			  if(flip)
			  {
				  btnsMicDetect[j].interrupted = 0;
				  if(active) // plugged
				  {
					  btnsMicDetect[j].active = 0;
					  /* TODO: do something */
					  // connect MIC j to outputs
					  cfg.audio_connections[0] |= (1<<j);
					  cfg.audio_connections[0] |= (1<<j);
					  audio_connections_apply(cfg.audio_connections);
				  }
				  else // unplugged
				  {
					  // disconnect MIC j to outputs
					  cfg.audio_connections[0] &= ~(1<<j);
					  cfg.audio_connections[0] &= ~(1<<j);
					  audio_connections_apply(cfg.audio_connections);
				  }
			  }
			  bitmapInt |= ((uint32_t)btnsMicDetect[j].interrupted << (i++));
		  }
	  }

	  /* TIM stop condition */
	  // Condition 1: no more interrupted buttons awaiting
	  // Condition 2: still buttons awaiting but its state is not active
	  bool stopTimer = false;
	  if(bitmapInt == 0 && bitmapAct == 0)
	  {
		  stopTimer = true;
	  }
	  else if(bitmapAct == 0) // bitmapInt > 0
	  {
		  cntTimer_EXTI += us; // accumulate idle counter
		  if(cntTimer_EXTI >= 1000000)
			  stopTimer = true;
	  }

	  if(stopTimer)
	  {
		  HAL_TIM_Base_Stop_IT(&htim2);
		  cntTimer_EXTI = 0;
	  }

  }
  /* USER CODE END Callback 1 */
}


void HAL_LTDC_LineEventCallback(LTDC_HandleTypeDef *hltdc)
{
	/*
	// SWITCH between dual framebuffers
	size_t addr;
	switch(g_gpu_state)
	{
	case 0:
		g_gpu_state = 1;
		addr = (size_t)framebuf2;
		break;
	case 1:
		g_gpu_state = 0;
		addr = (size_t)framebuf1;
		break;
	}
	HAL_LTDC_SetAddress_NoReload(hltdc, addr, 0);
	hltdc->Instance->SRCR = LTDC_RELOAD_VERTICAL_BLANKING;
	*/
}



/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
  int r = 0;
  /* USER CODE END 1 */

  /* MPU Configuration--------------------------------------------------------*/
  MPU_Config();

  /* Enable I-Cache---------------------------------------------------------*/
  SCB_EnableICache();

  /* Enable D-Cache---------------------------------------------------------*/
  SCB_EnableDCache();

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

/* Configure the peripherals common clocks */
  PeriphCommonClock_Config();

  /* USER CODE BEGIN SysInit */

  /* hold the power END */
  // Load DTCMRAM1_heap section from FLASH
  memcpy((void*)&_sDTCMRAM1_heap, (void*)&_siDTCMRAM1_heap, &_eDTCMRAM1_heap - &_sDTCMRAM1_heap);
  // Load DTCMRAM2_heap section from FLASH
  memcpy((void*)&_sDTCMRAM2_heap, (void*)&_siDTCMRAM2_heap, &_eDTCMRAM2_heap - &_sDTCMRAM2_heap);
  // Load ITCMRAM_heap section from FLASH
  memcpy((void*)&_sITCMRAM_heap, (void*)&_siITCMRAM_heap, &_eITCMRAM_heap - &_sITCMRAM_heap);
  // NOTE: CLEAN RAM_CD FOR USB usage
  USB_my_init_no_cache_memory();
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_SAI2_Init();
  MX_SPI1_Init();
  MX_SDMMC1_SD_Init();
  MX_USART1_UART_Init();
  MX_I2C1_Init();
  MX_LTDC_Init();
  MX_FATFS_Init();
  MX_TIM2_Init();
  MX_TIM5_Init();
//  MX_ADC2_Init();
//  MX_MDMA_Init();
  MX_DMA2D_Init();
#ifndef DEBUG
  MX_IWDG1_Init();
#endif
  /* USER CODE BEGIN 2 */
#ifdef PROFILER
  enable_DWT();
#endif

  /* hold the power */
  set_mic_LEDs(0, 1);
  hold_power_enable();

  /* test DSP */
//  enable_DWT();
//  audio_DSP_test();
  /* test DSP END */
  charging_disable();

  init_config(&cfg);

  /* init: start LTDC */
  // start LCD back light
  set_display_enable(1);
  HAL_LTDC_Init(&hltdc);
  set_back_light(1);
  memset(framebuf1, 0xff, sizeof(framebuf1));
  /** init audio related **/
  init_volume_control();
  /* init USB buffers */
  output_to_usb_init();
  init_USB_player();
  FS_player_init();
  /* init: Audio ADC*/
  NP_power_enable(1);
//  encocder_input_start();
//  HAL_Delay(2);
//  adcx120_reset(&adcx120);
//  HAL_Delay(5);
//  adcx120_init(&adcx120);
#ifdef DEBUG
  adcx120_test(&adcx120);
#endif
  // set default sample rate and bit depth

//  r = adcx120_test(&adcx120);

  /* init: Audio DAC */
  // in test mode keep on
  // turn off NP power first
  pcm179x_hw_reset();
  // generate 1024 system clocks using blank
//  r = HAL_SAI_Transmit(&hsai_BlockB2, (uint8_t*)bufDecoder, 2048, HAL_MAX_DELAY);
  output_to_decoder_start();
  HAL_Delay(1);
  pcm179x_init(&pcm179x, 24576000U);
//  PCM179x_set_attenuation(&pcm179x, 240U);
//  HAL_Delay(1);
  output_to_decoder_stop();
  sync_volumes(false);

  /* init: button debouncers */
  btndeb_init(&btnShutdown, SHUTDOWNX_GPIO_Port, SHUTDOWNX_Pin, 0,1000U);
  btndeb_init(btnsOutDetect + 0, OUT1_DETECT_GPIO_Port, OUT1_DETECT_Pin, 1,1000U);
  btndeb_init(btnsOutDetect + 1, OUT2_DETECT_GPIO_Port, OUT2_DETECT_Pin, 1,1000U);
  btndeb_init(btnsMicDetect + 0, MIC_DETECT1_GPIO_Port, MIC_DETECT1_Pin, 1,1000U);
  btndeb_init(btnsMicDetect + 1, MIC_DETECT2_GPIO_Port, MIC_DETECT2_Pin, 1,1000U);

  /* USER CODE END 2 */

  /* Init scheduler */
  osKernelInitialize();

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  mtxFSPlayer = osMutexNew(NULL);
  vQueueAddToRegistry(mtxFSPlayer, "mtxFSPlay");
  mtxConfig  = osMutexNew(NULL);
  vQueueAddToRegistry(mtxConfig, "mtxConfig");
  mtxTouchScreen = osMutexNew(NULL);
  vQueueAddToRegistry(mtxTouchScreen, "mtxTouch");
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */

  taskLVGLTimerHandle = osThreadNew(taskLVGLTimer, NULL, &taskLVGLTimer_attr);
  taskHumanInputHandle = osThreadNew(StartHumanInOutTask, NULL, &taskHumanInput_attr);


  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  HAL_IWDG_Refresh(&hiwdg1);
  /* add events, ... */
//  HAL_Delay(100);
  /* USER CODE END RTOS_EVENTS */

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */
  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Supply configuration update enable
  */
  HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);
  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}
  /** Macro to configure the PLL clock source
  */
  __HAL_RCC_PLL_PLLSOURCE_CONFIG(RCC_PLLSOURCE_HSE);
  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE | RCC_OSCILLATORTYPE_LSI;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 1;
#if defined(LOW_CPU_FREQ)
  RCC_OscInitStruct.PLL.PLLN = 16; // 128MHz HCLK
#else
  RCC_OscInitStruct.PLL.PLLN = 32; // 256MHz HCLK
#endif
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 16;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_3;
  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_D3PCLK1|RCC_CLOCKTYPE_D1PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_7) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief Peripherals Common Clock Configuration
  * @retval None
  */
void PeriphCommonClock_Config(void)
{
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  /** Initializes the peripherals clock
  */
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_USB|RCC_PERIPHCLK_ADC
                              |RCC_PERIPHCLK_I2C1|RCC_PERIPHCLK_SDMMC
                              |RCC_PERIPHCLK_SPI1|RCC_PERIPHCLK_USART1
                              |RCC_PERIPHCLK_LTDC;
  PeriphClkInitStruct.PLL2.PLL2M = 1;
  PeriphClkInitStruct.PLL2.PLL2N = 8;
  PeriphClkInitStruct.PLL2.PLL2P = 1;
  PeriphClkInitStruct.PLL2.PLL2Q = 16;
  PeriphClkInitStruct.PLL2.PLL2R = 6;
  PeriphClkInitStruct.PLL2.PLL2RGE = RCC_PLL2VCIRANGE_3;
  PeriphClkInitStruct.PLL2.PLL2VCOSEL = RCC_PLL2VCOWIDE;
  PeriphClkInitStruct.PLL2.PLL2FRACN = 0;
  PeriphClkInitStruct.PLL3.PLL3M = 1;
  PeriphClkInitStruct.PLL3.PLL3N = 9;
  PeriphClkInitStruct.PLL3.PLL3P = 32;
  PeriphClkInitStruct.PLL3.PLL3Q = 3;
  PeriphClkInitStruct.PLL3.PLL3R = 18;
  PeriphClkInitStruct.PLL3.PLL3RGE = RCC_PLL3VCIRANGE_3;
  PeriphClkInitStruct.PLL3.PLL3VCOSEL = RCC_PLL3VCOWIDE;
  PeriphClkInitStruct.PLL3.PLL3FRACN = 0;
  PeriphClkInitStruct.SdmmcClockSelection = RCC_SDMMCCLKSOURCE_PLL2;
  PeriphClkInitStruct.Spi123ClockSelection = RCC_SPI123CLKSOURCE_PLL3;
  PeriphClkInitStruct.Usart16ClockSelection = RCC_USART16910CLKSOURCE_PLL2;
  PeriphClkInitStruct.I2c123ClockSelection = RCC_I2C123CLKSOURCE_PLL3;
  PeriphClkInitStruct.UsbClockSelection = RCC_USBCLKSOURCE_PLL3;
  PeriphClkInitStruct.AdcClockSelection = RCC_ADCCLKSOURCE_PLL3;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC2_Init(void)
{

  /* USER CODE BEGIN ADC2_Init 0 */

  /* USER CODE END ADC2_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC2_Init 1 */

  /* USER CODE END ADC2_Init 1 */
  /** Common config
  */
  hadc2.Instance = ADC2;
  hadc2.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV1;
  hadc2.Init.Resolution = ADC_RESOLUTION_16B;
  hadc2.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hadc2.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  hadc2.Init.LowPowerAutoWait = DISABLE;
  hadc2.Init.ContinuousConvMode = DISABLE;
  hadc2.Init.NbrOfConversion = 1;
  hadc2.Init.DiscontinuousConvMode = DISABLE;
  hadc2.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc2.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc2.Init.ConversionDataManagement = ADC_CONVERSIONDATA_DR;
  hadc2.Init.Overrun = ADC_OVR_DATA_PRESERVED;
  hadc2.Init.LeftBitShift = ADC_LEFTBITSHIFT_NONE;
  hadc2.Init.OversamplingMode = DISABLE;
  if (HAL_ADC_Init(&hadc2) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_VBAT;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_1CYCLE_5;
  sConfig.SingleDiff = ADC_SINGLE_ENDED;
  sConfig.OffsetNumber = ADC_OFFSET_NONE;
  sConfig.Offset = 0;
  sConfig.OffsetSignedSaturation = DISABLE;
  if (HAL_ADC_ConfigChannel(&hadc2, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC2_Init 2 */

  /* USER CODE END ADC2_Init 2 */

}

/**
  * @brief DMA2D Initialization Function
  * @param None
  * @retval None
  */
static void MX_DMA2D_Init(void)
{

  /* USER CODE BEGIN DMA2D_Init 0 */

  /* USER CODE END DMA2D_Init 0 */

  /* USER CODE BEGIN DMA2D_Init 1 */

  /* USER CODE END DMA2D_Init 1 */
  hdma2d.Instance = DMA2D;
  hdma2d.Init.Mode = DMA2D_M2M;
  hdma2d.Init.ColorMode = DMA2D_OUTPUT_RGB565;
  hdma2d.Init.OutputOffset = 0;
  hdma2d.LayerCfg[1].InputOffset = 0;
  hdma2d.LayerCfg[1].InputColorMode = DMA2D_INPUT_RGB565;
  hdma2d.LayerCfg[1].AlphaMode = DMA2D_NO_MODIF_ALPHA;
  hdma2d.LayerCfg[1].InputAlpha = 0;
  hdma2d.LayerCfg[1].AlphaInverted = DMA2D_REGULAR_ALPHA;
  hdma2d.LayerCfg[1].RedBlueSwap = DMA2D_RB_REGULAR;
  hdma2d.LayerCfg[1].ChromaSubSampling = DMA2D_NO_CSS;
  if (HAL_DMA2D_Init(&hdma2d) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_DMA2D_ConfigLayer(&hdma2d, 1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN DMA2D_Init 2 */
  hdma2d.XferCpltCallback = DMA2D_XferCpltCallback;
  /* USER CODE END DMA2D_Init 2 */

}


/* IWDG1 init function */
static void MX_IWDG1_Init(void)
{

  /* USER CODE BEGIN IWDG1_Init 0 */

  /* USER CODE END IWDG1_Init 0 */

  /* USER CODE BEGIN IWDG1_Init 1 */

  /* USER CODE END IWDG1_Init 1 */
  hiwdg1.Instance = IWDG1;
  hiwdg1.Init.Prescaler = IWDG_PRESCALER_32;
  hiwdg1.Init.Window = 88888;
  hiwdg1.Init.Reload = 88888;
  if (HAL_IWDG_Init(&hiwdg1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN IWDG1_Init 2 */

  /* USER CODE END IWDG1_Init 2 */

}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.Timing = 0x0010040B; // 350kHz
//  hi2c1.Init.Timing = 0x2000090E; // 100kHz
//  hi2c1.Init.Timing = 0x00101e3f; // 100kHz
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief LTDC Initialization Function
  * @param None
  * @retval None
  */
static void MX_LTDC_Init(void)
{

  /* USER CODE BEGIN LTDC_Init 0 */

  /* USER CODE END LTDC_Init 0 */

  LTDC_LayerCfgTypeDef pLayerCfg = {0};

  /* USER CODE BEGIN LTDC_Init 1 */

  /* USER CODE END LTDC_Init 1 */
  hltdc.Instance = LTDC;
  hltdc.Init.HSPolarity = LTDC_HSPOLARITY_AL;
  hltdc.Init.VSPolarity = LTDC_VSPOLARITY_AL;
  hltdc.Init.DEPolarity = LTDC_DEPOLARITY_AL;
  hltdc.Init.PCPolarity = LTDC_PCPOLARITY_IPC;
  hltdc.Init.HorizontalSync = 4;
  hltdc.Init.VerticalSync = 4;
  hltdc.Init.AccumulatedHBP = 44;
  hltdc.Init.AccumulatedVBP = 12;
  hltdc.Init.AccumulatedActiveW = 524;
  hltdc.Init.AccumulatedActiveH = 284;
  hltdc.Init.TotalWidth = 529;
  hltdc.Init.TotalHeigh = 292;
  hltdc.Init.Backcolor.Blue = 32;
  hltdc.Init.Backcolor.Green = 0;
  hltdc.Init.Backcolor.Red = 127;
  if (HAL_LTDC_Init(&hltdc) != HAL_OK)
  {
    Error_Handler();
  }
  pLayerCfg.WindowX0 = 0;
  pLayerCfg.WindowX1 = 480;
  pLayerCfg.WindowY0 = 0;
  pLayerCfg.WindowY1 = 272;
  pLayerCfg.PixelFormat = LTDC_PIXEL_FORMAT_RGB565;
  pLayerCfg.Alpha = 255;
  pLayerCfg.Alpha0 = 255;
  pLayerCfg.BlendingFactor1 = LTDC_BLENDING_FACTOR1_CA;
  pLayerCfg.BlendingFactor2 = LTDC_BLENDING_FACTOR2_CA;
  pLayerCfg.FBStartAdress = 0x24000000;
  pLayerCfg.ImageWidth = 480;
  pLayerCfg.ImageHeight = 272;
  pLayerCfg.Backcolor.Blue = 0;
  pLayerCfg.Backcolor.Green = 0;
  pLayerCfg.Backcolor.Red = 0;
  if (HAL_LTDC_ConfigLayer(&hltdc, &pLayerCfg, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN LTDC_Init 2 */
  // trigger interrupt after scanning the most bottom line
  // to switch between dual frame buffers
//  HAL_LTDC_ProgramLineEvent(&hltdc, hltdc.Init.VerticalSync + hltdc.Init.AccumulatedVBP + pLayerCfg.ImageHeight);

  /* USER CODE END LTDC_Init 2 */

}

/**
  * @brief OCTOSPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_OCTOSPI1_Init(void)
{

  /* USER CODE BEGIN OCTOSPI1_Init 0 */

  /* USER CODE END OCTOSPI1_Init 0 */

  OSPIM_CfgTypeDef sOspiManagerCfg = {0};

  /* USER CODE BEGIN OCTOSPI1_Init 1 */

  /* USER CODE END OCTOSPI1_Init 1 */
  /* OCTOSPI1 parameter configuration*/
  hospi1.Instance = OCTOSPI1;
  hospi1.Init.FifoThreshold = 1;
  hospi1.Init.DualQuad = HAL_OSPI_DUALQUAD_DISABLE;
  hospi1.Init.MemoryType = HAL_OSPI_MEMTYPE_MICRON;
  hospi1.Init.DeviceSize = 23;
  hospi1.Init.ChipSelectHighTime = 4;
  hospi1.Init.FreeRunningClock = HAL_OSPI_FREERUNCLK_DISABLE;
  hospi1.Init.ClockMode = HAL_OSPI_CLOCK_MODE_0;
  hospi1.Init.WrapSize = HAL_OSPI_WRAP_NOT_SUPPORTED;
#if defined(LOW_CPU_FREQ)
  hospi1.Init.ClockPrescaler = 1;
#else
  hospi1.Init.ClockPrescaler = 3;
#endif
  hospi1.Init.SampleShifting = HAL_OSPI_SAMPLE_SHIFTING_NONE;
  hospi1.Init.DelayHoldQuarterCycle = HAL_OSPI_DHQC_DISABLE;
  hospi1.Init.ChipSelectBoundary = 0;
  hospi1.Init.ClkChipSelectHighTime = 0;
  hospi1.Init.DelayBlockBypass = HAL_OSPI_DELAY_BLOCK_BYPASSED;
  hospi1.Init.MaxTran = 0;
  hospi1.Init.Refresh = 0;
  if (HAL_OSPI_Init(&hospi1) != HAL_OK)
  {
    Error_Handler();
  }
  sOspiManagerCfg.ClkPort = 1;
  sOspiManagerCfg.NCSPort = 1;
  sOspiManagerCfg.IOLowPort = HAL_OSPIM_IOPORT_1_LOW;
  if (HAL_OSPIM_Config(&hospi1, &sOspiManagerCfg, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN OCTOSPI1_Init 2 */

  /* USER CODE END OCTOSPI1_Init 2 */

}

/**
  * @brief SAI2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SAI2_Init(void)
{

  /* USER CODE BEGIN SAI2_Init 0 */

  /* USER CODE END SAI2_Init 0 */

  /* USER CODE BEGIN SAI2_Init 1 */
# if 0
  /* USER CODE END SAI2_Init 1 */
  hsai_BlockA2.Instance = SAI2_Block_A;
  hsai_BlockA2.Init.AudioMode = SAI_MODEMASTER_RX;
  hsai_BlockA2.Init.Synchro = SAI_ASYNCHRONOUS;
  hsai_BlockA2.Init.OutputDrive = SAI_OUTPUTDRIVE_DISABLE;
  hsai_BlockA2.Init.NoDivider = SAI_MASTERDIVIDER_ENABLE;
  hsai_BlockA2.Init.FIFOThreshold = SAI_FIFOTHRESHOLD_HF;
  hsai_BlockA2.Init.AudioFrequency = SAI_AUDIO_FREQUENCY_96K;
  hsai_BlockA2.Init.SynchroExt = SAI_SYNCEXT_DISABLE;
  hsai_BlockA2.Init.MonoStereoMode = SAI_STEREOMODE;
  hsai_BlockA2.Init.CompandingMode = SAI_NOCOMPANDING;
  if (HAL_SAI_InitProtocol(&hsai_BlockA2, SAI_I2S_STANDARD, SAI_PROTOCOL_DATASIZE_24BIT, 2) != HAL_OK)
  {
    Error_Handler();
  }
  hsai_BlockB2.Instance = SAI2_Block_B;
  hsai_BlockB2.Init.AudioMode = SAI_MODEMASTER_TX;
  hsai_BlockB2.Init.Synchro = SAI_ASYNCHRONOUS;
  hsai_BlockB2.Init.OutputDrive = SAI_OUTPUTDRIVE_ENABLE;
  hsai_BlockB2.Init.NoDivider = SAI_MASTERDIVIDER_ENABLE;
  hsai_BlockB2.Init.FIFOThreshold = SAI_FIFOTHRESHOLD_HF;
  hsai_BlockB2.Init.AudioFrequency = SAI_AUDIO_FREQUENCY_96K;
  hsai_BlockB2.Init.SynchroExt = SAI_SYNCEXT_DISABLE;
  hsai_BlockB2.Init.MonoStereoMode = SAI_STEREOMODE;
  hsai_BlockB2.Init.CompandingMode = SAI_NOCOMPANDING;
  hsai_BlockB2.Init.TriState = SAI_OUTPUT_NOTRELEASED;
  if (HAL_SAI_InitProtocol(&hsai_BlockB2, SAI_I2S_STANDARD, SAI_PROTOCOL_DATASIZE_24BIT, 2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SAI2_Init 2 */
#else

  SAI_decoder_init(DEFAULT_SAMPLE_RATE_HZ);
  SAI_encoder_init(DEFAULT_SAMPLE_RATE_HZ);

#endif
  /* USER CODE END SAI2_Init 2 */

}

/**
  * @brief SDMMC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SDMMC1_SD_Init(void)
{

  /* USER CODE BEGIN SDMMC1_Init 0 */

  /* USER CODE END SDMMC1_Init 0 */

  /* USER CODE BEGIN SDMMC1_Init 1 */

  /* USER CODE END SDMMC1_Init 1 */
  hsd1.Instance = SDMMC1;
  hsd1.Init.ClockEdge = SDMMC_CLOCK_EDGE_RISING;
  hsd1.Init.ClockPowerSave = SDMMC_CLOCK_POWER_SAVE_DISABLE;
  hsd1.Init.BusWide = SDMMC_BUS_WIDE_1B;
  hsd1.Init.HardwareFlowControl = SDMMC_HARDWARE_FLOW_CONTROL_DISABLE;
  hsd1.Init.ClockDiv = 0;
  /* USER CODE BEGIN SDMMC1_Init 2 */

  /* USER CODE END SDMMC1_Init 2 */

}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8; // 500kHz
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 0x0;
  hspi1.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
  hspi1.Init.NSSPolarity = SPI_NSS_POLARITY_LOW;
  hspi1.Init.FifoThreshold = SPI_FIFO_THRESHOLD_01DATA;
  hspi1.Init.TxCRCInitializationPattern = SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;
  hspi1.Init.RxCRCInitializationPattern = SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;
  hspi1.Init.MasterSSIdleness = SPI_MASTER_SS_IDLENESS_00CYCLE;
  hspi1.Init.MasterInterDataIdleness = SPI_MASTER_INTERDATA_IDLENESS_00CYCLE;
  hspi1.Init.MasterReceiverAutoSusp = SPI_MASTER_RX_AUTOSUSP_DISABLE;
  hspi1.Init.MasterKeepIOState = SPI_MASTER_KEEP_IO_STATE_DISABLE;
  hspi1.Init.IOSwap = SPI_IO_SWAP_DISABLE;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
#if defined(LOW_CPU_FREQ)
  htim2.Init.Prescaler = 127;
#else
  htim2.Init.Prescaler = 255;
#endif
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 9999; //10ms
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

}



/* TIM5 init function */
void MX_TIM5_Init(void)
{

  /* USER CODE BEGIN TIM5_Init 0 */

  /* USER CODE END TIM5_Init 0 */

  TIM_SlaveConfigTypeDef sSlaveConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM5_Init 1 */

  /* USER CODE END TIM5_Init 1 */
  htim5.Instance = TIM5;
  htim5.Init.Prescaler = 0;
  htim5.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim5.Init.Period = 4294967295U; // MAX UINT32_T
  htim5.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim5.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim5) != HAL_OK)
  {
    Error_Handler();
  }
  sSlaveConfig.SlaveMode = TIM_SLAVEMODE_EXTERNAL1;
  sSlaveConfig.InputTrigger = TIM_TS_ETRF;
  sSlaveConfig.TriggerPolarity = TIM_TRIGGERPOLARITY_NONINVERTED;
  sSlaveConfig.TriggerPrescaler = TIM_TRIGGERPRESCALER_DIV1;
  sSlaveConfig.TriggerFilter = 2;
  if (HAL_TIM_SlaveConfigSynchro(&htim5, &sSlaveConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim5, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIMEx_RemapConfig(&htim5, TIM_TIM5_ETR_SAI2_FSA) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM5_Init 2 */

  /* USER CODE END TIM5_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart1.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&huart1, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&huart1, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Stream0_IRQn interrupt configuration SAI2 */
  HAL_NVIC_SetPriority(DMA1_Stream0_IRQn, 2, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream0_IRQn);
  /* DMA1_Stream1_IRQn interrupt configuration SAI2 */
  HAL_NVIC_SetPriority(DMA1_Stream1_IRQn, 2, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream1_IRQn);
  /* DMA1_Stream2_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream2_IRQn, 6, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream2_IRQn);
  /* DMA1_Stream3_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream3_IRQn, 6, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream3_IRQn);
}

/**
  * Enable MDMA controller clock
  * Configure MDMA for global transfers
  *   hmdma_mdma_channel40_sw_0
  */
static void MX_MDMA_Init(void)
{

  /* MDMA controller clock enable */
  __HAL_RCC_MDMA_CLK_ENABLE();
  /* Local variables */

  /* Configure MDMA channel MDMA_Channel0 */
  /* Configure MDMA request hmdma_mdma_channel40_sw_0 on MDMA_Channel0 */
  hmdma_mdma_channel40_sw_0.Instance = MDMA_Channel0;
  hmdma_mdma_channel40_sw_0.Init.TransferTriggerMode = MDMA_BLOCK_TRANSFER;
  hmdma_mdma_channel40_sw_0.Init.Priority = MDMA_PRIORITY_LOW;
  hmdma_mdma_channel40_sw_0.Init.Endianness = MDMA_LITTLE_ENDIANNESS_PRESERVE;
  hmdma_mdma_channel40_sw_0.Init.SourceInc = MDMA_SRC_INC_HALFWORD;
  hmdma_mdma_channel40_sw_0.Init.DestinationInc = MDMA_DEST_DEC_HALFWORD;
  hmdma_mdma_channel40_sw_0.Init.SourceDataSize = MDMA_SRC_DATASIZE_HALFWORD;
  hmdma_mdma_channel40_sw_0.Init.DestDataSize = MDMA_DEST_DATASIZE_HALFWORD;
  hmdma_mdma_channel40_sw_0.Init.DataAlignment = MDMA_DATAALIGN_PACKENABLE;
  hmdma_mdma_channel40_sw_0.Init.BufferTransferLength = 1;
  hmdma_mdma_channel40_sw_0.Init.SourceBurst = MDMA_SOURCE_BURST_SINGLE;
  hmdma_mdma_channel40_sw_0.Init.DestBurst = MDMA_DEST_BURST_SINGLE;
  hmdma_mdma_channel40_sw_0.Init.SourceBlockAddressOffset = 0;
  hmdma_mdma_channel40_sw_0.Init.DestBlockAddressOffset = 0;
  if (HAL_MDMA_Init(&hmdma_mdma_channel40_sw_0) != HAL_OK)
  {
    Error_Handler();
  }

  /* MDMA interrupt initialization */
  /* MDMA_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(MDMA_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(MDMA_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOE, NP_EN_Pin|TFT_DISP_Pin, 1);
  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOE, PWR_EN_Pin, 1);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(BMS_KEY_GPIO_Port, BMS_KEY_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, DECODER_RSTX_Pin|BK_EN_Pin|SPI1_CS_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD, PREAMP_PD1_Pin|PREAMP_PD2_Pin|LED_MIC1_Pin|LED_MIC2_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, NOCHRGX_Pin, GPIO_PIN_RESET); // stop charging when turned on

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(T_CSX_GPIO_Port, T_CSX_Pin, GPIO_PIN_SET);

  /*Configure GPIO pins : NP_EN_Pin TFT_DISP_Pin */
  GPIO_InitStruct.Pin = NP_EN_Pin|TFT_DISP_Pin|PWR_EN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pins : NOCHRGX_Pin */
  GPIO_InitStruct.Pin = NOCHRGX_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);


  /*Configure GPIO pin : BMS_KEY_Pin */
  GPIO_InitStruct.Pin = BMS_KEY_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(BMS_KEY_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : ENCA_Pin */
  GPIO_InitStruct.Pin = ENCA_Pin; // EXTI 7
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pins : ENCSW_Pin */
  GPIO_InitStruct.Pin = ENCSW_Pin; // EXTI 9
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);


  /*Configure GPIO pins : ENCB_Pin  */
  GPIO_InitStruct.Pin = ENCB_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pins : DECODER_RSTX_Pin BK_EN_Pin SPI1_CS_Pin */
  GPIO_InitStruct.Pin = DECODER_RSTX_Pin|BK_EN_Pin|SPI1_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : ZEROR_Pin ZEROL_Pin PREAMP_PD1_Pin PREAMP_PD2_Pin
                           T_CSX_Pin */
  GPIO_InitStruct.Pin = PREAMP_PD1_Pin|PREAMP_PD2_Pin|LED_MIC1_Pin|LED_MIC2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);


  /*Configure GPIO pins : OUT2_DETECT_Pin OUT1_DETECT_Pin */
  GPIO_InitStruct.Pin = OUT2_DETECT_Pin|OUT1_DETECT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pin : PC9 I2S CKIN */
  GPIO_InitStruct.Pin = GPIO_PIN_9;

  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF5_SPI1;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : DETECT_SDIO_Pin */
  GPIO_InitStruct.Pin = DETECT_SDIO_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(DETECT_SDIO_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : T_INTX_Pin */
  GPIO_InitStruct.Pin = T_INTX_Pin; // EXTI 4
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(T_INTX_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : T_BUSY_Pin */
  GPIO_InitStruct.Pin = T_BUSY_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(T_BUSY_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : T_CSX_Pin */
  GPIO_InitStruct.Pin = T_CSX_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pin : OUT1_DETECT_Pin OUT2_DETECT_Pin */
  GPIO_InitStruct.Pin = OUT1_DETECT_Pin|OUT2_DETECT_Pin; // EXTI 9, EXTI 8
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(OUT1_DETECT_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : SHUTDOWNX_Pin  */
  GPIO_InitStruct.Pin = SHUTDOWNX_Pin; // EXTI 4
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(SHUTDOWNX_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : MIC_DETECT1_Pin  */
  GPIO_InitStruct.Pin = MIC_DETECT1_Pin; // EXTI 8
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(MIC_DETECT1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : MIC_DETECT2_Pin  */
  GPIO_InitStruct.Pin = MIC_DETECT2_Pin; // EXTI 12
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(MIC_DETECT2_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  /*LIST OF EXTI INTERRUPTS
   * T_INTX:  4 FALLING
   * SHUTDOWNX: 4 FALLING
   * ENCA: 7	FALLING/RISING
   * ENCSW: 9	FALLING
   * OUT1 DETECT: 9 RISING
   * OUT2_DETECT: 8 RISING
   * MIC_DETECT1: 8 RISING
   * MIC_DETECT2: 12 RISING
   */
  HAL_NVIC_SetPriority(EXTI4_IRQn, 8, 0);
  HAL_NVIC_EnableIRQ(EXTI4_IRQn);
  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 8, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);
  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 8, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

}

/* USER CODE BEGIN 4 */
char bufLVGLDebug[256] = {0};
/* LVGL timer for tasks. */
void taskLVGLTimer(void * argument)
{
	portTASK_USES_FLOATING_POINT();
  /* init: start lvgl */
  init_LVGL_GUI();
  // wait for start signal fired by default task, when Config is successfully loaded from the filesystem
  osThreadFlagsWait(FLAG_TASK_START, osFlagsWaitAll, osWaitForever);
#ifdef DEBUG
  // invoke calibrate when screen is touch when turning on
  bool calibrate_touchscreen = !cfg.TSCalibInfo.isCalibrated;
  if(cfg.TSCalibInfo.isCalibrated)
  {
	  int x,y,z;
	  z = touch_read_XY_ADC(&x, &y);
	  if(z > 400)
		  calibrate_touchscreen = true;
  }
#else
  bool calibrate_touchscreen = !cfg.TSCalibInfo.isCalibrated;
#endif
  start_LVGL_input_tasks(calibrate_touchscreen);
  if(!calibrate_touchscreen)
  {
	  lv_lock();
	  display_main_widgets();
	  lv_unlock();
  }

  /* set GUI ready flag */
  isGUIReady = true;


  // show init debug info
#ifdef DEBUG
  if(bufLVGLDebug[0] != 0)
  {
	  show_toast(bufLVGLDebug);
	  bufLVGLDebug[0] = 0;
  }
#endif
  for(;;)
  {
#ifdef DEBUG
	  HAL_GPIO_WritePin(LED_MIC1_GPIO_Port, LED_MIC1_Pin, 0); // light
#endif
	lv_lock();
	uint32_t time_till_next_ms = lv_timer_handler();
	lv_unlock();
#ifdef DEBUG
	  HAL_GPIO_WritePin(LED_MIC1_GPIO_Port, LED_MIC1_Pin, 1); // off
#endif
	  HAL_IWDG_Refresh(&hiwdg1);
    osDelay(pdMS_TO_TICKS(time_till_next_ms));
  }
}

#define N_I2C_TRANSMIT_SLOTS 2
#define BYTES_I2C_TRANSMIT_BUFFER 128
#define BYTES_I2C_MAX_SINGLE_TRANSMIT 5
uint16_t arr_I2C_device_addrs[N_I2C_TRANSMIT_SLOTS] = {PCM179X_I2C_ADDR, ADCX120_I2C_ADDR};
// each packet = length,content
uint8_t arr_buf_I2C_transmit[N_I2C_TRANSMIT_SLOTS][BYTES_I2C_TRANSMIT_BUFFER];
KFIFO_DMA arr_fifo_I2C_transmit[N_I2C_TRANSMIT_SLOTS];

void init_async_i2c()
{
	for(int i = 0; i < N_I2C_TRANSMIT_SLOTS; ++i)
	{
		memset(arr_buf_I2C_transmit[i], 0, BYTES_I2C_TRANSMIT_BUFFER);
		kfifo_DMA_static_init(&arr_fifo_I2C_transmit[i], arr_buf_I2C_transmit[i], BYTES_I2C_TRANSMIT_BUFFER, 0);
	}
}

uint8_t transmit_async_i2c(uint8_t slot, uint8_t* packet, uint8_t len)
{
	uint8_t data[BYTES_I2C_MAX_SINGLE_TRANSMIT + 1];
	uint8_t lenTransmitted = 0;
	if(slot < N_I2C_TRANSMIT_SLOTS)
	{
		KFIFO_DMA* pfifo = &arr_fifo_I2C_transmit[slot];
		uint8_t sizeFree = kfifo_get_free_space(pfifo);
		if(sizeFree > len)
		{
			data[0] = len;
			memcpy(data+1, packet, len);
			kfifo_put(pfifo, data, len+1);
			lenTransmitted = len;
		}
	}
	return lenTransmitted;
}

void StartHumanInOutTask(void *argument)
{
	static uint8_t fsmPowerOff = 0;
	static uint8_t fsmMics[N_MICROPHONES] = {0};
	static uint8_t fsmOuts[2] = {0};
	int rflags;
	init_async_i2c();
	rflags = osThreadFlagsWait(FLAG_TASK_START, osFlagsWaitAll, osWaitForever);
	flagHumanInputTaskReady = 1;
	for(;;)
	{
		rflags = osThreadFlagsWait(0xFFF, osFlagsWaitAny, pdMS_TO_TICKS(100));
		HAL_IWDG_Refresh(&hiwdg1);
		// routine task: no flag received, but timeout, process shutdown delay
		if(rflags < 0)
		{
			// only judge if power off pin has been released
			if(fsmPowerOff)
			{
			  // TODO: shutdown
			  if((HAL_GPIO_ReadPin(SHUTDOWNX_GPIO_Port, SHUTDOWNX_Pin) == 1)) // button released
			  {
				  // to confirm
				  osDelay(pdMS_TO_TICKS(20));
				  if(HAL_GPIO_ReadPin(SHUTDOWNX_GPIO_Port, SHUTDOWNX_Pin) == 1) // confirmed that button has been released
				  {
					  fsmPowerOff = 0;
					  osMutexAcquire(mtxConfig, osWaitForever);
					  save_config_local();
					  osMutexRelease(mtxConfig);
					  osDelay(pdMS_TO_TICKS(200)); // give some time for saving
#ifndef DEBUG
					  power_off();

#else
					  // This task also conflicts with USB MSC mode, for the two applications will race on one SDIO device
					  cfg.audio_connections[0] = 0b1111;
					  cfg.audio_connections[1] = 0b1111;
					  audio_connections_apply(cfg.audio_connections);
					  FS_player_test();

#endif
				  }
			  }
			}
			/* TASK: process analog device reset BEGIN */
			if((audio_analog_chips_need_reset) && (hi2c1.State == HAL_I2C_STATE_READY) /* to avoid conflict with async I2C transmissions*/)
			{
				reset_audio_analog_chips();
				audio_analog_chips_need_reset = false;
			}
			/* TASK: process analog device reset END */
			/* TASK: process I2C BEGIN */
			for(int i = 0; i < N_I2C_TRANSMIT_SLOTS; ++i)
			{
				uint8_t sizeFilled = kfifo_get_filled_size(&arr_fifo_I2C_transmit[i]);
				if(sizeFilled > 0 )
				{
					uint8_t sizePack;
					kfifo_get(&arr_fifo_I2C_transmit[i], &sizePack, 1);
					if(sizePack > 0)
					{
						static uint8_t __attribute__((section(".DMA_no_cache"))) data[BYTES_I2C_MAX_SINGLE_TRANSMIT];
						kfifo_get(&arr_fifo_I2C_transmit[i], data, sizePack);
						uint16_t addrDevice = arr_I2C_device_addrs[i];
						if (__HAL_I2C_GET_FLAG(&hi2c1, I2C_FLAG_BUSY)) {
						    __HAL_I2C_CLEAR_FLAG(&hi2c1, I2C_FLAG_BUSY); // 清除 BUSY 标志
						    hi2c1.State = HAL_I2C_STATE_READY; // 将 I2C 状态设置为 READY
						}
						HAL_I2C_Master_Transmit_DMA(&hi2c1, addrDevice, data, sizePack);
						// the interval between tasks are sufficent for DMA transmission, so no need for syncnization with DMA finish signal
					}
				}
			}
			/* TASK: process I2C END */


			continue;
		};
		/* TASK: detect power off */
		if((rflags & (FLAG_SHUTDOWN_BUTTON_LONG_PRESSED | FLAG_SHUTDOWN_NOW)) != 0)
		{
		  // TODO: pop up prompt window
			lv_lock();
			show_msgbox_farewell();
			lv_unlock();
			osDelay(pdMS_TO_TICKS(2000));
			fsmPowerOff = 1;
		}
		/* TASK: detect back light off*/
		else if((rflags & FLAG_SHUTDOWN_BUTTON_SHORT_CLICKED) != 0)
		{
			if(is_back_light_on)
			{
				set_back_light(0);
				set_mic_LEDs(0, 1);
			}
			else
			{
				set_back_light(1);
			}
		}


		ENTER_BUTTON_CRITICAL;
		/* TASK: detect microphone plug (rising edge) and unplug (falling edge)*/
		for(int i = 0; i < N_MICROPHONES; ++i)
		{
			// detect plug in, send from EXTI ISR
			if(rflags & (FLAG_MIC_1_PLUGGED << i))
			{
				fsmMics[i] = 1;
				osMutexAcquire(mtxConfig, osWaitForever);
				// by default: link microphone i to USB output
				cfg.audio_connections[1] |= (1U << i);
				// prepare devices and etc
				audio_connections_apply(cfg.audio_connections);
				osMutexRelease(mtxConfig);
			}
			// detect unplug, judge from GPIO reading
			else if(fsmMics[i])
			{
				uint8_t v = HAL_GPIO_ReadPin(micports[i], micpins[i]);
				if(v == 0) // unplugged
				{
					// to confirm
				  osDelay(pdMS_TO_TICKS(20));
				  v = HAL_GPIO_ReadPin(micports[i], micpins[i]);
				  if(v == 0) // unplugged
				  {
					    fsmMics[i] = 0;
						osMutexAcquire(mtxConfig, osWaitForever);
						// by default: disconnect microphone i to USB output
						cfg.audio_connections[1] &= ~(1U << i);
						// prepare devices and etc
						audio_connections_apply(cfg.audio_connections);
						osMutexRelease(mtxConfig);
				  }
				}

			}
		}

		/* TASK: detect output plug (rising edge) and unplug (falling edge)*/
		for(int i = 0; i < N_HEADPHONES; ++i)
		{
			// detect plug
			if(rflags & (FLAG_OUT_1_PLUGGED << i))
			{
				fsmMics[i] = 1;
				osMutexAcquire(mtxConfig, osWaitForever);
				// by default: link decoder to FS and USB speaker
				cfg.audio_connections[0] |= 0b1100U;
				// prepare devices and etc
				audio_connections_apply(cfg.audio_connections);
				osMutexRelease(mtxConfig);
			}
			// detect unplug
			else if(fsmMics[i])
			{
				uint8_t v = HAL_GPIO_ReadPin(headphoneports[i], headphonepins[i]);
				if(v == 0) // unplugged
				{
					// to confirm
				  osDelay(pdMS_TO_TICKS(20));
				  v = HAL_GPIO_ReadPin(headphoneports[i], headphonepins[i]);
				  if(v == 0) // unplugged
				  {
						fsmOuts[i]=0;
						osMutexAcquire(mtxConfig, osWaitForever);
						// turn off decoder if all Outputs are unplugged
						if(fsmOuts[0] == 0 && fsmOuts[1] == 0)
						{
							cfg.audio_connections[1] = 0U;
							// prepare devices and etc
							audio_connections_apply(cfg.audio_connections);
						}
						osMutexRelease(mtxConfig);
				  }
				}
			}
		}
		EXIT_BUTTON_CRITICAL;


	}
}





/* USER CODE END 4 */
/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
FIL __attribute__((section(".DMA_no_cache"))) f;
DIR __attribute__((section(".DMA_no_cache"))) dir;
uint8_t __attribute__((aligned(32))) __attribute__((section(".DMA_no_cache"))) bufFile[512];

static void save_config_local()
{
	  /* TASK: save config on EEPROM */

	  if(isModified)
	  {
		  FRESULT fr = FR_OK;
		  if(!isFilesystemOK)
		  {
			  SD_Driver.disk_initialize(0);
			  fr = f_mount(&SDFatFS, (const TCHAR*)SDPath,1);
		  }
		  if(fr == FR_OK)
			  fr = f_open(&f, FS_PATH_CONFIG_FILE,FA_WRITE | FA_CREATE_ALWAYS);
		  if(fr == FR_OK)
		  {
			  memcpy(bufFile, &cfg, sizeof(cfg));
			  size_t lenWritten;
			  fr = f_write(&f, bufFile, sizeof(cfg), &lenWritten);
			  f_close(&f);
			  lv_lock();
			  show_toast("Config saved!");
			  lv_unlock();
			  osDelay(pdMS_TO_TICKS(1000));
		  }
		  else
		  {
			  number2text(bufLVGLDebug, fr, 0, 0);
			  lv_lock();
			  show_toast(bufLVGLDebug);
			  lv_unlock();
			  osDelay(pdMS_TO_TICKS(2000));
		  }
		  isModified = false;
	  }


}


/* USER CODE END Header_StartDefaultTask */
bool isFilesystemOK = false;
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN 5 */
  osDelay(pdMS_TO_TICKS(10)); // wait for SD card ready
  /* Init: mount SD card */
  SD_Driver.disk_initialize(0);
  FRESULT fr = f_mount(&SDFatFS, (const TCHAR*)SDPath,1);

  size_t lenRead;
  if(fr == FR_OK)
  {
	  /* NOTE: SDMMC1 buf cannot be put at AHB SRAMS, DMA will halt */
	  isFilesystemOK = true;
	  /* Init: test filesystem*/
#ifdef DEBUG
	  fr = f_open(&f, L"0:/test.txt", FA_READ);
	  if(fr == FR_OK)
	  {
		  f_read(&f, (uint8_t*)bufFile, 512, &lenRead);
		  f_close(&f);
		  strncpy(bufLVGLDebug, (char*)bufFile, 16);
	  }
	  else
	  {
		  strcpy(bufLVGLDebug, "unable to open test file");
	  }
#endif
	  /* Init: Load Config */
	  bool isConfigLoaded = false;
	  fr = f_open(&f, FS_PATH_CONFIG_FILE, FA_READ | FA_OPEN_EXISTING);
	  if(fr == FR_OK)
	  {
		  f_read(&f, bufFile, sizeof(cfg), &lenRead);
		  f_close(&f);
		  if(lenRead == sizeof(cfg))
		  {
			  osMutexAcquire(mtxConfig, osWaitForever);
			  if((fr == FR_OK) && (config_check_valid((Config*)bufFile)))
			  {
				  memcpy(&cfg, bufFile, lenRead);
				  isConfigLoaded = true;
			  }
			  osMutexRelease(mtxConfig);
		  }
	  }
	  // create config file if not loaded successfully
	  if(!isConfigLoaded)
	  {
		  strcpy(bufLVGLDebug, "unable to open config file");
		  fr = f_opendir(&dir, FS_FOLDER_SYS);
		  if(fr == FR_OK)
			  f_closedir(&dir);
		  else
			  fr = f_mkdir(FS_FOLDER_SYS);
		  osMutexAcquire(mtxConfig, osWaitForever);
		  fr = f_open(&f, FS_PATH_CONFIG_FILE, FA_WRITE | FA_CREATE_ALWAYS);
		  f_write(&f,(uint8_t*)&cfg, sizeof(Config), &lenRead);
		  f_close(&f);
		  osMutexRelease(mtxConfig);
	  }
	  // audio DSP init
	for(int i = 0; i < N_INPUTS; ++i)
	{
		// initialize DSP, before profile loaded in GUI
		if(arr_DSPs[i])
		{
			for(int j =0; j < arr_N_DSP_channels[i]; ++j)
			{
				// load preset for DSPs
				int r = -1;
				if(cfg.dsp_preset_ids[i] > 0)
				{
					r = load_and_apply_DSP_preset(&arr_DSPs[i][j], cfg.dsp_preset_ids[i]);

				}
				// if preset load failed, initialize
				if(r != 0)
				{
					audio_DSP_cfg_t dsp_cfg = {
							.cfg_compress = audio_compress_cfg_default,
							.cfg_EQ = audio_EQ_cfg_default,
							.cfg_reverb = audio_reverb_cfg_default,
							.function_bits = 0,
					};
					audio_DSP_init(&arr_DSPs[i][j], &dsp_cfg);
				}
			}
		}
	}
	  // apply audio connection and prepare transmissions
	  adcx120_init_t adcx120_init_cfg = {
			  .pregain_db2 = {cfg.nPregainsDB2_encoder[0], cfg.nPregainsDB2_encoder[1]},
			  .volume100 = {cfg.nVolume100_encoder[0], cfg.nVolume100_encoder[1]},
	  };
	  // adcx
	  encocder_input_start();
	  osDelay(pdMS_TO_TICKS(10));
	  adcx120_reset(&adcx120);
	  osDelay(pdMS_TO_TICKS(10));
	  adcx120_awake(&adcx120);
	  osDelay(pdMS_TO_TICKS(50));
	  adcx120_init(&adcx120, &adcx120_init_cfg);
	   // if enabled in config, will init all audio devices
	  taskENTER_CRITICAL();
	  sync_volumes(false); // blocked
	  taskEXIT_CRITICAL();
	  audio_connections_apply(cfg.audio_connections);
	  // sync dry/wet sound connections
	  audio_outpus_dsp_connection_apply(cfg.outputs_dsp_usage);
	  /* load DSP profiles BEGIN */
	  TCHAR* wcsProfilePath = wcsPathToPlay;
	  // build path
	  TCHAR* wcsID = strncpy_TCHAR(wcsProfilePath, PATH_DSP_PROFILE_FOLDER _T("/") FILENAME_DSP_PROFILE, _MAX_LFN);
	  // iterate all input dsps
	  uint8_t id = 0;
	  audio_DSP_cfg_t cfgDSP;
	  for(int iIn = 0; iIn < N_INPUTS; ++iIn)
	  {
		  if((id = cfg.dsp_preset_ids[iIn]) > 0)
		  {
			  audio_DSP_t* pDSP = arr_DSPs[iIn];
			  if(pDSP == NULL) continue;
			  uint8_t nChs = arr_num_channels_to_share_DSP[iIn];
			  /* make path */
			  // add number to path
			  *wcsID = _T('0') + id;
			  // terminate the string
			  *(wcsID+1) = 0;
			  /* open file */
			  // try to open the file
			  bool loaded = false;
			  fr = f_open(&f, wcsProfilePath, FA_READ | FA_OPEN_EXISTING);
			  if(fr == FR_OK)
			  {
				  fr = f_read(&f, bufFile, sizeof(audio_DSP_cfg_t), &lenRead);
				  f_close(&f);
				  if(fr == FR_OK)
				  {
					  if(lenRead == sizeof(audio_DSP_cfg_t))
						  loaded = true;
				  }
			  }
			  if(loaded)
			  {
				  memcpy(&cfgDSP, bufFile, sizeof(cfgDSP));
				  for(int idxCh = 0; idxCh < nChs; idxCh++)
				  {
					  audio_DSP_init(&(pDSP[idxCh]), &cfgDSP);
				  }
			  }
			  else
			  {
				  cfg.dsp_preset_ids[iIn] = 0; // mark the profile as invalid
				  isModified = true;
			  }

		  }
	  }

	  /* load DSP profiles END */
	  USB_switch_to_configuration(cfg.usbCfgDescSelector);
  }
  else
  {
	  strcpy(bufLVGLDebug, "unable to mount SD card");
  }
  /* Start Other dependent tasks*/
  // START HUMAN INPUT TASK
  osThreadFlagsSet(taskHumanInputHandle, FLAG_TASK_START);
  // START LVGL
  osThreadFlagsSet(taskLVGLTimerHandle, FLAG_TASK_START);

  /* init code for audio tasks */
  if(cfg.usbCfgDescSelector != USB_CFG_DESC_MASS_STORAGE)
	  FS_player_task_start();
//  USB_player_task_start();

  /* init code for USB_DEVICE */
  MX_USB_DEVICE_Init();
  HAL_IWDG_Refresh(&hiwdg1);
  /* test FS player */

  /* test FS player END */

  /* Infinite loop */
  for(;;)
  {
      osDelay(osWaitForever);
	  /* TASK: detect battery life */
	  static int VBAT_mv_avg = 0;
	  taskENTER_CRITICAL();
	  int VBAT_mv = touch_read_VBAT_mv();
	  taskEXIT_CRITICAL();
	  if(VBAT_mv_avg == 0)
		  VBAT_mv_avg = VBAT_mv;
	  else
		  VBAT_mv_avg = (VBAT_mv_avg * 15 + VBAT_mv) / 16;
	  // GUI updates
	  if(isGUIReady)
	  {
		  lv_lock();
		  if(VBAT_mv_avg > 4000U) // SOC: 100%
		  {
			  GUI_set_battery_SOC(4);
		  }
		  else if(VBAT_mv_avg > 3800) // SOC: 75%
		  {
			  GUI_set_battery_SOC(3);
		  }
		  else if(VBAT_mv_avg > 3600) // SOC: 50%
		  {
			  GUI_set_battery_SOC(2);
		  }
		  else if(VBAT_mv_avg > 3400) // SOC: 25%
		  {
			  GUI_set_battery_SOC(1);
		  }
		  else // SOC: empty
		  {
			  GUI_set_battery_SOC(0);
		  }
		  lv_unlock();

		  /* TASK: detect temperature */
		  static int temp_C_avg = 0;
		  taskENTER_CRITICAL();
		  int temp_C = touch_read_temperature();
		  taskEXIT_CRITICAL();
		  if(temp_C_avg == 0)
			  temp_C_avg = temp_C;
		  else
			  temp_C_avg = (temp_C_avg * 15 + temp_C) / 16;
		  lv_lock();
		  GUI_set_temperature(temp_C_avg);
		  lv_unlock();
	  }
  /* USER CODE END 5 */
  }
}

/* MPU Configuration */

void MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct = {0};

  /* Disables the MPU */
  HAL_MPU_Disable();
  /** Initializes and configures the Region and the memory to be protected
  */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
  MPU_InitStruct.BaseAddress = 0x08000000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_128KB;
  MPU_InitStruct.SubRegionDisable = 0x0;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_CACHEABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_BUFFERABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);
  /** Initializes and configures the Region and the memory to be protected
  */
  MPU_InitStruct.Number = MPU_REGION_NUMBER1;
  MPU_InitStruct.BaseAddress = 0x90000000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_16MB;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);
  /** Initializes and configures the Region and the memory to be protected
   * DTCM1
  */
  MPU_InitStruct.Number = MPU_REGION_NUMBER2;
  MPU_InitStruct.BaseAddress = 0x20000000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_64KB;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);
  /** Initializes and configures the Region and the memory to be protected
  * DTCM2
  */
  MPU_InitStruct.Number = MPU_REGION_NUMBER3;
  MPU_InitStruct.BaseAddress = 0x20010000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_64KB;
  HAL_MPU_ConfigRegion(&MPU_InitStruct);
  /** Initializes and configures the Region and the memory to be protected
  */
  // RAM 1024K
  MPU_InitStruct.Number = MPU_REGION_NUMBER4;
  MPU_InitStruct.BaseAddress = 0x24000000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_1MB;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_CACHEABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_BUFFERABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);



  /** Initializes and configures the Region and the memory to be protected
  */
  // framebuffer1 for LTDC DMA 256K, no cache
  MPU_InitStruct.Number = MPU_REGION_NUMBER5;
  MPU_InitStruct.BaseAddress = 0x24000000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_256KB;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_CACHEABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);
  /** Initializes and configures the Region and the memory to be protected
  */
  // for DMA (1KB)+32KB (+1kb from the residual space of framebuffer1), no cache
  MPU_InitStruct.Number = MPU_REGION_NUMBER6;
  MPU_InitStruct.BaseAddress = 0x24040000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_32KB;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);

  /** Initializes and configures the Region and the memory to be protected
  */
  // for DMA (1KB)+32KB+16KB (+1kb from the residual space of framebuffer1), no cache
  MPU_InitStruct.Number = MPU_REGION_NUMBER7;
  MPU_InitStruct.BaseAddress = 0x24048000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_16KB;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);

  /** Initializes and configures the Region and the memory to be protected
  */
  MPU_InitStruct.Number = MPU_REGION_NUMBER8;
  MPU_InitStruct.BaseAddress = 0x30000000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_128KB;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_CACHEABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_BUFFERABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);

  /** Initializes and configures the Region and the memory to be protected
  */
  // ITCM
  MPU_InitStruct.Number = MPU_REGION_NUMBER9;
  MPU_InitStruct.BaseAddress = 0x00000000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_64KB;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_CACHEABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_BUFFERABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);
  /* Enables the MPU */
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);

}



/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {

  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
