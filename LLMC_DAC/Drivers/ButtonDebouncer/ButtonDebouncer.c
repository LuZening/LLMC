/*
 * ButtonDebouncer.c
 *
 *  Created on: Oct 5, 2020
 *      Author: Zening
 */


#include "ButtonDebouncer.h"


void btndeb_init(ButtonDebouncer_t* p, GPIO_TypeDef* GPIOport, uint16_t GPIOpin, uint8_t lvlAct, unsigned int cntdnRefill)
{
	p->state = 1 - lvlAct;
	p->interrupted = 0;
	p->lvlAct = lvlAct;
	p->active = 0;
	p->GPIOport = GPIOport;
	p->GPIOpin = GPIOpin;
	p->cntdn = 0;
	p->cntdnRefill = cntdnRefill;
	p->cntLongPress = 0;
	p->on_flip = NULL;
}


void btndeb_interrupt(ButtonDebouncer_t* p, uint8_t state_init)
{
	if(p->interrupted == 0)
	{
		p->interrupted = 1;
		p->cntdn = p->cntdnRefill;
		//NOTE: when interrupted, the gpio pin's reading has already been reversed but is not yet steady
		//NOTE: therefore we need to recover the state before interrupt
		p->state = 1 - state_init;
		p->active = 0; //  by default inactive
		p->cntLongPress = 0;
	}
}

void btndeb_register_flip_callback(ButtonDebouncer_t* p, btnCallbackFunc_t fn)
{
	p->on_flip = fn;
}


bool btndeb_tick(ButtonDebouncer_t* p, unsigned int us)
{
	uint8_t stateNew = HAL_GPIO_ReadPin(p->GPIOport, p->GPIOpin);
	bool flip = false;
	/* STATE1: counting down in progress */
	if(p->cntdn > 0)
	{
		p->cntdn -= us;
		if(p->cntdn <= 0)
		{
			p->cntdn = 0;
			if(stateNew != p->state)
			{
				p->state = stateNew;
				p->active = ((stateNew == p->lvlAct)?(1):(0));
				if(p->on_flip != NULL)
					p->on_flip(stateNew);
				flip = true;
			}
		}
	}
	/* STATE2: counting down is not yet initiated*/
	else
	{
		// start a new counting down if state change detected
		if(stateNew != p->state)
			p->cntdn = p->cntdnRefill;
		// count long press time if state keeps
		else if(p->state == p->lvlAct)
			p->cntLongPress += us;
		else
			p->cntdn = 0;
	}
	return flip;
}
