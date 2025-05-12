/*
 * precise_counter.c
 *
 *  Created on: 2025年2月2日
 *      Author: cpholzn
 */


#include "precise_counter.h"

size_t precise_counter_update(precise_counter_t *p, size_t new_cnt, uint8_t rshift_bits)
{
	size_t diff, diff_shifted;
	if(p->state == PRECISE_COUNTER_COUNTING)
	{
		diff = new_cnt - p->prev_cnt + p->cum_err;
		diff_shifted = diff >> rshift_bits;
		size_t err = diff - (diff_shifted << rshift_bits);
		p->cum_err = err;
	}
	p->prev_cnt = new_cnt;
	return diff_shifted;
}
