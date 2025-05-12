/*
 * precise_counter.h
 *
 *  Created on: 2025年2月2日
 *      Author: cpholzn
 */

#ifndef INC_AUDIO_PRECISE_COUNTER_H_
#define INC_AUDIO_PRECISE_COUNTER_H_

#include <stdint.h>
#include <stddef.h>
typedef enum
{
		PRECISE_COUNTER_INIT = 0,
		PRECISE_COUNTER_COUNTING,
} precise_counter_state_t;

typedef struct
{
	precise_counter_state_t state;
	size_t prev_cnt;
	size_t cum_err;
	uint8_t interval_cnt; // keep track of how many times has been visited after last update
} precise_counter_t;

size_t precise_counter_update(precise_counter_t *p, size_t new_cnt, uint8_t rshift_bits);

#endif /* INC_AUDIO_PRECISE_COUNTER_H_ */
