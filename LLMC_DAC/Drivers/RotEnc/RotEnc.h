/*
 * rot_enc.h
 *
 *  Created on: Sep 27, 2020
 *      Author: Zening
 */

#ifndef ROTENC_H_
#define ROTENC_H_
#include <stdint.h>
#include <stdbool.h>
#include "main.h"
#include "ButtonDebouncer.h"

#define ROTENC_MAX_CALLBACKS 2


typedef struct {
	uint8_t cntBeforeTrig; // 记录下次触发前还需旋转的格数，反转重填
	uint8_t cntRequiredToTrig; // 定义触发前需要旋转的格数
	int8_t dirTrig; // trigger direction : -1 CCW only, 1 CW only, 2 both CCW and CW
	void (*on_trig) (void);
} RotEnc_prescaled_callback_t;

// FSM for rotary encoder
typedef enum {
	ROT_ENC_START = 0,
	ROT_ENC_WAIT_A_FALL_WITH_B_HIGH,
	ROT_ENC_WAIT_A_FALL_WITH_B_LOW,
	ROT_ENC_WAIT_A_RISE_WITH_B_HIGH,
	ROT_ENC_WAIT_A_RISE_WITH_B_LOW,
	ROT_ENC_CW_CONFIRMED,
	ROT_ENC_CCW_CONFIRMED,
} RotEnc_state_t;

typedef struct {
	ButtonDebouncer_t A;
	ButtonDebouncer_t B;
	ButtonDebouncer_t SW;

	RotEnc_state_t fsm;
	int cntRot; // 旋转计数器
	int8_t direction; // 当前旋转方向 1: CW, -1: CCW

	int cntdnBtwAB; // AB相信号间隔计时器(us)
	unsigned int cntdnBtwABRefill;

	uint8_t nCallbacks;
	RotEnc_prescaled_callback_t callbacks[ROTENC_MAX_CALLBACKS];

} RotEnc_t;

// cntdnRefill 消抖延迟us数
void RotEnc_init(RotEnc_t* p, unsigned int cntdnRefill,
		GPIO_TypeDef* portA, uint16_t pinA,
		GPIO_TypeDef* portB, uint16_t pinB,
		GPIO_TypeDef* portSW, uint16_t pinSW);

// 追加触发回调函数
void RotEnc_append_trigger_callback(RotEnc_t* p, uint8_t cntRequiredToTrig, int8_t dirTrig, void (*fn) (void));

// 方式1：旋转编码器的时间基准脉搏，当A相状态翻转时返回true
int8_t RotEnc_tick(RotEnc_t* p, int us);

// 方式2：中断模式旋转器
// @retval: direction of rotation, 1:CW -1:CCW 0:nosignal
// @params: fall_rise 0:fall, 1:rise
// @warning: callback function not yet implemented
int8_t RotEnc_interrupt(RotEnc_t*p,  uint8_t fall_rise);

#endif /* ROTENC_H_ */
