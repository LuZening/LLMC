/*
 * rot_enc.c
 *
 *  Created on: Sep 27, 2020
 *      Author: Zening
 */

#include "main.h"
#include "RotEnc.h"

void RotEnc_init(RotEnc_t* p, unsigned int cntdnRefill,
		GPIO_TypeDef* portA, uint16_t pinA,
		GPIO_TypeDef* portB, uint16_t pinB,
		GPIO_TypeDef* portSW, uint16_t pinSW)
{
	p->fsm = 0;
	// 初始化各个开关的消抖程序
	btndeb_init(&p->A, portA, pinA, 0, cntdnRefill);
	btndeb_init(&p->B, portB, pinB, 0, cntdnRefill);
	btndeb_init(&p->SW, portSW, pinSW, 0, cntdnRefill);
	p->cntRot = 0;
	p->direction = 1; // CW

	// 初始化旋转回调函数
	p->nCallbacks = 0;
	for (RotEnc_prescaled_callback_t* cb = p->callbacks; cb < p->callbacks + ROTENC_MAX_CALLBACKS; ++cb)
	{
		cb->cntBeforeTrig = 1;
		cb->cntRequiredToTrig = 1; // CW
		cb->dirTrig = 1; // trigger at CW only
		cb->on_trig = NULL;
	}
}


void RotEnc_append_trigger_callback(RotEnc_t* p, uint8_t cntRequiredToTrig, int8_t dirTrig, void (*fn)(void))
{
	if(p->nCallbacks <= ROTENC_MAX_CALLBACKS)
	{
		RotEnc_prescaled_callback_t* pCb = p->callbacks + p->nCallbacks;
		pCb->cntBeforeTrig = 0;
		pCb->cntRequiredToTrig = cntRequiredToTrig;
		pCb->dirTrig = dirTrig;
		pCb->on_trig = fn;
		p->nCallbacks++;
	}

}

int8_t RotEnc_tick(RotEnc_t* p, int us)
{
	bool isFlipA;
	bool isFlipB;
	bool isFlipSW;
	isFlipA = btndeb_tick(&p->A, us);
	isFlipB = btndeb_tick(&p->B, us);
	isFlipSW = btndeb_tick(&p->SW, us);
	UNUSED(isFlipB);
	UNUSED(isFlipSW);

	int8_t retval = 0;
	// 以A为基准
	if(isFlipA)
	{

		if((p->A.state == 0) && (p->B.state > 0)) // A上升B低 OR A下降B高 CCW
		{
			//CCW
			p->cntRot--;
			p->direction = -1;
		}
		else if((p->A.state == 0) && (p->B.state == 0))
		{
			//CW
			p->cntRot++;
			p->direction = 1;
		}
		for(int i = 0; i < p->nCallbacks; ++i)
		{
			RotEnc_prescaled_callback_t* pCb = p->callbacks + i;
			if(pCb->dirTrig == 2 ||  pCb->dirTrig == p->direction)
			{
				if(pCb->dirTrig <= 1)
					pCb->cntBeforeTrig -= 1;
				else
					pCb->dirTrig -= p->direction; // 双向触发则根据转向增减计数器
				if(pCb->cntBeforeTrig == 0) // 计数器归0触发并重填
				{
					if(pCb->on_trig != NULL)
						pCb->on_trig();
					pCb->cntBeforeTrig = pCb->cntRequiredToTrig;
				}
			}
			else
				pCb->cntBeforeTrig = pCb->cntRequiredToTrig; // 如果用户旋转方向与触发要求方向相反, 则重装计数器
		}
		retval = (uint8_t)isFlipA | ((uint8_t)isFlipB << 1) | ((uint8_t)isFlipSW << 2);
	}

	return retval;
}


int8_t RotEnc_interrupt(RotEnc_t*p,  uint8_t fall_rise)
{
	int8_t ret_dir = 0;
	uint8_t stateB = HAL_GPIO_ReadPin(p->B.GPIOport, p->B.GPIOpin);
	uint8_t idxCondition = (stateB << 1) | fall_rise;
//	typedef enum {
//		ROT_ENC_START = 0,
//		ROT_ENC_WAIT_A_FALL_WITH_B_HIGH,
//		ROT_ENC_WAIT_A_FALL_WITH_B_LOW,
//		ROT_ENC_WAIT_A_RISE_WITH_B_HIGH,
//		ROT_ENC_WAIT_A_RISE_WITH_B_LOW,
//	ROT_ENC_CW_CONFIRMED,
//	ROT_ENC_CCW_CONFIRMED,
//	} RotEnc_state_t;
	const RotEnc_state_t transfer[5][4] = {
			// START to: fall_low, rise_low, fall_high, rise_high
			{ROT_ENC_WAIT_A_RISE_WITH_B_LOW, ROT_ENC_WAIT_A_FALL_WITH_B_LOW, ROT_ENC_WAIT_A_RISE_WITH_B_HIGH, ROT_ENC_WAIT_A_FALL_WITH_B_HIGH},
			// ROT_ENC_WAIT_A_FALL_WITH_B_HIGH to: fall_low, rise_low, fall_high, rise_high
			{ROT_ENC_CCW_CONFIRMED, ROT_ENC_WAIT_A_FALL_WITH_B_HIGH, ROT_ENC_START, ROT_ENC_WAIT_A_FALL_WITH_B_HIGH},
			// ROT_ENC_WAIT_A_FALL_WITH_B_LOW to: fall_low, rise_low, fall_high, rise_high
			{ROT_ENC_START, ROT_ENC_WAIT_A_FALL_WITH_B_LOW, ROT_ENC_CW_CONFIRMED, ROT_ENC_WAIT_A_FALL_WITH_B_LOW},
			// ROT_ENC_WAIT_A_RISE_WITH_B_HIGH to: fall_low, rise_low, fall_high, rise_high
			{ROT_ENC_WAIT_A_RISE_WITH_B_HIGH, ROT_ENC_CW_CONFIRMED, ROT_ENC_WAIT_A_RISE_WITH_B_HIGH, ROT_ENC_START},
			// ROT_ENC_WAIT_A_RISE_WITH_B_LOW to: fall_low, rise_low, fall_high, rise_high
			{ROT_ENC_WAIT_A_RISE_WITH_B_LOW, ROT_ENC_START, ROT_ENC_WAIT_A_RISE_WITH_B_LOW, ROT_ENC_CCW_CONFIRMED}
	};
	RotEnc_state_t next_fsm = 0;
	if(p->fsm < ROT_ENC_WAIT_A_RISE_WITH_B_LOW)
	{
		next_fsm = transfer[p->fsm][idxCondition];
		if(next_fsm == ROT_ENC_CW_CONFIRMED)
		{
			p->cntRot++;
			p->direction = 1;
			ret_dir = 1;
			next_fsm = 0;
		}
		else if(next_fsm == ROT_ENC_CCW_CONFIRMED)
		{
			p->cntRot--;
			p->direction = -1;
			ret_dir = -1;
			next_fsm = 0;
		}
	}
	p->fsm = next_fsm;
	return ret_dir;

}
