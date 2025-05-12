/*
 * ButtonDebouncer.h
 *
 *  Created on: Oct 5, 2020
 *      Author: Zening
 */

#ifndef BUTTONDEBOUNCER_H_
#define BUTTONDEBOUNCER_H_

#include <stdbool.h>
#include <stdint.h>
#include "main.h"

typedef void (*btnCallbackFunc_t) (uint8_t);

typedef struct
{
	// 消抖后的按键状态 Low Effective 1: 高， 0: 低（按下)
	volatile uint8_t state;
	volatile uint8_t interrupted;
	volatile uint8_t active; // 1: active, 0: inactive
	uint8_t lvlAct; // 激活状态的电平
	GPIO_TypeDef* GPIOport;
	uint16_t GPIOpin;
	int cntdn; // 倒数定时器计数器()
	unsigned int cntdnRefill; // 倒数定时器重装填值(us)
	int cntLongPress;
	btnCallbackFunc_t on_flip; // 状态翻转时的回调函数
} ButtonDebouncer_t;

// 初始化按键消抖
// cntdnRetill 延迟us数
void btndeb_init(ButtonDebouncer_t* p, GPIO_TypeDef* GPIOport, uint16_t GPIOpin, uint8_t lvlAct, unsigned int cntdnRefill);

void btndeb_interrupt(ButtonDebouncer_t* p, uint8_t state_init);
// 时基脉膊的函数
// 状态翻转则返回true
bool btndeb_tick(ButtonDebouncer_t* p, unsigned int us);

// 注册翻转回调函数
void btndeb_register_flip_callback(ButtonDebouncer_t* p, btnCallbackFunc_t fn);

#endif /* BUTTONDEBOUNCER_H_ */
